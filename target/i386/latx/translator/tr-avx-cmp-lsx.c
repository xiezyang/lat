/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "common.h"
#include "reg-alloc.h"
#include "latx-options.h"
#include "translate.h"
#include "env.h"

#ifdef CONFIG_LATX_AVX_OPT

static inline void xcomisx(IR1_INST *pir1, bool is_double, bool qnan_exp)
{
    /**
     * (bit 6)ZF = 1 if EQ || UOR
     * (bit 2)PF = 1 if UOR (= ZF & CF)
     * (bit 0)CF = 1 if LT || UOR
     */
    lsassert(ir1_opnd_num(pir1) == 2);
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    /* 0. set flag = 0 */
    IR2_OPND flag_zf = ra_alloc_itemp();
    IR2_OPND flag_pf = ra_alloc_itemp();
    IR2_OPND flag = ra_alloc_itemp();
    la_mov64(flag, zero_ir2_opnd);

    /* 1. check ZF, are they equal & unordered? */
    if (is_double) {
        la_fcmp_cond_d(fcc0_ir2_opnd, dest, src, FCMP_COND_CUEQ + qnan_exp);
    } else {
        la_fcmp_cond_s(fcc0_ir2_opnd, dest, src, FCMP_COND_CUEQ + qnan_exp);
    }
    la_movcf2gr(flag_zf, fcc0_ir2_opnd);

    /* 2. check CF, are they less & unordered? */
    if (is_double) {
        la_fcmp_cond_d(fcc2_ir2_opnd, dest, src, FCMP_COND_CULT + qnan_exp);
    } else {
        la_fcmp_cond_s(fcc2_ir2_opnd, dest, src, FCMP_COND_CULT + qnan_exp);
    }
    la_movcf2gr(flag, fcc2_ir2_opnd);

    /* 3. check PF, are they unordered? (= ZF & CF) */
    la_and(flag_pf, flag, flag_zf);

    la_bstrins_w(flag, flag_zf, ZF_BIT_INDEX, ZF_BIT_INDEX);
    la_bstrins_w(flag, flag_pf, PF_BIT_INDEX, PF_BIT_INDEX);

    /* 4. mov flag to EFLAGS */
    la_x86mtflag(flag, 0x3f);

    ra_free_temp(flag_pf);
    ra_free_temp(flag_zf);
    ra_free_temp(flag);

}

static IR2_OPND avx_comis_load_scalar(IR1_OPND *opnd, bool is_double)
{
    if (ir1_opnd_is_mem(opnd)) {
        return is_double ? load_u64_from_ir1_mem_exact(opnd) :
                           load_u32_from_ir1_mem_exact(opnd);
    }

    lsassert(ir1_opnd_is_xmm(opnd));
    IR2_OPND value = ra_alloc_itemp();
    if (is_double) {
        la_vpickve2gr_du(value, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd)), 0);
    } else {
        la_vpickve2gr_w(value, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd)), 0);
    }
    return value;
}

static void avx_comis_apply_daz_and_mark_exceptions(IR2_OPND value,
                                                    IR2_OPND mxcsr,
                                                    IR2_OPND flags,
                                                    bool is_double,
                                                    bool qnan_invalid)
{
    uint64_t exponent_mask = is_double ? UINT64_C(0x7ff0000000000000) :
                                          UINT64_C(0x000000007f800000);
    uint64_t fraction_mask = is_double ? UINT64_C(0x000fffffffffffff) :
                                         UINT64_C(0x00000000007fffff);
    uint64_t quiet_mask = is_double ? UINT64_C(0x0008000000000000) :
                                      UINT64_C(0x0000000000400000);
    uint64_t sign_mask = is_double ? UINT64_C(0x8000000000000000) :
                                    UINT64_C(0x0000000080000000);
    IR2_OPND mask = ra_alloc_itemp();
    IR2_OPND field = ra_alloc_itemp();
    IR2_OPND check_subnormal = ra_alloc_label();
    IR2_OPND mark_denormal = ra_alloc_label();
    IR2_OPND done = ra_alloc_label();

    li_d(mask, exponent_mask);
    la_and(field, value, mask);
    la_beq(field, zero_ir2_opnd, check_subnormal);
    la_bne(field, mask, done);

    li_d(mask, fraction_mask);
    la_and(field, value, mask);
    la_beq(field, zero_ir2_opnd, done);
    la_ori(flags, flags, 0x40);
    if (qnan_invalid) {
        la_ori(flags, flags, 0x1);
    } else {
        li_d(mask, quiet_mask);
        la_and(field, value, mask);
        la_bne(field, zero_ir2_opnd, done);
        la_ori(flags, flags, 0x1);
    }
    la_b(done);

    la_label(check_subnormal);
    li_d(mask, fraction_mask);
    la_and(field, value, mask);
    la_beq(field, zero_ir2_opnd, done);
    la_andi(field, mxcsr, 0x40);
    la_beq(field, zero_ir2_opnd, mark_denormal);
    li_d(mask, sign_mask);
    la_and(value, value, mask);
    la_b(done);

    la_label(mark_denormal);
    la_ori(flags, flags, 0x2);
    la_label(done);
    ra_free_temp(field);
    ra_free_temp(mask);
}

