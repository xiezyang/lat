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
#endif