static void vucomisd_raise_unmasked_exception(IR1_INST *pir1,
                                               IR2_OPND mxcsr,
                                               IR2_OPND flags)
{
    IR2_OPND masks = ra_alloc_itemp();
    IR2_OPND unmasked = ra_alloc_itemp();
    IR2_OPND test = ra_alloc_itemp();
    IR2_OPND invalid_checked = ra_alloc_label();
    IR2_OPND flags_normalized = ra_alloc_label();
    IR2_OPND raise_exception = ra_alloc_label();
    IR2_OPND check_denormal = ra_alloc_label();
    IR2_OPND store_and_raise = ra_alloc_label();
    IR2_OPND done = ra_alloc_label();

    /* Any NaN result suppresses the denormal operand exception. */
    la_andi(test, flags, 0x40);
    la_beq(test, zero_ir2_opnd, invalid_checked);
    la_andi(flags, flags, 0x1);
    la_b(flags_normalized);
    la_label(invalid_checked);
    la_andi(flags, flags, 0x3);
    la_label(flags_normalized);

    la_srli_w(masks, mxcsr, 7);
    la_xori(masks, masks, 0x3f);
    la_and(unmasked, flags, masks);
    ra_free_temp(masks);
    la_bne(unmasked, zero_ir2_opnd, raise_exception);

    la_or(mxcsr, mxcsr, flags);
    la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    la_b(done);

    la_label(raise_exception);
    la_andi(test, unmasked, 0x1);
    la_beq(test, zero_ir2_opnd, check_denormal);
    la_andi(flags, flags, 0x1);
    la_b(store_and_raise);

    la_label(check_denormal);
    la_andi(flags, flags, 0x3);
    la_label(store_and_raise);
    la_or(mxcsr, mxcsr, flags);
    la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    ra_free_temp(test);

    IR2_OPND eip = ra_alloc_dbt_arg2();
    IR2_OPND helper = ra_alloc_itemp();
    li_d(eip, ir1_addr(pir1));
    la_store_addrx(eip, env_ir2_opnd, lsenv_offset_of_eip(lsenv));
    tr_save_registers_to_env(0xff, 0xff, option_save_xmm,
                             options_to_save());
#ifdef TARGET_X86_64
    tr_save_x64_8_registers_to_env(0xff, option_save_xmm);
#endif
    aot_load_host_addr(helper, (ADDR)helper_raise_simd_exception,
                       LOAD_HELPER_RAISE_SIMD_EXCEPTION, 0);
    la_mov64(a0_ir2_opnd, unmasked);
    la_jirl(zero_ir2_opnd, helper, 0);
    ra_free_temp(helper);
    la_label(done);
    ra_free_temp(unmasked);
}

static void vucomisd_commit_exception_flags(IR2_OPND mxcsr, IR2_OPND flags)
{
    IR2_OPND normalized = ra_alloc_itemp();
    la_andi(normalized, flags, 0x3f);
    la_or(mxcsr, mxcsr, normalized);
    la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    tr_gen_call_to_helper1((ADDR)update_mxcsr_status, 0,
                           LOAD_HELPER_UPDATE_MXCSR_STATUS);
    ra_free_temp(normalized);
}

static void avx_comis_write_flags(IR2_OPND lhs, IR2_OPND rhs, bool is_double)
{
    uint64_t exponent_mask = is_double ? UINT64_C(0x7ff0000000000000) :
                                          UINT64_C(0x000000007f800000);
    uint64_t fraction_mask = is_double ? UINT64_C(0x000fffffffffffff) :
                                         UINT64_C(0x00000000007fffff);
    uint64_t absolute_mask = is_double ? UINT64_C(0x7fffffffffffffff) :
                                         UINT64_C(0x000000007fffffff);
    uint64_t sign_mask = is_double ? UINT64_C(0x8000000000000000) :
                                    UINT64_C(0x0000000080000000);
    IR2_OPND flag = ra_alloc_itemp();
    IR2_OPND mask = ra_alloc_itemp();
    IR2_OPND field = ra_alloc_itemp();
    IR2_OPND check_rhs_nan = ra_alloc_label();
    IR2_OPND ordered = ra_alloc_label();
    IR2_OPND check_exact_equal = ra_alloc_label();
    IR2_OPND compare_signs = ra_alloc_label();
    IR2_OPND same_sign = ra_alloc_label();
    IR2_OPND negative = ra_alloc_label();
    IR2_OPND unordered = ra_alloc_label();
    IR2_OPND equal = ra_alloc_label();
    IR2_OPND less = ra_alloc_label();
    IR2_OPND greater = ra_alloc_label();
    IR2_OPND write = ra_alloc_label();

    li_d(mask, exponent_mask);
    la_and(field, lhs, mask);
    la_bne(field, mask, check_rhs_nan);
    li_d(mask, fraction_mask);
    la_and(field, lhs, mask);
    la_bne(field, zero_ir2_opnd, unordered);

    la_label(check_rhs_nan);
    li_d(mask, exponent_mask);
    la_and(field, rhs, mask);
    la_bne(field, mask, ordered);
    li_d(mask, fraction_mask);
    la_and(field, rhs, mask);
    la_bne(field, zero_ir2_opnd, unordered);

    la_label(ordered);
    li_d(mask, absolute_mask);
    la_and(field, lhs, mask);
    la_bne(field, zero_ir2_opnd, check_exact_equal);
    la_and(field, rhs, mask);
    la_beq(field, zero_ir2_opnd, equal);

    la_label(check_exact_equal);
    la_beq(lhs, rhs, equal);
    la_label(compare_signs);
    li_d(mask, sign_mask);
    la_and(mask, lhs, mask);
    li_d(field, sign_mask);
    la_and(field, rhs, field);
    la_beq(mask, field, same_sign);
    la_bne(mask, zero_ir2_opnd, less);
    la_b(greater);

    la_label(same_sign);
    la_bne(mask, zero_ir2_opnd, negative);
    la_bltu(lhs, rhs, less);
    la_b(greater);

    la_label(negative);
    la_bltu(rhs, lhs, less);
    la_b(greater);

    la_label(unordered);
    li_w(flag, (1 << ZF_BIT_INDEX) | (1 << PF_BIT_INDEX) |
               (1 << CF_BIT_INDEX));
    la_b(write);

    la_label(equal);
    li_w(flag, 1 << ZF_BIT_INDEX);
    la_b(write);

    la_label(less);
    li_w(flag, 1 << CF_BIT_INDEX);
    la_b(write);

    la_label(greater);
    la_or(flag, zero_ir2_opnd, zero_ir2_opnd);
    la_label(write);
    la_x86mtflag(flag, 0x3f);

    ra_free_temp(field);
    ra_free_temp(mask);
    ra_free_temp(flag);
}

static bool translate_avx_comis_lsx(IR1_INST *pir1, bool is_double,
                                    bool qnan_invalid)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    lsassert(ir1_opnd_num(pir1) == 2 && ir1_opnd_is_xmm(opnd0));
    lsassert(ir1_opnd_is_xmm(opnd1) ||
             (ir1_opnd_is_mem(opnd1) &&
              ir1_opnd_size(opnd1) == (is_double ? 64 : 32)));

    IR2_OPND lhs = avx_comis_load_scalar(opnd0, is_double);
    IR2_OPND rhs = avx_comis_load_scalar(opnd1, is_double);
    IR2_OPND mxcsr = ra_alloc_itemp();
    IR2_OPND flags = ra_alloc_itemp();

    la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    la_or(flags, zero_ir2_opnd, zero_ir2_opnd);
    avx_comis_apply_daz_and_mark_exceptions(lhs, mxcsr, flags,
                                            is_double, qnan_invalid);
    avx_comis_apply_daz_and_mark_exceptions(rhs, mxcsr, flags,
                                            is_double, qnan_invalid);
    vucomisd_raise_unmasked_exception(pir1, mxcsr, flags);
    avx_comis_write_flags(lhs, rhs, is_double);
    vucomisd_commit_exception_flags(mxcsr, flags);

    ra_free_temp(rhs);
    ra_free_temp(lhs);
    ra_free_temp(flags);
    ra_free_temp(mxcsr);
    return true;
}

bool translate_vucomisd_lsx(IR1_INST *pir1)
{
    return translate_avx_comis_lsx(pir1, true, false);
}

bool translate_vcomisd_lsx(IR1_INST *pir1)
{
    return translate_avx_comis_lsx(pir1, true, true);
}

bool translate_vcomiss_lsx(IR1_INST *pir1)
{
    return translate_avx_comis_lsx(pir1, false, true);
}

bool translate_vucomiss_lsx(IR1_INST *pir1)
{
    return translate_avx_comis_lsx(pir1, false, false);
}

typedef IR2_INST *(*latx_avx_integer_cmp_lsx_fn)(IR2_OPND, IR2_OPND,
                                                 IR2_OPND);

static void translate_avx_integer_cmp_lsx_apply(
    latx_avx_integer_cmp_lsx_fn cmp, bool reverse, IR2_OPND dest,
    IR2_OPND lhs, IR2_OPND rhs)
{
    if (reverse) {
        cmp(dest, rhs, lhs);
    } else {
        cmp(dest, lhs, rhs);
    }
}

static bool translate_avx_integer_cmp_lsx(
    IR1_INST *pir1, latx_avx_integer_cmp_lsx_fn cmp, bool reverse)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int dest_index = ir1_opnd_base_reg_num(opnd0);
    int src1_index = ir1_opnd_base_reg_num(opnd1);
    IR2_OPND src1_low = ra_alloc_ftemp();
    IR2_OPND src2_low;

    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
             (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd1));
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));

    if (ir1_opnd_is_ymm(opnd0)) {
        tr_save_ymm_to_env(UINT16_MAX);
    }
    la_vori_b(src1_low, ra_alloc_xmm(src1_index), 0);
    if (ir1_opnd_is_mem(opnd2)) {
        if (ir1_opnd_is_ymm(opnd0)) {
            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high;

            load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
            translate_avx_integer_cmp_lsx_apply(
                cmp, reverse, src1_low, src1_low, src2_low);
            translate_avx_integer_cmp_lsx_apply(
                cmp, reverse, src1_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
            ra_free_temp(src1_high);
            ra_free_temp(src2_high);
        } else {
            src2_low = load_v128_from_ir1_mem_exact(opnd2);
            translate_avx_integer_cmp_lsx_apply(
                cmp, reverse, src1_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            clear_ymm_high128_shadow(dest_index);
        }
    } else {
        int src2_index = ir1_opnd_base_reg_num(opnd2);

        lsassert((ir1_opnd_is_xmm(opnd2) && ir1_opnd_is_xmm(opnd0)) ||
                 (ir1_opnd_is_ymm(opnd2) && ir1_opnd_is_ymm(opnd0)));
        src2_low = ra_alloc_ftemp();
        la_vori_b(src2_low, ra_alloc_xmm(src2_index), 0);
        if (ir1_opnd_is_ymm(opnd0)) {
            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high = load_ymm_high128_shadow(src2_index);

            translate_avx_integer_cmp_lsx_apply(
                cmp, reverse, src1_low, src1_low, src2_low);
            translate_avx_integer_cmp_lsx_apply(
                cmp, reverse, src1_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
            ra_free_temp(src1_high);
            ra_free_temp(src2_high);
        } else {
            translate_avx_integer_cmp_lsx_apply(
                cmp, reverse, src1_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            clear_ymm_high128_shadow(dest_index);
        }
    }
    ra_free_temp(src2_low);
    ra_free_temp(src1_low);
    return true;
}

#define LATX_AVX_INTEGER_CMP_LSX_DEFINE(opcode, name, cmp, reverse) \
bool translate_v##name##_lsx(IR1_INST *pir1) \
{ \
    return translate_avx_integer_cmp_lsx(pir1, cmp, reverse); \
}
LATX_AVX_INTEGER_CMP_LSX_TABLE(LATX_AVX_INTEGER_CMP_LSX_DEFINE)
#undef LATX_AVX_INTEGER_CMP_LSX_DEFINE

static bool translate_avx_integer_cmp_opcode_lsx(IR1_INST *pir1)
{
    switch (ir1_opcode(pir1)) {
#define LATX_AVX_INTEGER_CMP_LSX_CASE(opcode, name, cmp, reverse) \
    case dt_X86_INS_##opcode: \
        return translate_v##name##_lsx(pir1);
        LATX_AVX_INTEGER_CMP_LSX_TABLE(LATX_AVX_INTEGER_CMP_LSX_CASE)
#undef LATX_AVX_INTEGER_CMP_LSX_CASE
    default:
        return false;
    }
}

bool translate_vpcmpeqx_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_cmp_opcode_lsx(pir1);
}

bool translate_vpcmpgtx_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_cmp_opcode_lsx(pir1);
}

static uint8_t avx_float_cmp_predicate_lsx(IR1_INST *pir1, int base_opcode)
{
    int opcode = ir1_opcode(pir1);

    if (opcode == base_opcode) {
        lsassert(ir1_opnd_num(pir1) == 4 &&
                 ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
        return ir1_opnd_uimm(ir1_get_opnd(pir1, 3)) & 0x1f;
    }

    lsassert(opcode > base_opcode && opcode <= base_opcode + 32);
    return opcode - base_opcode - 1;
}

static void avx_float_cmp_lsx_params(uint8_t predicate, int *condition,
                                     bool *reverse, bool *set_all_ones)
{
    *reverse = false;
    *set_all_ones = false;
    switch (predicate) {
    case 0:
        *condition = X86_FCMP_COND_EQ;
        break;
    case 1:
        *condition = X86_FCMP_COND_LT;
        break;
    case 2:
        *condition = X86_FCMP_COND_LE;
        break;
    case 3:
        *condition = X86_FCMP_COND_UNORD;
        break;
    case 4:
        *condition = X86_FCMP_COND_NEQ;
        break;
    case 5:
        *condition = X86_FCMP_COND_NLT;
        *reverse = true;
        break;
    case 6:
        *condition = X86_FCMP_COND_NLE;
        *reverse = true;
        break;
    case 7:
        *condition = X86_FCMP_COND_ORD;
        break;
    case 8:
        *condition = X86_FCMP_COND_EQ_UQ;
        break;
    case 9:
        *condition = X86_FCMP_COND_NGE;
        break;
    case 10:
        *condition = X86_FCMP_COND_NGT;
        break;
    case 11:
        *condition = X86_FCMP_COND_FALSE;
        break;
    case 12:
        *condition = X86_FCMP_COND_NEQ_OQ;
        break;
    case 13:
        *condition = X86_FCMP_COND_GE;
        *reverse = true;
        break;
    case 14:
        *condition = X86_FCMP_COND_GT;
        *reverse = true;
        break;
    case 15:
        *condition = X86_FCMP_COND_TRUE;
        *set_all_ones = true;
        break;
    case 16:
        *condition = X86_FCMP_COND_EQ_OS;
        break;
    case 17:
        *condition = X86_FCMP_COND_LT_OQ;
        break;
    case 18:
        *condition = X86_FCMP_COND_LE_OQ;
        break;
    case 19:
        *condition = X86_FCMP_COND_UNORD_S;
        break;
    case 20:
        *condition = X86_FCMP_COND_NEQ_US;
        break;
    case 21:
        *condition = X86_FCMP_COND_NLT_UQ;
        *reverse = true;
        break;
    case 22:
        *condition = X86_FCMP_COND_NLE_UQ;
        *reverse = true;
        break;
    case 23:
        *condition = X86_FCMP_COND_ORD_S;
        break;
    case 24:
        *condition = X86_FCMP_COND_EQ_US;
        break;
    case 25:
        *condition = X86_FCMP_COND_NGE_UQ;
        break;
    case 26:
        *condition = X86_FCMP_COND_NGT_UQ;
        break;
    case 27:
        *condition = X86_FCMP_COND_FALSE_OS;
        break;
    case 28:
        *condition = X86_FCMP_COND_NEQ_OS;
        break;
    case 29:
        *condition = X86_FCMP_COND_GE_OQ;
        *reverse = true;
        break;
    case 30:
        *condition = X86_FCMP_COND_GT_OQ;
        *reverse = true;
        break;
    case 31:
        *condition = X86_FCMP_COND_TRUE_US;
        *set_all_ones = true;
        break;
    default:
        lsassert(0);
    }
}

static void translate_avx_float_cmp_lsx_apply(bool double_precision,
                                               IR2_OPND dest, IR2_OPND lhs,
                                               IR2_OPND rhs, int condition,
                                               bool reverse, bool set_all_ones)
{
    if (double_precision) {
        if (reverse) {
            la_vfcmp_cond_d(dest, rhs, lhs, condition);
        } else {
            la_vfcmp_cond_d(dest, lhs, rhs, condition);
        }
    } else if (reverse) {
        la_vfcmp_cond_s(dest, rhs, lhs, condition);
    } else {
        la_vfcmp_cond_s(dest, lhs, rhs, condition);
    }
    if (set_all_ones) {
        la_vori_b(dest, dest, 0xff);
    }
}

static IR2_OPND load_avx_float_cmp_packed_lsx_source(IR1_OPND *opnd)
{
    IR2_OPND value;

    if (ir1_opnd_is_mem(opnd)) {
        return load_v128_from_ir1_mem_exact(opnd);
    }
    lsassert(ir1_opnd_is_xmm(opnd) || ir1_opnd_is_ymm(opnd));
    value = ra_alloc_ftemp();
    la_vori_b(value, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd)), 0);
    return value;
}

static IR2_OPND load_avx_float_cmp_scalar_lsx_source(IR1_OPND *opnd,
                                                       bool double_precision)
{
    IR2_OPND value = ra_alloc_ftemp();

    la_vxor_v(value, value, value);
    if (ir1_opnd_is_mem(opnd)) {
        IR2_OPND bits = double_precision ?
            load_u64_from_ir1_mem_exact(opnd) :
            load_u32_from_ir1_mem_exact(opnd);

        if (double_precision) {
            la_vinsgr2vr_d(value, bits, 0);
        } else {
            la_vinsgr2vr_w(value, bits, 0);
        }
        ra_free_temp(bits);
    } else {
        lsassert(ir1_opnd_is_xmm(opnd));
        la_vori_b(value, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd)), 0);
    }
    return value;
}

static bool translate_avx_float_cmp_packed_lsx(IR1_INST *pir1,
                                                int base_opcode,
                                                bool double_precision)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool is_ymm = ir1_opnd_is_ymm(opnd0);
    int dest_index = ir1_opnd_base_reg_num(opnd0);
    int src1_index = ir1_opnd_base_reg_num(opnd1);
    uint8_t predicate = avx_float_cmp_predicate_lsx(pir1, base_opcode);
    int condition;
    bool reverse;
    bool set_all_ones;
    IR2_OPND src1_low;
    IR2_OPND src2_low;

    lsassert((is_ymm && ir1_opnd_is_ymm(opnd1)) ||
             (!is_ymm && ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)));
    avx_float_cmp_lsx_params(predicate, &condition, &reverse,
                             &set_all_ones);
    lsassert(ir1_opnd_is_mem(opnd2) ||
             (is_ymm ? ir1_opnd_is_ymm(opnd2) : ir1_opnd_is_xmm(opnd2)));
    if (is_ymm) {
        tr_save_ymm_to_env(UINT16_MAX);
    }

    src1_low = load_avx_float_cmp_packed_lsx_source(opnd1);
    if (ir1_opnd_is_mem(opnd2) && is_ymm) {
        IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
        IR2_OPND src2_high;

        load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
        translate_avx_float_cmp_lsx_apply(double_precision, src1_low,
                                          src1_low, src2_low, condition,
                                          reverse, set_all_ones);
        translate_avx_float_cmp_lsx_apply(double_precision, src1_high,
                                          src1_high, src2_high, condition,
                                          reverse, set_all_ones);
        la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
        store_ymm_high128_shadow(src1_high, dest_index);
        ra_free_temp(src2_high);
        ra_free_temp(src1_high);
    } else {
        src2_low = load_avx_float_cmp_packed_lsx_source(opnd2);
        if (is_ymm) {
            int src2_index = ir1_opnd_base_reg_num(opnd2);
            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high = load_ymm_high128_shadow(src2_index);

            translate_avx_float_cmp_lsx_apply(double_precision, src1_low,
                                              src1_low, src2_low, condition,
                                              reverse, set_all_ones);
            translate_avx_float_cmp_lsx_apply(double_precision, src1_high,
                                              src1_high, src2_high, condition,
                                              reverse, set_all_ones);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
            ra_free_temp(src2_high);
            ra_free_temp(src1_high);
        } else {
            translate_avx_float_cmp_lsx_apply(double_precision, src1_low,
                                              src1_low, src2_low, condition,
                                              reverse, set_all_ones);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            clear_ymm_high128_shadow(dest_index);
        }
    }
    ra_free_temp(src2_low);
    ra_free_temp(src1_low);
    return true;
}

static bool translate_avx_float_cmp_scalar_lsx(IR1_INST *pir1,
                                                int base_opcode,
                                                bool double_precision)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int dest_index = ir1_opnd_base_reg_num(opnd0);
    int src1_index = ir1_opnd_base_reg_num(opnd1);
    uint8_t predicate = avx_float_cmp_predicate_lsx(pir1, base_opcode);
    int condition;
    bool reverse;
    bool set_all_ones;
    IR2_OPND src1_scalar;
    IR2_OPND src2_scalar;
    IR2_OPND result = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    avx_float_cmp_lsx_params(predicate, &condition, &reverse,
                             &set_all_ones);
    src1_scalar = load_avx_float_cmp_scalar_lsx_source(opnd1,
                                                        double_precision);
    src2_scalar = load_avx_float_cmp_scalar_lsx_source(opnd2,
                                                        double_precision);
    if (double_precision) {
        la_vreplve_d(src1_scalar, src1_scalar, zero_ir2_opnd);
        la_vreplve_d(src2_scalar, src2_scalar, zero_ir2_opnd);
    } else {
        la_vreplve_w(src1_scalar, src1_scalar, zero_ir2_opnd);
        la_vreplve_w(src2_scalar, src2_scalar, zero_ir2_opnd);
    }
    translate_avx_float_cmp_lsx_apply(double_precision, result,
                                      src1_scalar, src2_scalar, condition,
                                      reverse, set_all_ones);
    if (dest_index != src1_index) {
        la_vori_b(ra_alloc_xmm(dest_index),
                  ra_alloc_xmm(src1_index), 0);
    }
    if (double_precision) {
        la_vextrins_d(ra_alloc_xmm(dest_index), result, 0);
    } else {
        la_vextrins_w(ra_alloc_xmm(dest_index), result, 0);
    }
    clear_ymm_high128_shadow(dest_index);

    ra_free_temp(result);
    ra_free_temp(src2_scalar);
    ra_free_temp(src1_scalar);
    return true;
}

bool translate_vcmppd_lsx(IR1_INST *pir1)
{
    return translate_avx_float_cmp_packed_lsx(pir1, dt_X86_INS_VCMPPD, true);
}

bool translate_vcmpps_lsx(IR1_INST *pir1)
{
    return translate_avx_float_cmp_packed_lsx(pir1, dt_X86_INS_VCMPPS, false);
}

bool translate_vcmpsd_lsx(IR1_INST *pir1)
{
    return translate_avx_float_cmp_scalar_lsx(pir1, dt_X86_INS_VCMPSD, true);
}

bool translate_vcmpss_lsx(IR1_INST *pir1)
{
    return translate_avx_float_cmp_scalar_lsx(pir1, dt_X86_INS_VCMPSS, false);
}
#endif
