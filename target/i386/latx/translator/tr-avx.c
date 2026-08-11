/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "common.h"
#include "reg-alloc.h"
#include "latx-options.h"
#include "translate.h"
#include "hbr.h"
#include "tr-vpaes.h"
#include "pclmul.h"

#ifdef CONFIG_LATX_AVX_OPT
bool translate_vaddpd(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    IR2_OPND src2;

    if (ir1_opnd_is_ymm(opnd0)) {
        src2 = load_freg256_from_ir1(opnd2);
        la_xvfadd_d(dest, src1, src2);
    } else if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND temp = ra_alloc_ftemp();

        src2 = load_freg128_from_ir1(opnd2);
        la_vfadd_d(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vaddps(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    IR2_OPND src2;

    if (ir1_opnd_is_ymm(opnd0)) {
        src2 = load_freg256_from_ir1(opnd2);
        la_xvfadd_s(dest, src1, src2);
    } else if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND temp = ra_alloc_ftemp();

        src2 = load_freg128_from_ir1(opnd2);
        la_vfadd_s(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vaddsd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();

    la_fadd_d(temp, src1, src2);
    la_vshuf4i_d(temp, src1, 0xc);
    set_high128_xreg_to_zero(temp);
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vaddss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();

    la_fadd_s(temp, src1, src2);
    if (ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)) !=
        ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vsubpd(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    IR2_OPND src2;

    if (ir1_opnd_is_ymm(opnd0)) {
        src2 = load_freg256_from_ir1(opnd2);
        la_xvfsub_d(dest, src1, src2);
    } else if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND temp = ra_alloc_ftemp();

        src2 = load_freg128_from_ir1(opnd2);
        la_vfsub_d(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vsubps(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    IR2_OPND src2;

    if (ir1_opnd_is_ymm(opnd0)) {
        src2 = load_freg256_from_ir1(opnd2);
        la_xvfsub_s(dest, src1, src2);
    } else if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND temp = ra_alloc_ftemp();

        src2 = load_freg128_from_ir1(opnd2);
        la_vfsub_s(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vsubsd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_freg128_from_ir1(opnd2);
    IR2_OPND temp = ra_alloc_ftemp();
    la_fsub_d(temp, src1, src2);
    la_vshuf4i_d(temp, src1, 0xc);
    la_xvori_b(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vsubss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();

    la_fsub_s(temp, src1, src2);
    if (ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)) !=
        ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vmulpd(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    IR2_OPND src2;

    if (ir1_opnd_is_ymm(opnd0)) {
        src2 = load_freg256_from_ir1(opnd2);
        la_xvfmul_d(dest, src1, src2);
    } else if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND temp = ra_alloc_ftemp();

        src2 = load_freg128_from_ir1(opnd2);
        la_vfmul_d(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vmulps(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    IR2_OPND src2;

    if (ir1_opnd_is_ymm(opnd0)) {
        src2 = load_freg256_from_ir1(opnd2);
        la_xvfmul_s(dest, src1, src2);
    } else if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND temp = ra_alloc_ftemp();

        src2 = load_freg128_from_ir1(opnd2);
        la_vfmul_s(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

static void vmulsd_mark_snan(IR2_OPND value, IR2_OPND flags)
{
    IR2_OPND mask = ra_alloc_itemp();
    IR2_OPND field = ra_alloc_itemp();
    IR2_OPND done = ra_alloc_label();

    li_d(mask, UINT64_C(0x7ff0000000000000));
    la_and(field, value, mask);
    la_bne(field, mask, done);
    li_d(mask, UINT64_C(0x000fffffffffffff));
    la_and(field, value, mask);
    la_beq(field, zero_ir2_opnd, done);
    li_d(mask, UINT64_C(0x0008000000000000));
    la_and(field, value, mask);
    la_bne(field, zero_ir2_opnd, done);
    la_ori(flags, flags, 0x1);
    la_label(done);

    ra_free_temp(field);
    ra_free_temp(mask);
}

static void vmulsd_apply_daz(IR2_OPND value, IR2_OPND mxcsr,
                             IR2_OPND flags)
{
    IR2_OPND mask = ra_alloc_itemp();
    IR2_OPND field = ra_alloc_itemp();
    IR2_OPND keep_denormal = ra_alloc_label();
    IR2_OPND done = ra_alloc_label();

    li_d(mask, UINT64_C(0x7ff0000000000000));
    la_and(field, value, mask);
    la_bne(field, zero_ir2_opnd, done);
    li_d(mask, UINT64_C(0x000fffffffffffff));
    la_and(field, value, mask);
    la_beq(field, zero_ir2_opnd, done);

    la_andi(field, mxcsr, 0x40);
    la_beq(field, zero_ir2_opnd, keep_denormal);
    li_d(mask, UINT64_C(0x8000000000000000));
    la_and(value, value, mask);
    la_b(done);

    la_label(keep_denormal);
    la_ori(flags, flags, 0x2);
    la_label(done);

    ra_free_temp(field);
    ra_free_temp(mask);
}

static void vmulsd_map_fcsr_flags(IR2_OPND fcsr, IR2_OPND flags)
{
    IR2_OPND bit = ra_alloc_itemp();

    la_bstrpick_w(bit, fcsr, FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_V);
    la_or(flags, flags, bit);
    la_bstrpick_w(bit, fcsr, FCSR_OFF_FLAGS_Z, FCSR_OFF_FLAGS_Z);
    la_slli_w(bit, bit, 2);
    la_or(flags, flags, bit);
    la_bstrpick_w(bit, fcsr, FCSR_OFF_FLAGS_O, FCSR_OFF_FLAGS_O);
    la_slli_w(bit, bit, 3);
    la_or(flags, flags, bit);
    la_bstrpick_w(bit, fcsr, FCSR_OFF_FLAGS_U, FCSR_OFF_FLAGS_U);
    la_slli_w(bit, bit, 4);
    la_or(flags, flags, bit);
    la_bstrpick_w(bit, fcsr, FCSR_OFF_FLAGS_I, FCSR_OFF_FLAGS_I);
    la_slli_w(bit, bit, 5);
    la_or(flags, flags, bit);

    ra_free_temp(bit);
}

static void vmulsd_mark_unmasked_exact_underflow(IR2_OPND result,
                                                 IR2_OPND mxcsr,
                                                 IR2_OPND flags)
{
    IR2_OPND mask = ra_alloc_itemp();
    IR2_OPND field = ra_alloc_itemp();
    IR2_OPND done = ra_alloc_label();

    la_andi(field, mxcsr, 0x800);
    la_bne(field, zero_ir2_opnd, done);
    li_d(mask, UINT64_C(0x7ff0000000000000));
    la_and(field, result, mask);
    la_bne(field, zero_ir2_opnd, done);
    li_d(mask, UINT64_C(0x000fffffffffffff));
    la_and(field, result, mask);
    la_beq(field, zero_ir2_opnd, done);
    la_ori(flags, flags, 0x10);

    la_label(done);
    ra_free_temp(field);
    ra_free_temp(mask);
}

static void vmulsd_fix_special_result(IR2_OPND src1, IR2_OPND src2,
                                      IR2_OPND result, IR2_OPND mxcsr,
                                      IR2_OPND flags)
{
    IR2_OPND src1_not_nan = ra_alloc_label();
    IR2_OPND src2_not_nan = ra_alloc_label();
    IR2_OPND check_reverse_invalid = ra_alloc_label();
    IR2_OPND check_ftz = ra_alloc_label();
    IR2_OPND done = ra_alloc_label();

    vmulsd_mark_snan(src1, flags);
    vmulsd_mark_snan(src2, flags);

    IR2_OPND mask = ra_alloc_itemp();
    IR2_OPND field = ra_alloc_itemp();
    li_d(mask, UINT64_C(0x7ff0000000000000));
    la_and(field, src1, mask);
    la_bne(field, mask, src1_not_nan);
    li_d(mask, UINT64_C(0x000fffffffffffff));
    la_and(field, src1, mask);
    la_beq(field, zero_ir2_opnd, src1_not_nan);
    li_d(mask, UINT64_C(0x0008000000000000));
    la_or(result, src1, mask);
    la_b(done);

    la_label(src1_not_nan);
    li_d(mask, UINT64_C(0x7ff0000000000000));
    la_and(field, src2, mask);
    la_bne(field, mask, src2_not_nan);
    li_d(mask, UINT64_C(0x000fffffffffffff));
    la_and(field, src2, mask);
    la_beq(field, zero_ir2_opnd, src2_not_nan);
    li_d(mask, UINT64_C(0x0008000000000000));
    la_or(result, src2, mask);
    la_b(done);

    la_label(src2_not_nan);
    li_d(mask, UINT64_C(0x7fffffffffffffff));
    la_and(field, src1, mask);
    la_bne(field, zero_ir2_opnd, check_reverse_invalid);
    la_and(field, src2, mask);
    li_d(mask, UINT64_C(0x7ff0000000000000));
    la_bne(field, mask, check_ftz);
    li_d(result, UINT64_C(0xfff8000000000000));
    la_ori(flags, flags, 0x1);
    la_b(done);

    la_label(check_reverse_invalid);
    li_d(mask, UINT64_C(0x7fffffffffffffff));
    la_and(field, src2, mask);
    la_bne(field, zero_ir2_opnd, check_ftz);
    la_and(field, src1, mask);
    li_d(mask, UINT64_C(0x7ff0000000000000));
    la_bne(field, mask, check_ftz);
    li_d(result, UINT64_C(0xfff8000000000000));
    la_ori(flags, flags, 0x1);
    la_b(done);

    la_label(check_ftz);
    li_d(mask, UINT64_C(0x8000));
    la_and(field, mxcsr, mask);
    la_beq(field, zero_ir2_opnd, done);
    li_d(mask, UINT64_C(0x7ff0000000000000));
    la_and(field, result, mask);
    la_bne(field, zero_ir2_opnd, done);
    li_d(mask, UINT64_C(0x000fffffffffffff));
    la_and(field, result, mask);
    la_beq(field, zero_ir2_opnd, done);
    li_d(mask, UINT64_C(0x8000000000000000));
    la_and(result, result, mask);
    la_ori(flags, flags, 0x30);

    la_label(done);
    ra_free_temp(field);
    ra_free_temp(mask);
}

void helper_raise_simd_exception(uint32_t flags)
{
    CPUX86State *env = (CPUX86State *)lsenv->cpu_state;
    CPUState *cs = env_cpu(env);

    cs->exception_index = EXCP13_XM;
    env->error_code = flags;
    env->exception_is_int = 0;
    env->exception_next_eip = env->eip;
    cpu_loop_exit(cs);
}

static void vmulsd_limit_flags_at_unmasked_exception(IR2_OPND flags,
                                                     IR2_OPND unmasked)
{
    IR2_OPND test = ra_alloc_itemp();
    IR2_OPND denormal = ra_alloc_label();
    IR2_OPND divide = ra_alloc_label();
    IR2_OPND overflow = ra_alloc_label();
    IR2_OPND underflow = ra_alloc_label();
    IR2_OPND precision = ra_alloc_label();
    IR2_OPND done = ra_alloc_label();

    la_andi(test, unmasked, 0x1);
    la_beq(test, zero_ir2_opnd, denormal);
    la_andi(flags, flags, 0x1);
    la_b(done);

    la_label(denormal);
    la_andi(test, unmasked, 0x2);
    la_beq(test, zero_ir2_opnd, divide);
    la_andi(flags, flags, 0x3);
    la_b(done);

    la_label(divide);
    la_andi(test, unmasked, 0x4);
    la_beq(test, zero_ir2_opnd, overflow);
    la_andi(flags, flags, 0x7);
    la_b(done);

    la_label(overflow);
    la_andi(test, unmasked, 0x8);
    la_beq(test, zero_ir2_opnd, underflow);
    la_andi(flags, flags, 0xf);
    la_b(done);

    la_label(underflow);
    la_andi(test, unmasked, 0x10);
    la_beq(test, zero_ir2_opnd, precision);
    la_andi(flags, flags, 0x1f);
    la_b(done);

    la_label(precision);
    la_andi(flags, flags, 0x3f);
    la_label(done);
    ra_free_temp(test);
}

static void vmulsd_raise_unmasked_exception(IR1_INST *pir1, IR2_OPND mxcsr,
                                            IR2_OPND flags)
{
    IR2_OPND masks = ra_alloc_itemp();
    IR2_OPND unmasked = ra_alloc_itemp();
    IR2_OPND no_exception = ra_alloc_label();

    la_srli_w(masks, mxcsr, 7);
    la_xori(masks, masks, 0x3f);
    la_and(unmasked, flags, masks);
    ra_free_temp(masks);
    la_beq(unmasked, zero_ir2_opnd, no_exception);

    vmulsd_limit_flags_at_unmasked_exception(flags, unmasked);
    la_or(mxcsr, mxcsr, flags);
    la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));

    IR2_OPND eip = ra_alloc_dbt_arg2();
    IR2_OPND helper = ra_alloc_itemp();
    li_d(eip, ir1_addr(pir1));
    la_store_addrx(eip, env_ir2_opnd, lsenv_offset_of_eip(lsenv));
    tr_save_registers_to_env(0xff, 0xff, option_save_xmm,
                             options_to_save());
#ifdef TARGET_X86_64
    tr_save_x64_8_registers_to_env(0xff, option_save_xmm);
#endif
    tr_save_ymm_to_env(UINT16_MAX);
    aot_load_host_addr(helper, (ADDR)helper_raise_simd_exception,
                       LOAD_HELPER_RAISE_SIMD_EXCEPTION, 0);
    la_mov64(a0_ir2_opnd, unmasked);
    la_jirl(zero_ir2_opnd, helper, 0);

    ra_free_temp(helper);
    la_label(no_exception);
    ra_free_temp(unmasked);
}

bool translate_vmulsd_lsx(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    lsassert(ir1_opnd_is_xmm(opnd2) ||
             (ir1_opnd_is_mem(opnd2) && ir1_opnd_size(opnd2) == 64));

    {
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        IR2_OPND src1 = ra_alloc_ftemp();
        IR2_OPND src1_low = ra_alloc_itemp();
        IR2_OPND src2_low = ra_alloc_itemp();
        IR2_OPND mxcsr = ra_alloc_itemp();
        IR2_OPND flags = ra_alloc_itemp();

        /* Preserve all register sources before an aliased destination is written. */
        la_vori_b(src1, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1)), 0);
        la_vpickve2gr_du(src1_low, src1, 0);
        if (ir1_opnd_is_mem(opnd2)) {
            IR2_OPND memory_value = load_u64_from_ir1_mem_exact(opnd2);

            la_or(src2_low, memory_value, zero_ir2_opnd);
            ra_free_temp(memory_value);
        } else {
            la_vpickve2gr_du(
                src2_low, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd2)), 0);
        }

        la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
        la_or(flags, zero_ir2_opnd, zero_ir2_opnd);
        vmulsd_apply_daz(src1_low, mxcsr, flags);
        vmulsd_apply_daz(src2_low, mxcsr, flags);

        IR2_OPND src1_calc = ra_alloc_ftemp();
        IR2_OPND src2_calc = ra_alloc_ftemp();
        la_vxor_v(src1_calc, src1_calc, src1_calc);
        la_vinsgr2vr_d(src1_calc, src1_low, 0);
        la_vxor_v(src2_calc, src2_calc, src2_calc);
        la_vinsgr2vr_d(src2_calc, src2_low, 0);

        IR2_OPND fcsr = ra_alloc_itemp();
        IR2_OPND fcsr_save = ra_alloc_itemp();
        IR2_OPND rounding_bit13 = ra_alloc_label();
        IR2_OPND rounding_bit14 = ra_alloc_label();
        IR2_OPND rounding_ready = ra_alloc_label();
        la_movfcsr2gr(fcsr_save, fcsr_ir2_opnd);
        la_bstrpick_w(fcsr, mxcsr, 13, 13);
        la_bne(fcsr, zero_ir2_opnd, rounding_bit13);
        la_bstrpick_w(fcsr, mxcsr, 14, 14);
        la_bne(fcsr, zero_ir2_opnd, rounding_bit14);
        la_or(fcsr, zero_ir2_opnd, zero_ir2_opnd);
        la_b(rounding_ready);

        la_label(rounding_bit13);
        la_bstrpick_w(fcsr, mxcsr, 14, 14);
        la_bne(fcsr, zero_ir2_opnd, rounding_bit14);
        li_wu(fcsr, UINT32_C(0x300));
        la_b(rounding_ready);

        la_label(rounding_bit14);
        la_bstrpick_w(fcsr, mxcsr, 13, 13);
        IR2_OPND rounding_both_set = ra_alloc_label();
        la_bne(fcsr, zero_ir2_opnd, rounding_both_set);
        li_wu(fcsr, UINT32_C(0x200));
        la_b(rounding_ready);
        la_label(rounding_both_set);
        li_wu(fcsr, UINT32_C(0x100));
        la_label(rounding_ready);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr);

        IR2_OPND product = ra_alloc_ftemp();
        la_fmul_d(product, src1_calc, src2_calc);
        ra_free_temp(src2_calc);
        ra_free_temp(src1_calc);
        la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);
        ra_free_temp(fcsr_save);
        vmulsd_map_fcsr_flags(fcsr, flags);
        ra_free_temp(fcsr);

        IR2_OPND product_low = ra_alloc_itemp();
        la_vpickve2gr_du(product_low, product, 0);
        ra_free_temp(product);
        vmulsd_mark_unmasked_exact_underflow(product_low, mxcsr, flags);
        vmulsd_fix_special_result(
            src1_low, src2_low, product_low, mxcsr, flags);
        ra_free_temp(src2_low);
        ra_free_temp(src1_low);
        vmulsd_raise_unmasked_exception(pir1, mxcsr, flags);
        la_or(mxcsr, mxcsr, flags);
        la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));

        ra_free_temp(flags);
        ra_free_temp(mxcsr);

        IR2_OPND src1_high = ra_alloc_itemp();
        la_vpickve2gr_du(src1_high, src1, 1);
        ra_free_temp(src1);
        IR2_OPND result = ra_alloc_ftemp();
        la_vxor_v(result, result, result);
        la_vinsgr2vr_d(result, product_low, 0);
        la_vinsgr2vr_d(result, src1_high, 1);
        la_vori_b(ra_alloc_xmm(dest_index), result, 0);
        clear_ymm_high128_shadow(dest_index);

        ra_free_temp(result);
        ra_free_temp(src1_high);
        ra_free_temp(product_low);
    }
    return true;
}

bool translate_vmulsd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    int dest_index = ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0));
    IR2_OPND mxcsr = ra_alloc_itemp();
    IR2_OPND flags = ra_alloc_itemp();
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND src1_low = ra_alloc_itemp();
    IR2_OPND src2_low = ra_alloc_itemp();
    IR2_OPND temp = ra_alloc_ftemp();

    la_vpickve2gr_du(src1_low, src1, 0);
    la_vpickve2gr_du(src2_low, src2, 0);
    la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    la_or(flags, zero_ir2_opnd, zero_ir2_opnd);
    vmulsd_apply_daz(src1_low, mxcsr, flags);
    vmulsd_apply_daz(src2_low, mxcsr, flags);

    IR2_OPND src1_calc = ra_alloc_ftemp();
    IR2_OPND src2_calc = ra_alloc_ftemp();
    la_vxor_v(src1_calc, src1_calc, src1_calc);
    la_vinsgr2vr_d(src1_calc, src1_low, 0);
    la_vxor_v(src2_calc, src2_calc, src2_calc);
    la_vinsgr2vr_d(src2_calc, src2_low, 0);

    IR2_OPND fcsr = ra_alloc_itemp();
    IR2_OPND fcsr_save = ra_alloc_itemp();
    la_movfcsr2gr(fcsr_save, fcsr_ir2_opnd);
    la_fmul_d(temp, src1_calc, src2_calc);
    ra_free_temp(src2_calc);
    ra_free_temp(src1_calc);
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);
    vmulsd_map_fcsr_flags(fcsr, flags);
    ra_free_temp(fcsr);
    ra_free_temp(fcsr_save);

    IR2_OPND product_low = ra_alloc_itemp();
    la_vpickve2gr_du(product_low, temp, 0);
    vmulsd_mark_unmasked_exact_underflow(product_low, mxcsr, flags);
    vmulsd_fix_special_result(src1_low, src2_low, product_low, mxcsr, flags);
    la_vshuf4i_d(temp, src1, 0xc);
    la_vinsgr2vr_d(temp, product_low, 0);
    ra_free_temp(src2_low);
    ra_free_temp(src1_low);
    ra_free_temp(product_low);
    vmulsd_raise_unmasked_exception(pir1, mxcsr, flags);
    la_or(mxcsr, mxcsr, flags);
    la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));

    set_high128_xreg_to_zero(temp);
    la_xvori_b(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    clear_ymm_high128_shadow(dest_index);
    ra_free_temp(flags);
    ra_free_temp(mxcsr);
    return true;
}

bool translate_vmulss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();

    la_fmul_s(temp, src1, src2);
    if (ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)) !=
        ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vdivpd(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    IR2_OPND src2;

    if (ir1_opnd_is_ymm(opnd0)) {
        src2 = load_freg256_from_ir1(opnd2);
        la_xvfdiv_d(dest, src1, src2);
    } else if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND temp = ra_alloc_ftemp();

        src2 = load_freg128_from_ir1(opnd2);
        la_vfdiv_d(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vdivps(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) && ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    IR2_OPND src2;

    if (ir1_opnd_is_ymm(opnd0)) {
        src2 = load_freg256_from_ir1(opnd2);
        la_xvfdiv_s(dest, src1, src2);
    } else if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND temp = ra_alloc_ftemp();

        src2 = load_freg128_from_ir1(opnd2);
        la_vfdiv_s(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vdivsd_lsx(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    lsassert(ir1_opnd_is_xmm(opnd2) ||
             (ir1_opnd_is_mem(opnd2) && ir1_opnd_size(opnd2) == 64));
    {
        /* LSX-only path */
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        IR2_OPND fcsr_opnd = set_fpu_fcsr_rounding_field_by_x86();
        IR2_OPND src1 = ra_alloc_ftemp();
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND src1_low = ra_alloc_itemp();
        IR2_OPND src2_low = ra_alloc_itemp();
        IR2_OPND mxcsr = ra_alloc_itemp();
        IR2_OPND flags = ra_alloc_itemp();
        IR2_OPND quotient = ra_alloc_ftemp();
        IR2_OPND quotient_low = ra_alloc_itemp();
        IR2_OPND nonzero_div = ra_alloc_label();
        IR2_OPND force_qnan = ra_alloc_label();
        IR2_OPND result_ready = ra_alloc_label();

        /* Read all sources before changing FCSR or an aliased dest. */
        la_vori_b(src1, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1)), 0);
        la_vpickve2gr_du(src1_low, src1, 0);
        la_vpickve2gr_du(src2_low, src2, 0);
        la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
        la_or(flags, zero_ir2_opnd, zero_ir2_opnd);
        vmulsd_apply_daz(src1_low, mxcsr, flags);
        vmulsd_apply_daz(src2_low, mxcsr, flags);
        vmulsd_mark_snan(src1_low, flags);
        vmulsd_mark_snan(src2_low, flags);
        la_vinsgr2vr_d(src1, src1_low, 0);
        la_vinsgr2vr_d(src1, zero_ir2_opnd, 1);
        la_vinsgr2vr_d(src2, src2_low, 0);
        la_vinsgr2vr_d(src2, zero_ir2_opnd, 1);

        la_or(src1_low, src1_low, src2_low);
        la_bne(src1_low, zero_ir2_opnd, nonzero_div);
        li_d(src1_low, 1);
        la_bstrins_w(flags, src1_low, 31, 31);
        la_label(nonzero_div);
        la_movfcsr2gr(src2_low, fcsr_ir2_opnd);
        la_bstrins_w(src2_low, zero_ir2_opnd,
                     FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_I);
        la_bstrins_w(src2_low, zero_ir2_opnd,
                     FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_I);
        la_movgr2fcsr(fcsr_ir2_opnd, src2_low);
        /* Keep the inactive LSX lane non-exceptional. */
        li_d(src1_low, UINT64_C(0x3ff0000000000000));
        li_d(src2_low, UINT64_C(0x3ff0000000000000));
        la_vinsgr2vr_d(src1, src1_low, 1);
        la_vinsgr2vr_d(src2, src2_low, 1);
        la_vfdiv_d(quotient, src1, src2);
        la_vpickve2gr_du(quotient_low, quotient, 0);
        la_blt(flags, zero_ir2_opnd, force_qnan);
        la_b(result_ready);
        la_label(force_qnan);
        li_d(quotient_low, UINT64_C(0xfff8000000000000));
        la_label(result_ready);
        la_bstrins_w(flags, zero_ir2_opnd, 31, 31);
        la_movfcsr2gr(src2_low, fcsr_ir2_opnd);
        set_fpu_rounding_mode(fcsr_opnd);
        ra_free_temp_auto(fcsr_opnd);
        vmulsd_map_fcsr_flags(src2_low, flags);
        vmulsd_mark_unmasked_exact_underflow(quotient_low, mxcsr, flags);
        ra_free_temp(quotient);
        ra_free_temp(src2_low);
        ra_free_temp(src1_low);
        vmulsd_raise_unmasked_exception(pir1, mxcsr, flags);
        la_or(mxcsr, mxcsr, flags);
        la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
        ra_free_temp(flags);
        ra_free_temp(mxcsr);
        IR2_OPND src1_high = ra_alloc_itemp();
        la_vpickve2gr_du(src1_high,
                         ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1)), 1);
        la_vinsgr2vr_d(src1, quotient_low, 0);
        la_vinsgr2vr_d(src1, src1_high, 1);
        la_vori_b(ra_alloc_xmm(dest_index), src1, 0);
        clear_ymm_high128_shadow(dest_index);
        ra_free_temp(quotient_low);
        ra_free_temp(src1_high);
        ra_free_temp(src1);
    }
    return true;
}

bool translate_vdivsd(IR1_INST * pir1) {
    IR2_OPND fcsr_opnd = set_fpu_fcsr_rounding_field_by_x86();
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND src1_low = ra_alloc_itemp();
    IR2_OPND src2_low = ra_alloc_itemp();
    IR2_OPND mxcsr = ra_alloc_itemp();
    IR2_OPND flags = ra_alloc_itemp();
    IR2_OPND fcsr = ra_alloc_itemp();
    IR2_OPND src1_calc = ra_alloc_ftemp();
    IR2_OPND src2_calc = ra_alloc_ftemp();
    IR2_OPND temp = ra_alloc_ftemp();

    la_vpickve2gr_du(src1_low, src1, 0);
    la_vpickve2gr_du(src2_low, src2, 0);
    la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    la_or(flags, zero_ir2_opnd, zero_ir2_opnd);
    vmulsd_apply_daz(src1_low, mxcsr, flags);
    vmulsd_apply_daz(src2_low, mxcsr, flags);
    la_vxor_v(src1_calc, src1_calc, src1_calc);
    la_vinsgr2vr_d(src1_calc, src1_low, 0);
    la_vxor_v(src2_calc, src2_calc, src2_calc);
    la_vinsgr2vr_d(src2_calc, src2_low, 0);

    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_bstrins_w(fcsr, zero_ir2_opnd,
                 FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_I);
    la_bstrins_w(fcsr, zero_ir2_opnd,
                 FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_I);
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
    la_fdiv_d(temp, src1_calc, src2_calc);
    ra_free_temp(src2_calc);
    ra_free_temp(src1_calc);
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    set_fpu_rounding_mode(fcsr_opnd);
    ra_free_temp_auto(fcsr_opnd);
    vmulsd_map_fcsr_flags(fcsr, flags);
    ra_free_temp(fcsr);
    vmulsd_mark_snan(src1_low, flags);
    vmulsd_mark_snan(src2_low, flags);
    IR2_OPND quotient_low = ra_alloc_itemp();
    la_vpickve2gr_du(quotient_low, temp, 0);
    vmulsd_mark_unmasked_exact_underflow(quotient_low, mxcsr, flags);
    ra_free_temp(src2_low);
    ra_free_temp(src1_low);
    vmulsd_raise_unmasked_exception(pir1, mxcsr, flags);
    la_or(mxcsr, mxcsr, flags);
    la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    la_vshuf4i_d(temp, src1, 0xc);
    la_vinsgr2vr_d(temp, quotient_low, 0);
    set_high128_xreg_to_zero(temp);
    la_xvori_b(dest, temp, 0);
    ra_free_temp(quotient_low);
    ra_free_temp(flags);
    ra_free_temp(mxcsr);
    ra_free_temp(temp);
    return true;
}

bool translate_vdivss(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_freg128_from_ir1(opnd2);
    IR2_OPND temp = ra_alloc_ftemp();
    la_fdiv_s(temp, src1, src2);
    if (ir1_opnd_base_reg_num(opnd0) != ir1_opnd_base_reg_num(opnd1)) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vsqrtpd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));

        la_xvfsqrt_d(dest, src);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vfsqrt_d(temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vsqrtps(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));

        la_xvfsqrt_s(dest, src);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vfsqrt_s(temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vsqrtsd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_freg128_from_ir1(opnd2);
    IR2_OPND temp = ra_alloc_ftemp();
    la_fsqrt_d(temp, src2);
    if (ir1_opnd_base_reg_num(opnd0) != ir1_opnd_base_reg_num(opnd1)) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_d(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vsqrtss(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_freg128_from_ir1(opnd2);
    IR2_OPND temp = ra_alloc_ftemp();
    la_fsqrt_s(temp, src2);
    if (ir1_opnd_base_reg_num(opnd0) != ir1_opnd_base_reg_num(opnd1)) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vaddsubpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND add_src1 = ra_alloc_ftemp();
    IR2_OPND add_src2 = ra_alloc_ftemp();
    IR2_OPND sub_src1 = ra_alloc_ftemp();
    IR2_OPND sub_src2 = ra_alloc_ftemp();

    la_xvpackev_d(sub_src1, src1, src1);
    la_xvpackev_d(sub_src2, src2, src2);
    la_xvpackod_d(add_src1, src1, src1);
    la_xvpackod_d(add_src2, src2, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        la_vfsub_d(sub_src1, sub_src1, sub_src2);
        la_vfadd_d(add_src1, add_src1, add_src2);
    } else {
        la_xvfsub_d(sub_src1, sub_src1, sub_src2);
        la_xvfadd_d(add_src1, add_src1, add_src2);
    }
    la_xvpackev_d(dest, add_src1, sub_src1);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vaddsubps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);

    IR2_OPND add_src1 = ra_alloc_ftemp();
    IR2_OPND add_src2 = ra_alloc_ftemp();
    IR2_OPND sub_src1 = ra_alloc_ftemp();
    IR2_OPND sub_src2 = ra_alloc_ftemp();
    la_xvpackev_w(sub_src1, src1, src1);
    la_xvpackev_w(sub_src2, src2, src2);
    la_xvpackod_w(add_src1, src1, src1);
    la_xvpackod_w(add_src2, src2, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        la_vfsub_s(sub_src1, sub_src1, sub_src2);
        la_vfadd_s(add_src1, add_src1, add_src2);
    } else {
        la_xvfsub_s(sub_src1, sub_src1, sub_src2);
        la_xvfadd_s(add_src1, add_src1, add_src2);
    }
    la_xvpackev_w(dest, add_src1, sub_src1);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }

    return true;
}

bool translate_vhaddpd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_xvpickev_d(temp1, src2, src1);
        la_xvpickod_d(temp2, src2, src1);
        la_xvfadd_d(dest, temp1, temp2);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_vpickev_d(temp1, src2, src1);
        la_vpickod_d(temp2, src2, src1);
        la_vfadd_d(temp, temp1, temp2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vhaddps(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_xvpickev_w(temp1, src2, src1);
        la_xvpickod_w(temp2, src2, src1);
        la_xvfadd_s(dest, temp1, temp2);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_vpickev_w(temp1, src2, src1);
        la_vpickod_w(temp2, src2, src1);
        la_vfadd_s(temp, temp1, temp2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vhsubpd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_xvpickev_d(temp1, src2, src1);
        la_xvpickod_d(temp2, src2, src1);
        la_xvfsub_d(dest, temp1, temp2);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_vpickev_d(temp1, src2, src1);
        la_vpickod_d(temp2, src2, src1);
        la_vfsub_d(temp, temp1, temp2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vhsubps(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_xvpickev_w(temp1, src2, src1);
        la_xvpickod_w(temp2, src2, src1);
        la_xvfsub_s(dest, temp1, temp2);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_vpickev_w(temp1, src2, src1);
        la_vpickod_w(temp2, src2, src1);
        la_vfsub_s(temp, temp1, temp2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vandnpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));

        la_xvandn_v(dest, src1, src2);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vandn_v(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vandnps(IR1_INST * pir1) {
    translate_vandnpd(pir1);
    return true;
}

bool translate_vandpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));

        la_xvand_v(dest, src1, src2);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vand_v(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vandps(IR1_INST * pir1) {
    translate_vandpd(pir1);
    return true;
}

bool translate_vorps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));

        la_xvor_v(dest, src1, src2);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vor_v(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vorpd(IR1_INST * pir1) {
    translate_vorps(pir1);
    return true;
}

bool translate_vxorps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));

        la_xvxor_v(dest, src1, src2);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vxor_v(temp, src1, src2);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vxorpd_lsx(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);

    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
             (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    {
        /* LSX-only path */
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        int src1_index = ir1_opnd_base_reg_num(opnd1);
        IR2_OPND src1_low = ra_alloc_ftemp();

        /* Materialize both sources before writing a possibly aliased dest. */
        la_vori_b(src1_low, ra_alloc_xmm(src1_index), 0);
        if (ir1_opnd_is_xmm(opnd0)) {
            IR2_OPND src2_low;

            if (ir1_opnd_is_mem(opnd2)) {
                src2_low = load_v128_from_ir1_mem_exact(opnd2);
            } else {
                lsassert(ir1_opnd_is_xmm(opnd2));
                src2_low = ra_alloc_ftemp();
                la_vori_b(src2_low,
                          ra_alloc_xmm(ir1_opnd_base_reg_num(opnd2)), 0);
            }
            la_vxor_v(src1_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            clear_ymm_high128_shadow(dest_index);
            ra_free_temp(src2_low);
            ra_free_temp(src1_low);
        } else {
            IR2_OPND src1_high;
            IR2_OPND src2_low;
            IR2_OPND src2_high;

            if (ir1_opnd_is_mem(opnd2)) {
                load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
            } else {
                int src2_index;

                lsassert(ir1_opnd_is_ymm(opnd2));
                src2_index = ir1_opnd_base_reg_num(opnd2);
                src2_low = ra_alloc_ftemp();
                la_vori_b(src2_low, ra_alloc_xmm(src2_index), 0);
                src2_high = load_ymm_high128_shadow(src2_index);
            }
            src1_high = load_ymm_high128_shadow(src1_index);
            la_vxor_v(src1_low, src1_low, src2_low);
            la_vxor_v(src1_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
            ra_free_temp(src2_high);
            ra_free_temp(src1_high);
            ra_free_temp(src2_low);
            ra_free_temp(src1_low);
        }
    }
    return true;
}

bool translate_vxorpd(IR1_INST * pir1) {
    translate_vxorps(pir1);
    return true;
}

bool translate_vminsx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)));
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_freg128_from_ir1(opnd2);
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, int);
    IR2_INST * ( * tr_insert)(IR2_OPND, IR2_OPND, int);
    IR2_OPND temp = ra_alloc_ftemp();
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VMINSS:
            tr_inst = la_fcmp_cond_s;
            tr_insert = la_xvinsve0_w;
            break;
        case dt_X86_INS_VMINSD:
            tr_inst = la_fcmp_cond_d;
            tr_insert = la_xvinsve0_d;
            break;
        default:
            tr_inst = NULL;
            tr_insert =NULL;
            lsassert(0);
            break;
    }
    tr_inst(fcc0_ir2_opnd, src1, src2, 0x3);
    la_fsel(temp, src2, src1, fcc0_ir2_opnd);
    if (ir1_opnd_base_reg_num(opnd0) != ir1_opnd_base_reg_num(opnd1)) {
        la_xvori_b(dest, src1, 0);
    }
    tr_insert(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vminpx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_INST * ( * tr_inst_128)(IR2_OPND, IR2_OPND, IR2_OPND, int);
    IR2_INST * ( * tr_inst_256)(IR2_OPND, IR2_OPND, IR2_OPND, int);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND mask = ra_alloc_ftemp();
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VMINPS:
            tr_inst_256 = la_xvfcmp_cond_s;
            tr_inst_128 = la_vfcmp_cond_s;
            break;
        case dt_X86_INS_VMINPD:
            tr_inst_256 = la_xvfcmp_cond_d;
            tr_inst_128 = la_vfcmp_cond_d;
            break;
        default:
            tr_inst_256 = NULL;
            tr_inst_128 = NULL;
            lsassert(0);
            break;
    }
    if (ir1_opnd_is_ymm(opnd0))
        tr_inst_256(mask, src1, src2, 0x3);
    else
        tr_inst_128(mask, src1, src2, 0x3);
    la_xvand_v(temp, src1, mask);
    la_xvandn_v(mask, mask, src2);
    la_xvor_v(dest, temp, mask);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vmaxsx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)));
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_freg128_from_ir1(opnd2);
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, int);
    IR2_INST * ( * tr_insert)(IR2_OPND, IR2_OPND, int);
    IR2_OPND temp = ra_alloc_ftemp();
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VMAXSS:
            tr_inst = la_fcmp_cond_s;
            tr_insert = la_xvinsve0_w;
            break;
        case dt_X86_INS_VMAXSD:
            tr_inst = la_fcmp_cond_d;
            tr_insert = la_xvinsve0_d;
            break;
        default:
            tr_inst = NULL;
            tr_insert = NULL;
            lsassert(0);
            break;
    }
    tr_inst(fcc0_ir2_opnd, src2, src1, 0x3);
    la_fsel(temp, src2, src1, fcc0_ir2_opnd);
    if (ir1_opnd_base_reg_num(opnd0) != ir1_opnd_base_reg_num(opnd1)) {
        la_xvori_b(dest, src1, 0);
    }
    tr_insert(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vmaxpx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_INST * ( * tr_inst_128)(IR2_OPND, IR2_OPND, IR2_OPND, int);
    IR2_INST * ( * tr_inst_256)(IR2_OPND, IR2_OPND, IR2_OPND, int);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND mask = ra_alloc_ftemp();
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VMAXPS:
            tr_inst_256 = la_xvfcmp_cond_s;
            tr_inst_128 = la_vfcmp_cond_s;
            break;
        case dt_X86_INS_VMAXPD:
            tr_inst_256 = la_xvfcmp_cond_d;
            tr_inst_128 = la_vfcmp_cond_d;
            break;
        default:
            tr_inst_256 = NULL;
            tr_inst_128 = NULL;
            lsassert(0);
            break;
    }
    if (ir1_opnd_is_ymm(opnd0))
        tr_inst_256(mask, src2, src1, 0x3);
    else
        tr_inst_128(mask, src2, src1, 0x3);
    la_xvand_v(temp, src1, mask);
    la_xvandn_v(mask, mask, src2);
    la_xvor_v(dest, temp, mask);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vblendvpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);

    lsassert((ir1_opnd_is_xmm(opnd0) &&
            ir1_opnd_is_xmm(opnd1) &&
            ir1_opnd_is_xmm(opnd3)) ||
        (ir1_opnd_is_ymm(opnd0) &&
            ir1_opnd_is_ymm(opnd1) &&
            ir1_opnd_is_ymm(opnd3)));
    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND src3 = load_freg256_from_ir1(opnd3);
        IR2_OPND temp = ra_alloc_ftemp();
        la_xvslti_d(temp, src3, 0);
        la_xvbitsel_v(dest, src1, src2, temp);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND src3 = load_freg128_from_ir1(opnd3);
        IR2_OPND temp = ra_alloc_ftemp();
        la_vslti_d(temp, src3, 0);
        la_vbitsel_v(temp, src1, src2, temp);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vblendpd(IR1_INST * pir1) {
    lsassert(ir1_opnd_num(pir1) == 4 &&
        ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
            ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) &&
            ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));
    IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    IR2_OPND temp = ra_alloc_ftemp();
    la_xvori_b(temp, src1, 0);
    if (imm & 0x1)
        la_vextrins_d(temp, src2, VEXTRINS_IMM_4_0(0, 0));
    if (imm & 0x2)
        la_vextrins_d(temp, src2, VEXTRINS_IMM_4_0(1, 1));
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_xvori_b(temp2, src1, 0);
        if (imm & 0x4)
            la_xvextrins_d(temp2, src2, VEXTRINS_IMM_4_0(0, 0));
        if (imm & 0x8)
            la_xvextrins_d(temp2, src2, VEXTRINS_IMM_4_0(1, 1));
        la_xvpermi_q(temp, temp2, VEXTRINS_IMM_4_0(1, 2));
    }
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vblendps(IR1_INST * pir1) {
    lsassert(ir1_opnd_num(pir1) == 4 &&
        ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
            ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) &&
            ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));
    IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND rmask = ra_alloc_itemp();


    uint64_t mask = 0;
    for (int i = 0; i < 8; i++) {
        if (imm & (1 << i)) {
            mask |= (0xFFULL) << (i * 8);
        }
    }

    li_d(rmask, mask);
    la_movgr2fr_d(temp, rmask);
    la_vext2xv_w_b(temp, temp);
    la_xvbitsel_v(dest, src1, src2, temp);

    if (ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vblendvps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);
    lsassert((ir1_opnd_is_xmm(opnd0) &&
            ir1_opnd_is_xmm(opnd1) &&
            ir1_opnd_is_xmm(opnd3)) ||
        (ir1_opnd_is_ymm(opnd0) &&
            ir1_opnd_is_ymm(opnd1) &&
            ir1_opnd_is_ymm(opnd3)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND src3 = load_freg256_from_ir1(opnd3);
    IR2_OPND temp = ra_alloc_ftemp();
    la_xvslti_w(temp, src3, 0);
    la_xvbitsel_v(dest, src1, src2, temp);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

static IR2_OPND load_broadcast_scalar_lsx(IR1_OPND *opnd, int bits)
{
    if (ir1_opnd_is_mem(opnd)) {
        switch (bits) {
        case 8:
            return load_u8_from_ir1_mem_exact(opnd);
        case 16:
            return load_u16_from_ir1_mem_exact(opnd);
        case 32:
            return load_u32_from_ir1_mem_exact(opnd);
        case 64:
            return load_u64_from_ir1_mem_exact(opnd);
        default:
            lsassert(0);
        }
    }

    IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd));
    IR2_OPND value = ra_alloc_itemp();

    switch (bits) {
    case 8:
        la_vpickve2gr_bu(value, src, 0);
        break;
    case 16:
        la_vpickve2gr_hu(value, src, 0);
        break;
    case 32:
        la_vpickve2gr_w(value, src, 0);
        break;
    case 64:
        la_vpickve2gr_du(value, src, 0);
        break;
    default:
        lsassert(0);
    }
    return value;
}

static IR2_OPND replicate_scalar_lsx(IR2_OPND value, int bits)
{
    IR2_OPND result = ra_alloc_ftemp();
    int lanes;

    la_vxor_v(result, result, result);
    if (bits == 64) {
        la_vreplgr2vr_d(result, value);
        return result;
    }

    lanes = 128 / bits;
    for (int i = 0; i < lanes; ++i) {
        switch (bits) {
        case 8:
            la_vinsgr2vr_b(result, value, i);
            break;
        case 16:
            la_vinsgr2vr_h(result, value, i);
            break;
        case 32:
            la_vinsgr2vr_w(result, value, i);
            break;
        default:
            lsassert(0);
        }
    }
    return result;
}

static bool translate_vbroadcast_scalar_lsx(IR1_INST *pir1, int bits)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR2_OPND value = load_broadcast_scalar_lsx(ir1_get_opnd(pir1, 1), bits);
    IR2_OPND result = replicate_scalar_lsx(value, bits);
    int dest_index;

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ir1_opnd_is_ymm(dest_opnd));
    dest_index = ir1_opnd_base_reg_num(dest_opnd);
    la_vori_b(ra_alloc_xmm(dest_index), result, 0);
    if (ir1_opnd_is_xmm(dest_opnd)) {
        clear_ymm_high128_shadow(dest_index);
    } else {
        store_ymm_high128_shadow(result, dest_index);
    }
    ra_free_temp(result);
    ra_free_temp(value);
    return true;
}

static bool translate_vbroadcast128_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    IR2_OPND src;
    int dest_index;

    lsassert(ir1_opnd_is_ymm(dest_opnd));
    lsassert(ir1_opnd_size(src_opnd) == 128);
    src = ir1_opnd_is_mem(src_opnd) ?
        load_v128_from_ir1_mem_exact(src_opnd) :
        ra_alloc_xmm(ir1_opnd_base_reg_num(src_opnd));
    dest_index = ir1_opnd_base_reg_num(dest_opnd);
    la_vori_b(ra_alloc_xmm(dest_index), src, 0);
    store_ymm_high128_shadow(src, dest_index);
    if (ir1_opnd_is_mem(src_opnd))
        ra_free_temp(src);
    return true;
}

bool translate_vbroadcastf128_lsx(IR1_INST *pir1)
{
    return translate_vbroadcast128_lsx(pir1);
}

bool translate_vbroadcasti128_lsx(IR1_INST *pir1)
{
    return translate_vbroadcast128_lsx(pir1);
}

bool translate_vbroadcastsd_lsx(IR1_INST *pir1)
{
    return translate_vbroadcast_scalar_lsx(pir1, 64);
}

bool translate_vbroadcastss_lsx(IR1_INST *pir1)
{
    return translate_vbroadcast_scalar_lsx(pir1, 32);
}

bool translate_vpbroadcastb_lsx(IR1_INST *pir1)
{
    return translate_vbroadcast_scalar_lsx(pir1, 8);
}

bool translate_vpbroadcastw_lsx(IR1_INST *pir1)
{
    return translate_vbroadcast_scalar_lsx(pir1, 16);
}

bool translate_vpbroadcastd_lsx(IR1_INST *pir1)
{
    return translate_vbroadcast_scalar_lsx(pir1, 32);
}

bool translate_vpbroadcastq_lsx(IR1_INST *pir1)
{
    return translate_vbroadcast_scalar_lsx(pir1, 64);
}

bool translate_vbroadcastsd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));
    IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    la_xvreplve0_d(dest, src);
    return true;
}

bool translate_vbroadcastss(IR1_INST * pir1) {
#ifdef CONFIG_LATX_TS
    if (!ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        !ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        return false;
    }
#else
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));
#endif
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        la_xvreplve0_w(dest, src);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();
        la_xvreplve0_w(temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vbroadcastf128(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));
    IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    la_xvreplve0_q(dest, src);
    return true;
}

bool translate_vbroadcasti128(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));
    IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    la_xvreplve0_q(dest, src);
    return true;
}



bool translate_vextractf128(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_mem(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)) &&
        ir1_opnd_is_imm(ir1_get_opnd(pir1, 2)));
    IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 2)) & 0x1;
    if (ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND temp = ra_alloc_ftemp();
        if (!imm) {
            la_xvpermi_q(temp, src, VEXTRINS_IMM_4_0(3, 0));
        } else {
            la_xvpermi_q(temp, src, VEXTRINS_IMM_4_0(3, 1));
        }
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    } else {
        if (!imm) {
            store_freg128_to_ir1_mem(src, ir1_get_opnd(pir1, 0));
        } else {
            IR2_OPND temp = ra_alloc_ftemp();
            la_xvpermi_q(temp, src, VEXTRINS_IMM_4_0(3, 1));
            store_freg128_to_ir1_mem(temp, ir1_get_opnd(pir1, 0));
        }
    }
    return true;
}

static bool translate_vextract128_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 2)) & 0x1;
    int src_index = ir1_opnd_base_reg_num(src_opnd);
    IR2_OPND src = imm ? load_ymm_high128_shadow(src_index) :
        ra_alloc_xmm(src_index);

    lsassert(ir1_opnd_is_xmm(dest) || ir1_opnd_is_mem(dest));
    lsassert(ir1_opnd_is_ymm(src_opnd) &&
             ir1_opnd_is_imm(ir1_get_opnd(pir1, 2)));
    if (ir1_opnd_is_xmm(dest)) {
        int dest_index = ir1_opnd_base_reg_num(dest);

        la_vori_b(ra_alloc_xmm(dest_index), src, 0);
        clear_ymm_high128_shadow(dest_index);
    } else {
        store_v128_to_ir1_mem_exact(src, dest);
    }
    if (imm)
        ra_free_temp(src);
    return true;
}

bool translate_vextractf128_lsx(IR1_INST *pir1)
{
    return translate_vextract128_lsx(pir1);
}

bool translate_vextracti128_lsx(IR1_INST *pir1)
{
    return translate_vextract128_lsx(pir1);
}

bool translate_vextracti128(IR1_INST * pir1) {
    return translate_vextractf128(pir1);
}

bool translate_vextractps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert(ir1_opnd_is_gpr(opnd0) || ir1_opnd_is_mem(opnd0));
    IR2_OPND src = load_freg128_from_ir1(opnd1);
    IR2_OPND temp = ra_alloc_ftemp();
    uint8_t imm = ir1_opnd_uimm(opnd2) & 0x3;
    la_xvpickve_w(temp, src, imm);
    if (ir1_opnd_is_gpr(opnd0)) {
        IR2_OPND dest = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd0));
        la_movfr2gr_d(dest, temp);
    } else {
        store_freg_to_ir1(temp, opnd0, false, false);
    }
    return true;
}

bool translate_vextractps_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 2));
    IR2_OPND src;
    IR2_OPND value = ra_alloc_itemp();

    lsassert(ir1_opnd_is_gpr(dest) || ir1_opnd_is_mem(dest));
    lsassert(ir1_opnd_is_xmm(src_opnd) || ir1_opnd_is_ymm(src_opnd));
    lsassert(ir1_opnd_is_imm(ir1_get_opnd(pir1, 2)));
    if (ir1_opnd_is_ymm(src_opnd) && (imm & 0x4)) {
        src = load_ymm_high128_shadow(ir1_opnd_base_reg_num(src_opnd));
    } else {
        src = ra_alloc_xmm(ir1_opnd_base_reg_num(src_opnd));
    }
    la_vpickve2gr_w(value, src, imm & 0x3);
    if (ir1_opnd_is_gpr(dest)) {
        store_ireg_to_ir1(value, dest, false);
    } else {
        store_u32_to_ir1_mem_exact(value, dest);
    }
    ra_free_temp(value);
    if (ir1_opnd_is_ymm(src_opnd) && (imm & 0x4))
        ra_free_temp(src);
    return true;
}

bool translate_vinsertps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert(ir1_opnd_num(pir1) == 4 &&
        ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_freg128_from_ir1(opnd2);
    IR2_OPND temp = ra_alloc_ftemp();
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    uint8_t count_s = (imm >> 6) & 0x3;
    uint8_t count_d = (imm >> 4) & 0x3;
    uint8_t zmask = imm & 0xf;
    if (ir1_opnd_is_mem(opnd2)) {
        count_s = 0;
    }
    la_vori_b(temp, src1, 0);
    la_vextrins_w(temp, src2, VEXTRINS_IMM_4_0(count_d, count_s));
    if (zmask & 0x1)
        la_vinsgr2vr_w(temp, zero_ir2_opnd, 0);
    if (zmask & 0x2)
        la_vinsgr2vr_w(temp, zero_ir2_opnd, 1);
    if (zmask & 0x4)
        la_vinsgr2vr_w(temp, zero_ir2_opnd, 2);
    if (zmask & 0x8)
        la_vinsgr2vr_w(temp, zero_ir2_opnd, 3);
    la_vori_b(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vinsertps_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src1_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *src2_opnd = ir1_get_opnd(pir1, 2);
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    uint8 source_lane = (imm >> 6) & 0x3;
    uint8 dest_lane = (imm >> 4) & 0x3;
    uint8 zero_mask = imm & 0xf;
    IR2_OPND src1;
    IR2_OPND value;
    IR2_OPND result;
    int dest_index;

    lsassert(ir1_opnd_is_xmm(dest_opnd) && ir1_opnd_is_xmm(src1_opnd));
    lsassert(ir1_opnd_is_xmm(src2_opnd) ||
             (ir1_opnd_is_mem(src2_opnd) && ir1_opnd_size(src2_opnd) == 32));
    lsassert(ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(src1_opnd));
    if (ir1_opnd_is_mem(src2_opnd)) {
        value = load_u32_from_ir1_mem_exact(src2_opnd);
        source_lane = 0;
    } else {
        value = ra_alloc_itemp();
        la_vpickve2gr_w(value, ra_alloc_xmm(
            ir1_opnd_base_reg_num(src2_opnd)), source_lane);
    }
    result = ra_alloc_ftemp();
    la_vori_b(result, src1, 0);
    la_vinsgr2vr_w(result, value, dest_lane);
    for (int i = 0; i < 4; ++i) {
        if (zero_mask & (1 << i)) {
            la_vinsgr2vr_w(result, zero_ir2_opnd, i);
        }
    }
    dest_index = ir1_opnd_base_reg_num(dest_opnd);
    la_vori_b(ra_alloc_xmm(dest_index), result, 0);
    clear_ymm_high128_shadow(dest_index);
    ra_free_temp(result);
    ra_free_temp(value);
    return true;
}

static bool translate_vinsert128_lsx(IR1_INST *pir1)
{
    lsassert(ir1_opnd_num(pir1) == 4);
    lsassert(ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)) &&
        ir1_opnd_size(ir1_get_opnd(pir1, 2)) == 128 &&
        ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    {
        /* LSX-only path */
        IR1_OPND *dest = ir1_get_opnd(pir1, 0);
        IR1_OPND *src1 = ir1_get_opnd(pir1, 1);
        IR1_OPND *src2 = ir1_get_opnd(pir1, 2);
        int dest_index = ir1_opnd_base_reg_num(dest);
        int src1_index = ir1_opnd_base_reg_num(src1);
        uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3)) & 1;
        IR2_OPND src1_low = ra_alloc_ftemp();
        IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
        IR2_OPND src2_value;

        la_vori_b(src1_low, ra_alloc_xmm(src1_index), 0);
        if (ir1_opnd_is_mem(src2)) {
            src2_value = load_v128_from_ir1_mem_exact(src2);
        } else {
            src2_value = ra_alloc_ftemp();
            la_vori_b(src2_value,
                      ra_alloc_xmm(ir1_opnd_base_reg_num(src2)), 0);
        }

        if (imm == 0) {
            la_vori_b(ra_alloc_xmm(dest_index), src2_value, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
        } else {
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src2_value, dest_index);
        }
        ra_free_temp(src2_value);
        ra_free_temp(src1_high);
        ra_free_temp(src1_low);
    }
    return true;
}

bool translate_vinsertf128_lsx(IR1_INST *pir1)
{
    return translate_vinsert128_lsx(pir1);
}

bool translate_vinserti128_lsx(IR1_INST *pir1)
{
    return translate_vinsert128_lsx(pir1);
}

bool translate_vinserti128(IR1_INST * pir1) {
    return translate_vinsertf128(pir1);
}

bool translate_vinsertf128(IR1_INST * pir1) {
    lsassert(ir1_opnd_num(pir1) == 4);
    lsassert(ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)) &&
        ir1_opnd_size(ir1_get_opnd(pir1, 2)) == 128 &&
        ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3)) & 0x1;
    IR2_OPND temp = ra_alloc_ftemp();
    la_xvori_b(temp, src1, 0);
    if (!imm) {
        la_xvpermi_q(temp, src2, VEXTRINS_IMM_4_0(3, 0));
    } else {
        la_xvpermi_q(temp, src2, VEXTRINS_IMM_4_0(0, 2));
    }
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vshufpd(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
            ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) &&
            ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));

    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp_l = ra_alloc_ftemp();
        IR2_OPND temp_h = ra_alloc_ftemp();
        uint8_t imm8 = ir1_opnd_uimm(ir1_get_opnd(pir1, 3)) & 0xf;
        uint8_t l = imm8 & 0x3;
        uint8_t h = imm8 >> 2;
        if (l == 0) {
            l = 0x8;
        } else if (l == 1) {
            l = 0x9;
        } else if (l == 2) {
            l = 0xc;
        } else if (l == 3) {
            l = 0xd;
        } else {
            lsassert(0);
        }

        if (h == 0) {
            h = 0x8;
        } else if (h == 1) {
            h = 0x9;
        } else if (h == 2) {
            h = 0xc;
        } else if (h == 3) {
            h = 0xd;
        } else {
            lsassert(0);
        }
        la_xvori_b(temp_l, src1, 0);
        la_xvori_b(temp_h, src1, 0);
        la_xvshuf4i_d(temp_l, src2, l);
        la_xvshuf4i_d(temp_h, src2, h);
        la_xvpermi_q(temp_h, temp_l, VEXTRINS_IMM_4_0(3, 0));
        la_xvori_b(dest, temp_h, 0);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();
        uint8_t imm8 = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
        imm8 = imm8 & 3;
        if (imm8 == 0) {
            imm8 = 0x8;
        } else if (imm8 == 1) {
            imm8 = 0x9;
        } else if (imm8 == 2) {
            imm8 = 0xc;
        } else if (imm8 == 3) {
            imm8 = 0xd;
        } else {
            lsassert(0);
        }
        la_xvori_b(temp, src1, 0);
        la_vshuf4i_d(temp, src2, imm8);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vshufps(IR1_INST * pir1) {
    lsassert((ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
            ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) ||
        (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) &&
            ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1))));
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
        uint64_t imm8 = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_xvshuf4i_w(temp1, src1, imm8);
        la_xvshuf4i_w(temp2, src2, imm8 >> 4);
        la_xvpickev_d(dest, temp2, temp1);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        uint64_t imm8 = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_vshuf4i_w(temp1, src1, imm8);
        la_vshuf4i_w(temp2, src2, imm8 >> 4);
        la_vpickev_d(temp1, temp2, temp1);
        set_high128_xreg_to_zero(temp1);
        la_xvori_b(dest, temp1, 0);
    }
    return true;
}

bool translate_vunpckhpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvori_b(temp, src1, 0);
        la_xvshuf4i_d(temp, src2, 0xd);
        la_xvori_b(dest, temp, 0);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vori_b(temp, src1, 0);
        la_vshuf4i_d(temp, src2, 0xd);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vunpckhps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));

        la_xvilvh_w(dest, src2, src1);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vilvh_w(temp, src2, src1);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vunpcklpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvori_b(temp, src1, 0);
        la_xvshuf4i_d(temp, src2, 0x8);
        la_xvori_b(dest, temp, 0);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vori_b(temp, src1, 0);
        la_vshuf4i_d(temp, src2, 0x8);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vunpcklps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));

        la_xvilvl_w(dest, src2, src1);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vilvl_w(temp, src2, src1);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vpabsx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert(ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND vzero = ra_alloc_ftemp();
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_INST * ( * tr_inst_128)(IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_INST * ( * tr_inst_256)(IR2_OPND, IR2_OPND, IR2_OPND);
    switch (ir1_opcode(pir1)) {
        case dt_X86_INS_VPABSB:
            tr_inst_256 = la_xvabsd_b;
            tr_inst_128 = la_vabsd_b;
            break;
        case dt_X86_INS_VPABSW:
            tr_inst_256 = la_xvabsd_h;
            tr_inst_128 = la_vabsd_h;
            break;
        case dt_X86_INS_VPABSD:
            tr_inst_256 = la_xvabsd_w;
            tr_inst_128 = la_vabsd_w;
            break;
        default:
            tr_inst_256 = NULL;
            tr_inst_128 = NULL;
            lsassert(0);
            break;
    }

    la_vreplgr2vr_d(vzero, zero_ir2_opnd);
    if (ir1_opnd_is_ymm(opnd0))
        tr_inst_256(temp, src, vzero);
    else
        tr_inst_128(temp, src, vzero);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);

    return true;
}

bool translate_vpackusxx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2;
    IR1_OPCODE op = ir1_opcode(pir1);
    IR2_INST * ( * cmp_inst)(IR2_OPND, IR2_OPND, int);
    IR2_INST * ( * cvt_inst)(IR2_OPND, IR2_OPND, int);
    switch (op) {
        case dt_X86_INS_VPACKUSDW:
            cmp_inst = la_xvslti_w;
            cvt_inst = la_xvssrani_hu_w;
            break;
        case dt_X86_INS_VPACKUSWB:
            cmp_inst = la_xvslti_h;
            cvt_inst = la_xvssrani_bu_h;
            break;
        default:
            cmp_inst = NULL;
            cvt_inst = NULL;
            lsassert(0);
            break;
    }
    cmp_inst(temp1, src1, 0);
    la_xvandn_v(temp1, temp1, src1);
    if ((ir1_opnd_is_xmm(opnd2) || ir1_opnd_is_ymm(opnd2)) &&
        ir1_opnd_base_reg_num(opnd1) == ir1_opnd_base_reg_num(opnd2)) {
        temp2 = temp1;
    } else {
        temp2 = ra_alloc_ftemp();
        cmp_inst(temp2, src2, 0);
        la_xvandn_v(temp2, temp2, src2);
    }
    cvt_inst(temp2, temp1, 0);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(temp2);
    }
    la_xvori_b(dest, temp2, 0);
    return true;
}

typedef IR2_INST *(*latx_avx_integer_3op_lsx_fn)(IR2_OPND, IR2_OPND,
                                                 IR2_OPND);

static bool translate_avx_integer_3op_lsx(
    IR1_INST *pir1, latx_avx_integer_3op_lsx_fn lsx_op)
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
            lsx_op(src1_low, src1_low, src2_low);
            lsx_op(src1_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
            ra_free_temp(src1_high);
            ra_free_temp(src2_low);
            ra_free_temp(src2_high);
        } else {
            src2_low = load_v128_from_ir1_mem_exact(opnd2);
            lsx_op(src1_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            clear_ymm_high128_shadow(dest_index);
            ra_free_temp(src2_low);
        }
    } else {
        int src2_index = ir1_opnd_base_reg_num(opnd2);

        lsassert(ir1_opnd_is_xmm(opnd2) || ir1_opnd_is_ymm(opnd2));
        src2_low = ra_alloc_ftemp();
        la_vori_b(src2_low, ra_alloc_xmm(src2_index), 0);
        if (ir1_opnd_is_ymm(opnd0)) {
            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high = load_ymm_high128_shadow(src2_index);

            lsx_op(src1_low, src1_low, src2_low);
            lsx_op(src1_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
            ra_free_temp(src1_high);
            ra_free_temp(src2_low);
            ra_free_temp(src2_high);
        } else {
            lsx_op(src1_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            clear_ymm_high128_shadow(dest_index);
            ra_free_temp(src2_low);
        }
    }

    ra_free_temp(src1_low);

    return true;
}

#define LATX_AVX_INTEGER_3OP_LSX_DEFINE(opcode, name, lsx_op) \
bool translate_v##name##_lsx(IR1_INST *pir1) \
{ \
    return translate_avx_integer_3op_lsx(pir1, lsx_op); \
}
LATX_AVX_INTEGER_3OP_LSX_TABLE(LATX_AVX_INTEGER_3OP_LSX_DEFINE)
#undef LATX_AVX_INTEGER_3OP_LSX_DEFINE

#define LATX_AVX_INTEGER_REMAINING_3OP_LSX_DEFINE(opcode, name, lsx_op) \
bool translate_v##name##_lsx(IR1_INST *pir1) \
{ \
    return translate_avx_integer_3op_lsx(pir1, lsx_op); \
}
LATX_AVX_INTEGER_REMAINING_3OP_LSX_TABLE(
    LATX_AVX_INTEGER_REMAINING_3OP_LSX_DEFINE)
#undef LATX_AVX_INTEGER_REMAINING_3OP_LSX_DEFINE

static IR2_INST *translate_vpsignb_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                            IR2_OPND src2)
{
    return la_vsigncov_b(dest, src2, src1);
}

static IR2_INST *translate_vpsignd_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                            IR2_OPND src2)
{
    return la_vsigncov_w(dest, src2, src1);
}

static IR2_INST *translate_vpsignw_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                            IR2_OPND src2)
{
    return la_vsigncov_h(dest, src2, src1);
}

bool translate_vpsignb_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_lsx(pir1, translate_vpsignb_lane_lsx);
}

bool translate_vpsignd_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_lsx(pir1, translate_vpsignd_lane_lsx);
}

bool translate_vpsignw_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_lsx(pir1, translate_vpsignw_lane_lsx);
}

typedef void (*latx_avx_integer_3op_lsx_custom_fn)(IR2_OPND, IR2_OPND,
                                                   IR2_OPND);

static bool translate_avx_integer_3op_custom_lsx(
    IR1_INST *pir1, latx_avx_integer_3op_lsx_custom_fn lsx_op)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int dest_index = ir1_opnd_base_reg_num(opnd0);
    int src1_index = ir1_opnd_base_reg_num(opnd1);
    IR2_OPND src1_low = ra_alloc_ftemp();
    IR2_OPND src2_low;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
             (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd1));
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));

    la_vori_b(src1_low, ra_alloc_xmm(src1_index), 0);
    if (ir1_opnd_is_mem(opnd2)) {
        if (ir1_opnd_is_ymm(opnd0)) {
            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high;
            IR2_OPND result_high = ra_alloc_ftemp();

            load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
            lsx_op(result_low, src1_low, src2_low);
            lsx_op(result_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
            store_ymm_high128_shadow(result_high, dest_index);
            ra_free_temp(src1_high);
            ra_free_temp(src2_low);
            ra_free_temp(src2_high);
            ra_free_temp(result_high);
        } else {
            src2_low = load_v128_from_ir1_mem_exact(opnd2);
            lsx_op(result_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
            clear_ymm_high128_shadow(dest_index);
            ra_free_temp(src2_low);
        }
    } else {
        int src2_index = ir1_opnd_base_reg_num(opnd2);

        lsassert(ir1_opnd_is_xmm(opnd2) || ir1_opnd_is_ymm(opnd2));
        src2_low = ra_alloc_ftemp();
        la_vori_b(src2_low, ra_alloc_xmm(src2_index), 0);
        if (ir1_opnd_is_ymm(opnd0)) {
            lsx_op(result_low, src1_low, src2_low);

            /* Keep high-half sources out of the low-half helper's temp set. */
            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high = load_ymm_high128_shadow(src2_index);
            IR2_OPND result_high = ra_alloc_ftemp();

            lsx_op(result_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
            store_ymm_high128_shadow(result_high, dest_index);
            ra_free_temp(src1_high);
            ra_free_temp(src2_high);
            ra_free_temp(result_high);
        } else {
            lsx_op(result_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
            clear_ymm_high128_shadow(dest_index);
        }
        ra_free_temp(src2_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    return true;
}

static void translate_vpmaddwd_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                        IR2_OPND src2)
{
    IR2_OPND temp = ra_alloc_ftemp();

    la_vxor_v(temp, temp, temp);
    la_vmaddwev_w_h(temp, src1, src2);
    la_vmaddwod_w_h(temp, src1, src2);
    la_vbsll_v(dest, temp, 0);
    ra_free_temp(temp);
}

static void translate_vpmaddubsw_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                          IR2_OPND src2)
{
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();
    IR2_OPND temp3 = ra_alloc_ftemp();
    IR2_OPND temp4 = ra_alloc_ftemp();
    IR2_OPND temp5 = ra_alloc_ftemp();
    IR2_OPND one = ra_alloc_itemp();

    /* Unsigned src1 multiplied by signed src2, then saturated to halfwords. */
    la_vreplgr2vr_d(temp1, zero_ir2_opnd);
    la_vabsd_b(temp3, src2, temp1);
    la_vmaddwev_h_bu(temp1, src1, temp3);
    la_vreplgr2vr_d(temp2, zero_ir2_opnd);
    la_vmaddwod_h_bu(temp2, src1, temp3);

    la_ori(one, zero_ir2_opnd, 1);
    la_vreplgr2vr_b(temp3, one);
    la_vsigncov_b(temp4, src2, temp3);
    la_vmulwev_h_b(temp5, temp4, temp3);
    la_vmulwod_h_b(temp3, temp4, temp3);
    la_vmul_h(temp1, temp1, temp5);
    la_vmul_h(temp2, temp2, temp3);
    la_vsadd_h(dest, temp2, temp1);
    ra_free_temp(one);
    ra_free_temp(temp5);
    ra_free_temp(temp4);
    ra_free_temp(temp3);
    ra_free_temp(temp2);
    ra_free_temp(temp1);
}

static void translate_vpmulhrsw_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                          IR2_OPND src2)
{
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();
    IR2_OPND temp3 = ra_alloc_ftemp();

    la_vmulwev_w_h(temp1, src1, src2);
    la_vmulwod_w_h(temp2, src1, src2);
    la_vsrai_w(temp1, temp1, 0xe);
    la_vsrai_w(temp2, temp2, 0xe);
    la_vxor_v(temp3, temp3, temp3);
    la_vandi_b(temp3, temp3, 0);
    la_vbitseti_w(temp3, temp3, 0);
    la_vadd_w(temp1, temp1, temp3);
    la_vadd_w(temp2, temp2, temp3);
    la_vsrai_w(temp1, temp1, 1);
    la_vsrai_w(temp2, temp2, 1);
    la_vpackev_h(dest, temp2, temp1);
    ra_free_temp(temp3);
    ra_free_temp(temp2);
    ra_free_temp(temp1);
}

static void translate_vphaddw_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                        IR2_OPND src2)
{
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();

    la_vpickev_h(even, src2, src1);
    la_vpickod_h(odd, src2, src1);
    la_vadd_h(dest, even, odd);
    ra_free_temp(odd);
    ra_free_temp(even);
}

static void translate_vphaddd_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                        IR2_OPND src2)
{
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();

    la_vpickev_w(even, src2, src1);
    la_vpickod_w(odd, src2, src1);
    la_vadd_w(dest, even, odd);
    ra_free_temp(odd);
    ra_free_temp(even);
}

static void translate_vphaddsw_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                         IR2_OPND src2)
{
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();

    la_vpickev_h(even, src2, src1);
    la_vpickod_h(odd, src2, src1);
    la_vsadd_h(dest, even, odd);
    ra_free_temp(odd);
    ra_free_temp(even);
}

static void translate_vphsubw_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                        IR2_OPND src2)
{
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();

    la_vpickev_h(even, src2, src1);
    la_vpickod_h(odd, src2, src1);
    la_vsub_h(dest, even, odd);
    ra_free_temp(odd);
    ra_free_temp(even);
}

static void translate_vphsubd_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                        IR2_OPND src2)
{
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();

    la_vpickev_w(even, src2, src1);
    la_vpickod_w(odd, src2, src1);
    la_vsub_w(dest, even, odd);
    ra_free_temp(odd);
    ra_free_temp(even);
}

static void translate_vphsubsw_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                         IR2_OPND src2)
{
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();

    la_vpickev_h(even, src2, src1);
    la_vpickod_h(odd, src2, src1);
    la_vssub_h(dest, even, odd);
    ra_free_temp(odd);
    ra_free_temp(even);
}

bool translate_vpmaddwd_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vpmaddwd_lane_lsx);
}

bool translate_vpmaddubsw_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vpmaddubsw_lane_lsx);
}

bool translate_vpmulhrsw_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vpmulhrsw_lane_lsx);
}

bool translate_vphaddw_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vphaddw_lane_lsx);
}

bool translate_vphaddd_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vphaddd_lane_lsx);
}

bool translate_vphaddsw_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vphaddsw_lane_lsx);
}

bool translate_vphsubw_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vphsubw_lane_lsx);
}

bool translate_vphsubd_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vphsubd_lane_lsx);
}

bool translate_vphsubsw_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vphsubsw_lane_lsx);
}

static void translate_vpsadbw_lane_lsx(IR2_OPND dest, IR2_OPND src1,
                                        IR2_OPND src2)
{
    IR2_OPND temp = ra_alloc_ftemp();

    la_vabsd_bu(temp, src1, src2);
    la_vhaddw_hu_bu(temp, temp, temp);
    la_vhaddw_wu_hu(temp, temp, temp);
    la_vhaddw_du_wu(dest, temp, temp);
    ra_free_temp(temp);
}

bool translate_vpsadbw_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_3op_custom_lsx(
        pir1, translate_vpsadbw_lane_lsx);
}

typedef IR2_INST *(*latx_vpmovx_extend_lsx_fn)(IR2_OPND, IR2_OPND);

static void translate_vpmovx_extend_lane_lsx(IR2_OPND dest, IR2_OPND src,
                                             bool is_unsigned, int src_bits,
                                             int dest_bits)
{
    latx_vpmovx_extend_lsx_fn extend_h;
    latx_vpmovx_extend_lsx_fn extend_w;
    latx_vpmovx_extend_lsx_fn extend_d;
    IR2_OPND temp = ra_alloc_ftemp();

    extend_h = is_unsigned ? la_vexth_hu_bu : la_vexth_h_b;
    extend_w = is_unsigned ? la_vexth_wu_hu : la_vexth_w_h;
    extend_d = is_unsigned ? la_vexth_du_wu : la_vexth_d_w;
    la_vori_b(temp, src, 0);
    if (src_bits == 8) {
        extend_h(temp, temp);
    } else if (src_bits == 16) {
        extend_w(temp, temp);
    } else {
        extend_d(temp, temp);
    }
    if (dest_bits == 16) {
        la_vori_b(dest, temp, 0);
    } else if (dest_bits == 32) {
        if (src_bits == 8) {
            extend_w(temp, temp);
        }
        la_vori_b(dest, temp, 0);
    } else {
        if (src_bits == 8) {
            extend_w(temp, temp);
        }
        if (src_bits != 32) {
            extend_d(temp, temp);
        }
        la_vori_b(dest, temp, 0);
    }
    ra_free_temp(temp);
}

static IR2_OPND load_vpmovx_source_lsx(IR1_OPND *src_opnd, int source_bytes)
{
    if (!ir1_opnd_is_mem(src_opnd)) {
        IR2_OPND src = ra_alloc_ftemp();
        la_vori_b(src, ra_alloc_xmm(ir1_opnd_base_reg_num(src_opnd)), 0);
        return src;
    }

    if (source_bytes == 16) {
        return load_v128_from_ir1_mem_exact(src_opnd);
    }
    IR2_OPND src = ra_alloc_ftemp();
    la_vxor_v(src, src, src);
    switch (source_bytes) {
    case 8:
        la_vinsgr2vr_d(src, load_u64_from_ir1_mem_exact(src_opnd), 0);
        break;
    case 4:
        la_vinsgr2vr_w(src, load_u32_from_ir1_mem_exact(src_opnd), 0);
        break;
    case 2:
        la_vinsgr2vr_h(src, load_u16_from_ir1_mem_exact(src_opnd), 0);
        break;
    default:
        lsassert(0);
    }
    return src;
}

static bool translate_vpmovx_lsx(IR1_INST *pir1, bool is_unsigned,
                                 int src_bits, int dest_bits)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    int source_bytes = (128 / dest_bits) * (src_bits / 8);
    IR2_OPND src;
    IR2_OPND low_src = ra_alloc_ftemp();
    IR2_OPND low_result = ra_alloc_ftemp();
    int dest_index = ir1_opnd_base_reg_num(dest_opnd);

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ir1_opnd_is_ymm(dest_opnd));
    lsassert(ir1_opnd_is_xmm(src_opnd) || ir1_opnd_is_mem(src_opnd));
    src = load_vpmovx_source_lsx(
        src_opnd, source_bytes * (ir1_opnd_is_ymm(dest_opnd) ? 2 : 1));
    la_vbsll_v(low_src, src, 16 - source_bytes);
    translate_vpmovx_extend_lane_lsx(low_result, low_src, is_unsigned,
                                      src_bits, dest_bits);
    la_vori_b(ra_alloc_xmm(dest_index), low_result, 0);
    if (ir1_opnd_is_ymm(dest_opnd)) {
        IR2_OPND high_src = ra_alloc_ftemp();
        IR2_OPND high_result = ra_alloc_ftemp();

        la_vbsll_v(high_src, src, 16 - 2 * source_bytes);
        translate_vpmovx_extend_lane_lsx(high_result, high_src, is_unsigned,
                                          src_bits, dest_bits);
        store_ymm_high128_shadow(high_result, dest_index);
        ra_free_temp(high_result);
        ra_free_temp(high_src);
    } else {
        clear_ymm_high128_shadow(dest_index);
    }
    ra_free_temp(low_result);
    ra_free_temp(low_src);
    ra_free_temp(src);
    return true;
}

bool translate_vpmovsxxx_lsx(IR1_INST *pir1)
{
    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPMOVSXBW:
        return translate_vpmovx_lsx(pir1, false, 8, 16);
    case dt_X86_INS_VPMOVSXBD:
        return translate_vpmovx_lsx(pir1, false, 8, 32);
    case dt_X86_INS_VPMOVSXBQ:
        return translate_vpmovx_lsx(pir1, false, 8, 64);
    case dt_X86_INS_VPMOVSXWD:
        return translate_vpmovx_lsx(pir1, false, 16, 32);
    case dt_X86_INS_VPMOVSXWQ:
        return translate_vpmovx_lsx(pir1, false, 16, 64);
    case dt_X86_INS_VPMOVSXDQ:
        return translate_vpmovx_lsx(pir1, false, 32, 64);
    default:
        lsassert(0);
        return false;
    }
}

bool translate_vpmovzxxx_lsx(IR1_INST *pir1)
{
    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPMOVZXBW:
        return translate_vpmovx_lsx(pir1, true, 8, 16);
    case dt_X86_INS_VPMOVZXBD:
        return translate_vpmovx_lsx(pir1, true, 8, 32);
    case dt_X86_INS_VPMOVZXBQ:
        return translate_vpmovx_lsx(pir1, true, 8, 64);
    case dt_X86_INS_VPMOVZXWD:
        return translate_vpmovx_lsx(pir1, true, 16, 32);
    case dt_X86_INS_VPMOVZXWQ:
        return translate_vpmovx_lsx(pir1, true, 16, 64);
    case dt_X86_INS_VPMOVZXDQ:
        return translate_vpmovx_lsx(pir1, true, 32, 64);
    default:
        lsassert(0);
        return false;
    }
}

bool translate_vphminposuw_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    bool src_is_temp = ir1_opnd_is_mem(src_opnd);
    IR2_OPND src = src_is_temp ?
        load_v128_from_ir1_mem_exact(src_opnd) :
        ra_alloc_xmm(ir1_opnd_base_reg_num(src_opnd));
    IR2_OPND min = ra_alloc_itemp();
    IR2_OPND value = ra_alloc_itemp();
    IR2_OPND index = ra_alloc_itemp();
    IR2_OPND result = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(dest_opnd));
    la_vpickve2gr_hu(min, src, 0);
    li_wu(index, 0);
    for (int i = 1; i < 8; ++i) {
        IR2_OPND keep = ra_alloc_label();

        la_vpickve2gr_hu(value, src, i);
        la_bgeu(value, min, keep);
        la_or(min, value, zero_ir2_opnd);
        li_wu(index, i);
        la_label(keep);
    }
    la_vxor_v(result, result, result);
    la_vinsgr2vr_h(result, min, 0);
    la_vinsgr2vr_h(result, index, 1);
    la_vori_b(ra_alloc_xmm(ir1_opnd_base_reg_num(dest_opnd)), result, 0);
    clear_ymm_high128_shadow(ir1_opnd_base_reg_num(dest_opnd));
    ra_free_temp(result);
    ra_free_temp(index);
    ra_free_temp(value);
    ra_free_temp(min);
    if (src_is_temp) {
        ra_free_temp(src);
    }
    return true;
}

bool translate_vpabsx_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    IR2_INST *(*abs_op)(IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND src_low;
    IR2_OPND zero_low = ra_alloc_ftemp();
    int dest_index = ir1_opnd_base_reg_num(dest_opnd);

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ir1_opnd_is_ymm(dest_opnd));
    lsassert(ir1_opnd_is_mem(src_opnd) || ir1_opnd_is_xmm(src_opnd) ||
             ir1_opnd_is_ymm(src_opnd));

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPABSB:
        abs_op = la_vabsd_b;
        break;
    case dt_X86_INS_VPABSW:
        abs_op = la_vabsd_h;
        break;
    case dt_X86_INS_VPABSD:
        abs_op = la_vabsd_w;
        break;
    default:
        lsassert(0);
    }

    la_vxor_v(zero_low, zero_low, zero_low);
    if (ir1_opnd_is_mem(src_opnd)) {
        if (ir1_opnd_is_ymm(dest_opnd)) {
            IR2_OPND src_high;
            IR2_OPND result_high = ra_alloc_ftemp();

            load_v256_from_ir1_mem_exact(src_opnd, &src_low, &src_high);
            abs_op(src_low, src_low, zero_low);
            abs_op(result_high, src_high, zero_low);
            la_vori_b(ra_alloc_xmm(dest_index), src_low, 0);
            store_ymm_high128_shadow(result_high, dest_index);
            ra_free_temp(result_high);
            ra_free_temp(src_high);
            ra_free_temp(src_low);
        } else {
            src_low = load_v128_from_ir1_mem_exact(src_opnd);
            abs_op(src_low, src_low, zero_low);
            la_vori_b(ra_alloc_xmm(dest_index), src_low, 0);
            clear_ymm_high128_shadow(dest_index);
            ra_free_temp(src_low);
        }
    } else {
        int src_index = ir1_opnd_base_reg_num(src_opnd);
        IR2_OPND src_high;

        src_low = ra_alloc_ftemp();
        la_vori_b(src_low, ra_alloc_xmm(src_index), 0);
        abs_op(src_low, src_low, zero_low);
        la_vori_b(ra_alloc_xmm(dest_index), src_low, 0);
        if (ir1_opnd_is_ymm(dest_opnd)) {
            src_high = load_ymm_high128_shadow(src_index);
            abs_op(src_high, src_high, zero_low);
            store_ymm_high128_shadow(src_high, dest_index);
            ra_free_temp(src_high);
        } else {
            clear_ymm_high128_shadow(dest_index);
        }
        ra_free_temp(src_low);
    }
    ra_free_temp(zero_low);
    return true;
}

bool translate_vpaddx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND);
    IR1_OPCODE op = ir1_opcode(pir1);
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    switch (op) {
        case dt_X86_INS_VPADDB:
            tr_inst = la_xvadd_b;
            break;
        case dt_X86_INS_VPADDW:
            tr_inst = la_xvadd_h;
            break;
        case dt_X86_INS_VPADDD:
            tr_inst = la_xvadd_w;
            break;
        case dt_X86_INS_VPADDQ:
            tr_inst = la_xvadd_d;
            break;
        case dt_X86_INS_VPADDSB:
            tr_inst = la_xvsadd_b;
            break;
        case dt_X86_INS_VPADDSW:
            tr_inst = la_xvsadd_h;
            break;
        case dt_X86_INS_VPADDUSB:
            tr_inst = la_xvsadd_bu;
            break;
        case dt_X86_INS_VPADDUSW:
            tr_inst = la_xvsadd_hu;
            break;

        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src1, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpand_lsx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));

    {
        /* LSX-only path */
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        int src1_index = ir1_opnd_base_reg_num(opnd1);
        IR2_OPND src1_low = ra_alloc_ftemp();
        IR2_OPND src2_low;

        la_vori_b(src1_low, ra_alloc_xmm(src1_index), 0);
        if (ir1_opnd_is_mem(opnd2)) {
            if (ir1_opnd_is_ymm(opnd0)) {
                IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
                IR2_OPND src2_high;

                load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
                la_vand_v(src1_low, src1_low, src2_low);
                la_vand_v(src1_high, src1_high, src2_high);
                la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
                store_ymm_high128_shadow(src1_high, dest_index);
            } else {
                src2_low = load_v128_from_ir1_mem_exact(opnd2);
                la_vand_v(src1_low, src1_low, src2_low);
                la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
                clear_ymm_high128_shadow(dest_index);
            }
        } else {
            int src2_index = ir1_opnd_base_reg_num(opnd2);

            src2_low = ra_alloc_ftemp();
            la_vori_b(src2_low, ra_alloc_xmm(src2_index), 0);
            if (ir1_opnd_is_ymm(opnd0)) {
                IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
                IR2_OPND src2_high = load_ymm_high128_shadow(src2_index);

                la_vand_v(src1_low, src1_low, src2_low);
                la_vand_v(src1_high, src1_high, src2_high);
                la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
                store_ymm_high128_shadow(src1_high, dest_index);
            } else {
                la_vand_v(src1_low, src1_low, src2_low);
                la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
                clear_ymm_high128_shadow(dest_index);
            }
        }
    }
    return true;
}

bool translate_vpandn_lsx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));

    int dest_index = ir1_opnd_base_reg_num(opnd0);
    int src1_index = ir1_opnd_base_reg_num(opnd1);
    IR2_OPND src1_low = ra_alloc_ftemp();
    IR2_OPND src2_low;

    la_vori_b(src1_low, ra_alloc_xmm(src1_index), 0);
    if (ir1_opnd_is_mem(opnd2)) {
        if (ir1_opnd_is_ymm(opnd0)) {
            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high;

            load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
            la_vandn_v(src1_low, src1_low, src2_low);
            la_vandn_v(src1_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
        } else {
            src2_low = load_v128_from_ir1_mem_exact(opnd2);
            la_vandn_v(src1_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            clear_ymm_high128_shadow(dest_index);
        }
    } else {
        int src2_index = ir1_opnd_base_reg_num(opnd2);

        src2_low = ra_alloc_ftemp();
        la_vori_b(src2_low, ra_alloc_xmm(src2_index), 0);
        if (ir1_opnd_is_ymm(opnd0)) {
            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high = load_ymm_high128_shadow(src2_index);

            la_vandn_v(src1_low, src1_low, src2_low);
            la_vandn_v(src1_high, src1_high, src2_high);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            store_ymm_high128_shadow(src1_high, dest_index);
        } else {
            la_vandn_v(src1_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
            clear_ymm_high128_shadow(dest_index);
        }
    }
    return true;
}

bool translate_vpand(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);

    la_xvand_v(dest, src1, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpandn(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);

    if (ir1_opnd_is_ymm(opnd0)) {
        la_xvandn_v(dest, src1, src2);
        return true;
    }
    IR2_OPND temp = ra_alloc_ftemp();
    la_xvandn_v(temp, src1, src2);
    set_high128_xreg_to_zero(temp);
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vpblendd(IR1_INST * pir1) {
    lsassert(ir1_opnd_num(pir1) == 4);
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    uint8_t imm = ir1_opnd_uimm(opnd3);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND rmask = ra_alloc_itemp();

    if (ir1_opnd_is_xmm(opnd0)) {
        if((imm & 0xf) == 0xf) {
            la_xvori_b(dest, src2, 0);
            set_high128_xreg_to_zero(dest);
            return true;
        } else if((imm & 0xf) == 0x0){
            la_xvori_b(dest, src1, 0);
            set_high128_xreg_to_zero(dest);
            return true;
        }
        uint64_t mask = 0;
        for (int i = 0; i < 4; i++) {
            if (imm & (1 << i)) {
                mask |= (0xFFULL) << (i * 8);
            }
        }
        li_d(rmask, mask);
        la_movgr2fr_d(temp, rmask);
        la_vext2xv_w_b(temp, temp);
        la_xvbitsel_v(dest, src1, src2, temp);

        set_high128_xreg_to_zero(dest);
        return true;
    }
        if((imm & 0xff) == 0xff) {
            la_xvori_b(dest, src2, 0);
            return true;
        } else if((imm & 0xff) == 0x0){
            la_xvori_b(dest, src1, 0);
            return true;
        }
        uint64_t mask = 0;
        for (int i = 0; i < 8; i++) {
            if (imm & (1 << i)) {
                mask |= (0xFFULL) << (i * 8);
            }
        }
        li_d(rmask, mask);
        la_movgr2fr_d(temp, rmask);
        la_vext2xv_w_b(temp, temp);
        la_xvbitsel_v(dest, src1, src2, temp);

    return true;
}

bool translate_vpblendvb_lsx(IR1_INST * pir1) {
    lsassert(ir1_opnd_num(pir1) == 4);
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd3)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd3)));
    lsassert(ir1_opnd_is_mem(opnd2) || ir1_opnd_is_xmm(opnd2) ||
        ir1_opnd_is_ymm(opnd2));

    {
        /* LSX-only path */
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        int src1_index = ir1_opnd_base_reg_num(opnd1);
        int mask_index = ir1_opnd_base_reg_num(opnd3);
        bool is_ymm = ir1_opnd_is_ymm(opnd0);
        IR2_OPND src1_low = ra_alloc_ftemp();
        IR2_OPND src2_low;
        IR2_OPND mask_low = ra_alloc_ftemp();
        IR2_OPND src1_high = { 0 };
        IR2_OPND src2_high = { 0 };
        IR2_OPND mask_high = { 0 };

        /* Preserve every register source before writing an aliased dest. */
        la_vori_b(src1_low, ra_alloc_xmm(src1_index), 0);
        la_vori_b(mask_low, ra_alloc_xmm(mask_index), 0);
        if (is_ymm) {
            src1_high = load_ymm_high128_shadow(src1_index);
            mask_high = load_ymm_high128_shadow(mask_index);
        }

        if (ir1_opnd_is_mem(opnd2)) {
            if (is_ymm) {
                load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
            } else {
                src2_low = load_v128_from_ir1_mem_exact(opnd2);
            }
        } else {
            int src2_index = ir1_opnd_base_reg_num(opnd2);

            src2_low = ra_alloc_ftemp();
            la_vori_b(src2_low, ra_alloc_xmm(src2_index), 0);
            if (is_ymm) {
                src2_high = load_ymm_high128_shadow(src2_index);
            }
        }

        /* A negative signed byte means its high bit selects src2. */
        la_vslti_b(mask_low, mask_low, 0);
        la_vbitsel_v(src1_low, src1_low, src2_low, mask_low);
        if (is_ymm) {
            la_vslti_b(mask_high, mask_high, 0);
            la_vbitsel_v(src1_high, src1_high, src2_high, mask_high);
        }

        la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
        if (is_ymm) {
            store_ymm_high128_shadow(src1_high, dest_index);
        } else {
            clear_ymm_high128_shadow(dest_index);
        }
    }
    return true;
}

bool translate_vpblendvb(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND src3 = load_freg256_from_ir1(opnd3);
    IR2_OPND temp = ra_alloc_ftemp();
    la_xvslti_b(temp, src3, 0);
    la_xvbitsel_v(temp, src1, src2, temp);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(temp);
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vpblendw(IR1_INST * pir1) {
    lsassert(ir1_opnd_num(pir1) == 4);
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    uint8_t imm = ir1_opnd_uimm(opnd3);
    if (imm == 0xff) {
        la_xvand_v(dest, src2, src2);
        if (ir1_opnd_is_xmm(opnd0))
            set_high128_xreg_to_zero(dest);
        return true;
    } else if (imm == 0) {
        la_xvand_v(dest, src1, src1);
        if (ir1_opnd_is_xmm(opnd0))
            set_high128_xreg_to_zero(dest);
        return true;
    }
    IR2_OPND temp = ra_alloc_ftemp();
    la_xvori_b(temp, src1, 0);
    /* 64 bit fast path */
    if ((imm & 0xf) == 0xf) {
        la_xvextrins_d(temp, src2, VEXTRINS_IMM_4_0(0, 0));
        imm &= ~0xf;
    }
    if ((imm & 0xf0) == 0xf0) {
        la_xvextrins_d(temp, src2, VEXTRINS_IMM_4_0(1, 1));
        imm &= ~0xf0;
    }

    /* 32 bit fast path */
    if ((imm & 0x3) == 0x3) {
        la_xvextrins_w(temp, src2, VEXTRINS_IMM_4_0(0, 0));
        imm &= ~0x3;
    }
    if ((imm & 0xc) == 0xc) {
        la_xvextrins_w(temp, src2, VEXTRINS_IMM_4_0(1, 1));
        imm &= ~0xc;
    }
    if ((imm & 0x30) == 0x30) {
        la_xvextrins_w(temp, src2, VEXTRINS_IMM_4_0(2, 2));
        imm &= ~0x30;
    }
    if ((imm & 0xc0) == 0xc0) {
        la_xvextrins_w(temp, src2, VEXTRINS_IMM_4_0(3, 3));
        imm &= ~0xc0;
    }

    /* 16 bit slow path */
    if (imm & 0x1)
        la_xvextrins_h(temp, src2, VEXTRINS_IMM_4_0(0, 0));
    if (imm & 0x2)
        la_xvextrins_h(temp, src2, VEXTRINS_IMM_4_0(1, 1));
    if (imm & 0x4)
        la_xvextrins_h(temp, src2, VEXTRINS_IMM_4_0(2, 2));
    if (imm & 0x8)
        la_xvextrins_h(temp, src2, VEXTRINS_IMM_4_0(3, 3));
    if (imm & 0x10)
        la_xvextrins_h(temp, src2, VEXTRINS_IMM_4_0(4, 4));
    if (imm & 0x20)
        la_xvextrins_h(temp, src2, VEXTRINS_IMM_4_0(5, 5));
    if (imm & 0x40)
        la_xvextrins_h(temp, src2, VEXTRINS_IMM_4_0(6, 6));
    if (imm & 0x80)
        la_xvextrins_h(temp, src2, VEXTRINS_IMM_4_0(7, 7));

    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(temp);
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vperm2f128(IR1_INST * pir1) {
    translate_vperm2i128(pir1);
    return true;
}

bool translate_vperm2i128(IR1_INST * pir1) {
    lsassert(ir1_opnd_num(pir1) == 4);
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);
    lsassert((ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_is_imm(opnd3));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    uint8 imm = ir1_opnd_uimm(opnd3) & 0xff;
    bool zero_l = (imm >> 3) & 0x1, zero_h = (imm >> 7) & 0x1;
    if (zero_l && zero_h) {
        la_xvandi_b(dest, dest, 0);
        return true;
    }
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND temp = ra_alloc_ftemp();
    la_xvori_b(temp, src2, 0);
    la_xvpermi_q(temp, src1, imm);
    if (zero_l) {
        la_xvinsgr2vr_d(temp, zero_ir2_opnd, 0);
        la_xvinsgr2vr_d(temp, zero_ir2_opnd, 1);
    }
    if (zero_h) {
        la_xvinsgr2vr_d(temp, zero_ir2_opnd, 2);
        la_xvinsgr2vr_d(temp, zero_ir2_opnd, 3);
    }
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vpermd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    la_xvperm_w(dest, src2, src1);
    return true;
}

bool translate_vpermq(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_imm(ir1_get_opnd(pir1, 2)));
    IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 2)) & 0xff;
    la_xvpermi_d(dest, src, imm);
    return true;
}

bool translate_vpextrx(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) &&
        ir1_opnd_is_imm(ir1_get_opnd(pir1, 2)));
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND dest;
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, int);
    bool is_reg = ir1_opnd_is_gpr(opnd0);
    if (is_reg)
        dest = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd0));
    else
        dest = ra_alloc_itemp();
    switch (ir1_opcode(pir1)) {
        case dt_X86_INS_VPEXTRB:
            tr_inst = la_vpickve2gr_bu;
            imm &= 0xf;
            break;
        case dt_X86_INS_VPEXTRW:
            tr_inst = la_vpickve2gr_hu;
            imm &= 0x7;
            break;
        case dt_X86_INS_VPEXTRD:
            tr_inst = la_vpickve2gr_wu;
            imm &= 0x3;
            break;
        case dt_X86_INS_VPEXTRQ:
            tr_inst = la_vpickve2gr_du;
            imm &= 0x1;
            break;
        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src, imm);
    if (!is_reg)
        store_ireg_to_ir1(dest, opnd0, false);
    return true;
}

bool translate_vpextrx_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 2));
    IR2_OPND src;
    IR2_OPND value = ra_alloc_itemp();
    bool is_reg = ir1_opnd_is_gpr(dest);

    lsassert(ir1_opnd_is_xmm(src_opnd));
    lsassert(is_reg || ir1_opnd_is_mem(dest));
    lsassert(ir1_opnd_is_imm(ir1_get_opnd(pir1, 2)));
    src = ra_alloc_xmm(ir1_opnd_base_reg_num(src_opnd));
    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPEXTRB:
        imm &= 0xf;
        la_vpickve2gr_bu(value, src, imm);
        if (is_reg) {
            store_ireg_to_ir1(value, dest, false);
        } else {
            store_u8_to_ir1_mem_exact(value, dest);
        }
        break;
    case dt_X86_INS_VPEXTRW:
        imm &= 0x7;
        la_vpickve2gr_hu(value, src, imm);
        if (is_reg) {
            store_ireg_to_ir1(value, dest, false);
        } else {
            /* Check each byte separately so a cross-page fault reports the
             * protected second page as SEGV_ACCERR, matching x86. */
            IR2_OPND address = convert_mem_to_itemp(dest);
            IR2_OPND byte = ra_alloc_itemp();

            gen_test_page_flag_force_range(address, 0, 1,
                                           PAGE_WRITE | PAGE_WRITE_ORG);
            la_st_b(value, address, 0);
            la_addi_d(address, address, 1);
            gen_test_page_flag_force_range(address, 0, 1,
                                           PAGE_WRITE | PAGE_WRITE_ORG);
            la_srli_d(byte, value, 8);
            la_st_b(byte, address, 0);
            ra_free_temp(byte);
            ra_free_temp(address);
        }
        break;
    case dt_X86_INS_VPEXTRD:
        imm &= 0x3;
        la_vpickve2gr_wu(value, src, imm);
        if (is_reg) {
            store_ireg_to_ir1(value, dest, false);
        } else {
            store_u32_to_ir1_mem_exact(value, dest);
        }
        break;
    case dt_X86_INS_VPEXTRQ:
        imm &= 0x1;
        la_vpickve2gr_du(value, src, imm);
        if (is_reg) {
            store_ireg_to_ir1(value, dest, false);
        } else {
            store_u64_to_ir1_mem_exact(value, dest);
        }
        break;
    default:
        lsassert(0);
    }
    ra_free_temp(value);
    return true;
}

bool translate_vpminux(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND);
    switch (ir1_opcode(pir1)) {
        case dt_X86_INS_VPMINUB:
            tr_inst = la_xvmin_bu;
            break;
        case dt_X86_INS_VPMINUW:
            tr_inst = la_xvmin_hu;
            break;
        case dt_X86_INS_VPMINUD:
            tr_inst = la_xvmin_wu;
            break;
        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src1, src2);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);

    return true;
}

bool translate_vpmovsxxx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert(ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VPMOVSXBW:
            tr_inst = la_vext2xv_h_b;
            break;
        case dt_X86_INS_VPMOVSXBD:
            tr_inst = la_vext2xv_w_b;
            break;
        case dt_X86_INS_VPMOVSXBQ:
            tr_inst = la_vext2xv_d_b;
            break;
        case dt_X86_INS_VPMOVSXWD:
            tr_inst = la_vext2xv_w_h;
            break;
        case dt_X86_INS_VPMOVSXWQ:
            tr_inst = la_vext2xv_d_h;
            break;
        case dt_X86_INS_VPMOVSXDQ:
            tr_inst = la_vext2xv_d_w;
            break;
        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpmovzxxx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert(ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VPMOVZXBW:
            tr_inst = la_vext2xv_hu_bu;
            break;
        case dt_X86_INS_VPMOVZXBD:
            tr_inst = la_vext2xv_wu_bu;
            break;
        case dt_X86_INS_VPMOVZXBQ:
            tr_inst = la_vext2xv_du_bu;
            break;
        case dt_X86_INS_VPMOVZXWD:
            tr_inst = la_vext2xv_wu_hu;
            break;
        case dt_X86_INS_VPMOVZXWQ:
            tr_inst = la_vext2xv_du_hu;
            break;
        case dt_X86_INS_VPMOVZXDQ:
            tr_inst = la_vext2xv_du_wu;
            break;
        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpmullx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND);
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VPMULLD:
            tr_inst = la_xvmul_w;
            break;
        case dt_X86_INS_VPMULLW:
            tr_inst = la_xvmul_h;
            break;
        case dt_X86_INS_VPMULUDQ:
            tr_inst = la_xvmulwev_d_wu;
            break;
        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src1, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpor_lsx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));

    {
        /* LSX-only path */
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        int src1_index = ir1_opnd_base_reg_num(opnd1);
        IR2_OPND src1_low = ra_alloc_ftemp();
        IR2_OPND src2_low;

        la_vori_b(src1_low, ra_alloc_xmm(src1_index), 0);
        if (ir1_opnd_is_mem(opnd2)) {
            if (ir1_opnd_is_ymm(opnd0)) {
                IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
                IR2_OPND src2_high;

                load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
                la_vor_v(src1_low, src1_low, src2_low);
                la_vor_v(src1_high, src1_high, src2_high);
                la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
                store_ymm_high128_shadow(src1_high, dest_index);
            } else {
                src2_low = load_v128_from_ir1_mem_exact(opnd2);
                la_vor_v(src1_low, src1_low, src2_low);
                la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
                clear_ymm_high128_shadow(dest_index);
            }
        } else {
            int src2_index = ir1_opnd_base_reg_num(opnd2);

            src2_low = ra_alloc_ftemp();
            la_vori_b(src2_low, ra_alloc_xmm(src2_index), 0);
            if (ir1_opnd_is_ymm(opnd0)) {
                IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
                IR2_OPND src2_high = load_ymm_high128_shadow(src2_index);

                la_vor_v(src1_low, src1_low, src2_low);
                la_vor_v(src1_high, src1_high, src2_high);
                la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
                store_ymm_high128_shadow(src1_high, dest_index);
            } else {
                la_vor_v(src1_low, src1_low, src2_low);
                la_vori_b(ra_alloc_xmm(dest_index), src1_low, 0);
                clear_ymm_high128_shadow(dest_index);
            }
        }
    }
    return true;
}

bool translate_vpor(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);

    la_xvor_v(dest, src1, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpshufb(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);

    IR2_OPND index = ra_alloc_ftemp();
    IR2_OPND mask = ra_alloc_ftemp();
    la_xvandi_b(index, src2, 0xf);
    la_xvslti_b(mask, src2, 0);
    la_xvshuf_b(dest, src1, src1, index);
    la_xvandn_v(dest, mask, dest);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpsubx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND);
    IR1_OPCODE op = ir1_opcode(pir1);
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    switch (op) {
        case dt_X86_INS_VPSUBB:
            tr_inst = la_xvsub_b;
            break;
        case dt_X86_INS_VPSUBW:
            tr_inst = la_xvsub_h;
            break;
        case dt_X86_INS_VPSUBD:
            tr_inst = la_xvsub_w;
            break;
        case dt_X86_INS_VPSUBQ:
            tr_inst = la_xvsub_d;
            break;
        case dt_X86_INS_VPSUBSB:
            tr_inst = la_xvssub_b;
            break;
        case dt_X86_INS_VPSUBSW:
            tr_inst = la_xvssub_h;
            break;
        case dt_X86_INS_VPSUBUSB:
            tr_inst = la_xvssub_bu;
            break;
        case dt_X86_INS_VPSUBUSW:
            tr_inst = la_xvssub_hu;
            break;
        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src1, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vptest(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert(ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND label_1 = ra_alloc_label();
    IR2_OPND label_2 = ra_alloc_label();
    IR2_OPND n4095_opnd = ra_alloc_num_4095();

    if (ir1_opnd_is_xmm(opnd0)) {
        la_x86mtflag(zero_ir2_opnd, 0x3f);
        la_vand_v(temp, src, dest);
        la_vseteqz_v(fcc0_ir2_opnd, temp);
        la_bceqz(fcc0_ir2_opnd, label_1);
        la_x86mtflag(n4095_opnd, ZF_USEDEF_BIT);

        la_label(label_1);
        la_vandn_v(temp, dest, src);
        la_vseteqz_v(fcc0_ir2_opnd, temp);
        la_bceqz(fcc0_ir2_opnd, label_2);
        la_x86mtflag(n4095_opnd, CF_USEDEF_BIT);
        la_label(label_2);
    } else {
        la_x86mtflag(zero_ir2_opnd, 0x3f);
        la_xvand_v(temp, src, dest);
        la_xvseteqz_v(fcc0_ir2_opnd, temp);
        la_bceqz(fcc0_ir2_opnd, label_1);
        la_x86mtflag(n4095_opnd, ZF_USEDEF_BIT);

        la_label(label_1);
        la_xvandn_v(temp, dest, src);
        la_xvseteqz_v(fcc0_ir2_opnd, temp);
        la_bceqz(fcc0_ir2_opnd, label_2);
        la_x86mtflag(n4095_opnd, CF_USEDEF_BIT);
        la_label(label_2);
    }
    ra_free_num_4095(n4095_opnd);
    return true;
}

static void load_avx_lsx_operand(IR1_OPND *opnd, bool ymm,
                                 IR2_OPND *low, IR2_OPND *high);

bool translate_vptest_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    IR2_OPND dest_low;
    IR2_OPND dest_high;
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND and_result = ra_alloc_ftemp();
    IR2_OPND andn_result = ra_alloc_ftemp();
    IR2_OPND half_result = ra_alloc_ftemp();
    IR2_OPND n4095_opnd = ra_alloc_num_4095();
    IR2_OPND zf_done = ra_alloc_label();
    IR2_OPND cf_done = ra_alloc_label();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_xmm(src_opnd) == !ymm ||
             ir1_opnd_is_ymm(src_opnd) == ymm ||
             ir1_opnd_is_mem(src_opnd));
    if (ymm) {
        tr_save_ymm_to_env(UINT16_MAX);
    }
    load_avx_lsx_operand(dest_opnd, ymm, &dest_low, &dest_high);
    load_avx_lsx_operand(src_opnd, ymm, &src_low, &src_high);

    /* VPTEST sets ZF from dest & src and CF from dest & ~src. */
    la_vand_v(and_result, dest_low, src_low);
    la_vandn_v(andn_result, dest_low, src_low);
    if (ymm) {
        la_vand_v(half_result, dest_high, src_high);
        la_vor_v(and_result, and_result, half_result);
        la_vandn_v(half_result, dest_high, src_high);
        la_vor_v(andn_result, andn_result, half_result);
    }

    /* VPTEST changes only ZF and CF; preserve the other arithmetic flags. */
    la_x86mtflag(zero_ir2_opnd, ZF_USEDEF_BIT | CF_USEDEF_BIT);
    la_vseteqz_v(fcc0_ir2_opnd, and_result);
    la_bceqz(fcc0_ir2_opnd, zf_done);
    la_x86mtflag(n4095_opnd, ZF_USEDEF_BIT);
    la_label(zf_done);

    la_vseteqz_v(fcc0_ir2_opnd, andn_result);
    la_bceqz(fcc0_ir2_opnd, cf_done);
    la_x86mtflag(n4095_opnd, CF_USEDEF_BIT);
    la_label(cf_done);

    ra_free_num_4095(n4095_opnd);
    ra_free_temp(half_result);
    ra_free_temp(andn_result);
    ra_free_temp(and_result);
    ra_free_temp(src_low);
    ra_free_temp(dest_low);
    if (ymm) {
        ra_free_temp(src_high);
        ra_free_temp(dest_high);
    }
    return true;
}

bool translate_vpunpckhxx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND);
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VPUNPCKHBW:
            tr_inst = la_xvilvh_b;
            break;
        case dt_X86_INS_VPUNPCKHWD:
            tr_inst = la_xvilvh_h;
            break;
        case dt_X86_INS_VPUNPCKHDQ:
            tr_inst = la_xvilvh_w;
            break;
        case dt_X86_INS_VPUNPCKHQDQ:
            tr_inst = la_xvilvh_d;
            break;
        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src2, src1);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

typedef IR2_INST *(*avx_lsx_lane_3op_fn)(IR2_OPND, IR2_OPND, IR2_OPND);
typedef IR2_INST *(*avx_lsx_narrow_fn)(IR2_OPND, IR2_OPND, int);

static void load_avx_lsx_operand(IR1_OPND *opnd, bool ymm,
                                 IR2_OPND *low, IR2_OPND *high)
{
    lsassert(ir1_opnd_is_mem(opnd) || ir1_opnd_is_xmm(opnd) ||
             ir1_opnd_is_ymm(opnd));
    if (ir1_opnd_is_mem(opnd)) {
        if (ymm) {
            load_v256_from_ir1_mem_exact(opnd, low, high);
        } else {
            *low = load_v128_from_ir1_mem_exact(opnd);
        }
        return;
    }

    *low = ra_alloc_ftemp();
    la_vori_b(*low, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd)), 0);
    if (ymm) {
        *high = load_ymm_high128_shadow(ir1_opnd_base_reg_num(opnd));
    }
}

static void store_avx_lsx_result(IR1_OPND *opnd, IR2_OPND low,
                                 IR2_OPND high)
{
    int dest_index = ir1_opnd_base_reg_num(opnd);

    la_vori_b(ra_alloc_xmm(dest_index), low, 0);
    if (ir1_opnd_is_ymm(opnd)) {
        store_ymm_high128_shadow(high, dest_index);
    } else {
        clear_ymm_high128_shadow(dest_index);
    }
}

static bool translate_avx_lane_3op_lsx(IR1_INST *pir1,
                                       avx_lsx_lane_3op_fn tr_inst)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(opnd0) || ymm);
    lsassert(ir1_opnd_is_xmm(opnd1) == !ymm ||
             ir1_opnd_is_ymm(opnd1) == ymm);
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));
    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    tr_inst(result_low, src2_low, src1_low);

    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();
        tr_inst(result_high, src2_high, src1_high);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    } else {
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    return true;
}

static bool translate_avx_pack_lsx(IR1_INST *pir1,
                                   avx_lsx_narrow_fn cvt_inst,
                                   avx_lsx_narrow_fn negative_cmp_inst)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    IR2_OPND narrow1_low = ra_alloc_ftemp();
    IR2_OPND narrow2_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(opnd0) || ymm);
    lsassert(ir1_opnd_is_xmm(opnd1) == !ymm ||
             ir1_opnd_is_ymm(opnd1) == ymm);
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));
    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    if (negative_cmp_inst) {
        negative_cmp_inst(narrow1_low, src1_low, 0);
        la_vandn_v(narrow1_low, narrow1_low, src1_low);
        negative_cmp_inst(narrow2_low, src2_low, 0);
        la_vandn_v(narrow2_low, narrow2_low, src2_low);
    } else {
        la_vori_b(narrow1_low, src1_low, 0);
        la_vori_b(narrow2_low, src2_low, 0);
    }
    cvt_inst(narrow1_low, narrow1_low, 0);
    cvt_inst(narrow2_low, narrow2_low, 0);
    la_vilvl_d(result_low, narrow2_low, narrow1_low);

    if (ymm) {
        IR2_OPND narrow1_high = ra_alloc_ftemp();
        IR2_OPND narrow2_high = ra_alloc_ftemp();
        IR2_OPND result_high = ra_alloc_ftemp();
        if (negative_cmp_inst) {
            negative_cmp_inst(narrow1_high, src1_high, 0);
            la_vandn_v(narrow1_high, narrow1_high, src1_high);
            negative_cmp_inst(narrow2_high, src2_high, 0);
            la_vandn_v(narrow2_high, narrow2_high, src2_high);
        } else {
            la_vori_b(narrow1_high, src1_high, 0);
            la_vori_b(narrow2_high, src2_high, 0);
        }
        cvt_inst(narrow1_high, narrow1_high, 0);
        cvt_inst(narrow2_high, narrow2_high, 0);
        la_vilvl_d(result_high, narrow2_high, narrow1_high);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(narrow1_high);
        ra_free_temp(narrow2_high);
    } else {
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(narrow1_low);
    ra_free_temp(narrow2_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    return true;
}

bool translate_vpackssxx_lsx(IR1_INST *pir1)
{
    avx_lsx_narrow_fn cvt_inst;

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPACKSSDW:
        cvt_inst = la_vssrani_h_w;
        break;
    case dt_X86_INS_VPACKSSWB:
        cvt_inst = la_vssrani_b_h;
        break;
    default:
        lsassert(0);
        return false;
    }
    return translate_avx_pack_lsx(pir1, cvt_inst, NULL);
}

bool translate_vpackusxx_lsx(IR1_INST *pir1)
{
    avx_lsx_narrow_fn cvt_inst;
    avx_lsx_narrow_fn negative_cmp_inst;

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPACKUSDW:
        cvt_inst = la_vssrani_hu_w;
        negative_cmp_inst = la_vslti_w;
        break;
    case dt_X86_INS_VPACKUSWB:
        cvt_inst = la_vssrani_bu_h;
        negative_cmp_inst = la_vslti_h;
        break;
    default:
        lsassert(0);
        return false;
    }
    return translate_avx_pack_lsx(pir1, cvt_inst, negative_cmp_inst);
}

bool translate_vpunpckhxx_lsx(IR1_INST *pir1)
{
    avx_lsx_lane_3op_fn tr_inst;

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPUNPCKHBW:
        tr_inst = la_vilvh_b;
        break;
    case dt_X86_INS_VPUNPCKHWD:
        tr_inst = la_vilvh_h;
        break;
    case dt_X86_INS_VPUNPCKHDQ:
        tr_inst = la_vilvh_w;
        break;
    case dt_X86_INS_VPUNPCKHQDQ:
        tr_inst = la_vilvh_d;
        break;
    default:
        lsassert(0);
        return false;
    }
    return translate_avx_lane_3op_lsx(pir1, tr_inst);
}

bool translate_vpunpcklxx_lsx(IR1_INST *pir1)
{
    avx_lsx_lane_3op_fn tr_inst;

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPUNPCKLBW:
        tr_inst = la_vilvl_b;
        break;
    case dt_X86_INS_VPUNPCKLWD:
        tr_inst = la_vilvl_h;
        break;
    case dt_X86_INS_VPUNPCKLDQ:
        tr_inst = la_vilvl_w;
        break;
    case dt_X86_INS_VPUNPCKLQDQ:
        tr_inst = la_vilvl_d;
        break;
    default:
        lsassert(0);
        return false;
    }
    return translate_avx_lane_3op_lsx(pir1, tr_inst);
}

bool translate_vpunpcklqdq_lsx(IR1_INST *pir1)
{
    return translate_vpunpcklxx_lsx(pir1);
}

static bool translate_vunpckxx_lsx(IR1_INST *pir1, bool high,
                                   bool packed_double)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    avx_lsx_lane_3op_fn tr_inst;
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(opnd0) || ymm);
    lsassert(ir1_opnd_is_xmm(opnd1) == !ymm ||
             ir1_opnd_is_ymm(opnd1) == ymm);
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));

    if (packed_double) {
        tr_inst = high ? la_vilvh_d : la_vilvl_d;
    } else {
        tr_inst = high ? la_vilvh_w : la_vilvl_w;
    }
    if (ymm) {
        tr_save_ymm_to_env(UINT16_MAX);
    }
    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    tr_inst(result_low, src2_low, src1_low);

    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();
        tr_inst(result_high, src2_high, src1_high);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    } else {
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    return true;
}

bool translate_vunpckhpd_lsx(IR1_INST *pir1)
{
    return translate_vunpckxx_lsx(pir1, true, true);
}

bool translate_vunpckhps_lsx(IR1_INST *pir1)
{
    return translate_vunpckxx_lsx(pir1, true, false);
}

bool translate_vunpcklpd_lsx(IR1_INST *pir1)
{
    return translate_vunpckxx_lsx(pir1, false, true);
}

bool translate_vunpcklps_lsx(IR1_INST *pir1)
{
    return translate_vunpckxx_lsx(pir1, false, false);
}

static uint8_t map_vshufpd_lsx_imm(uint8_t selector)
{
    static const uint8_t map[4] = { 0x8, 0x9, 0xc, 0xd };

    lsassert(selector < 4);
    return map[selector];
}

static void translate_vshufpd_lane_lsx(IR2_OPND result, IR2_OPND src1,
                                       IR2_OPND src2, uint8_t selector)
{
    la_vori_b(result, src1, 0);
    la_vshuf4i_d(result, src2, map_vshufpd_lsx_imm(selector));
}

bool translate_vshufpd_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(opnd0) || ymm);
    lsassert(ir1_opnd_is_xmm(opnd1) == !ymm ||
             ir1_opnd_is_ymm(opnd1) == ymm);
    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    translate_vshufpd_lane_lsx(result_low, src1_low, src2_low, imm & 3);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();
        translate_vshufpd_lane_lsx(result_high, src1_high, src2_high,
                                   (imm >> 2) & 3);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    } else {
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    return true;
}

static void translate_vshufps_lane_lsx(IR2_OPND result, IR2_OPND src1,
                                       IR2_OPND src2, uint8_t imm)
{
    IR2_OPND src1_shuffled = ra_alloc_ftemp();
    IR2_OPND src2_shuffled = ra_alloc_ftemp();

    la_vshuf4i_w(src1_shuffled, src1, imm & 0xf);
    la_vshuf4i_w(src2_shuffled, src2, imm >> 4);
    la_vpickev_d(result, src2_shuffled, src1_shuffled);
    ra_free_temp(src1_shuffled);
    ra_free_temp(src2_shuffled);
}

bool translate_vshufps_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(opnd0) || ymm);
    lsassert(ir1_opnd_is_xmm(opnd1) == !ymm ||
             ir1_opnd_is_ymm(opnd1) == ymm);
    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    translate_vshufps_lane_lsx(result_low, src1_low, src2_low, imm);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();
        translate_vshufps_lane_lsx(result_high, src1_high, src2_high, imm);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    } else {
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    return true;
}

static void translate_vpshufb_lane_lsx(IR2_OPND result, IR2_OPND src,
                                       IR2_OPND control)
{
    IR2_OPND index = ra_alloc_ftemp();
    IR2_OPND zero_mask = ra_alloc_ftemp();

    la_vandi_b(index, control, 0xf);
    la_vslti_b(zero_mask, control, 0);
    la_vshuf_b(result, src, src, index);
    la_vandn_v(result, zero_mask, result);
    ra_free_temp(zero_mask);
    ra_free_temp(index);
}

bool translate_vpshufb_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *control_opnd = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND control_low;
    IR2_OPND control_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_xmm(src_opnd) == !ymm ||
             ir1_opnd_is_ymm(src_opnd) == ymm);
    if (ymm) {
        tr_save_ymm_to_env(UINT16_MAX);
    }
    load_avx_lsx_operand(src_opnd, ymm, &src_low, &src_high);
    load_avx_lsx_operand(control_opnd, ymm, &control_low, &control_high);
    translate_vpshufb_lane_lsx(result_low, src_low, control_low);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        translate_vpshufb_lane_lsx(result_high, src_high, control_high);
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src_high);
        ra_free_temp(control_high);
    } else {
        store_avx_lsx_result(dest_opnd, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src_low);
    ra_free_temp(control_low);
    return true;
}

bool translate_vpshufd_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 2));
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_mem(src_opnd) ||
             ir1_opnd_is_xmm(src_opnd) == !ymm ||
             ir1_opnd_is_ymm(src_opnd) == ymm);
    if (ymm) {
        tr_save_ymm_to_env(UINT16_MAX);
    }
    load_avx_lsx_operand(src_opnd, ymm, &src_low, &src_high);
    la_vshuf4i_w(result_low, src_low, imm);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        la_vshuf4i_w(result_high, src_high, imm);
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src_high);
    } else {
        store_avx_lsx_result(dest_opnd, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src_low);
    return true;
}

static void translate_vpshufh_lane_lsx(IR2_OPND result, IR2_OPND src,
                                       uint8_t imm, bool high_half)
{
    IR2_OPND shuffled = ra_alloc_ftemp();
    IR2_OPND value = ra_alloc_itemp();

    la_vshuf4i_h(shuffled, src, imm);
    la_vpickve2gr_du(value, src, high_half ? 0 : 1);
    la_vxor_v(result, result, result);
    la_vinsgr2vr_d(result, value, high_half ? 0 : 1);
    la_vpickve2gr_du(value, shuffled, high_half ? 1 : 0);
    la_vinsgr2vr_d(result, value, high_half ? 1 : 0);
    ra_free_temp(value);
    ra_free_temp(shuffled);
}

static bool translate_vpshufh_lsx(IR1_INST *pir1, bool high_half)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 2));
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_mem(src_opnd) ||
             ir1_opnd_is_xmm(src_opnd) == !ymm ||
             ir1_opnd_is_ymm(src_opnd) == ymm);
    if (ymm) {
        tr_save_ymm_to_env(UINT16_MAX);
    }
    load_avx_lsx_operand(src_opnd, ymm, &src_low, &src_high);
    translate_vpshufh_lane_lsx(result_low, src_low, imm, high_half);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        translate_vpshufh_lane_lsx(result_high, src_high, imm, high_half);
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src_high);
    } else {
        store_avx_lsx_result(dest_opnd, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src_low);
    return true;
}

bool translate_vpshufhw_lsx(IR1_INST *pir1)
{
    return translate_vpshufh_lsx(pir1, true);
}

bool translate_vpshuflw_lsx(IR1_INST *pir1)
{
    return translate_vpshufh_lsx(pir1, false);
}

static IR2_OPND build_blend_mask_lsx(int element_bits, uint8_t imm,
                                     int elements)
{
    IR2_OPND mask = ra_alloc_ftemp();
    IR2_OPND all_ones = ra_alloc_itemp();

    la_vxor_v(mask, mask, mask);
    li_d(all_ones, UINT64_MAX);
    for (int i = 0; i < elements; ++i) {
        if (!(imm & (1u << i))) {
            continue;
        }
        switch (element_bits) {
        case 16:
            la_vinsgr2vr_h(mask, all_ones, i);
            break;
        case 32:
            la_vinsgr2vr_w(mask, all_ones, i);
            break;
        case 64:
            la_vinsgr2vr_d(mask, all_ones, i);
            break;
        default:
            lsassert(0);
        }
    }
    ra_free_temp(all_ones);
    return mask;
}

static bool translate_blend_imm_lsx(IR1_INST *pir1, int element_bits)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src1_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *src2_opnd = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    int elements = 128 / element_bits;
    IR2_OPND src1_low;
    IR2_OPND src1_high;
    IR2_OPND src2_low;
    IR2_OPND src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_xmm(src1_opnd) == !ymm ||
             ir1_opnd_is_ymm(src1_opnd) == ymm);
    lsassert(ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    load_avx_lsx_operand(src1_opnd, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(src2_opnd, ymm, &src2_low, &src2_high);
    IR2_OPND mask_low = build_blend_mask_lsx(element_bits, imm, elements);
    la_vbitsel_v(result_low, src1_low, src2_low, mask_low);
    ra_free_temp(mask_low);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();
        uint8_t high_imm = element_bits == 16 ? imm : imm >> elements;
        IR2_OPND mask_high = build_blend_mask_lsx(element_bits, high_imm,
                                                  elements);

        la_vbitsel_v(result_high, src1_high, src2_high, mask_high);
        ra_free_temp(mask_high);
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    } else {
        store_avx_lsx_result(dest_opnd, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    return true;
}

static bool translate_blend_variable_lsx(IR1_INST *pir1, int element_bits)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src1_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *src2_opnd = ir1_get_opnd(pir1, 2);
    IR1_OPND *mask_opnd = ir1_get_opnd(pir1, 3);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    IR2_OPND src1_low;
    IR2_OPND src1_high;
    IR2_OPND src2_low;
    IR2_OPND src2_high;
    IR2_OPND mask_low;
    IR2_OPND mask_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_xmm(src1_opnd) == !ymm ||
             ir1_opnd_is_ymm(src1_opnd) == ymm);
    lsassert(ir1_opnd_is_xmm(mask_opnd) == !ymm ||
             ir1_opnd_is_ymm(mask_opnd) == ymm);
    load_avx_lsx_operand(src1_opnd, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(src2_opnd, ymm, &src2_low, &src2_high);
    load_avx_lsx_operand(mask_opnd, ymm, &mask_low, &mask_high);
    if (element_bits == 32) {
        la_vslti_w(mask_low, mask_low, 0);
    } else {
        la_vslti_d(mask_low, mask_low, 0);
    }
    la_vbitsel_v(result_low, src1_low, src2_low, mask_low);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();
        if (element_bits == 32) {
            la_vslti_w(mask_high, mask_high, 0);
        } else {
            la_vslti_d(mask_high, mask_high, 0);
        }
        la_vbitsel_v(result_high, src1_high, src2_high, mask_high);
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
        ra_free_temp(mask_high);
    } else {
        store_avx_lsx_result(dest_opnd, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    ra_free_temp(mask_low);
    return true;
}

bool translate_vblendpd_lsx(IR1_INST *pir1)
{
    return translate_blend_imm_lsx(pir1, 64);
}

bool translate_vblendps_lsx(IR1_INST *pir1)
{
    return translate_blend_imm_lsx(pir1, 32);
}

bool translate_vblendvpd_lsx(IR1_INST *pir1)
{
    return translate_blend_variable_lsx(pir1, 64);
}

bool translate_vblendvps_lsx(IR1_INST *pir1)
{
    return translate_blend_variable_lsx(pir1, 32);
}

bool translate_vpblendd_lsx(IR1_INST *pir1)
{
    return translate_blend_imm_lsx(pir1, 32);
}

bool translate_vpblendw_lsx(IR1_INST *pir1)
{
    return translate_blend_imm_lsx(pir1, 16);
}

static void translate_vpalignr_lane_lsx(IR2_OPND result, IR2_OPND src1,
                                        IR2_OPND src2, uint8_t imm)
{
    if (imm >= 32) {
        la_vxor_v(result, result, result);
    } else if (imm >= 16) {
        la_vbsrl_v(result, src1, imm - 16);
    } else if (imm == 0) {
        la_vori_b(result, src2, 0);
    } else {
        IR2_OPND shifted_src2 = ra_alloc_ftemp();

        la_vbsrl_v(shifted_src2, src2, imm);
        la_vbsll_v(result, src1, 16 - imm);
        la_vor_v(result, shifted_src2, result);
        ra_free_temp(shifted_src2);
    }
}

bool translate_vpalignr_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src1_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *src2_opnd = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    IR2_OPND src1_low;
    IR2_OPND src1_high;
    IR2_OPND src2_low;
    IR2_OPND src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_xmm(src1_opnd) == !ymm ||
             ir1_opnd_is_ymm(src1_opnd) == ymm);
    lsassert(ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    load_avx_lsx_operand(src1_opnd, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(src2_opnd, ymm, &src2_low, &src2_high);
    translate_vpalignr_lane_lsx(result_low, src1_low, src2_low, imm);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        translate_vpalignr_lane_lsx(result_high, src1_high, src2_high, imm);
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    } else {
        store_avx_lsx_result(dest_opnd, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    return true;
}

static IR2_OPND select_128_lane_lsx(IR2_OPND src1_low, IR2_OPND src1_high,
                                    IR2_OPND src2_low, IR2_OPND src2_high,
                                    uint8_t selector, bool zero)
{
    IR2_OPND result = ra_alloc_ftemp();

    if (zero) {
        la_vxor_v(result, result, result);
    } else {
        switch (selector) {
        case 0:
            la_vori_b(result, src1_low, 0);
            break;
        case 1:
            la_vori_b(result, src1_high, 0);
            break;
        case 2:
            la_vori_b(result, src2_low, 0);
            break;
        case 3:
            la_vori_b(result, src2_high, 0);
            break;
        default:
            lsassert(0);
        }
    }
    return result;
}

static bool translate_vperm2f128_lsx_common(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src1_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *src2_opnd = ir1_get_opnd(pir1, 2);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    IR2_OPND src1_low;
    IR2_OPND src1_high;
    IR2_OPND src2_low;
    IR2_OPND src2_high;
    IR2_OPND result_low;
    IR2_OPND result_high;

    lsassert(ir1_opnd_is_ymm(dest_opnd) && ir1_opnd_is_ymm(src1_opnd));
    lsassert(ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    load_avx_lsx_operand(src1_opnd, true, &src1_low, &src1_high);
    load_avx_lsx_operand(src2_opnd, true, &src2_low, &src2_high);
    result_low = select_128_lane_lsx(src1_low, src1_high, src2_low,
                                     src2_high, imm & 0x3, (imm & 0x8) != 0);
    result_high = select_128_lane_lsx(src1_low, src1_high, src2_low,
                                      src2_high, (imm >> 4) & 0x3,
                                      (imm & 0x80) != 0);
    store_avx_lsx_result(dest_opnd, result_low, result_high);
    ra_free_temp(result_low);
    ra_free_temp(result_high);
    ra_free_temp(src1_low);
    ra_free_temp(src1_high);
    ra_free_temp(src2_low);
    ra_free_temp(src2_high);
    return true;
}

bool translate_vperm2f128_lsx(IR1_INST *pir1)
{
    return translate_vperm2f128_lsx_common(pir1);
}

bool translate_vperm2i128_lsx(IR1_INST *pir1)
{
    return translate_vperm2f128_lsx_common(pir1);
}

static uint8_t map_vpermilpd_imm_lsx(uint8_t selector)
{
    return (selector & 1) | (((selector >> 1) & 1) << 2);
}

static void translate_vpermilpd_lane_lsx(IR2_OPND result, IR2_OPND src,
                                         IR2_OPND control, bool immediate,
                                         uint8_t imm)
{
    if (immediate) {
        la_vori_b(result, src, 0);
        la_vshuf4i_d(result, src, map_vpermilpd_imm_lsx(imm));
    } else {
        la_vsrli_d(control, control, 1);
        la_vandi_b(control, control, 1);
        la_vshuf_d(control, src, src);
        la_vori_b(result, control, 0);
    }
}

static void translate_vpermilps_lane_lsx(IR2_OPND result, IR2_OPND src,
                                         IR2_OPND control, bool immediate,
                                         uint8_t imm)
{
    if (immediate) {
        la_vshuf4i_w(result, src, imm);
    } else {
        la_vandi_b(control, control, 3);
        la_vshuf_w(control, src, src);
        la_vori_b(result, control, 0);
    }
}

static bool translate_vpermil_lsx(IR1_INST *pir1, bool pd)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *control_opnd = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    bool immediate = ir1_opnd_is_imm(control_opnd);
    uint8_t imm = immediate ? ir1_opnd_uimm(control_opnd) : 0;
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND control_low = { 0 };
    IR2_OPND control_high = { 0 };
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_mem(src_opnd) ||
             ir1_opnd_is_xmm(src_opnd) == !ymm ||
             ir1_opnd_is_ymm(src_opnd) == ymm);
    load_avx_lsx_operand(src_opnd, ymm, &src_low, &src_high);
    if (!immediate) {
        load_avx_lsx_operand(control_opnd, ymm, &control_low, &control_high);
    }
    if (pd) {
        translate_vpermilpd_lane_lsx(result_low, src_low, control_low,
                                     immediate, imm & 0x3);
    } else {
        translate_vpermilps_lane_lsx(result_low, src_low, control_low,
                                     immediate, imm);
    }
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        if (pd) {
            translate_vpermilpd_lane_lsx(result_high, src_high, control_high,
                                         immediate, (imm >> 2) & 0x3);
        } else {
            translate_vpermilps_lane_lsx(result_high, src_high, control_high,
                                         immediate, imm);
        }
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src_low);
        ra_free_temp(src_high);
        if (!immediate) {
            ra_free_temp(control_low);
            ra_free_temp(control_high);
        }
    } else {
        store_avx_lsx_result(dest_opnd, result_low, result_low);
        ra_free_temp(src_low);
        if (!immediate) {
            ra_free_temp(control_low);
        }
    }
    ra_free_temp(result_low);
    return true;
}

bool translate_vpermilpd_lsx(IR1_INST *pir1)
{
    return translate_vpermil_lsx(pir1, true);
}

bool translate_vpermilps_lsx(IR1_INST *pir1)
{
    return translate_vpermil_lsx(pir1, false);
}

static void translate_vpermute_q_imm_lsx(IR2_OPND result_low,
                                         IR2_OPND result_high,
                                         IR2_OPND src_low, IR2_OPND src_high,
                                         uint8_t imm)
{
    la_vxor_v(result_low, result_low, result_low);
    la_vxor_v(result_high, result_high, result_high);
    for (int i = 0; i < 4; ++i) {
        int selector = (imm >> (i * 2)) & 0x3;
        IR2_OPND value = ra_alloc_itemp();

        if (selector < 2) {
            la_vpickve2gr_du(value, src_low, selector);
        } else {
            la_vpickve2gr_du(value, src_high, selector - 2);
        }
        if (i < 2) {
            la_vinsgr2vr_d(result_low, value, i);
        } else {
            la_vinsgr2vr_d(result_high, value, i - 2);
        }
        ra_free_temp(value);
    }
}

static void translate_vpermute_w_dynamic_lsx(IR2_OPND result,
                                             IR2_OPND data_low,
                                             IR2_OPND data_high,
                                             IR2_OPND index)
{
    IR2_OPND control = ra_alloc_ftemp();

    la_vandi_b(control, index, 7);
    /* vshuf.w takes its selectors from the old destination and combines
     * vk[0..3] with vj[0..3].  Put the low half in vk and high half in vj. */
    la_vshuf_w(control, data_high, data_low);
    la_vori_b(result, control, 0);
    ra_free_temp(control);
}

bool translate_vpermd_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *index_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *data_opnd = ir1_get_opnd(pir1, 2);
    IR2_OPND index_low;
    IR2_OPND index_high;
    IR2_OPND data_low;
    IR2_OPND data_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    IR2_OPND result_high = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_ymm(dest_opnd) && ir1_opnd_is_ymm(index_opnd));
    load_avx_lsx_operand(index_opnd, true, &index_low, &index_high);
    load_avx_lsx_operand(data_opnd, true, &data_low, &data_high);
    translate_vpermute_w_dynamic_lsx(result_low, data_low, data_high,
                                     index_low);
    translate_vpermute_w_dynamic_lsx(result_high, data_low, data_high,
                                     index_high);
    store_avx_lsx_result(dest_opnd, result_low, result_high);
    ra_free_temp(result_low);
    ra_free_temp(result_high);
    ra_free_temp(index_low);
    ra_free_temp(index_high);
    ra_free_temp(data_low);
    ra_free_temp(data_high);
    return true;
}

bool translate_vpermpx_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *index_or_imm = ir1_get_opnd(pir1, 2);
    IR2_OPND data_low;
    IR2_OPND data_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    IR2_OPND result_high = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_ymm(dest_opnd));
    if (ir1_opcode(pir1) == dt_X86_INS_VPERMPD) {
        lsassert(ir1_opnd_is_imm(index_or_imm));
        load_avx_lsx_operand(ir1_get_opnd(pir1, 1), true,
                             &data_low, &data_high);
        translate_vpermute_q_imm_lsx(result_low, result_high,
                                     data_low, data_high,
                                     ir1_opnd_uimm(index_or_imm));
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(data_low);
        ra_free_temp(data_high);
    } else {
        IR1_OPND *index_opnd = ir1_get_opnd(pir1, 1);
        IR1_OPND *data_opnd = ir1_get_opnd(pir1, 2);
        IR2_OPND index_low;
        IR2_OPND index_high;

        load_avx_lsx_operand(data_opnd, true,
                             &data_low, &data_high);
        load_avx_lsx_operand(index_opnd, true, &index_low, &index_high);
        translate_vpermute_w_dynamic_lsx(result_low, data_low, data_high,
                                         index_low);
        translate_vpermute_w_dynamic_lsx(result_high, data_low, data_high,
                                         index_high);
        store_avx_lsx_result(dest_opnd, result_low, result_high);
        ra_free_temp(index_low);
        ra_free_temp(index_high);
        ra_free_temp(data_low);
        ra_free_temp(data_high);
    }
    ra_free_temp(result_low);
    ra_free_temp(result_high);
    return true;
}

bool translate_vpermq_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *imm_opnd = ir1_get_opnd(pir1, 2);
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    IR2_OPND result_high = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_ymm(dest_opnd) && ir1_opnd_is_imm(imm_opnd));
    load_avx_lsx_operand(src_opnd, true, &src_low, &src_high);
    translate_vpermute_q_imm_lsx(result_low, result_high, src_low, src_high,
                                 ir1_opnd_uimm(imm_opnd));
    store_avx_lsx_result(dest_opnd, result_low, result_high);
    ra_free_temp(result_low);
    ra_free_temp(result_high);
    ra_free_temp(src_low);
    ra_free_temp(src_high);
    return true;
}

bool translate_vpunpcklxx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR1_OPCODE op = ir1_opcode(pir1);

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND);
    switch (op) {
        case dt_X86_INS_VPUNPCKLBW:
            tr_inst = la_xvilvl_b;
            break;
        case dt_X86_INS_VPUNPCKLWD:
            tr_inst = la_xvilvl_h;
            break;
        case dt_X86_INS_VPUNPCKLDQ:
            tr_inst = la_xvilvl_w;
            break;
        case dt_X86_INS_VPUNPCKLQDQ:
            tr_inst = la_xvilvl_d;
            break;
        default:
            tr_inst = NULL;
            lsassert(0);
            break;
    }
    tr_inst(dest, src2, src1);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpxor_lsx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    {
        /* LSX-only path */
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        IR2_OPND dest = ra_alloc_xmm(dest_index);
        IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));

        if (ir1_opnd_is_xmm(opnd0)) {
            IR2_OPND src2 = load_freg128_from_ir1(opnd2);

            la_vxor_v(dest, src1, src2);
            clear_ymm_high128_shadow(dest_index);
        } else {
            IR2_OPND src1_high = load_ymm_high128_shadow(
                ir1_opnd_base_reg_num(opnd1));
            IR2_OPND src2;
            IR2_OPND src2_high;

            if (ir1_opnd_is_ymm(opnd2)) {
                int src2_index = ir1_opnd_base_reg_num(opnd2);

                src2 = ra_alloc_xmm(src2_index);
                src2_high = load_ymm_high128_shadow(src2_index);
            } else {
                int mem_imm;
                IR2_OPND mem_opnd = convert_mem(opnd2, &mem_imm);

                lsassert(ir1_opnd_is_mem(opnd2));
                src2 = ra_alloc_ftemp();
                src2_high = ra_alloc_ftemp();
                gen_test_page_flag(mem_opnd, mem_imm, PAGE_READ);
                la_vld(src2, mem_opnd, mem_imm);
                mem_opnd = mem_imm_add_disp(mem_opnd, &mem_imm, 16);
                gen_test_page_flag(mem_opnd, mem_imm, PAGE_READ);
                la_vld(src2_high, mem_opnd, mem_imm);
            }

            la_vxor_v(dest, src1, src2);
            la_vxor_v(src1_high, src1_high, src2_high);
            store_ymm_high128_shadow(src1_high, dest_index);
        }
    }
    return true;
}

bool translate_vpxor(IR1_INST * pir1) {
    translate_vxorps(pir1);
    return true;
}

bool translate_vfmaddxxxss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFMADD132SS:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFMADD231SS:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFMADD213SS:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    la_fmadd_s(temp, temp1, temp2, temp3);
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfmaddxxxsd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFMADD132SD:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFMADD231SD:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFMADD213SD:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    la_fmadd_d(temp, temp1, temp2, temp3);
    la_xvinsve0_d(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfmaddxxxpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFMADD132PD:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFMADD231PD:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFMADD213PD:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    tr_inst = ir1_opnd_is_xmm(opnd0) ? la_vfmadd_d : la_xvfmadd_d;
    tr_inst(temp, temp1, temp2, temp3);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfmaddxxxps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFMADD132PS:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFMADD231PS:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFMADD213PS:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    tr_inst = ir1_opnd_is_xmm(opnd0) ? la_vfmadd_s : la_xvfmadd_s;
    tr_inst(temp, temp1, temp2, temp3);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfmsubxxxss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFMSUB132SS:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFMSUB231SS:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFMSUB213SS:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    la_fmsub_s(temp, temp1, temp2, temp3);
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfmsubxxxsd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFMSUB132SD:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFMSUB231SD:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFMSUB213SD:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    la_fmsub_d(temp, temp1, temp2, temp3);
    la_xvinsve0_d(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfmsubxxxpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFMSUB132PD:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFMSUB231PD:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFMSUB213PD:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    tr_inst = ir1_opnd_is_xmm(opnd0) ? la_vfmsub_d : la_xvfmsub_d;
    tr_inst(temp, temp1, temp2, temp3);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfmsubxxxps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFMSUB132PS:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFMSUB231PS:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFMSUB213PS:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    tr_inst = ir1_opnd_is_xmm(opnd0) ? la_vfmsub_s : la_xvfmsub_s;
    tr_inst(temp, temp1, temp2, temp3);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfnmaddxxxss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFNMADD132SS:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFNMADD231SS:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFNMADD213SS:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    la_fneg_s(temp, temp1);
    la_fmadd_s(temp, temp, temp2, temp3);

    IR2_OPND label_over = ra_alloc_label();
    /* check if result is NaN */
    la_fcmp_cond_s(fcc0_ir2_opnd, temp, temp, 0x8);

    /* if no NaN happend, compution done */
    la_bceqz(fcc0_ir2_opnd, label_over);

    /* if INVALID NaN did happen, use original operands to generate correct NaN */
    la_fmsub_s(temp, src1, src2, dest);

    la_label(label_over);
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfnmaddxxxsd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFNMADD132SD:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFNMADD231SD:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFNMADD213SD:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    la_fneg_d(temp, temp1);
    la_fmadd_d(temp, temp, temp2, temp3);

    IR2_OPND label_over = ra_alloc_label();
    /* check if result is NaN */
    la_fcmp_cond_d(fcc0_ir2_opnd, temp, temp, 0x8);

    /* if no NaN happend, compution done */
    la_bceqz(fcc0_ir2_opnd, label_over);

    /* if INVALID NaN did happen, use original operands to generate correct NaN */
    la_fmsub_d(temp, src1, src2, dest);

    la_label(label_over);
    la_xvinsve0_d(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfnmaddxxxpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR2_OPND itemp = ra_alloc_itemp();
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFNMADD132PD:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFNMADD231PD:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFNMADD213PD:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    tr_inst = ir1_opnd_is_xmm(opnd0) ? la_vfmadd_d : la_xvfmadd_d;

    /* change the first operand sign bit*/
    la_lu52i_d(itemp, zero_ir2_opnd, 0x800);
    la_vinsgr2vr_d(temp, itemp, 0);
    la_xvreplve0_d(temp, temp);
    la_xvxor_v(temp, temp1, temp);
    /* compute the result*/
    tr_inst(temp, temp, temp2, temp3);

    IR2_OPND mask = ra_alloc_ftemp();
    IR2_OPND src1_temp = ra_alloc_ftemp();
    IR2_OPND src2_temp = ra_alloc_ftemp();
    IR2_OPND src3_temp = ra_alloc_ftemp();

    /* check if result is NaN */
    la_xvfcmp_cond_d(mask, temp, temp, 0x8);
    la_xvand_v(src1_temp, mask, dest);
    la_xvand_v(src2_temp, mask, src1);
    la_xvand_v(src3_temp, mask, src2);
    tr_inst(src1_temp, src2_temp, src3_temp, src1_temp);

    la_xvbitsel_v(temp, temp, src1_temp, mask);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);

    ra_free_temp(src1_temp);
    ra_free_temp(src2_temp);
    ra_free_temp(src3_temp);
    ra_free_temp(temp);
    ra_free_temp(mask);
    return true;
}

bool translate_vfnmaddxxxps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR2_OPND ftemp = ra_alloc_ftemp();
    IR2_OPND itemp = ra_alloc_itemp();
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFNMADD132PS:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFNMADD231PS:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFNMADD213PS:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    tr_inst = ir1_opnd_is_xmm(opnd0) ? la_vfmadd_s : la_xvfmadd_s;
    la_lu12i_w(itemp, 0x80000);
    la_vinsgr2vr_w(ftemp, itemp, 0);
    la_xvreplve0_w(ftemp, ftemp);
    la_xvxor_v(temp, temp1, ftemp);

    /* compute the result*/
    tr_inst(temp, temp, temp2, temp3);

    IR2_OPND mask = ra_alloc_ftemp();
    IR2_OPND src1_temp = ra_alloc_ftemp();
    IR2_OPND src2_temp = ra_alloc_ftemp();
    IR2_OPND src3_temp = ra_alloc_ftemp();

    /* check if result is NaN */
    la_xvfcmp_cond_s(mask, temp, temp, 0x8);
    la_xvand_v(src1_temp, mask, dest);
    la_xvand_v(src2_temp, mask, src1);
    la_xvand_v(src3_temp, mask, src2);
    tr_inst(src1_temp, src2_temp, src3_temp, src1_temp);

    la_xvbitsel_v(temp, temp, src1_temp, mask);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    ra_free_temp(src1_temp);
    ra_free_temp(src2_temp);
    ra_free_temp(src3_temp);
    ra_free_temp(temp);
    ra_free_temp(mask);
    return true;
}

bool translate_vfnmsubxxxss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFNMSUB132SS:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFNMSUB231SS:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFNMSUB213SS:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    la_fneg_s(temp, temp1);
    la_fmsub_s(temp, temp, temp2, temp3);

    IR2_OPND label_over = ra_alloc_label();
    /* check if result is NaN */
    la_fcmp_cond_s(fcc0_ir2_opnd, temp, temp, 0x8);

    /* if no NaN happend, compution done */
    la_bceqz(fcc0_ir2_opnd, label_over);

    /* if INVALID NaN did happen, use original operands to generate correct NaN */
    la_fmsub_s(temp, src1, src2, dest);

    la_label(label_over);
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfnmsubxxxsd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFNMSUB132SD:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFNMSUB231SD:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFNMSUB213SD:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }
    /* if the dest is NaN , translation may take mistake
     * becasue x86 vfnmsub and 3a5000 vfmsub may produce different NaN*/
    la_fneg_d(temp, temp1);
    la_fmsub_d(temp, temp, temp2, temp3);

    IR2_OPND label_over = ra_alloc_label();
    /* check if result is NaN */
    la_fcmp_cond_d(fcc0_ir2_opnd, temp, temp, 0x8);

    /* if no NaN happend, compution done */
    la_bceqz(fcc0_ir2_opnd, label_over);

    /* if INVALID NaN did happen, use original operands to generate correct NaN */
    la_fmsub_d(temp, src1, src2, dest);

    la_label(label_over);
    la_xvinsve0_d(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vfnmsubxxxpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR2_OPND ftemp = ra_alloc_ftemp();
    IR2_OPND itemp = ra_alloc_itemp();
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFNMSUB132PD:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFNMSUB231PD:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFNMSUB213PD:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }
    /* if the dest is NaN , translation may take mistake
     * becasue x86 vfnmsub and 3a5000 vfmsub may produce different NaN*/
    tr_inst = ir1_opnd_is_xmm(opnd0) ? la_vfmsub_d : la_xvfmsub_d;
    la_lu52i_d(itemp, zero_ir2_opnd, 0x800);
    la_vinsgr2vr_d(ftemp, itemp, 0);
    la_xvreplve0_d(ftemp, ftemp);
    la_xvxor_v(temp, temp1, ftemp);
    tr_inst(temp, temp, temp2, temp3);

    IR2_OPND mask = ra_alloc_ftemp();
    IR2_OPND src1_temp = ra_alloc_ftemp();
    IR2_OPND src2_temp = ra_alloc_ftemp();
    IR2_OPND src3_temp = ra_alloc_ftemp();

    /* check if result is NaN */
    la_xvfcmp_cond_d(mask, temp, temp, 0x8);
    la_xvand_v(src1_temp, mask, dest);
    la_xvand_v(src2_temp, mask, src1);
    la_xvand_v(src3_temp, mask, src2);
    tr_inst(src1_temp, src2_temp, src3_temp, src1_temp);

    la_xvbitsel_v(temp, temp, src1_temp, mask);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    ra_free_temp(src1_temp);
    ra_free_temp(src2_temp);
    ra_free_temp(src3_temp);
    ra_free_temp(temp);
    ra_free_temp(mask);
    return true;
}

bool translate_vfnmsubxxxps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1, temp2, temp3;
    IR2_OPND ftemp = ra_alloc_ftemp();
    IR2_OPND itemp = ra_alloc_itemp();
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VFNMSUB132PS:
            temp1 = dest, temp2 = src2, temp3 = src1;
            break;
        case dt_X86_INS_VFNMSUB231PS:
            temp1 = src1, temp2 = src2, temp3 = dest;
            break;
        case dt_X86_INS_VFNMSUB213PS:
            temp1 = src1, temp2 = dest, temp3 = src2;
            break;
        default:
            lsassert(0);
            break;
    }

    tr_inst = ir1_opnd_is_xmm(opnd0) ? la_vfmsub_s : la_xvfmsub_s;
    la_lu12i_w(itemp, 0x80000);
    la_vinsgr2vr_w(ftemp, itemp, 0);
    la_xvreplve0_w(ftemp, ftemp);
    la_xvxor_v(temp, temp1, ftemp);
    tr_inst(temp, temp, temp2, temp3);

    IR2_OPND mask = ra_alloc_ftemp();
    IR2_OPND src1_temp = ra_alloc_ftemp();
    IR2_OPND src2_temp = ra_alloc_ftemp();
    IR2_OPND src3_temp = ra_alloc_ftemp();

    /* check if result is NaN */
    la_xvfcmp_cond_s(mask, temp, temp, 0x8);
    la_xvand_v(src1_temp, mask, dest);
    la_xvand_v(src2_temp, mask, src1);
    la_xvand_v(src3_temp, mask, src2);
    tr_inst(src1_temp, src2_temp, src3_temp, src1_temp);

    la_xvbitsel_v(temp, temp, src1_temp, mask);
    la_xvori_b(dest, temp, 0);
    if (ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    ra_free_temp(src1_temp);
    ra_free_temp(src2_temp);
    ra_free_temp(src3_temp);
    ra_free_temp(temp);
    ra_free_temp(mask);
    return true;
}

bool translate_vpbroadcastq(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src = ir1_opnd_is_mem(opnd1) ?
        load_freg128_from_ir1(opnd1) :
        ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));

    if (ir1_opnd_is_xmm(opnd0)) {
        la_xvreplve0_d(dest, src);
        set_high128_xreg_to_zero(dest);
    } else {
        la_xvreplve0_d(dest, src);
        la_xvinsve0_d(dest, dest, 2);
        la_xvinsve0_d(dest, dest, 3);
    }
    return true;
}

bool translate_vpaddq(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));

    if (ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        la_vadd_d(dest, src1, src2);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        la_xvadd_d(dest, src1, src2);
    } else {
        lsassert(0);
    }
    return true;
}

bool translate_vzeroupper_lsx(IR1_INST *pir1)
{
    clear_all_ymm_high128_shadows();
    return true;
}

bool translate_vzeroupper(IR1_INST *pir1)
{
    int reg_xmm = 8;
#ifdef TARGET_X86_64
    reg_xmm = 16;
#endif

    for (int i = 0; i < reg_xmm; ++i) {
        IR2_OPND dest = ra_alloc_xmm(i);
        set_high128_xreg_to_zero(dest);
    }
    return true;
}
#if 0
bool translate_vpinsrb(IR1_INST *pir1)
{
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src0 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src1 = load_ireg_from_ir1(ir1_get_opnd(pir1, 2), UNKNOWN_EXTENSION, false);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    la_vand_v(dest, src0, src0);
    la_vinsgr2vr_b(dest, src1, imm);
    set_high128_xreg_to_zero(dest);
    return true;
}

#endif

bool translate_vpinsrx_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src1_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *src2_opnd = ir1_get_opnd(pir1, 2);
    uint8 imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    IR2_OPND src2;
    IR2_OPND dest;
    IR2_OPND src1;
    int dest_index;

    lsassert(ir1_opnd_is_xmm(dest_opnd) && ir1_opnd_is_xmm(src1_opnd));
    lsassert(ir1_opnd_is_gpr(src2_opnd) || ir1_opnd_is_mem(src2_opnd));
    lsassert(ir1_opnd_is_imm(ir1_get_opnd(pir1, 3)));
    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPINSRB:
        lsassert(ir1_opnd_size(src2_opnd) == 8 ||
                 ir1_opnd_size(src2_opnd) == 32);
        imm &= 0xf;
        src2 = ir1_opnd_is_mem(src2_opnd) ?
            load_u8_from_ir1_mem_exact(src2_opnd) :
            load_ireg_from_ir1(src2_opnd, UNKNOWN_EXTENSION, false);
        break;
    case dt_X86_INS_VPINSRW:
        lsassert(ir1_opnd_size(src2_opnd) == 16 ||
                 ir1_opnd_size(src2_opnd) == 32);
        imm &= 0x7;
        src2 = ir1_opnd_is_mem(src2_opnd) ?
            load_u16_from_ir1_mem_exact(src2_opnd) :
            load_ireg_from_ir1(src2_opnd, UNKNOWN_EXTENSION, false);
        break;
    case dt_X86_INS_VPINSRD:
        lsassert(ir1_opnd_size(src2_opnd) == 32);
        imm &= 0x3;
        src2 = ir1_opnd_is_mem(src2_opnd) ?
            load_u32_from_ir1_mem_exact(src2_opnd) :
            load_ireg_from_ir1(src2_opnd, UNKNOWN_EXTENSION, false);
        break;
    default:
        lsassert(0);
    }
    src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(src1_opnd));
    dest_index = ir1_opnd_base_reg_num(dest_opnd);
    dest = ra_alloc_xmm(dest_index);
    la_vori_b(dest, src1, 0);
    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPINSRB:
        la_vinsgr2vr_b(dest, src2, imm);
        break;
    case dt_X86_INS_VPINSRW:
        la_vinsgr2vr_h(dest, src2, imm);
        break;
    case dt_X86_INS_VPINSRD:
        la_vinsgr2vr_w(dest, src2, imm);
        break;
    default:
        lsassert(0);
    }
    clear_ymm_high128_shadow(dest_index);
    if (ir1_opnd_is_mem(src2_opnd))
        ra_free_temp(src2);
    return true;
}

bool translate_vpinsrq_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    uint8_t imm = ir1_opnd_uimm(opnd3) & 0x1;

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    lsassert(ir1_opnd_is_gpr(opnd2) || ir1_opnd_is_mem(opnd2));
    lsassert(ir1_opnd_size(opnd2) == 64 && ir1_opnd_is_imm(opnd3));

    int dest_index = ir1_opnd_base_reg_num(opnd0);
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = ir1_opnd_is_mem(opnd2) ?
        load_u64_from_ir1_mem_exact(opnd2) :
        load_ireg_from_ir1(opnd2, UNKNOWN_EXTENSION, false);

    la_vori_b(dest, src1, 0);
    la_vinsgr2vr_d(dest, src2, imm);
    clear_ymm_high128_shadow(dest_index);
    if (ir1_opnd_is_mem(opnd2))
        ra_free_temp(src2);

    return true;
}

bool translate_vpinsrq(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    uint8_t imm = ir1_opnd_uimm(opnd3) & 0x1;

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    lsassert(ir1_opnd_is_gpr(opnd2) || ir1_opnd_is_mem(opnd2));
    lsassert(ir1_opnd_size(opnd2) == 64 && ir1_opnd_is_imm(opnd3));

    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = ir1_opnd_is_mem(opnd2) ?
        load_u64_from_ir1_mem_exact(opnd2) :
        load_ireg_from_ir1(opnd2, UNKNOWN_EXTENSION, false);

    la_vori_b(dest, src1, 0);
    la_vinsgr2vr_d(dest, src2, imm);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_xgetbv(IR1_INST *pir1)
{
    tr_gen_call_to_helper_xgetbv();
    return true;
}

bool translate_xsetbv(IR1_INST *pir1)
{
    IR2_OPND eax_opnd = ra_alloc_gpr(eax_index);
    IR2_OPND ecx_opnd = ra_alloc_gpr(ecx_index);
    IR2_OPND edx_opnd = ra_alloc_gpr(edx_index);
    IR2_OPND temp_rfbm = ra_alloc_itemp();

    la_bstrins_d(temp_rfbm, eax_opnd, 31, 0);
    la_bstrins_d(temp_rfbm, edx_opnd, 63, 32);
    tr_gen_call_to_helper_vfll((ADDR)helper_xsetbv, ecx_opnd, temp_rfbm, 0,
            false, LOAD_HELPER_XSETBV);
    return true;
}

bool translate_xsave(IR1_INST *pir1)
{
    IR2_OPND eax_opnd = ra_alloc_gpr(eax_index);
    IR2_OPND edx_opnd = ra_alloc_gpr(edx_index);
    IR2_OPND temp_rfbm = ra_alloc_itemp();
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR2_OPND mem_opnd = convert_mem_no_offset(opnd0);

    la_bstrins_d(temp_rfbm, eax_opnd, 31, 0);
    la_bstrins_d(temp_rfbm, edx_opnd, 63, 32);
    tr_gen_call_to_helper_vfll((ADDR)helper_xsave, mem_opnd, temp_rfbm, 1,
            true, LOAD_HELPER_XSAVE);
    return true;
}

bool translate_xsaveopt(IR1_INST *pir1)
{
    IR2_OPND eax_opnd = ra_alloc_gpr(eax_index);
    IR2_OPND edx_opnd = ra_alloc_gpr(edx_index);
    IR2_OPND temp_rfbm = ra_alloc_itemp();
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR2_OPND mem_opnd = convert_mem_no_offset(opnd0);

    la_bstrins_d(temp_rfbm, eax_opnd, 31, 0);
    la_bstrins_d(temp_rfbm, edx_opnd, 63, 32);
    tr_gen_call_to_helper_vfll((ADDR)helper_xsaveopt, mem_opnd, temp_rfbm, 1,
            true, LOAD_HELPER_XSAVEOPT);
    return true;
}

bool translate_xrstor(IR1_INST *pir1)
{
    IR2_OPND eax_opnd = ra_alloc_gpr(eax_index);
    IR2_OPND edx_opnd = ra_alloc_gpr(edx_index);
    IR2_OPND temp_rfbm = ra_alloc_itemp();
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR2_OPND mem_opnd = convert_mem_no_offset(opnd0);

    la_bstrins_d(temp_rfbm, eax_opnd, 31, 0);
    la_bstrins_d(temp_rfbm, edx_opnd, 63, 32);
    tr_gen_call_to_helper_vfll((ADDR)helper_xrstor, mem_opnd, temp_rfbm, 1,
            false, LOAD_HELPER_XRSTOR);
    tr_load_ymm_high_from_env(UINT16_MAX);
    return true;
}

bool translate_vpbroadcastd(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src;
    if (ir1_opnd_is_mem(opnd1)) {
        src = load_freg128_from_ir1(opnd1);
    } else if (ir1_opnd_is_xmm(opnd1)) {
        src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    } else {
        lsassert(0);
    }

    if (ir1_opnd_is_xmm(opnd0)) {
        la_xvreplve0_w(dest, src);
    } else if (ir1_opnd_is_ymm(opnd0)) {
        la_xvreplve0_w(dest, src);
        la_xvinsve0_d(dest, dest, 2);
        la_xvinsve0_d(dest, dest, 3);
    } else {
        lsassert(0);
    }

    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }

    return true;
}
#if 0
bool translate_vpmaxsd(IR1_INST *pir1)
{
    if (ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        la_vmax_w(dest, src1, src2);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
        la_xvmax_w(dest, src1, src2);
    } else {
        lsassert(0);
    }
    return true;
}

bool translate_vpminsd(IR1_INST *pir1)
{
    if (ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        la_vmin_w(dest, src1, src2);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
        la_xvmin_w(dest, src1, src2);
    } else {
        lsassert(0);
    }
    return true;
}
#endif
bool translate_vrsqrtss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();

    /* this x86 instruction has no exception ,so we need mask all exception */
    la_frsqrt_s(temp, src2);
    la_xvori_b(dest, src1, 0x0);
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);

    return true;
}

bool translate_vrsqrtps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);

    /* this x86 instruction has no exception ,so we need mask all exception */
    if (ir1_opnd_is_xmm(opnd0)) {
        la_vfrsqrt_s(dest, src);
        set_high128_xreg_to_zero(dest);
    } else {
        la_xvfrsqrt_s(dest, src);
    }

    return true;
}

bool translate_vrcpps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);

    /* this x86 instruction has no exception ,so we need mask all exception */
    if (ir1_opnd_is_xmm(opnd0)) {
        la_vfrecip_s(dest, src);
        set_high128_xreg_to_zero(dest);
    } else {
        la_xvfrecip_s(dest, src);
    }
    return true;
}

bool translate_vrcpss(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();

    /* this x86 instruction has no exception ,so we need mask all exception */
    la_frecip_s(temp, src2);
    la_xvori_b(dest, src1, 0x0);
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);

    return true;
}

bool translate_vpsignb(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        la_vsigncov_b(dest, src2, src1);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        la_xvsigncov_b(dest, src2, src1);
    }
    return true;
}

bool translate_vpsignd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        la_vsigncov_w(dest, src2, src1);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        la_xvsigncov_w(dest, src2, src1);
    }
    return true;
}

bool translate_vpsignw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        la_vsigncov_h(dest, src2, src1);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        la_xvsigncov_h(dest, src2, src1);
    }
    return true;
}

bool translate_vpmulhw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        la_vmuh_h(dest, src1, src2);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        la_xvmuh_h(dest, src1, src2);
    }
    return true;
}

bool translate_vpmulhuw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        la_vmuh_hu(dest, src1, src2);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        la_xvmuh_hu(dest, src1, src2);
    }
    return true;
}

bool translate_vpmaddwd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();

        la_vxor_v(temp, temp, temp);
        la_vmaddwev_w_h(temp, src1, src2);
        la_vmaddwod_w_h(temp, src1, src2);
        la_vbsll_v(dest, temp, 0);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvxor_v(temp, temp, temp);
        la_xvmaddwev_w_h(temp, src1, src2);
        la_xvmaddwod_w_h(temp, src1, src2);
        la_xvbsll_v(dest, temp, 0);
    }
    return true;
}

bool translate_vpmaddubsw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);

        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        IR2_OPND temp3 = ra_alloc_ftemp();
        IR2_OPND temp4 = ra_alloc_ftemp();
        IR2_OPND temp5 = ra_alloc_ftemp();
        IR2_OPND itmp = ra_alloc_itemp();
        /* unsigned src1 * signed src2 */
        la_vreplgr2vr_d(temp1, zero_ir2_opnd);
        la_vabsd_b(temp3, src2, temp1);
        la_vmaddwev_h_bu(temp1, src1, temp3);
        la_vreplgr2vr_d(temp2, zero_ir2_opnd);
        la_vmaddwod_h_bu(temp2, src1, temp3);

        la_ori(itmp, zero_ir2_opnd, 0x1);
        la_vreplgr2vr_b(temp3, itmp);
        la_vsigncov_b(temp4, src2, temp3);

        la_vmulwev_h_b(temp5, temp4, temp3);
        la_vmulwod_h_b(temp3, temp4, temp3);

        la_vmul_h(temp1, temp1, temp5);
        la_vmul_h(temp2, temp2, temp3);
        la_vsadd_h(dest, temp2, temp1);

        set_high128_xreg_to_zero(dest);

    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        IR2_OPND temp3 = ra_alloc_ftemp();
        IR2_OPND temp4 = ra_alloc_ftemp();
        IR2_OPND temp5 = ra_alloc_ftemp();
        IR2_OPND itmp = ra_alloc_itemp();
        /* unsigned src1 * signed src2 */
        la_xvreplgr2vr_d(temp1, zero_ir2_opnd);
        la_xvabsd_b(temp3, src2, temp1);
        la_xvmaddwev_h_bu(temp1, src1, temp3);
        la_xvreplgr2vr_d(temp2, zero_ir2_opnd);
        la_xvmaddwod_h_bu(temp2, src1, temp3);

        la_ori(itmp, zero_ir2_opnd, 0x1);
        la_xvreplgr2vr_b(temp3, itmp);
        la_xvsigncov_b(temp4, src2, temp3);

        la_xvmulwev_h_b(temp5, temp4, temp3);
        la_xvmulwod_h_b(temp3, temp4, temp3);

        la_xvmul_h(temp1, temp1, temp5);
        la_xvmul_h(temp2, temp2, temp3);
        la_xvsadd_h(dest, temp2, temp1);
    }
    return true;
}

bool translate_vphsubw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_vhsubw_w_h(temp1, src1, src1);
        la_vneg_h(temp1, temp1);
        la_vhsubw_w_h(temp2, src2, src2);
        la_vneg_h(temp2, temp2);

        la_vpickev_h(dest, temp2, temp1);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_xvhsubw_w_h(temp1, src1, src1);
        la_xvneg_h(temp1, temp1);
        la_xvhsubw_w_h(temp2, src2, src2);
        la_xvneg_h(temp2, temp2);

        la_xvpickev_h(dest, temp2, temp1);
    }
    return true;
}

bool translate_vphsubd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_vhsubw_d_w(temp1, src1, src1);
        la_vneg_w(temp1, temp1);
        la_vhsubw_d_w(temp2, src2, src2);
        la_vneg_w(temp2, temp2);

        la_vpickev_w(dest, temp2, temp1);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_xvhsubw_d_w(temp1, src1, src1);
        la_xvneg_w(temp1, temp1);
        la_xvhsubw_d_w(temp2, src2, src2);
        la_xvneg_w(temp2, temp2);

        la_xvpickev_w(dest, temp2, temp1);
    }
    return true;
}

bool translate_vphsubsw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        IR2_OPND temp3 = ra_alloc_ftemp();
        IR2_OPND temp4 = ra_alloc_ftemp();

        la_vpickev_h(temp1, src1, src1);
        la_vpickod_h(temp2, src1, src1);
        la_vssub_h(temp3, temp1, temp2); //temp1-temp2

        la_vpickev_h(temp1, src2, src2);
        la_vpickod_h(temp2, src2, src2);
        la_vssub_h(temp4, temp1, temp2); //temp1-temp2

        la_vshuf4i_d(temp4, temp3, 0b00000110);
        la_vori_b(dest, temp4, 0x0);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        IR2_OPND temp3 = ra_alloc_ftemp();
        IR2_OPND temp4 = ra_alloc_ftemp();
        la_xvpickev_h(temp1, src1, src1);
        la_xvpickod_h(temp2, src1, src1);
        la_xvssub_h(temp3, temp1, temp2); //temp1-temp2

        la_xvpickev_h(temp1, src2, src2);
        la_xvpickod_h(temp2, src2, src2);
        la_xvssub_h(temp4, temp1, temp2); //temp1-temp2

        la_xvshuf4i_d(temp4, temp3, 0b00000110);
        la_xvori_b(dest, temp4, 0x0);
    }
    return true;
}


bool translate_vpmaxxx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND);
    tr_inst = NULL;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VPMAXSD:
            tr_inst = la_xvmax_w;
            break;
        case dt_X86_INS_VPMAXSW:
            tr_inst = la_xvmax_h;
            break;
        case dt_X86_INS_VPMAXSB:
            tr_inst = la_xvmax_b;
            break;
        case dt_X86_INS_VPMAXUD:
            tr_inst = la_xvmax_wu;
            break;
        case dt_X86_INS_VPMAXUW:
            tr_inst = la_xvmax_hu;
            break;
        case dt_X86_INS_VPMAXUB:
            tr_inst = la_xvmax_bu;
            break;
        default:
            break;
    }

    if (ir1_opnd_is_xmm(opnd0)) {

        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        tr_inst(dest, src1, src2);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        tr_inst(dest, src1, src2);
    }

    return true;
}

bool translate_vpminxx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND);
    tr_inst = NULL;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VPMINSD:
            tr_inst = la_xvmin_w;
            break;
        case dt_X86_INS_VPMINSW:
            tr_inst = la_xvmin_h;
            break;
        case dt_X86_INS_VPMINSB:
            tr_inst = la_xvmin_b;
            break;
        default:
            break;
    }

    if (ir1_opnd_is_xmm(opnd0)) {

        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        tr_inst(dest, src1, src2);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        tr_inst(dest, src1, src2);
    }

    return true;
}

bool translate_vpmuldq(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opnd_is_xmm(opnd0)) {

        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        la_vmulwev_d_w(dest, src1, src2);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        la_xvmulwev_d_w(dest, src1, src2);
    }

    return true;
}


bool translate_vpshufd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        uint8_t imm = ir1_opnd_uimm(opnd2);
        la_xvori_b(dest, src1, 0x00);
        la_vpermi_w(dest, src1, imm);
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        uint8_t imm = ir1_opnd_uimm(opnd2);
        la_xvori_b(dest, src1, 0x00);
        la_xvpermi_w(dest, src1, imm);
    }
    return true;
}


static void vpmaskmov_lsx_lane(IR2_OPND vector, IR2_OPND mask,
                               IR2_OPND address, int vector_lane,
                               int memory_lane,
                               int element_size, bool store)
{
    IR2_OPND mask_value = ra_alloc_itemp();
    IR2_OPND mask_sign = ra_alloc_itemp();
    IR2_OPND value = ra_alloc_itemp();
    IR2_OPND skip = ra_alloc_label();
    int offset = memory_lane * element_size;

    if (element_size == 4) {
        la_vpickve2gr_wu(mask_value, mask, vector_lane);
        la_bstrpick_d(mask_sign, mask_value, 31, 31);
    } else {
        la_vpickve2gr_du(mask_value, mask, vector_lane);
        la_bstrpick_d(mask_sign, mask_value, 63, 63);
    }
    la_beq(mask_sign, zero_ir2_opnd, skip);

    if (store) {
        if (element_size == 4) {
            la_vpickve2gr_wu(value, vector, vector_lane);
            gen_test_page_flag_force(address, offset,
                                     PAGE_WRITE | PAGE_WRITE_ORG);
            la_st_w(value, address, offset);
        } else {
            la_vpickve2gr_du(value, vector, vector_lane);
            gen_test_page_flag_force(address, offset,
                                     PAGE_WRITE | PAGE_WRITE_ORG);
            la_st_d(value, address, offset);
        }
    } else if (element_size == 4) {
        gen_test_page_flag_force(address, offset, PAGE_READ);
        la_ld_w(value, address, offset);
        la_vinsgr2vr_w(vector, value, vector_lane);
    } else {
        gen_test_page_flag_force(address, offset, PAGE_READ);
        la_ld_d(value, address, offset);
        la_vinsgr2vr_d(vector, value, vector_lane);
    }

    la_label(skip);
    ra_free_temp(value);
    ra_free_temp(mask_sign);
    ra_free_temp(mask_value);
}

bool translate_vpmaskmovx_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool store = ir1_opnd_is_mem(opnd0);
    bool ymm = ir1_opnd_is_ymm(opnd1);
    bool quadword = ir1_opcode(pir1) == dt_X86_INS_VPMASKMOVQ;
    int element_size = quadword ? 8 : 4;
    int lane_count = quadword ? 2 : 4;

    lsassert(ir1_opcode(pir1) == dt_X86_INS_VPMASKMOVD ||
             ir1_opcode(pir1) == dt_X86_INS_VPMASKMOVQ);
    lsassert(ir1_opnd_is_xmm(opnd1) || ir1_opnd_is_ymm(opnd1));
    lsassert(ymm ? (ir1_opnd_is_ymm(opnd2) &&
                    (store || ir1_opnd_is_ymm(opnd0))) :
                    (ir1_opnd_is_xmm(opnd2) &&
                     (store || ir1_opnd_is_xmm(opnd0))));
    lsassert(store ? (ir1_opnd_is_mem(opnd0) &&
                      (ir1_opnd_is_xmm(opnd2) || ir1_opnd_is_ymm(opnd2))) :
                    (ir1_opnd_is_mem(opnd2) &&
                     (ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0))));

    if (ymm) {
        tr_save_ymm_to_env(UINT16_MAX);
    }

    if (store) {
        IR2_OPND address = convert_mem_to_itemp(opnd0);
        int src_index = ir1_opnd_base_reg_num(opnd2);
        int mask_index = ir1_opnd_base_reg_num(opnd1);
        IR2_OPND src_low = ra_alloc_xmm(src_index);
        IR2_OPND mask_low = ra_alloc_xmm(mask_index);

        for (int lane = 0; lane < lane_count; ++lane) {
            vpmaskmov_lsx_lane(src_low, mask_low, address, lane, lane,
                               element_size, true);
        }
        if (ymm) {
            IR2_OPND src_high = load_ymm_high128_shadow(src_index);
            IR2_OPND mask_high = load_ymm_high128_shadow(mask_index);

            for (int lane = 0; lane < lane_count; ++lane) {
                vpmaskmov_lsx_lane(src_high, mask_high, address, lane,
                                   lane + lane_count, element_size, true);
            }
            ra_free_temp(mask_high);
            ra_free_temp(src_high);
        }
        ra_free_temp(address);
    } else {
        IR2_OPND address = convert_mem_to_itemp(opnd2);
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        int mask_index = ir1_opnd_base_reg_num(opnd1);
        IR2_OPND dest_low = ra_alloc_xmm(dest_index);
        IR2_OPND mask_low = ra_alloc_ftemp();

        /* Copy the mask before writing dest, including the dest/mask alias. */
        la_vori_b(mask_low, ra_alloc_xmm(mask_index), 0);
        la_vxor_v(dest_low, dest_low, dest_low);
        if (!ymm) {
            clear_ymm_high128_shadow(dest_index);
        }
        for (int lane = 0; lane < lane_count; ++lane) {
            vpmaskmov_lsx_lane(dest_low, mask_low, address, lane, lane,
                               element_size, false);
        }
        if (ymm) {
            IR2_OPND dest_high = load_ymm_high128_shadow(dest_index);
            IR2_OPND mask_high = load_ymm_high128_shadow(mask_index);

            la_vxor_v(dest_high, dest_high, dest_high);
            for (int lane = 0; lane < lane_count; ++lane) {
                vpmaskmov_lsx_lane(dest_high, mask_high, address, lane,
                                   lane + lane_count, element_size, false);
            }
            store_ymm_high128_shadow(dest_high, dest_index);
            ra_free_temp(mask_high);
            ra_free_temp(dest_high);
        }
        ra_free_temp(mask_low);
        ra_free_temp(address);
    }
    return true;
}

bool translate_vpmaskmovx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR2_INST * ( * tr_inst1)(IR2_OPND, IR2_OPND);
    IR2_INST * ( * tr_inst2)(IR2_OPND, IR2_OPND, int);
    tr_inst1 = NULL;
    tr_inst2 = NULL;
    IR1_OPCODE op = ir1_opcode(pir1);
    if (ir1_opnd_is_xmm(opnd1)) {
        switch (op) {
            case dt_X86_INS_VPMASKMOVD:
                tr_inst1 = la_vclz_w;
                tr_inst2 = la_vseqi_w;
                break;
            case dt_X86_INS_VPMASKMOVQ:
                tr_inst1 = la_vclz_d;
                tr_inst2 = la_vseqi_d;
                break;
            default:
                break;

        }
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND dest_temp = ra_alloc_ftemp();
        la_xvori_b(dest_temp, dest, 0x0);

        tr_inst1(temp1, src1);
        tr_inst2(temp1, temp1, 0x0);
        la_vand_v(dest, temp1, src2);
        if (ir1_opnd_is_mem(opnd0)) {

            la_vandn_v(temp1, temp1, dest_temp);
            la_vxor_v(dest, dest, temp1);
            store_freg128_to_ir1_mem(dest, opnd0);
        } else {
            set_high128_xreg_to_zero(dest);
        }

    } else if (ir1_opnd_is_ymm(opnd1)) {
        switch (op) {
            case dt_X86_INS_VPMASKMOVD:
                tr_inst1 = la_xvclz_w;
                tr_inst2 = la_xvseqi_w;
                break;
            case dt_X86_INS_VPMASKMOVQ:
                tr_inst1 = la_xvclz_d;
                tr_inst2 = la_xvseqi_d;
                break;
            default:
                break;

        }
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND dest_temp = ra_alloc_ftemp();
        la_xvori_b(dest_temp, dest, 0x0);

        tr_inst1(temp1, src1);
        tr_inst2(temp1, temp1, 0x0);
        la_xvand_v(dest, temp1, src2);
        if (ir1_opnd_is_ymm(opnd2)) {
            la_xvandn_v(temp1, temp1, dest_temp);
            la_xvxor_v(dest, dest, temp1);
            store_freg256_to_ir1_mem(dest, opnd0);
        }
    }
    return true;
}


bool translate_vpinsrx(IR1_INST * pir1) {

    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);

    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_ireg_from_ir1(opnd2, UNKNOWN_EXTENSION, false);
    uint8_t imm = ir1_opnd_uimm(opnd3);

    IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, int);
    tr_inst = NULL;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VPINSRB:
            imm %= 0b10000;
            tr_inst = la_vinsgr2vr_b;
            break;

        case dt_X86_INS_VPINSRW:
            imm %= 0b1000;
            tr_inst = la_vinsgr2vr_h;
            break;

        case dt_X86_INS_VPINSRD:
            imm %= 0b100;
            tr_inst = la_vinsgr2vr_w;
            break;

        case dt_X86_INS_VPINSRQ:
            imm %= 0b10;
            tr_inst = la_vinsgr2vr_d;
            break;

        default:
            break;

    }

    la_vori_b(dest, src1, 0x0);
    tr_inst(dest, src2, imm);
    set_high128_xreg_to_zero(dest);
    return true;
}


bool translate_vpsllvd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);

        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_vslei_wu(temp, src2, 31);
        la_vand_v(temp2, temp, src2);
        la_vsll_w(dest, src1, temp2);
        la_vand_v(dest, dest, temp);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);

        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_xvslei_wu(temp, src2, 31);
        la_xvand_v(temp2, temp, src2);
        la_xvsll_w(dest, src1, temp2);
        la_xvand_v(dest, dest, temp);
    }

    return true;
}

bool translate_vpsllvq(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);

        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_vandi_b(temp,temp,0);
        la_vldi(temp,0b0110000111111);

        la_vsle_du(temp, src2, temp);
        la_vand_v(temp2, temp, src2);
        la_vsll_d(dest, src1, temp2);
        la_vand_v(dest, dest, temp);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);

        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        la_xvandi_b(temp,temp,0);
        la_xvldi(temp,0b0110000111111);

        la_xvsle_du(temp, src2, temp);
        la_xvand_v(temp2, temp, src2);
        la_xvsll_d(dest, src1, temp2);
        la_xvand_v(dest, dest, temp);
    }

    return true;
}

bool translate_vpsravd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp01 = ra_alloc_ftemp();
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp1 = ra_alloc_ftemp();

        la_vclz_w(temp01, src1);
        la_vseqi_w(temp01, temp01, 0x0);

        la_vslei_wu(temp, src2, 31);
        la_vand_v(temp1, temp, src2);
        la_vsra_w(temp1, src1, temp1);
        la_vbitsel_v(dest, temp01, temp1, temp);

        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp01 = ra_alloc_ftemp();
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp1 = ra_alloc_ftemp();

        la_xvclz_w(temp01, src1);
        la_xvseqi_w(temp01, temp01, 0x0);

        la_xvslei_wu(temp, src2, 31);
        la_xvand_v(temp1, temp, src2);
        la_xvsra_w(temp1, src1, temp1);
        la_xvbitsel_v(dest, temp01, temp1, temp);
    }

    return true;
}

bool translate_vpermpx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2;
    uint8_t imm;
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VPERMPD:
            imm = ir1_opnd_uimm(opnd2);
            la_xvpermi_d(dest, src1, imm);
            break;

        case dt_X86_INS_VPERMPS:
            src2 = load_freg256_from_ir1(opnd2);
            la_xvperm_w(dest, src2, src1);
            break;
        default:
            break;
    }

    return true;
}


bool translate_vpermilps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        if (ir1_opnd_is_imm(opnd2)) {
            uint8_t imm = ir1_opnd_uimm(opnd2);
            la_vshuf4i_w(dest, src1, imm);

        } else {
            IR2_OPND src2 = load_freg128_from_ir1(opnd2);
            IR2_OPND temp = ra_alloc_ftemp();
            la_vandi_b(temp, src2, 0b00000011);
            la_vshuf_w(temp, src2, src1);
            la_vori_b(dest, temp, 0x0);
        }
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        if (ir1_opnd_is_imm(opnd2)) {
            uint8_t imm = ir1_opnd_uimm(opnd2);
            la_xvshuf4i_w(dest, src1, imm);

        } else {
            IR2_OPND src2 = load_freg256_from_ir1(opnd2);
            IR2_OPND temp = ra_alloc_ftemp();
            la_xvandi_b(temp, src2, 0b00000011);
            la_xvshuf_w(temp, src2, src1);
            la_xvori_b(dest, temp, 0x0);

        }

    }
    return true;
}

bool translate_vpermilpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);

    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        if (ir1_opnd_is_imm(opnd2)) {
            uint8_t imm = ir1_opnd_uimm(opnd2);
            imm = 0b10100000 + ((imm & 0b00001000) << 3) + ((imm & 0b00000100) << 2) + ((imm & 0b00000010) << 1) + (imm & 0b00000001);
            la_xvpermi_d(dest, src1, imm);

        } else {
            IR2_OPND src2 = load_freg128_from_ir1(opnd2);
            IR2_OPND temp = ra_alloc_ftemp();
            la_vsrli_d(temp, src2, 1);
            la_vandi_b(temp, temp, 0b00000001);
            la_vshuf_d(temp, src2, src1);
            la_vori_b(dest, temp, 0x0);

        }
        set_high128_xreg_to_zero(dest);
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        if (ir1_opnd_is_imm(opnd2)) {
            uint8_t imm = ir1_opnd_uimm(opnd2);
            imm = 0b10100000 + ((imm & 0b00001000) << 3) + ((imm & 0b00000100) << 2) + ((imm & 0b00000010) << 1) + (imm & 0b00000001);
            la_xvpermi_d(dest, src1, imm);

        } else {
            IR2_OPND src2 = load_freg256_from_ir1(opnd2);
            IR2_OPND temp = ra_alloc_ftemp();
            la_xvsrli_d(temp, src2, 1);
            la_xvandi_b(temp, temp, 0b00000001);
            la_xvshuf_d(temp, src2, src1);
            la_xvori_b(dest, temp, 0x0);

        }

    }

    return true;
}

bool translate_vpalignr(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);

    uint8_t imm = ir1_opnd_uimm(opnd3);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);

        /* fast path */
        if (imm >= 32) {
            la_vxor_v(dest, dest, dest);
        } else if (imm >= 16 && imm < 32) {
            la_vbsrl_v(dest, src1, imm - 16);
            set_high128_xreg_to_zero(dest);
        } else {
            /* slow path */
            if (imm == 0) {
                la_vori_b(dest, src2, 0);
                set_high128_xreg_to_zero(dest);
            } else {
                IR2_OPND temp_src2 = ra_alloc_ftemp();
                la_vbsrl_v(temp_src2, src2, imm);
                la_vbsll_v(dest, src1, 16 - imm);
                la_vor_v(dest, temp_src2, dest);
                set_high128_xreg_to_zero(dest);
            }
        }
    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);

        /* fast path */
        if (imm >= 32) {
            la_vxor_v(dest, dest, dest);
        } else if (imm >= 16 && imm < 32) {
            la_xvbsrl_v(dest, src1, imm - 16);
        } else {
            /* slow path */
            if (imm == 0) {
                la_vori_b(dest, src2, 0);
            } else {
                IR2_OPND temp_src2 = ra_alloc_ftemp();
                la_xvbsrl_v(temp_src2, src2, imm);
                la_xvbsll_v(dest, src1, 16 - imm);
                la_xvor_v(dest, temp_src2, dest);

            }
        }
    }
    return true;
}

bool translate_vphminposuw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);

    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src = load_freg128_from_ir1(opnd1);
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();

    la_xvori_b(temp, src, 0x0);
    la_vsrli_d(temp, temp, 0x10);
    la_vmin_hu(temp, temp, src);
    la_vsrli_d(temp, temp, 0x10);
    la_vmin_hu(temp, temp, src);
    la_vsrli_d(temp, temp, 0x10);
    la_vmin_hu(temp, temp, src);

    la_xvpickve_d(temp2, temp, 1);
    la_vmin_hu(temp, temp, temp2);
    la_xvreplve0_h(temp, temp);
    la_vseq_h(temp1, src, temp);
    la_vandi_b(temp2, temp2, 0x0);
    la_vfrstp_h(temp2, temp1, temp2);
    la_vpackev_h(dest, temp2, temp);
    la_xvpickve_w(dest, dest, 0);
    return true;
}

bool translate_vpmulhrsw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        IR2_OPND temp3 = ra_alloc_ftemp();
        la_vmulwev_w_h(temp1, src1, src2);
        la_vmulwod_w_h(temp2, src1, src2);

        la_vsrai_w(temp1, temp1, 0xe);
        la_vsrai_w(temp2, temp2, 0xe);

        la_vandi_b(temp3, temp3, 0x0);
        la_vbitseti_w(temp3, temp3, 0);

        la_vadd_w(temp1, temp1, temp3);
        la_vadd_w(temp2, temp2, temp3);

        la_vsrai_w(temp1, temp1, 0x1);
        la_vsrai_w(temp2, temp2, 0x1);

        la_vpackev_h(dest, temp2, temp1);

        set_high128_xreg_to_zero(dest);

    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();
        IR2_OPND temp3 = ra_alloc_ftemp();
        la_xvmulwev_w_h(temp1, src1, src2);
        la_xvmulwod_w_h(temp2, src1, src2);

        la_xvsrai_w(temp1, temp1, 0xe);
        la_xvsrai_w(temp2, temp2, 0xe);

        la_xvandi_b(temp3, temp3, 0x0);
        la_xvbitseti_w(temp3, temp3, 0);

        la_xvadd_w(temp1, temp1, temp3);
        la_xvadd_w(temp2, temp2, temp3);

        la_xvsrai_w(temp1, temp1, 0x1);
        la_xvsrai_w(temp2, temp2, 0x1);

        la_xvpackev_h(dest, temp2, temp1);
    }

    return true;
}


bool translate_vzeroall_lsx(IR1_INST *pir1)
{
    int reg_xmm = 8;
    IR2_OPND zero = ra_alloc_ftemp();

#ifdef TARGET_X86_64
    reg_xmm = 16;
#endif
    la_vxor_v(zero, zero, zero);
    for (int i = 0; i < reg_xmm; ++i) {
        la_vxor_v(ra_alloc_xmm(i), ra_alloc_xmm(i), zero);
    }
    clear_all_ymm_high128_shadows();
    ra_free_temp(zero);
    return true;
}

bool translate_vzeroall(IR1_INST * pir1) {
    int reg_xmm = 8;
    #ifdef TARGET_X86_64
    reg_xmm = 16;
    #endif

    for (int i = 0; i < reg_xmm; ++i) {
        IR2_OPND dest = ra_alloc_xmm(i);
        la_xvandi_b(dest, dest, 0x0);
    }

    return true;
}

bool translate_vtestps(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src = load_freg128_from_ir1(opnd1);

        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND n4095_opnd = ra_alloc_num_4095();
        IR2_OPND label_1 = ra_alloc_label();
        IR2_OPND label_2 = ra_alloc_label();

        la_x86mtflag(zero_ir2_opnd, 0x3f);
        la_vand_v(temp, dest, src);
        la_vsrli_w(temp, temp, 0x1f);
        la_vseteqz_v(fcc0_ir2_opnd, temp); //temp==0 fcc=1;
        la_bceqz(fcc0_ir2_opnd, label_1); //fcc==1,not jump
        la_x86mtflag(n4095_opnd, ZF_USEDEF_BIT);

        la_label(label_1);
        la_vandn_v(temp, dest, src);
        la_vsrli_w(temp, temp, 0x1f);
        la_vseteqz_v(fcc0_ir2_opnd, temp); //temp==0 fcc=1;
        la_bceqz(fcc0_ir2_opnd, label_2); //fcc==1,not jump
        la_x86mtflag(n4095_opnd, CF_USEDEF_BIT);
        la_label(label_2);

    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src = load_freg256_from_ir1(opnd1);

        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND n4095_opnd = ra_alloc_num_4095();
        IR2_OPND label_1 = ra_alloc_label();
        IR2_OPND label_2 = ra_alloc_label();

        la_x86mtflag(zero_ir2_opnd, 0x3f);
        la_xvand_v(temp, dest, src);
        la_xvsrli_w(temp, temp, 0x1f);
        la_xvseteqz_v(fcc0_ir2_opnd, temp); //temp==0 fcc=1;
        la_bceqz(fcc0_ir2_opnd, label_1); //fcc==1,not jump
        la_x86mtflag(n4095_opnd, ZF_USEDEF_BIT);

        la_label(label_1);
        la_xvandn_v(temp, dest, src);
        la_xvsrli_w(temp, temp, 0x1f);
        la_xvseteqz_v(fcc0_ir2_opnd, temp); //temp==0 fcc=1;
        la_bceqz(fcc0_ir2_opnd, label_2); //fcc==1,not jump
        la_x86mtflag(n4095_opnd, CF_USEDEF_BIT);
        la_label(label_2);
    }

    return true;
}

bool translate_vtestpd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src = load_freg128_from_ir1(opnd1);

        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND n4095_opnd = ra_alloc_num_4095();
        IR2_OPND label_1 = ra_alloc_label();
        IR2_OPND label_2 = ra_alloc_label();

        la_x86mtflag(zero_ir2_opnd, 0x3f);
        la_vand_v(temp, dest, src);
        la_vsrli_d(temp, temp, 0x3f);
        la_vseteqz_v(fcc0_ir2_opnd, temp); //temp==0 fcc=1;
        la_bceqz(fcc0_ir2_opnd, label_1); //fcc==1,not jump
        la_x86mtflag(n4095_opnd, ZF_USEDEF_BIT);

        la_label(label_1);
        la_vandn_v(temp, dest, src);
        la_vsrli_d(temp, temp, 0x3f);
        la_vseteqz_v(fcc0_ir2_opnd, temp); //temp==0 fcc=1;
        la_bceqz(fcc0_ir2_opnd, label_2); //fcc==1,not jump
        la_x86mtflag(n4095_opnd, CF_USEDEF_BIT);
        la_label(label_2);

    } else {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src = load_freg256_from_ir1(opnd1);

        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND n4095_opnd = ra_alloc_num_4095();
        IR2_OPND label_1 = ra_alloc_label();
        IR2_OPND label_2 = ra_alloc_label();

        la_x86mtflag(zero_ir2_opnd, 0x3f);
        la_xvand_v(temp, dest, src);
        la_xvsrli_d(temp, temp, 0x3f);
        la_xvseteqz_v(fcc0_ir2_opnd, temp); //temp==0 fcc=1;
        la_bceqz(fcc0_ir2_opnd, label_1); //fcc==1,not jump
        la_x86mtflag(n4095_opnd, ZF_USEDEF_BIT);

        la_label(label_1);
        la_xvandn_v(temp, dest, src);
        la_xvsrli_d(temp, temp, 0x3f);
        la_xvseteqz_v(fcc0_ir2_opnd, temp); //temp==0 fcc=1;
        la_bceqz(fcc0_ir2_opnd, label_2); //fcc==1,not jump
        la_x86mtflag(n4095_opnd, CF_USEDEF_BIT);
        la_label(label_2);

    }

    return true;
}

bool translate_vpackssxx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND temp;
    IR2_INST * ( * cvt_inst)(IR2_OPND, IR2_OPND, int);
    switch (ir1_opcode(pir1)) {
        case dt_X86_INS_VPACKSSDW:
            cvt_inst = la_xvssrani_h_w;
            break;
        case dt_X86_INS_VPACKSSWB:
            cvt_inst = la_xvssrani_b_h;
            break;
        default:
            cvt_inst = NULL;
            lsassert(0);
            break;
    }
    if (ir1_opnd_is_xmm(opnd2) || ir1_opnd_is_ymm(opnd2)) {
        temp = ra_alloc_ftemp();
        la_xvori_b(temp, src2, 0);
    } else {
        temp = src2;
    }
    cvt_inst(temp, src1, 0);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(temp);
    }
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vpshufhw(IR1_INST *pir1)
{
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
             ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) );
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND temp = ra_alloc_ftemp();
    uint64_t imm8 = ir1_opnd_uimm(opnd2);
    la_xvshuf4i_h(temp, src, imm8);
    la_xvshuf4i_d(temp, src, 0x66);
    la_xvori_b(dest, temp, 0);
    if(ir1_opnd_is_xmm(opnd0)){
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpshuflw(IR1_INST *pir1)
{
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
             ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)) );
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND temp = ra_alloc_ftemp();
    uint64_t imm8 = ir1_opnd_uimm(opnd2);
    la_xvshuf4i_h(temp, src, imm8);
    la_xvshuf4i_d(temp, src, 0xcc);
    la_xvori_b(dest, temp, 0);
    if(ir1_opnd_is_xmm(opnd0)){
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpavgb(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    la_xvavgr_bu(dest, src1, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpavgw(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);

    la_xvavgr_hu(dest, src1, src2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}


bool translate_vdppd(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    lsassert(ir1_opnd_is_xmm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();
    uint8_t imm = ir1_opnd_uimm(opnd3);
    la_xvxor_v(temp1, temp1, temp1);
    la_xvxor_v(temp2, temp2, temp2);
    if(imm & 0x10){
        la_xvextrins_d(temp1, src1, 0x00);
        la_xvextrins_d(temp2, src2, 0x00);
    }
    if(imm & 0x20){
        la_xvextrins_d(temp1, src1, 0x11);
        la_xvextrins_d(temp2, src2, 0x11);
    }
    if(ir1_opnd_is_xmm(opnd0))
        la_vfmul_d(temp1, temp1, temp2);
    else
        la_xvfmul_d(temp1, temp1, temp2);
    la_xvpackod_d(temp2, temp1, temp1);
    la_xvpackev_d(temp1, temp1, temp1);
    if(ir1_opnd_is_xmm(opnd0))
        la_vfadd_d(temp1, temp1, temp2);
    else
        la_xvfadd_d(temp1, temp1, temp2);
    la_xvxor_v(dest, dest, dest);
    if(imm & 0x1){
        la_xvextrins_d(dest, temp1, 0x00);
    }
    if(imm & 0x2){
        la_xvextrins_d(dest, temp1, 0x11);
    }
    if(ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vdpps(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    lsassert(ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();
    uint8_t imm = ir1_opnd_uimm(opnd3);
    la_xvxor_v(temp1, temp1, temp1);
    la_xvxor_v(temp2, temp2, temp2);
    if(imm & 0x10){
        la_xvextrins_w(temp1, src1, 0x00);
        la_xvextrins_w(temp2, src2, 0x00);
    }
    if(imm & 0x20){
        la_xvextrins_w(temp1, src1, 0x11);
        la_xvextrins_w(temp2, src2, 0x11);
    }
    if(imm & 0x40){
        la_xvextrins_w(temp1, src1, 0x22);
        la_xvextrins_w(temp2, src2, 0x22);
    }
    if(imm & 0x80){
        la_xvextrins_w(temp1, src1, 0x33);
        la_xvextrins_w(temp2, src2, 0x33);
    }
    if(ir1_opnd_is_xmm(opnd0))
        la_vfmul_s(temp1, temp1, temp2);
    else
        la_xvfmul_s(temp1, temp1, temp2);
    la_xvpackod_w(temp2, temp1, temp1);
    la_xvpackev_w(temp1, temp1, temp1);
    if(ir1_opnd_is_xmm(opnd0))
        la_vfadd_s(temp1, temp1, temp2);
    else
        la_xvfadd_s(temp1, temp1, temp2);
    la_xvpackod_d(temp2, temp1, temp1);
    la_xvpackev_d(temp1, temp1, temp1);
    if(ir1_opnd_is_xmm(opnd0))
        la_vfadd_s(temp1, temp1, temp2);
    else
        la_xvfadd_s(temp1, temp1, temp2);

    la_xvxor_v(dest, dest, dest);
    if(imm & 0x1){
        la_xvextrins_w(dest, temp1, 0x00);
    }
    if(imm & 0x2){
        la_xvextrins_w(dest, temp1, 0x11);
    }
    if(imm & 0x4){
        la_xvextrins_w(dest, temp1, 0x22);
    }
    if(imm & 0x8){
        la_xvextrins_w(dest, temp1, 0x33);
    }
    if(ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vmpsadbw(IR1_INST *pir1)
{

    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    lsassert(ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    uint8_t imm = ir1_opnd_uimm(opnd3);
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();
    IR2_OPND temp3 = ra_alloc_ftemp();
    IR2_OPND temp_dest = ra_alloc_ftemp();

    la_vreplvei_w(temp1, src2, imm & 3);
    la_vmepatmsk_v(temp2, 1, imm & 0x4);
    la_vmepatmsk_v(temp3, 2, imm & 0x4);
    la_vshuf_b(temp2, src1, src1, temp2);
    la_vshuf_b(temp3, src1, src1, temp3);
    la_vabsd_bu(temp2, temp2, temp1);
    la_vabsd_bu(temp3, temp3, temp1);
    la_vhaddw_hu_bu(temp2, temp2, temp2);
    la_vhaddw_wu_hu(temp2, temp2, temp2);
    la_vhaddw_hu_bu(temp3, temp3, temp3);
    la_vhaddw_wu_hu(temp_dest, temp3, temp3);
    la_vsrlni_h_w(temp_dest, temp2, 0);
    if(ir1_opnd_is_xmm(opnd0)){
        set_high128_xreg_to_zero(temp_dest);
        la_xvori_b(dest, temp_dest, 0);
        return true;
    }
    imm = imm>>3;
    la_vmepatmsk_v(temp2, 1, imm & 0x4);
    la_vmepatmsk_v(temp3, 2, imm & 0x4);
    la_xvpermi_q(temp1, src1, 0x11);
    la_vshuf_b(temp2, temp1, temp1, temp2);
    la_vshuf_b(temp3, temp1, temp1, temp3);
    la_xvpermi_q(temp1, src2, 0x11);
    la_vreplvei_w(temp1, temp1, imm & 3);
    la_vabsd_bu(temp2, temp2, temp1);
    la_vabsd_bu(temp3, temp3, temp1);
    la_vhaddw_hu_bu(temp2, temp2, temp2);
    la_vhaddw_wu_hu(temp2, temp2, temp2);
    la_vhaddw_hu_bu(temp3, temp3, temp3);
    la_vhaddw_wu_hu(temp3, temp3, temp3);
    la_vsrlni_h_w(temp3, temp2, 0);
    la_xvpermi_q(temp3, temp_dest, 0x20);
    la_xvori_b(dest, temp3, 0);
    return true;
}

static void translate_vmpsadbw_lane_lsx(IR2_OPND result, IR2_OPND src1,
                                        IR2_OPND src2, uint8_t imm)
{
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();
    IR2_OPND temp3 = ra_alloc_ftemp();
    IR2_OPND temp_dest = ra_alloc_ftemp();

    la_vreplvei_w(temp1, src2, imm & 3);
    la_vmepatmsk_v(temp2, 1, imm & 0x4);
    la_vmepatmsk_v(temp3, 2, imm & 0x4);
    la_vshuf_b(temp2, src1, src1, temp2);
    la_vshuf_b(temp3, src1, src1, temp3);
    la_vabsd_bu(temp2, temp2, temp1);
    la_vabsd_bu(temp3, temp3, temp1);
    la_vhaddw_hu_bu(temp2, temp2, temp2);
    la_vhaddw_wu_hu(temp2, temp2, temp2);
    la_vhaddw_hu_bu(temp3, temp3, temp3);
    la_vhaddw_wu_hu(temp_dest, temp3, temp3);
    la_vsrlni_h_w(temp_dest, temp2, 0);
    la_vori_b(result, temp_dest, 0);
    ra_free_temp(temp_dest);
    ra_free_temp(temp3);
    ra_free_temp(temp2);
    ra_free_temp(temp1);
}

bool translate_vmpsadbw_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    uint8_t imm = ir1_opnd_uimm(opnd3);
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_xmm(opnd0) || ymm);
    lsassert(ir1_opnd_is_xmm(opnd1) == !ymm ||
             ir1_opnd_is_ymm(opnd1) == ymm);
    lsassert(ir1_opnd_size(opnd0) == ir1_opnd_size(opnd2));
    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    translate_vmpsadbw_lane_lsx(result_low, src1_low, src2_low, imm);

    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        translate_vmpsadbw_lane_lsx(result_high, src1_high, src2_high,
                                     (imm >> 3) & 7);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    } else {
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    return true;
}

bool translate_vphaddw(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();

    la_xvpickev_h(temp1, src2, src1);
    la_xvpickod_h(temp2, src2, src1);
    la_xvadd_h(dest, temp1, temp2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vphaddd(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();

    la_xvpickev_w(temp1, src2, src1);
    la_xvpickod_w(temp2, src2, src1);
    la_xvadd_w(dest, temp1, temp2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vphaddsw(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    IR2_OPND temp1 = ra_alloc_ftemp();
    IR2_OPND temp2 = ra_alloc_ftemp();

    la_xvpickev_h(temp1, src2, src1);
    la_xvpickod_h(temp2, src2, src1);
    la_xvsadd_h(dest, temp1, temp2);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpsadbw(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    la_xvabsd_bu(dest, src1, src2);
    la_xvhaddw_hu_bu(dest, dest, dest);
    la_xvhaddw_wu_hu(dest, dest, dest);
    la_xvhaddw_du_wu(dest, dest, dest);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }

    return true;
}

bool translate_vroundps(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert(ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    uint8_t imm = ir1_opnd_uimm(opnd2);

    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND fcsr = ra_alloc_itemp();
    IR2_OPND fcsr_save = ra_alloc_itemp();
    IR2_OPND mxcsr = ra_alloc_itemp();
    bool is_xmm = ir1_opnd_is_xmm(opnd0);
    if(imm & 0x8){
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, int);
        tr_inst = is_xmm ? la_vfcmp_cond_s : la_xvfcmp_cond_s;
        tr_inst(temp, src, src, 0x8);
    }
    else{
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrint_s : la_xvfrint_s;
        tr_inst(temp, src);
    }
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_bstrpick_w(fcsr_save, fcsr, 31, 0);
    la_ld_wu(mxcsr, env_ir2_opnd,
            lsenv_offset_of_mxcsr(lsenv));
    la_bstrins_w(fcsr, zero_ir2_opnd, 4, 0);
    if (imm & 0x4) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrint_s : la_xvfrint_s;

        temp = ra_alloc_itemp();
        la_bstrpick_w(temp, mxcsr, 14, 13);
        IR2_OPND temp_int = ra_alloc_itemp_internal();
        la_andi(temp_int, temp, 0x1);
        IR2_OPND label1 = ra_alloc_label();
        la_beq(temp_int, zero_ir2_opnd, label1);
        la_xori(temp, temp, 0x2);
        la_label(label1);
        la_bstrins_w(fcsr, temp, 9, 8);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
        tr_inst(dest, src);
        if (ir1_opnd_is_xmm(opnd0)) {
            set_high128_xreg_to_zero(dest);
        }
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);

        ra_free_temp(temp);
        ra_free_temp(fcsr);
        ra_free_temp(fcsr_save);
        ra_free_temp(mxcsr);
        return true;
    }
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
    if ((imm & 0x3) == 0x0) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrne_s : la_xvfrintrne_s;
        tr_inst(dest, src);
    } else if ((imm & 0x3) == 0x1) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrm_s : la_xvfrintrm_s;
        tr_inst(dest, src);
    } else if ((imm & 0x3) == 0x2) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrp_s : la_xvfrintrp_s;
        tr_inst(dest, src);
    } else if ((imm & 0x3) == 0x3) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrz_s : la_xvfrintrz_s;
        tr_inst(dest, src);
	}
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);

    ra_free_temp(temp);
    ra_free_temp(fcsr);
    ra_free_temp(fcsr_save);
    ra_free_temp(mxcsr);
    return true;
}

bool translate_vroundpd(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert(ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    uint8_t imm = ir1_opnd_uimm(opnd2);

    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND fcsr = ra_alloc_itemp();
    IR2_OPND fcsr_save = ra_alloc_itemp();
    IR2_OPND mxcsr = ra_alloc_itemp();
    bool is_xmm = ir1_opnd_is_xmm(opnd0);
    if(imm & 0x8){
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, int);
        tr_inst = is_xmm ? la_vfcmp_cond_d : la_xvfcmp_cond_d;
        tr_inst(temp, src, src, 0x8);
    }
    else{
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrint_d : la_xvfrint_d;
        tr_inst(temp, src);
    }
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_bstrpick_w(fcsr_save, fcsr, 31, 0);
    la_ld_wu(mxcsr, env_ir2_opnd,
            lsenv_offset_of_mxcsr(lsenv));
    la_bstrins_w(fcsr, zero_ir2_opnd, 4, 0);
    if (imm & 0x4) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrint_d : la_xvfrint_d;

        temp = ra_alloc_itemp();
        la_bstrpick_w(temp, mxcsr, 14, 13);
        IR2_OPND temp_int = ra_alloc_itemp_internal();
        la_andi(temp_int, temp, 0x1);
        IR2_OPND label1 = ra_alloc_label();
        la_beq(temp_int, zero_ir2_opnd, label1);
        la_xori(temp, temp, 0x2);
        la_label(label1);
        la_bstrins_w(fcsr, temp, 9, 8);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
        tr_inst(dest, src);
        if (ir1_opnd_is_xmm(opnd0)) {
            set_high128_xreg_to_zero(dest);
        }
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);

        ra_free_temp(temp);
        ra_free_temp(fcsr);
        ra_free_temp(fcsr_save);
        ra_free_temp(mxcsr);
        return true;
    }
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
    if ((imm & 0x3) == 0x0) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrne_d : la_xvfrintrne_d;
        tr_inst(dest, src);
    } else if ((imm & 0x3) == 0x1) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrm_d : la_xvfrintrm_d;
        tr_inst(dest, src);
    } else if ((imm & 0x3) == 0x2) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrp_d : la_xvfrintrp_d;
        tr_inst(dest, src);
    } else if ((imm & 0x3) == 0x3) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrz_d : la_xvfrintrz_d;
        tr_inst(dest, src);
	}
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);

    ra_free_temp(temp);
    ra_free_temp(fcsr);
    ra_free_temp(fcsr_save);
    ra_free_temp(mxcsr);
    return true;
}

bool translate_vroundss(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    lsassert(ir1_opnd_is_xmm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    uint8_t imm = ir1_opnd_uimm(opnd3);

    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp_dest = ra_alloc_ftemp();
    IR2_OPND src = ra_alloc_ftemp();
    IR2_OPND fcsr = ra_alloc_itemp();
    IR2_OPND fcsr_save = ra_alloc_itemp();
    IR2_OPND mxcsr = ra_alloc_itemp();
    bool is_xmm = ir1_opnd_is_xmm(opnd0);
    la_xvreplve0_w(src, src2);
    if(imm & 0x8){
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, int);
        tr_inst = is_xmm ? la_vfcmp_cond_s : la_xvfcmp_cond_s;
        tr_inst(temp, src, src, 0x8);
    }
    else{
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrint_s : la_xvfrint_s;
        tr_inst(temp, src);
    }
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_bstrpick_w(fcsr_save, fcsr, 31, 0);
    la_ld_wu(mxcsr, env_ir2_opnd,
            lsenv_offset_of_mxcsr(lsenv));
    la_bstrins_w(fcsr, zero_ir2_opnd, 4, 0);
    if (imm & 0x4) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrint_s : la_xvfrint_s;

        temp = ra_alloc_itemp();
        la_bstrpick_w(temp, mxcsr, 14, 13);
        IR2_OPND temp_int = ra_alloc_itemp_internal();
        la_andi(temp_int, temp, 0x1);
        IR2_OPND label1 = ra_alloc_label();
        la_beq(temp_int, zero_ir2_opnd, label1);
        la_xori(temp, temp, 0x2);
        la_label(label1);
        la_bstrins_w(fcsr, temp, 9, 8);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
        tr_inst(temp_dest, src);
        if (ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)) !=
            ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))) {
            la_xvori_b(dest, src1, 0);
        }
        la_xvinsve0_w(dest, temp_dest, 0);
        set_high128_xreg_to_zero(dest);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);

        ra_free_temp(temp);
        ra_free_temp(fcsr);
        ra_free_temp(fcsr_save);
        ra_free_temp(mxcsr);
        return true;
    }
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
    if ((imm & 0x3) == 0x0) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrne_s : la_xvfrintrne_s;
        tr_inst(temp_dest, src);
    } else if ((imm & 0x3) == 0x1) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrm_s : la_xvfrintrm_s;
        tr_inst(temp_dest, src);
    } else if ((imm & 0x3) == 0x2) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrp_s : la_xvfrintrp_s;
        tr_inst(temp_dest, src);
    } else if ((imm & 0x3) == 0x3) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrz_s : la_xvfrintrz_s;
        tr_inst(temp_dest, src);
	}
    if (ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)) !=
        ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_w(dest, temp_dest, 0);
    set_high128_xreg_to_zero(dest);
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);

    ra_free_temp(temp);
    ra_free_temp(fcsr);
    ra_free_temp(fcsr_save);
    ra_free_temp(mxcsr);
    return true;
}

bool translate_vroundsd(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    lsassert(ir1_opnd_is_xmm(opnd0));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src1 = load_freg256_from_ir1(opnd1);
    IR2_OPND src2 = load_freg256_from_ir1(opnd2);
    uint8_t imm = ir1_opnd_uimm(opnd3);

    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND temp_dest = ra_alloc_ftemp();
    IR2_OPND src = ra_alloc_ftemp();
    IR2_OPND fcsr = ra_alloc_itemp();
    IR2_OPND fcsr_save = ra_alloc_itemp();
    IR2_OPND mxcsr = ra_alloc_itemp();
    bool is_xmm = ir1_opnd_is_xmm(opnd0);
    la_xvreplve0_d(src, src2);
    if(imm & 0x8){
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND, IR2_OPND, int);
        tr_inst = is_xmm ? la_vfcmp_cond_d : la_xvfcmp_cond_d;
        tr_inst(temp, src, src, 0x8);
    }
    else{
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrint_d : la_xvfrint_d;
        tr_inst(temp, src);
    }
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_bstrpick_w(fcsr_save, fcsr, 31, 0);
    la_ld_wu(mxcsr, env_ir2_opnd,
            lsenv_offset_of_mxcsr(lsenv));
    la_bstrins_w(fcsr, zero_ir2_opnd, 4, 0);
    if (imm & 0x4) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrint_d : la_xvfrint_d;

        temp = ra_alloc_itemp();
        la_bstrpick_w(temp, mxcsr, 14, 13);
        IR2_OPND temp_int = ra_alloc_itemp_internal();
        la_andi(temp_int, temp, 0x1);
        IR2_OPND label1 = ra_alloc_label();
        la_beq(temp_int, zero_ir2_opnd, label1);
        la_xori(temp, temp, 0x2);
        la_label(label1);
        la_bstrins_w(fcsr, temp, 9, 8);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
        tr_inst(temp_dest, src);
        if (ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)) !=
            ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))) {
            la_xvori_b(dest, src1, 0);
        }
        la_xvinsve0_d(dest, temp_dest, 0);
        set_high128_xreg_to_zero(dest);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);

        ra_free_temp(temp);
        ra_free_temp(fcsr);
        ra_free_temp(fcsr_save);
        ra_free_temp(mxcsr);
        return true;
    }
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
    if ((imm & 0x3) == 0x0) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrne_d : la_xvfrintrne_d;
        tr_inst(temp_dest, src);
    } else if ((imm & 0x3) == 0x1) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrm_d : la_xvfrintrm_d;
        tr_inst(temp_dest, src);
    } else if ((imm & 0x3) == 0x2) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrp_d : la_xvfrintrp_d;
        tr_inst(temp_dest, src);
    } else if ((imm & 0x3) == 0x3) {
        IR2_INST * ( * tr_inst)(IR2_OPND, IR2_OPND);
        tr_inst = is_xmm ? la_vfrintrz_d : la_xvfrintrz_d;
        tr_inst(temp_dest, src);
	}
    if (ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)) !=
        ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_d(dest, temp_dest, 0);
    set_high128_xreg_to_zero(dest);
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr_save);

    ra_free_temp(temp);
    ra_free_temp(fcsr);
    ra_free_temp(fcsr_save);
    ra_free_temp(mxcsr);
    return true;
}

static bool translate_vpcmpxstrx_lsx(IR1_INST *pir1, ADDR helper_func,
                                     enum aot_rel_kind rel_kind,
                                     bool explicit_lengths, bool writes_mask)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int d = ir1_opnd_base_reg_num(opnd0);
    int imm = ir1_opnd_uimm(opnd2);

    lsassert(ir1_opnd_is_xmm(opnd0));
    lsassert(ir1_opnd_is_xmm(opnd1) || ir1_opnd_is_mem(opnd1));
    if (explicit_lengths) {
#ifdef TARGET_X86_64
        imm |= ir1_rex_w(pir1) << 8;
#endif
    }

    if (ir1_opnd_is_xmm(opnd1)) {
        tr_gen_call_to_helper_pcmpxstrx(helper_func, d,
                                        ir1_opnd_base_reg_num(opnd1), imm,
                                        rel_kind);
    } else {
        int temp_index = writes_mask ? (d + 1) % 7 + 1 : (d + 1) % 8;
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm(temp_index);

        la_vori_b(temp, src, 0);
        load_freg128_from_ir1_mem(src, opnd1);
        tr_gen_call_to_helper_pcmpxstrx(helper_func, d, temp_index, imm,
                                        rel_kind);
        la_vori_b(src, temp, 0);
        ra_free_temp(temp);
    }
    if (writes_mask) {
        clear_ymm_high128_shadow(0);
    }
    return true;
}

bool translate_vpcmpestri_lsx(IR1_INST *pir1)
{
    return translate_vpcmpxstrx_lsx(pir1, (ADDR)helper_pcmpestri_xmm,
                                    LOAD_HELPER_PCMPESTRI_XMM, true, false);
}

bool translate_vpcmpestrm_lsx(IR1_INST *pir1)
{
    return translate_vpcmpxstrx_lsx(pir1, (ADDR)helper_pcmpestrm_xmm,
                                    LOAD_HELPER_PCMPESTRM_XMM, true, true);
}

bool translate_vpcmpistri_lsx(IR1_INST *pir1)
{
    return translate_vpcmpxstrx_lsx(pir1, (ADDR)helper_pcmpistri_xmm,
                                    LOAD_HELPER_PCMPISTRI_XMM, false, false);
}

bool translate_vpcmpistrm_lsx(IR1_INST *pir1)
{
    return translate_vpcmpxstrx_lsx(pir1, (ADDR)helper_pcmpistrm_xmm,
                                    LOAD_HELPER_PCMPISTRM_XMM, false, true);
}

bool translate_vpcmpestrm(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int d = ir1_opnd_base_reg_num(opnd0);
    int imm = ir1_opnd_uimm(opnd2);
#ifdef TARGET_X86_64
    /* Presence of REX.W is indicated by bit 8*/
    imm |= ir1_rex_w(pir1) << 8;
#endif
    if (ir1_opnd_is_xmm(opnd1)) {
        int s = ir1_opnd_base_reg_num(opnd1);
        tr_gen_call_to_helper_pcmpxstrx((ADDR)helper_pcmpestrm_xmm, d, s, imm,
                LOAD_HELPER_PCMPESTRM_XMM);
    } else {
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm((d + 1) % 7 + 1);
        la_xvor_v(temp, src, src);
        load_freg128_from_ir1_mem(src, opnd1);
         tr_gen_call_to_helper_pcmpxstrx((ADDR)helper_pcmpestrm_xmm, d,
                 (d + 1) % 7 + 1, imm, LOAD_HELPER_PCMPESTRM_XMM);
        la_xvor_v(src, temp, temp);
    }
    set_high128_xreg_to_zero(ra_alloc_xmm(0));
    /* TODO:fix eflags and mem opnd */
    return true;
}

bool translate_vpcmpistrm(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int d = ir1_opnd_base_reg_num(opnd0);
    int imm = ir1_opnd_uimm(opnd2);
    if (ir1_opnd_is_xmm(opnd1)) {
        int s = ir1_opnd_base_reg_num(opnd1);
        tr_gen_call_to_helper_pcmpxstrx((ADDR)helper_pcmpistrm_xmm, d, s, imm,
                LOAD_HELPER_PCMPISTRM_XMM);
    } else {
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm((d + 1) % 7 + 1);
        la_xvor_v(temp, src, src);
        load_freg128_from_ir1_mem(src, opnd1);
         tr_gen_call_to_helper_pcmpxstrx((ADDR)helper_pcmpistrm_xmm, d,
                 (d + 1) % 7 + 1, imm, LOAD_HELPER_PCMPISTRM_XMM);
        la_xvor_v(src, temp, temp);
    }
    set_high128_xreg_to_zero(ra_alloc_xmm(0));
    /* TODO:fix eflags and mem opnd */
    return true;
}

bool translate_vpclmulqdq(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND * opnd3 = ir1_get_opnd(pir1, 3);

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    int s0 = ir1_opnd_base_reg_num(opnd0);
    int s1 = ir1_opnd_base_reg_num(opnd1);
    uint8_t ctrl = ir1_opnd_uimm(opnd3);

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = ir1_opnd_is_mem(opnd2) ? ra_alloc_ftemp() : load_freg256_from_ir1(opnd2);
        IR2_OPND src1_copy = src1;
        IR2_OPND src2_copy = src2;

        if (ir1_opnd_is_mem(opnd2)) {
            if (ir1_opnd_size(opnd2) == 128) {
                load_freg128_from_ir1_mem(src2, opnd2);
            } else {
                load_freg256_from_ir1_mem(src2, opnd2);
            }
        }
        if (s0 == s1) {
            src1_copy = ra_alloc_ftemp();
            la_xvori_b(src1_copy, src1, 0);
        }
        if (!ir1_opnd_is_mem(opnd2) && s0 == ir1_opnd_base_reg_num(opnd2)) {
            src2_copy = ra_alloc_ftemp();
            la_xvori_b(src2_copy, src2, 0);
        }

        IR2_OPND lhs = ra_alloc_itemp();
        IR2_OPND rhs = ra_alloc_itemp();
        IR2_OPND res_lo = ra_alloc_itemp();
        IR2_OPND res_hi = ra_alloc_itemp();

        la_xvxor_v(dest, dest, dest);

        la_xvpickve2gr_d(lhs, src1_copy, (ctrl & 1) ? 1 : 0);
        la_xvpickve2gr_d(rhs, src2_copy, (ctrl & 0x10) ? 1 : 0);
        emit_pclmul_ctz_loop(lhs, rhs, res_lo, res_hi);
        la_xvinsgr2vr_d(dest, res_lo, 0);
        la_xvinsgr2vr_d(dest, res_hi, 1);

        la_xvpickve2gr_d(lhs, src1_copy, (ctrl & 1) ? 3 : 2);
        la_xvpickve2gr_d(rhs, src2_copy, (ctrl & 0x10) ? 3 : 2);
        emit_pclmul_ctz_loop(lhs, rhs, res_lo, res_hi);
        la_xvinsgr2vr_d(dest, res_lo, 2);
        la_xvinsgr2vr_d(dest, res_hi, 3);

        ra_free_temp_auto(src2);
        ra_free_temp_auto(src1_copy);
        if (!IR2_OPND_EQ(src2_copy, src2)) {
            ra_free_temp_auto(src2_copy);
        }
        ra_free_temp(lhs);
        ra_free_temp(rhs);
        ra_free_temp(res_lo);
        ra_free_temp(res_hi);
    } else {
        IR2_OPND src1 = ra_alloc_xmm(s1);
        IR2_OPND src2;
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND ftemp = ra_alloc_ftemp();

        if (!ir1_opnd_is_mem(opnd2)) {
            src2 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd2));
        } else {
            src2 = ra_alloc_ftemp();
            load_freg128_from_ir1_mem(src2, opnd2);
        }

        IR2_OPND ctrlp = ra_alloc_itemp();
        IR2_OPND lhs = ra_alloc_itemp();
        IR2_OPND rhs = ra_alloc_itemp();
        IR2_OPND res_lo = ra_alloc_itemp();
        IR2_OPND res_hi = ra_alloc_itemp();

        li_d(ctrlp, ctrl);
        la_andi(lhs, ctrlp, 1);
        la_vreplve_d(ftemp, src1, lhs);
        la_vpickve2gr_d(lhs, ftemp, 0);
        la_bstrpick_d(rhs, ctrlp, 4, 4);
        la_vreplve_d(ftemp, src2, rhs);
        la_vpickve2gr_d(rhs, ftemp, 0);
        ra_free_temp(ctrlp);

        emit_pclmul_ctz_loop(lhs, rhs, res_lo, res_hi);
        la_vxor_v(temp, temp, temp);
        la_vinsgr2vr_d(temp, res_lo, 0);
        la_vinsgr2vr_d(temp, res_hi, 1);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
        ra_free_temp_auto(src2);
        ra_free_temp(temp);
        ra_free_temp(ftemp);
        ra_free_temp(lhs);
        ra_free_temp(rhs);
        ra_free_temp(res_lo);
        ra_free_temp(res_hi);
    }
    if (ir1_opnd_size(opnd2) == 128) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

static void emit_pclmul_lsx_lane(IR2_OPND dest, IR2_OPND src1,
                                 IR2_OPND src2, uint8_t ctrl)
{
    IR2_OPND ftemp = ra_alloc_ftemp();
    IR2_OPND lhs = ra_alloc_itemp();
    IR2_OPND rhs = ra_alloc_itemp();
    IR2_OPND res_lo = ra_alloc_itemp();
    IR2_OPND res_hi = ra_alloc_itemp();
    IR2_OPND lhs_lane_op = ra_alloc_itemp();
    IR2_OPND rhs_lane_op = ra_alloc_itemp();
    int lhs_lane = (ctrl & 1) ? 1 : 0;
    int rhs_lane = (ctrl & 0x10) ? 1 : 0;

    li_d(lhs_lane_op, lhs_lane);
    la_vreplve_d(ftemp, src1, lhs_lane_op);
    la_vpickve2gr_d(lhs, ftemp, 0);
    li_d(rhs_lane_op, rhs_lane);
    la_vreplve_d(ftemp, src2, rhs_lane_op);
    la_vpickve2gr_d(rhs, ftemp, 0);
    emit_pclmul_ctz_loop(lhs, rhs, res_lo, res_hi);
    la_vxor_v(dest, dest, dest);
    la_vinsgr2vr_d(dest, res_lo, 0);
    la_vinsgr2vr_d(dest, res_hi, 1);

    ra_free_temp(ftemp);
    ra_free_temp(lhs);
    ra_free_temp(rhs);
    ra_free_temp(res_lo);
    ra_free_temp(res_hi);
    ra_free_temp(lhs_lane_op);
    ra_free_temp(rhs_lane_op);
}

bool translate_vpclmulqdq_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR1_OPND *opnd3 = ir1_get_opnd(pir1, 3);
    uint8_t ctrl = ir1_opnd_uimm(opnd3);

    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));

        emit_pclmul_lsx_lane(dest, src1, src2, ctrl);
        clear_ymm_high128_shadow(ir1_opnd_base_reg_num(opnd0));
        ra_free_temp_auto(src2);
        return true;
    }

    tr_save_ymm_to_env(UINT16_MAX);
    int dest_index = ir1_opnd_base_reg_num(opnd0);
    int src1_index = ir1_opnd_base_reg_num(opnd1);
    IR2_OPND src1_low = ra_alloc_xmm(src1_index);
    IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
    IR2_OPND src2_low;
    IR2_OPND src2_high;

    if (ir1_opnd_is_mem(opnd2)) {
        load_v256_from_ir1_mem_exact(opnd2, &src2_low, &src2_high);
    } else {
        int src2_index = ir1_opnd_base_reg_num(opnd2);
        src2_low = ra_alloc_xmm(src2_index);
        src2_high = load_ymm_high128_shadow(src2_index);
    }

    IR2_OPND dest_low = ra_alloc_xmm(dest_index);
    IR2_OPND dest_high = ra_alloc_ftemp();
    emit_pclmul_lsx_lane(dest_low, src1_low, src2_low, ctrl);
    emit_pclmul_lsx_lane(dest_high, src1_high, src2_high, ctrl);
    store_ymm_high128_shadow(dest_high, dest_index);
    ra_free_temp(dest_high);
    ra_free_temp(src1_high);
    ra_free_temp_auto(src2_low);
    ra_free_temp_auto(src2_high);
    return true;
}

static void adjust_vsib_index(IR2_OPND dest, IR2_OPND base,
                    IR2_OPND index, int scale)
{
    IR2_INST *(*la_alsl)(IR2_OPND, IR2_OPND, IR2_OPND, int);
#ifdef TARGET_X86_64
    la_alsl = &la_alsl_d;
#else
    la_alsl = &la_alsl_wu;
#endif
    switch (scale) {
    case 1:
        la_add(dest, base, index);
        return;
    case 2:
        la_alsl(dest, index, base, 0);
        break;
    case 4:
        la_alsl(dest, index, base, 1);
        break;
    case 8:
        la_alsl(dest, index, base, 2);
        break;
    default:
        lsassert(0);
    }
}

bool translate_vaesdec(IR1_INST *pir1)
{
    if (option_vpaes) {
        return latx_translate_vaesdec_vpaes(pir1);
    }

    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int d = ir1_opnd_base_reg_num(opnd0);
    int s1 = ir1_opnd_base_reg_num(opnd1);

    ADDR helper_func;
    int helper_kind;

    if (ir1_opnd_is_ymm(opnd0)) {
        helper_func = (ADDR)helper_vaesdec_ymm;
        helper_kind = LOAD_HELPER_VAESDEC_YMM;
    } else {
        helper_func = (ADDR)helper_vaesdec_xmm;
        helper_kind = LOAD_HELPER_VAESDEC_XMM;
    }

    if (!ir1_opnd_is_mem(opnd2)) {
        int s2 = ir1_opnd_base_reg_num(opnd2);
        tr_gen_call_to_helper_aes((ADDR)helper_func, d, s1, s2,
                helper_kind);
    } else {
        int s2 = 0;
        while (s2 < 8) {
            if (s2 != d && s2 != s1) {
                break;
            }
            s2++;
        }
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm(s2);
        la_xvor_v(temp, src, src);
        if (ir1_opnd_size(opnd2) == 128) {
            load_freg128_from_ir1_mem(src, opnd2);
        } else {
            load_freg256_from_ir1_mem(src, opnd2);
        }
        tr_gen_call_to_helper_aes((ADDR)helper_func, d, s1, s2,
                helper_kind);
        la_xvor_v(src, temp, temp);
    }
    if (!ir1_opnd_is_ymm(opnd0)) {
        set_high128_xreg_to_zero(ra_alloc_xmm(d));
    }
    /* TODO: need to check */
    return true;
}

bool translate_vaesdeclast(IR1_INST *pir1)
{
    if (option_vpaes) {
        return latx_translate_vaesdeclast_vpaes(pir1);
    }

    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int d = ir1_opnd_base_reg_num(opnd0);
    int s1 = ir1_opnd_base_reg_num(opnd1);

    ADDR helper_func;
    int helper_kind;

    if (ir1_opnd_is_ymm(opnd0)) {
        helper_func = (ADDR)helper_vaesdeclast_ymm;
        helper_kind = LOAD_HELPER_VAESDECLAST_YMM;
    } else {
        helper_func = (ADDR)helper_vaesdeclast_xmm;
        helper_kind = LOAD_HELPER_VAESDECLAST_XMM;
    }

    if (!ir1_opnd_is_mem(opnd2)) {
        int s2 = ir1_opnd_base_reg_num(opnd2);
        tr_gen_call_to_helper_aes((ADDR)helper_func, d, s1, s2, helper_kind);
    } else {
        int s2 = 0;
        while (s2 < 8) {
            if (s2 != d && s2 != s1) {
                break;
            }
            s2++;
        }
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm(s2);
        la_xvor_v(temp, src, src);
        if (ir1_opnd_size(opnd2) == 128) {
            load_freg128_from_ir1_mem(src, opnd2);
        } else {
            load_freg256_from_ir1_mem(src, opnd2);
        }
        tr_gen_call_to_helper_aes((ADDR)helper_func, d, s1, s2, helper_kind);
        la_xvor_v(src, temp, temp);
    }
    if (!ir1_opnd_is_ymm(opnd0)) {
        set_high128_xreg_to_zero(ra_alloc_xmm(d));
    }
    /* TODO: need to check */
    return true;
}

bool translate_vaesenc(IR1_INST *pir1)
{
    if (option_vpaes) {
        return latx_translate_vaesenc_vpaes(pir1);
    }

    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int d = ir1_opnd_base_reg_num(opnd0);
    int s1 = ir1_opnd_base_reg_num(opnd1);

    ADDR helper_func;
    int helper_kind;

    if (ir1_opnd_is_ymm(opnd0)) {
        helper_func = (ADDR)helper_vaesenc_ymm;
        helper_kind = LOAD_HELPER_VAESENC_YMM;
    } else {
        helper_func = (ADDR)helper_vaesenc_xmm;
        helper_kind = LOAD_HELPER_VAESENC_XMM;
    }

    if (!ir1_opnd_is_mem(opnd2)) {
        int s2 = ir1_opnd_base_reg_num(opnd2);
        tr_gen_call_to_helper_aes((ADDR)helper_func, d, s1, s2,
                helper_kind);
    } else {
        int s2 = 0;
        while (s2 < 8) {
            if (s2 != d && s2 != s1) {
                break;
            }
            s2++;
        }
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm(s2);
        la_xvor_v(temp, src, src);
        if (ir1_opnd_size(opnd2) == 128) {
            load_freg128_from_ir1_mem(src, opnd2);
        } else {
            load_freg256_from_ir1_mem(src, opnd2);
        }
        tr_gen_call_to_helper_aes((ADDR)helper_func, d, s1, s2,
                helper_kind);
        la_xvor_v(src, temp, temp);
    }
    if (!ir1_opnd_is_ymm(opnd0)) {
        set_high128_xreg_to_zero(ra_alloc_xmm(d));
    }
    /* TODO: need to check */
    return true;
}

bool translate_vaesenclast(IR1_INST *pir1)
{
    if (option_vpaes) {
        return latx_translate_vaesenclast_vpaes(pir1);
    }

    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int d = ir1_opnd_base_reg_num(opnd0);
    int s1 = ir1_opnd_base_reg_num(opnd1);

    ADDR helper_func;
    int helper_kind;

    if (ir1_opnd_is_ymm(opnd0)) {
        helper_func = (ADDR)helper_vaesenclast_ymm;
        helper_kind = LOAD_HELPER_VAESENCLAST_YMM;
    } else {
        helper_func = (ADDR)helper_vaesenclast_xmm;
        helper_kind = LOAD_HELPER_VAESENCLAST_XMM;
    }

    if (!ir1_opnd_is_mem(opnd2)) {
        int s2 = ir1_opnd_base_reg_num(opnd2);
        tr_gen_call_to_helper_aes((ADDR)helper_func, d, s1, s2,
                helper_kind);
    } else {
        int s2 = 0;
        while (s2 < 8) {
            if (s2 != d && s2 != s1) {
                break;
            }
            s2++;
        }
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm(s2);
        la_xvor_v(temp, src, src);
        if (ir1_opnd_size(opnd2) == 128) {
            load_freg128_from_ir1_mem(src, opnd2);
        } else {
            load_freg256_from_ir1_mem(src, opnd2);
        }
        tr_gen_call_to_helper_aes((ADDR)helper_func, d, s1, s2,
                helper_kind);
        la_xvor_v(src, temp, temp);
    }
    if (!ir1_opnd_is_ymm(opnd0)) {
        set_high128_xreg_to_zero(ra_alloc_xmm(d));
    }
    /* TODO: need to check */
    return true;
}

bool translate_vaesimc(IR1_INST *pir1)
{
    if (option_vpaes) {
        return latx_translate_vaesimc_vpaes(pir1);
    }

    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    int d = ir1_opnd_base_reg_num(opnd0);
    if (ir1_opnd_is_xmm(opnd1)) {
        int s = ir1_opnd_base_reg_num(opnd1);
        tr_gen_call_to_helper_pcmpxstrx((ADDR)helper_aesimc_xmm, d, s, 0,
                LOAD_HELPER_AESIMC_XMM);
    } else {
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm((d + 1) % 7 + 1);
        la_xvor_v(temp, src, src);
        load_freg128_from_ir1_mem(src, opnd1);
        tr_gen_call_to_helper_pcmpxstrx((ADDR)helper_aesimc_xmm, d,
                (d + 1) % 7 + 1, 0, LOAD_HELPER_AESIMC_XMM);
        la_xvor_v(src, temp, temp);
    }
    set_high128_xreg_to_zero(ra_alloc_xmm(d));
    /* TODO: IMM 0 do not need to save */
    return true;
}

bool translate_vaeskeygenassist(IR1_INST *pir1)
{
    if (option_vpaes) {
        return latx_translate_vaeskeygenassist_vpaes(pir1);
    }

    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int d = ir1_opnd_base_reg_num(opnd0);
    int imm = ir1_opnd_uimm(opnd2);
    if (ir1_opnd_is_xmm(opnd1)) {
        int s = ir1_opnd_base_reg_num(opnd1);
        tr_gen_call_to_helper_pcmpxstrx((ADDR)helper_aeskeygenassist_xmm, d, s, imm,
                LOAD_HELPER_AESKEYGENASSIST_XMM);
    } else {
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src = ra_alloc_xmm((d + 1) % 7 + 1);
        la_xvor_v(temp, src, src);
        load_freg128_from_ir1_mem(src, opnd1);
        tr_gen_call_to_helper_pcmpxstrx((ADDR)helper_aeskeygenassist_xmm, d,
                (d + 1) % 7 + 1, imm, LOAD_HELPER_AESKEYGENASSIST_XMM);
        la_xvor_v(src, temp, temp);
    }
    set_high128_xreg_to_zero(ra_alloc_xmm(d));
    /* TODO: need to check */
    return true;
}

bool translate_vaesdec_lsx(IR1_INST *pir1)
{
    return latx_translate_vaesdec_lsx(pir1);
}

bool translate_vaesdeclast_lsx(IR1_INST *pir1)
{
    return latx_translate_vaesdeclast_lsx(pir1);
}

bool translate_vaesenc_lsx(IR1_INST *pir1)
{
    return latx_translate_vaesenc_lsx(pir1);
}

bool translate_vaesenclast_lsx(IR1_INST *pir1)
{
    return latx_translate_vaesenclast_lsx(pir1);
}

bool translate_vaesimc_lsx(IR1_INST *pir1)
{
    return latx_translate_vaesimc_vpaes(pir1);
}

bool translate_vaeskeygenassist_lsx(IR1_INST *pir1)
{
    return latx_translate_vaeskeygenassist_vpaes(pir1);
}

bool translate_vpsrlvd(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();

        la_vslei_wu(temp, src2, 31);
        la_vsrl_w(dest, src1, src2);
        la_vand_v(dest, dest, temp);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvslei_wu(temp, src2, 31);
        la_xvsrl_w(dest, src1, src2);
        la_xvand_v(dest, dest, temp);
    }

    return true;
}

bool translate_vpsrlvq(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    if (ir1_opnd_is_xmm(opnd0)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);
        IR2_OPND src2 = load_freg128_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();

        la_vldi(temp,0b0110000111111);
        la_vsle_du(temp, src2, temp);
        la_vsrl_d(dest, src1, src2);
        la_vand_v(dest, dest, temp);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        IR2_OPND src1 = load_freg256_from_ir1(opnd1);
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvldi(temp,0b0110000111111);
        la_xvsle_du(temp, src2, temp);
        la_xvsrl_d(dest, src1, src2);
        la_xvand_v(dest, dest, temp);
    }

    return true;
}

bool translate_vpgatherdd(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd2)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd2)));
    lsassert(ir1_opnd_is_mem(opnd1) && ir1_opnd_has_index(opnd1));
    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND mask = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd2));
    IR2_OPND index_op = ra_alloc_xmm(ir1_opnd_vsib_index_reg_num(opnd1));
    IR2_OPND temp_addr = ra_alloc_itemp();
    IR2_OPND temp_index = ra_alloc_itemp();
    IR2_OPND temp_mask = ra_alloc_ftemp();
    IR2_OPND temp_dest = ra_alloc_ftemp();
    bool has_base = ir1_opnd_has_base(opnd1);
    longx offset = ir1_opnd_simm(opnd1);

    la_xvslti_w(temp_mask, mask, 0);
    li_guest_addr(temp_addr, offset);
    if(has_base){
        IR2_OPND base_op = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd1));
        la_add(temp_addr, temp_addr, base_op);
    }

    /* fetch the first element in vsib */
    la_xvpickve2gr_w(temp_index, index_op, 0);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_w(temp_index, temp_index, 0);
    la_xvinsgr2vr_w(temp_dest, temp_index, 0);

    /* fetch the second element in vsib */
    la_xvpickve2gr_w(temp_index, index_op, 1);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_w(temp_index, temp_index, 0);
    la_xvinsgr2vr_w(temp_dest, temp_index, 1);

    /* fetch the third element in vsib */
    la_xvpickve2gr_w(temp_index, index_op, 2);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_w(temp_index, temp_index, 0);
    la_xvinsgr2vr_w(temp_dest, temp_index, 2);

    /* fetch the fourth element in vsib */
    la_xvpickve2gr_w(temp_index, index_op, 3);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_w(temp_index, temp_index, 0);
    la_xvinsgr2vr_w(temp_dest, temp_index, 3);

    if(ir1_opnd_is_ymm(opnd0)){
        /* fetch the fifth element in vsib */
        la_xvpickve2gr_w(temp_index, index_op, 4);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_w(temp_index, temp_index, 0);
        la_xvinsgr2vr_w(temp_dest, temp_index, 4);

        /* fetch the sixth element in vsib */
        la_xvpickve2gr_w(temp_index, index_op, 5);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_w(temp_index, temp_index, 0);
        la_xvinsgr2vr_w(temp_dest, temp_index, 5);

        /* fetch the seventh element in vsib */
        la_xvpickve2gr_w(temp_index, index_op, 6);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_w(temp_index, temp_index, 0);
        la_xvinsgr2vr_w(temp_dest, temp_index, 6);

        /* fetch the eighth element in vsib */
        la_xvpickve2gr_w(temp_index, index_op, 7);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_w(temp_index, temp_index, 0);
        la_xvinsgr2vr_w(temp_dest, temp_index, 7);
    }
    la_xvbitsel_v(dest, dest, temp_dest, temp_mask);
    la_xvxor_v(mask, mask, mask);
    if(ir1_opnd_is_xmm(opnd0))
        set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vpgatherqd(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd2)));
    lsassert(ir1_opnd_is_mem(opnd1) && ir1_opnd_has_index(opnd1));
    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND mask = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd2));
    IR2_OPND index_op = ra_alloc_xmm(ir1_opnd_vsib_index_reg_num(opnd1));
    IR2_OPND temp_addr = ra_alloc_itemp();
    IR2_OPND temp_index = ra_alloc_itemp();
    IR2_OPND temp_mask = ra_alloc_ftemp();
    IR2_OPND temp_dest = ra_alloc_ftemp();
    bool has_base = ir1_opnd_has_base(opnd1);
    longx offset = ir1_opnd_simm(opnd1);

    la_xvslti_w(temp_mask, mask, 0);
    li_guest_addr(temp_addr, offset);
    if(has_base){
        IR2_OPND base_op = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd1));
        la_add(temp_addr, temp_addr, base_op);
    }

    /* fetch the first element in vsib */
    la_xvpickve2gr_d(temp_index, index_op, 0);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_w(temp_index, temp_index, 0);
    la_xvinsgr2vr_w(temp_dest, temp_index, 0);

    /* fetch the second element in vsib */
    la_xvpickve2gr_d(temp_index, index_op, 1);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_w(temp_index, temp_index, 0);
    la_xvinsgr2vr_w(temp_dest, temp_index, 1);

    if(ir1_index_reg_is_ymm(opnd1)){
        /* fetch the third element in vsib */
        la_xvpickve2gr_d(temp_index, index_op, 2);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_w(temp_index, temp_index, 0);
        la_xvinsgr2vr_w(temp_dest, temp_index, 2);

        /* fetch the fourth element in vsib */
        la_xvpickve2gr_d(temp_index, index_op, 3);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_w(temp_index, temp_index, 0);
        la_xvinsgr2vr_w(temp_dest, temp_index, 3);
    }
    if(ir1_index_reg_is_ymm(opnd1)) {
        la_xvbitsel_v(dest, dest, temp_dest, temp_mask);
        set_high128_xreg_to_zero(dest);
    }
    else {
        la_xvbitsel_v(temp_dest, dest, temp_dest, temp_mask);
        la_xvxor_v(dest, dest, dest);
        la_xvinsve0_d(dest, temp_dest, 0);
    }
    la_xvxor_v(mask, mask, mask);

    return true;
}

bool translate_vpgatherdq(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd2)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd2)));
    lsassert(ir1_opnd_is_mem(opnd1) && ir1_opnd_has_index(opnd1));
    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND mask = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd2));
    IR2_OPND index_op = ra_alloc_xmm(ir1_opnd_vsib_index_reg_num(opnd1));
    IR2_OPND temp_addr = ra_alloc_itemp();
    IR2_OPND temp_index = ra_alloc_itemp();
    IR2_OPND temp_mask = ra_alloc_ftemp();
    IR2_OPND temp_dest = ra_alloc_ftemp();
    bool has_base = ir1_opnd_has_base(opnd1);
    longx offset = ir1_opnd_simm(opnd1);

    la_xvslti_d(temp_mask, mask, 0);
    li_guest_addr(temp_addr, offset);
    if(has_base){
        IR2_OPND base_op = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd1));
        la_add(temp_addr, temp_addr, base_op);
    }

    /* fetch the first element in vsib */
    la_xvpickve2gr_w(temp_index, index_op, 0);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_d(temp_index, temp_index, 0);
    la_xvinsgr2vr_d(temp_dest, temp_index, 0);

    /* fetch the second element in vsib */
    la_xvpickve2gr_w(temp_index, index_op, 1);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_d(temp_index, temp_index, 0);
    la_xvinsgr2vr_d(temp_dest, temp_index, 1);

    if(ir1_opnd_is_ymm(opnd0)){
        /* fetch the third element in vsib */
        la_xvpickve2gr_w(temp_index, index_op, 2);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_d(temp_index, temp_index, 0);
        la_xvinsgr2vr_d(temp_dest, temp_index, 2);

        /* fetch the fourth element in vsib */
        la_xvpickve2gr_w(temp_index, index_op, 3);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_d(temp_index, temp_index, 0);
        la_xvinsgr2vr_d(temp_dest, temp_index, 3);
    }
    la_xvbitsel_v(dest, dest, temp_dest, temp_mask);
    if(ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    la_xvxor_v(mask, mask, mask);

    return true;
}

bool translate_vpgatherqq(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd2)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd2)));
    lsassert(ir1_opnd_is_mem(opnd1) && ir1_opnd_has_index(opnd1));
    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND mask = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd2));
    IR2_OPND index_op = ra_alloc_xmm(ir1_opnd_vsib_index_reg_num(opnd1));
    IR2_OPND temp_addr = ra_alloc_itemp();
    IR2_OPND temp_index = ra_alloc_itemp();
    IR2_OPND temp_mask = ra_alloc_ftemp();
    IR2_OPND temp_dest = ra_alloc_ftemp();
    bool has_base = ir1_opnd_has_base(opnd1);
    longx offset = ir1_opnd_simm(opnd1);

    la_xvslti_d(temp_mask, mask, 0);
    li_guest_addr(temp_addr, offset);
    if(has_base){
        IR2_OPND base_op = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd1));
        la_add(temp_addr, temp_addr, base_op);
    }

    /* fetch the first element in vsib */
    la_xvpickve2gr_d(temp_index, index_op, 0);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_d(temp_index, temp_index, 0);
    la_xvinsgr2vr_d(temp_dest, temp_index, 0);

    /* fetch the second element in vsib */
    la_xvpickve2gr_d(temp_index, index_op, 1);
    adjust_vsib_index(temp_index, temp_addr, temp_index,
                    ir1_opnd_scale(opnd1));
    la_ld_d(temp_index, temp_index, 0);
    la_xvinsgr2vr_d(temp_dest, temp_index, 1);

    if(ir1_opnd_is_ymm(opnd0)){
        /* fetch the third element in vsib */
        la_xvpickve2gr_d(temp_index, index_op, 2);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_d(temp_index, temp_index, 0);
        la_xvinsgr2vr_d(temp_dest, temp_index, 2);

        /* fetch the fourth element in vsib */
        la_xvpickve2gr_d(temp_index, index_op, 3);
        adjust_vsib_index(temp_index, temp_addr, temp_index,
                        ir1_opnd_scale(opnd1));
        la_ld_d(temp_index, temp_index, 0);
        la_xvinsgr2vr_d(temp_dest, temp_index, 3);
    }
    la_xvbitsel_v(dest, dest, temp_dest, temp_mask);
    if(ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    la_xvxor_v(mask, mask, mask);

    return true;
}

bool translate_vpbroadcastb(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src;
    if (ir1_opnd_is_mem(opnd1)) {
        src = load_freg128_from_ir1(opnd1);
    } else if (ir1_opnd_is_xmm(opnd1)) {
        src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    } else {
        lsassert(0);
    }

    if (ir1_opnd_is_xmm(opnd0)) {
        la_xvreplve0_b(dest, src);
    } else if (ir1_opnd_is_ymm(opnd0)) {
        la_xvreplve0_b(dest, src);
        la_xvinsve0_d(dest, dest, 2);
        la_xvinsve0_d(dest, dest, 3);
    } else {
        lsassert(0);
    }

    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }

    return true;
}

bool translate_vpbroadcastw(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src;
    if (ir1_opnd_is_mem(opnd1)) {
        src = load_freg128_from_ir1(opnd1);
    } else if (ir1_opnd_is_xmm(opnd1)) {
        src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
    } else {
        lsassert(0);
    }

    if (ir1_opnd_is_xmm(opnd0)) {
        la_xvreplve0_h(dest, src);
    } else if (ir1_opnd_is_ymm(opnd0)) {
        la_xvreplve0_h(dest, src);
        la_xvinsve0_d(dest, dest, 2);
        la_xvinsve0_d(dest, dest, 3);
    } else {
        lsassert(0);
    }

    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }

    return true;
}

typedef IR2_INST *(*avx_lsx_fp_binary_fn)(IR2_OPND, IR2_OPND, IR2_OPND);

typedef struct LsxFpStatus {
    IR2_OPND mxcsr;
    IR2_OPND flags;
    IR2_OPND saved_fcsr;
} LsxFpStatus;

static void lsx_fp_clear_fcsr_flags(void)
{
    IR2_OPND fcsr = ra_alloc_itemp();

    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_bstrins_w(fcsr, zero_ir2_opnd,
                 FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_I);
    la_bstrins_w(fcsr, zero_ir2_opnd,
                 FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_I);
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
    ra_free_temp(fcsr);
}

static void lsx_fp_status_begin(LsxFpStatus *status)
{
    status->saved_fcsr = set_fpu_fcsr_rounding_field_by_x86();
    status->mxcsr = ra_alloc_itemp();
    status->flags = ra_alloc_itemp();
    la_ld_wu(status->mxcsr, env_ir2_opnd,
             lsenv_offset_of_mxcsr(lsenv));
    la_or(status->flags, zero_ir2_opnd, zero_ir2_opnd);
    lsx_fp_clear_fcsr_flags();
}

static void lsx_fp_apply_daz(IR2_OPND value, IR2_OPND mxcsr,
                             IR2_OPND flags, bool double_precision,
                             int lanes)
{
    uint64_t exponent_mask = double_precision ?
        UINT64_C(0x7ff0000000000000) : UINT64_C(0x000000007f800000);
    uint64_t fraction_mask = double_precision ?
        UINT64_C(0x000fffffffffffff) : UINT64_C(0x00000000007fffff);
    uint64_t sign_mask = double_precision ?
        UINT64_C(0x8000000000000000) : UINT64_C(0x0000000080000000);
    IR2_INST *(*pick)(IR2_OPND, IR2_OPND, int) = double_precision ?
        la_vpickve2gr_du : la_vpickve2gr_w;
    IR2_INST *(*insert)(IR2_OPND, IR2_OPND, int) = double_precision ?
        la_vinsgr2vr_d : la_vinsgr2vr_w;
    IR2_OPND exponent = ra_alloc_itemp();

    li_d(exponent, exponent_mask);

    for (int lane = 0; lane < lanes; lane++) {
        IR2_OPND bits = ra_alloc_itemp();
        IR2_OPND field = ra_alloc_itemp();
        IR2_OPND not_nan = ra_alloc_label();
        IR2_OPND keep_denormal = ra_alloc_label();
        IR2_OPND done = ra_alloc_label();

        pick(bits, value, lane);
        la_and(field, bits, exponent);
        la_bne(field, exponent, not_nan);
        li_d(field, fraction_mask);
        la_and(field, bits, field);
        la_beq(field, zero_ir2_opnd, done);
        li_d(field, double_precision ? UINT64_C(0x0008000000000000) :
             UINT64_C(0x0000000000400000));
        la_and(field, bits, field);
        la_bne(field, zero_ir2_opnd, done);
        la_ori(flags, flags, 0x1);
        la_b(done);

        la_label(not_nan);
        la_bne(field, zero_ir2_opnd, done);
        li_d(field, fraction_mask);
        la_and(field, bits, field);
        la_beq(field, zero_ir2_opnd, done);

        la_andi(field, mxcsr, 0x40);
        la_beq(field, zero_ir2_opnd, keep_denormal);
        li_d(field, sign_mask);
        la_and(bits, bits, field);
        insert(value, bits, lane);
        la_b(done);

        la_label(keep_denormal);
        la_ori(flags, flags, 0x2);
        la_label(done);
        ra_free_temp(done);
        ra_free_temp(keep_denormal);
        ra_free_temp(not_nan);
        ra_free_temp(field);
        ra_free_temp(bits);
    }
    ra_free_temp(exponent);
}

static void lsx_fp_apply_fz(IR2_OPND value, IR2_OPND mxcsr,
                            IR2_OPND flags, bool double_precision,
                            int lanes)
{
    uint64_t exponent_mask = double_precision ?
        UINT64_C(0x7ff0000000000000) : UINT64_C(0x000000007f800000);
    uint64_t fraction_mask = double_precision ?
        UINT64_C(0x000fffffffffffff) : UINT64_C(0x00000000007fffff);
    uint64_t sign_mask = double_precision ?
        UINT64_C(0x8000000000000000) : UINT64_C(0x0000000080000000);
    IR2_INST *(*pick)(IR2_OPND, IR2_OPND, int) = double_precision ?
        la_vpickve2gr_du : la_vpickve2gr_w;
    IR2_INST *(*insert)(IR2_OPND, IR2_OPND, int) = double_precision ?
        la_vinsgr2vr_d : la_vinsgr2vr_w;

    for (int lane = 0; lane < lanes; lane++) {
        IR2_OPND bits = ra_alloc_itemp();
        IR2_OPND field = ra_alloc_itemp();
        IR2_OPND check_subnormal = ra_alloc_label();
        IR2_OPND done = ra_alloc_label();

        la_andi(field, mxcsr, 0x8000);
        la_beq(field, zero_ir2_opnd, done);
        la_andi(field, mxcsr, 0x800);
        la_beq(field, zero_ir2_opnd, done);
        pick(bits, value, lane);
        li_d(field, exponent_mask);
        la_and(field, bits, field);
        la_beq(field, zero_ir2_opnd, check_subnormal);
        la_b(done);
        la_label(check_subnormal);
        li_d(field, fraction_mask);
        la_and(field, bits, field);
        la_beq(field, zero_ir2_opnd, done);
        li_d(field, sign_mask);
        la_and(bits, bits, field);
        insert(value, bits, lane);
        la_ori(flags, flags, 0x30);
        la_label(done);
        ra_free_temp(done);
        ra_free_temp(check_subnormal);
        ra_free_temp(field);
        ra_free_temp(bits);
    }
}

static void lsx_fp_status_finish(IR1_INST *pir1, LsxFpStatus *status)
{
    IR2_OPND fcsr = ra_alloc_itemp();

    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    vmulsd_map_fcsr_flags(fcsr, status->flags);
    set_fpu_rounding_mode(status->saved_fcsr);
    ra_free_temp_auto(status->saved_fcsr);
    ra_free_temp(fcsr);

    vmulsd_raise_unmasked_exception(pir1, status->mxcsr, status->flags);
    la_or(status->mxcsr, status->mxcsr, status->flags);
    la_st_w(status->mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    ra_free_temp(status->flags);
    ra_free_temp(status->mxcsr);
}

static bool translate_avx_fp_binary_lsx(IR1_INST *pir1,
                                        avx_lsx_fp_binary_fn tr_inst,
                                        bool double_precision,
                                        bool track_fp_status)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    LsxFpStatus status;

    lsassert(ir1_opnd_is_xmm(opnd0) || ymm);
    lsassert(ir1_opnd_is_xmm(opnd1) == !ymm ||
             ir1_opnd_is_ymm(opnd1) == ymm);
    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    if (track_fp_status) {
        lsx_fp_status_begin(&status);
        lsx_fp_apply_daz(src1_low, status.mxcsr, status.flags,
                         double_precision, double_precision ? 2 : 4);
        lsx_fp_apply_daz(src2_low, status.mxcsr, status.flags,
                         double_precision, double_precision ? 2 : 4);
        if (ymm) {
            lsx_fp_apply_daz(src1_high, status.mxcsr, status.flags,
                             double_precision, double_precision ? 2 : 4);
            lsx_fp_apply_daz(src2_high, status.mxcsr, status.flags,
                             double_precision, double_precision ? 2 : 4);
        }
    }
    tr_inst(result_low, src1_low, src2_low);
    if (track_fp_status) {
        lsx_fp_apply_fz(result_low, status.mxcsr, status.flags,
                        double_precision, double_precision ? 2 : 4);
    }

    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        tr_inst(result_high, src1_high, src2_high);
        if (track_fp_status) {
            lsx_fp_apply_fz(result_high, status.mxcsr, status.flags,
                            double_precision, double_precision ? 2 : 4);
            lsx_fp_status_finish(pir1, &status);
        }
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
    } else {
        if (track_fp_status) {
            lsx_fp_status_finish(pir1, &status);
        }
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    if (ymm) {
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    }
    return true;
}

static IR2_OPND load_avx_lsx_scalar_operand(IR1_OPND *opnd,
                                            bool double_precision,
                                            bool *is_temp)
{
    if (!ir1_opnd_is_mem(opnd)) {
        *is_temp = false;
        return load_freg128_from_ir1(opnd);
    }

    IR2_OPND value = double_precision ?
        load_u64_from_ir1_mem_exact(opnd) :
        load_u32_from_ir1_mem_exact(opnd);
    IR2_OPND result = ra_alloc_ftemp();

    la_vxor_v(result, result, result);
    if (double_precision) {
        la_vinsgr2vr_d(result, value, 0);
    } else {
        la_vinsgr2vr_w(result, value, 0);
    }
    ra_free_temp(value);
    *is_temp = true;
    return result;
}

static bool translate_avx_fp_scalar_lsx(IR1_INST *pir1,
                                        avx_lsx_fp_binary_fn tr_inst,
                                        bool is_double,
                                        bool track_fp_status)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR2_OPND dest;
    IR2_OPND src1;
    IR2_OPND src2;
    IR2_OPND temp = ra_alloc_ftemp();
    bool src2_is_temp;
    LsxFpStatus status;

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    dest = load_freg128_from_ir1(opnd0);
    src1 = load_freg128_from_ir1(opnd1);
    src2 = load_avx_lsx_scalar_operand(opnd2, is_double, &src2_is_temp);
    if (track_fp_status) {
        lsx_fp_status_begin(&status);
        lsx_fp_apply_daz(src1, status.mxcsr, status.flags,
                         is_double, 1);
        lsx_fp_apply_daz(src2, status.mxcsr, status.flags,
                         is_double, 1);
    }
    tr_inst(temp, src1, src2);
    if (track_fp_status) {
        lsx_fp_apply_fz(temp, status.mxcsr, status.flags, is_double, 1);
    }
    la_vori_b(dest, src1, 0);
    if (is_double) {
        la_vextrins_d(dest, temp, 0);
    } else {
        la_vextrins_w(dest, temp, 0);
    }
    if (track_fp_status) {
        lsx_fp_status_finish(pir1, &status);
    }
    store_avx_lsx_result(opnd0, dest, dest);
    ra_free_temp(temp);
    if (src2_is_temp) {
        ra_free_temp(src2);
    }
    return true;
}

bool translate_vaddpd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vfadd_d, true, true);
}

bool translate_vaddps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vfadd_s, false, true);
}

bool translate_vaddsd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_lsx(pir1, la_vfadd_d, true, true);
}

bool translate_vaddss_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_lsx(pir1, la_vfadd_s, false, true);
}

bool translate_vsubpd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vfsub_d, true, true);
}

bool translate_vsubps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vfsub_s, false, true);
}

bool translate_vsubsd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_lsx(pir1, la_vfsub_d, true, true);
}

bool translate_vsubss_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_lsx(pir1, la_vfsub_s, false, true);
}

bool translate_vmulpd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vfmul_d, true, true);
}

bool translate_vmulps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vfmul_s, false, true);
}

bool translate_vmulss_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_lsx(pir1, la_vfmul_s, false, true);
}

bool translate_vdivpd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vfdiv_d, true, true);
}

bool translate_vdivps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vfdiv_s, false, true);
}

bool translate_vdivss_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_lsx(pir1, la_vfdiv_s, false, true);
}

bool translate_vandnpd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vandn_v, true, false);
}

bool translate_vandnps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vandn_v, false, false);
}

bool translate_vandpd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vand_v, true, false);
}

bool translate_vandps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vand_v, false, false);
}

bool translate_vorpd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vor_v, true, false);
}

bool translate_vorps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vor_v, false, false);
}

bool translate_vxorps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_binary_lsx(pir1, la_vxor_v, false, false);
}

typedef void (*avx_lsx_lane_fn)(IR2_OPND, IR2_OPND, IR2_OPND);

static void translate_avx_addsub_lane_lsx(IR2_OPND result,
                                          IR2_OPND src1,
                                          IR2_OPND src2,
                                          bool is_double)
{
    IR2_OPND sub1 = ra_alloc_ftemp();
    IR2_OPND sub2 = ra_alloc_ftemp();
    IR2_OPND add1 = ra_alloc_ftemp();
    IR2_OPND add2 = ra_alloc_ftemp();

    if (is_double) {
        la_vpickev_d(sub1, src1, src1);
        la_vpickev_d(sub2, src2, src2);
        la_vpickod_d(add1, src1, src1);
        la_vpickod_d(add2, src2, src2);
        la_vfsub_d(sub1, sub1, sub2);
        la_vfadd_d(add1, add1, add2);
        la_vpickev_d(result, add1, sub1);
    } else {
        la_vpickev_w(sub1, src1, src1);
        la_vpickev_w(sub2, src2, src2);
        la_vpickod_w(add1, src1, src1);
        la_vpickod_w(add2, src2, src2);
        la_vfsub_s(sub1, sub1, sub2);
        la_vfadd_s(add1, add1, add2);
        la_vpickev_w(result, add1, sub1);
    }
    ra_free_temp(add2);
    ra_free_temp(add1);
    ra_free_temp(sub2);
    ra_free_temp(sub1);
}

static void translate_avx_hadd_lane_lsx(IR2_OPND result,
                                        IR2_OPND src1,
                                        IR2_OPND src2,
                                        bool is_double,
                                        bool subtract)
{
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();

    if (is_double) {
        la_vpickev_d(even, src2, src1);
        la_vpickod_d(odd, src2, src1);
        if (subtract) {
            la_vfsub_d(result, even, odd);
        } else {
            la_vfadd_d(result, even, odd);
        }
    } else {
        la_vpickev_w(even, src2, src1);
        la_vpickod_w(odd, src2, src1);
        if (subtract) {
            la_vfsub_s(result, even, odd);
        } else {
            la_vfadd_s(result, even, odd);
        }
    }
    ra_free_temp(odd);
    ra_free_temp(even);
}

static bool translate_avx_pairwise_lsx(IR1_INST *pir1,
                                       avx_lsx_lane_fn tr_inst,
                                       bool double_precision)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    LsxFpStatus status;

    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    lsx_fp_status_begin(&status);
    lsx_fp_apply_daz(src1_low, status.mxcsr, status.flags,
                     double_precision, double_precision ? 2 : 4);
    lsx_fp_apply_daz(src2_low, status.mxcsr, status.flags,
                     double_precision, double_precision ? 2 : 4);
    tr_inst(result_low, src1_low, src2_low);
    lsx_fp_apply_fz(result_low, status.mxcsr, status.flags,
                    double_precision, double_precision ? 2 : 4);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        lsx_fp_apply_daz(src1_high, status.mxcsr, status.flags,
                         double_precision, double_precision ? 2 : 4);
        lsx_fp_apply_daz(src2_high, status.mxcsr, status.flags,
                         double_precision, double_precision ? 2 : 4);
        tr_inst(result_high, src1_high, src2_high);
        lsx_fp_apply_fz(result_high, status.mxcsr, status.flags,
                        double_precision, double_precision ? 2 : 4);
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
    } else {
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    if (ymm) {
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    }
    return true;
}

static void translate_avx_haddpd_lane_lsx(IR2_OPND result,
                                          IR2_OPND src1,
                                          IR2_OPND src2)
{
    translate_avx_hadd_lane_lsx(result, src1, src2, true, false);
}

static void translate_avx_addsubpd_lane_lsx(IR2_OPND result,
                                            IR2_OPND src1,
                                            IR2_OPND src2)
{
    translate_avx_addsub_lane_lsx(result, src1, src2, true);
}

static void translate_avx_addsubps_lane_lsx(IR2_OPND result,
                                            IR2_OPND src1,
                                            IR2_OPND src2)
{
    translate_avx_addsub_lane_lsx(result, src1, src2, false);
}

static void translate_avx_haddps_lane_lsx(IR2_OPND result,
                                          IR2_OPND src1,
                                          IR2_OPND src2)
{
    translate_avx_hadd_lane_lsx(result, src1, src2, false, false);
}

static void translate_avx_hsubpd_lane_lsx(IR2_OPND result,
                                          IR2_OPND src1,
                                          IR2_OPND src2)
{
    translate_avx_hadd_lane_lsx(result, src1, src2, true, true);
}

static void translate_avx_hsubps_lane_lsx(IR2_OPND result,
                                          IR2_OPND src1,
                                          IR2_OPND src2)
{
    translate_avx_hadd_lane_lsx(result, src1, src2, false, true);
}

bool translate_vaddsubpd_lsx(IR1_INST *pir1)
{
    return translate_avx_pairwise_lsx(pir1, translate_avx_addsubpd_lane_lsx,
                                      true);
}

bool translate_vaddsubps_lsx(IR1_INST *pir1)
{
    return translate_avx_pairwise_lsx(pir1, translate_avx_addsubps_lane_lsx,
                                      false);
}

bool translate_vhaddpd_lsx(IR1_INST *pir1)
{
    return translate_avx_pairwise_lsx(pir1, translate_avx_haddpd_lane_lsx,
                                      true);
}

bool translate_vhaddps_lsx(IR1_INST *pir1)
{
    return translate_avx_pairwise_lsx(pir1, translate_avx_haddps_lane_lsx,
                                      false);
}

bool translate_vhsubpd_lsx(IR1_INST *pir1)
{
    return translate_avx_pairwise_lsx(pir1, translate_avx_hsubpd_lane_lsx,
                                      true);
}

bool translate_vhsubps_lsx(IR1_INST *pir1)
{
    return translate_avx_pairwise_lsx(pir1, translate_avx_hsubps_lane_lsx,
                                      false);
}

static void translate_avx_minmax_lane_lsx(IR2_OPND result,
                                          IR2_OPND src1,
                                          IR2_OPND src2,
                                          bool is_double,
                                          bool is_max)
{
    IR2_OPND mask = ra_alloc_ftemp();
    IR2_OPND selected1 = ra_alloc_ftemp();
    IR2_OPND selected2 = ra_alloc_ftemp();

    if (is_double) {
        if (is_max) {
            la_vfcmp_cond_d(mask, src1, src2, 0x3);
        } else {
            la_vfcmp_cond_d(mask, src2, src1, 0x3);
        }
    } else if (is_max) {
        la_vfcmp_cond_s(mask, src1, src2, 0x3);
    } else {
        la_vfcmp_cond_s(mask, src2, src1, 0x3);
    }
    la_vand_v(selected1, src1, mask);
    la_vandn_v(selected2, mask, src2);
    la_vor_v(result, selected1, selected2);
    ra_free_temp(selected2);
    ra_free_temp(selected1);
    ra_free_temp(mask);
}

static bool translate_avx_minmax_lsx(IR1_INST *pir1,
                                     bool is_double,
                                     bool is_max,
                                     bool scalar)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR2_OPND src1;
    IR2_OPND src2;
    IR2_OPND result;
    LsxFpStatus status;

    if (scalar) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        bool src2_is_temp;

        src1 = load_freg128_from_ir1(opnd1);
        src2 = load_avx_lsx_scalar_operand(opnd2, is_double, &src2_is_temp);
        result = ra_alloc_ftemp();
        lsx_fp_status_begin(&status);
        lsx_fp_apply_daz(src1, status.mxcsr, status.flags,
                         is_double, 1);
        lsx_fp_apply_daz(src2, status.mxcsr, status.flags,
                         is_double, 1);
        translate_avx_minmax_lane_lsx(result, src1, src2,
                                      is_double, is_max);
        lsx_fp_apply_fz(result, status.mxcsr, status.flags,
                        is_double, 1);
        la_vori_b(dest, src1, 0);
        if (is_double) {
            la_vextrins_d(dest, result, 0);
        } else {
            la_vextrins_w(dest, result, 0);
        }
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, dest, dest);
        ra_free_temp(result);
        if (src2_is_temp) {
            ra_free_temp(src2);
        }
        return true;
    }

    bool ymm = ir1_opnd_is_ymm(opnd0);
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();

    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    lsx_fp_status_begin(&status);
    lsx_fp_apply_daz(src1_low, status.mxcsr, status.flags,
                     is_double, is_double ? 2 : 4);
    lsx_fp_apply_daz(src2_low, status.mxcsr, status.flags,
                     is_double, is_double ? 2 : 4);
    translate_avx_minmax_lane_lsx(result_low, src1_low, src2_low,
                                  is_double, is_max);
    lsx_fp_apply_fz(result_low, status.mxcsr, status.flags,
                    is_double, is_double ? 2 : 4);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        lsx_fp_apply_daz(src1_high, status.mxcsr, status.flags,
                         is_double, is_double ? 2 : 4);
        lsx_fp_apply_daz(src2_high, status.mxcsr, status.flags,
                         is_double, is_double ? 2 : 4);
        translate_avx_minmax_lane_lsx(result_high, src1_high, src2_high,
                                      is_double, is_max);
        lsx_fp_apply_fz(result_high, status.mxcsr, status.flags,
                        is_double, is_double ? 2 : 4);
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
    } else {
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    if (ymm) {
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    }
    return true;
}

bool translate_vmaxpd_lsx(IR1_INST *pir1)
{
    return translate_avx_minmax_lsx(pir1, true, true, false);
}

bool translate_vmaxps_lsx(IR1_INST *pir1)
{
    return translate_avx_minmax_lsx(pir1, false, true, false);
}

bool translate_vmaxsd_lsx(IR1_INST *pir1)
{
    return translate_avx_minmax_lsx(pir1, true, true, true);
}

bool translate_vmaxss_lsx(IR1_INST *pir1)
{
    return translate_avx_minmax_lsx(pir1, false, true, true);
}

bool translate_vminpd_lsx(IR1_INST *pir1)
{
    return translate_avx_minmax_lsx(pir1, true, false, false);
}

bool translate_vminps_lsx(IR1_INST *pir1)
{
    return translate_avx_minmax_lsx(pir1, false, false, false);
}

bool translate_vminsd_lsx(IR1_INST *pir1)
{
    return translate_avx_minmax_lsx(pir1, true, false, true);
}

bool translate_vminss_lsx(IR1_INST *pir1)
{
    return translate_avx_minmax_lsx(pir1, false, false, true);
}

typedef IR2_INST *(*avx_lsx_fp_unary_fn)(IR2_OPND, IR2_OPND);

static bool translate_avx_fp_unary_lsx(IR1_INST *pir1,
                                       avx_lsx_fp_unary_fn tr_inst,
                                       bool double_precision)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    IR2_OPND src_low, src_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    LsxFpStatus status;

    load_avx_lsx_operand(opnd1, ymm, &src_low, &src_high);
    lsx_fp_status_begin(&status);
    lsx_fp_apply_daz(src_low, status.mxcsr, status.flags,
                     double_precision, double_precision ? 2 : 4);
    tr_inst(result_low, src_low);
    lsx_fp_apply_fz(result_low, status.mxcsr, status.flags,
                    double_precision, double_precision ? 2 : 4);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        lsx_fp_apply_daz(src_high, status.mxcsr, status.flags,
                         double_precision, double_precision ? 2 : 4);
        tr_inst(result_high, src_high);
        lsx_fp_apply_fz(result_high, status.mxcsr, status.flags,
                        double_precision, double_precision ? 2 : 4);
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
    } else {
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src_low);
    if (ymm) {
        ra_free_temp(src_high);
    }
    return true;
}

static bool translate_avx_fp_scalar_unary_lsx(IR1_INST *pir1,
                                              avx_lsx_fp_unary_fn tr_inst,
                                              bool is_double)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR2_OPND dest = load_freg128_from_ir1(opnd0);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    bool src2_is_temp;
    IR2_OPND src2 = load_avx_lsx_scalar_operand(opnd2, is_double,
                                                &src2_is_temp);
    IR2_OPND temp = ra_alloc_ftemp();
    LsxFpStatus status;

    lsx_fp_status_begin(&status);
    lsx_fp_apply_daz(src2, status.mxcsr, status.flags, is_double, 1);
    tr_inst(temp, src2);
    lsx_fp_apply_fz(temp, status.mxcsr, status.flags, is_double, 1);
    la_vori_b(dest, src1, 0);
    if (is_double) {
        la_vextrins_d(dest, temp, 0);
    } else {
        la_vextrins_w(dest, temp, 0);
    }
    lsx_fp_status_finish(pir1, &status);
    store_avx_lsx_result(opnd0, dest, dest);
    ra_free_temp(temp);
    if (src2_is_temp) {
        ra_free_temp(src2);
    }
    return true;
}

bool translate_vsqrtpd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_unary_lsx(pir1, la_vfsqrt_d, true);
}

bool translate_vsqrtps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_unary_lsx(pir1, la_vfsqrt_s, false);
}

bool translate_vsqrtsd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_unary_lsx(pir1, la_vfsqrt_d, true);
}

bool translate_vsqrtss_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_unary_lsx(pir1, la_vfsqrt_s, false);
}

bool translate_vrcpps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_unary_lsx(pir1, la_vfrecip_s, false);
}

bool translate_vrcpss_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_unary_lsx(pir1, la_vfrecip_s, false);
}

bool translate_vrsqrtps_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_unary_lsx(pir1, la_vfrsqrt_s, false);
}

bool translate_vrsqrtss_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_unary_lsx(pir1, la_vfrsqrt_s, false);
}

static void translate_avx_dppd_lane_lsx(IR2_OPND result,
                                        IR2_OPND src1,
                                        IR2_OPND src2,
                                        uint8_t imm)
{
    IR2_OPND selected1 = ra_alloc_ftemp();
    IR2_OPND selected2 = ra_alloc_ftemp();
    IR2_OPND product = ra_alloc_ftemp();
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();
    IR2_OPND sum = ra_alloc_ftemp();

    la_vxor_v(selected1, selected1, selected1);
    la_vxor_v(selected2, selected2, selected2);
    if (imm & 0x10) {
        la_vextrins_d(selected1, src1, VEXTRINS_IMM_4_0(0, 0));
        la_vextrins_d(selected2, src2, VEXTRINS_IMM_4_0(0, 0));
    }
    if (imm & 0x20) {
        la_vextrins_d(selected1, src1, VEXTRINS_IMM_4_0(1, 1));
        la_vextrins_d(selected2, src2, VEXTRINS_IMM_4_0(1, 1));
    }
    la_vfmul_d(product, selected1, selected2);
    la_vpickev_d(even, product, product);
    la_vpickod_d(odd, product, product);
    la_vfadd_d(sum, even, odd);
    la_vxor_v(result, result, result);
    if (imm & 0x1) {
        la_vextrins_d(result, sum, VEXTRINS_IMM_4_0(0, 0));
    }
    if (imm & 0x2) {
        la_vextrins_d(result, sum, VEXTRINS_IMM_4_0(1, 1));
    }
    ra_free_temp(sum);
    ra_free_temp(odd);
    ra_free_temp(even);
    ra_free_temp(product);
    ra_free_temp(selected2);
    ra_free_temp(selected1);
}

static void translate_avx_dpps_lane_lsx(IR2_OPND result,
                                        IR2_OPND src1,
                                        IR2_OPND src2,
                                        uint8_t imm)
{
    IR2_OPND selected1 = ra_alloc_ftemp();
    IR2_OPND selected2 = ra_alloc_ftemp();
    IR2_OPND product = ra_alloc_ftemp();
    IR2_OPND even = ra_alloc_ftemp();
    IR2_OPND odd = ra_alloc_ftemp();
    IR2_OPND sum = ra_alloc_ftemp();

    la_vxor_v(selected1, selected1, selected1);
    la_vxor_v(selected2, selected2, selected2);
    if (imm & 0x10) {
        la_vextrins_w(selected1, src1, VEXTRINS_IMM_4_0(0, 0));
        la_vextrins_w(selected2, src2, VEXTRINS_IMM_4_0(0, 0));
    }
    if (imm & 0x20) {
        la_vextrins_w(selected1, src1, VEXTRINS_IMM_4_0(1, 1));
        la_vextrins_w(selected2, src2, VEXTRINS_IMM_4_0(1, 1));
    }
    if (imm & 0x40) {
        la_vextrins_w(selected1, src1, VEXTRINS_IMM_4_0(2, 2));
        la_vextrins_w(selected2, src2, VEXTRINS_IMM_4_0(2, 2));
    }
    if (imm & 0x80) {
        la_vextrins_w(selected1, src1, VEXTRINS_IMM_4_0(3, 3));
        la_vextrins_w(selected2, src2, VEXTRINS_IMM_4_0(3, 3));
    }
    la_vfmul_s(product, selected1, selected2);
    la_vpickev_w(even, product, product);
    la_vpickod_w(odd, product, product);
    la_vfadd_s(even, even, odd);
    la_vpickev_d(sum, even, even);
    la_vpickod_d(odd, even, even);
    la_vfadd_s(sum, sum, odd);
    la_vxor_v(result, result, result);
    if (imm & 0x1) {
        la_vextrins_w(result, sum, VEXTRINS_IMM_4_0(0, 0));
    }
    if (imm & 0x2) {
        la_vextrins_w(result, sum, VEXTRINS_IMM_4_0(1, 1));
    }
    if (imm & 0x4) {
        la_vextrins_w(result, sum, VEXTRINS_IMM_4_0(2, 2));
    }
    if (imm & 0x8) {
        la_vextrins_w(result, sum, VEXTRINS_IMM_4_0(3, 3));
    }
    ra_free_temp(sum);
    ra_free_temp(odd);
    ra_free_temp(even);
    ra_free_temp(product);
    ra_free_temp(selected2);
    ra_free_temp(selected1);
}

bool translate_vdppd_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);
    IR2_OPND src2 = load_freg128_from_ir1(opnd2);
    IR2_OPND result = ra_alloc_ftemp();
    LsxFpStatus status;

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    lsx_fp_status_begin(&status);
    lsx_fp_apply_daz(src1, status.mxcsr, status.flags, true, 2);
    lsx_fp_apply_daz(src2, status.mxcsr, status.flags, true, 2);
    translate_avx_dppd_lane_lsx(result, src1, src2, imm);
    lsx_fp_apply_fz(result, status.mxcsr, status.flags, true, 2);
    lsx_fp_status_finish(pir1, &status);
    store_avx_lsx_result(opnd0, result, result);
    ra_free_temp(result);
    return true;
}

bool translate_vdpps_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    uint8_t imm = ir1_opnd_uimm(ir1_get_opnd(pir1, 3));
    bool ymm = ir1_opnd_is_ymm(opnd0);
    IR2_OPND src1_low, src1_high, src2_low, src2_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    LsxFpStatus status;

    load_avx_lsx_operand(opnd1, ymm, &src1_low, &src1_high);
    load_avx_lsx_operand(opnd2, ymm, &src2_low, &src2_high);
    lsx_fp_status_begin(&status);
    lsx_fp_apply_daz(src1_low, status.mxcsr, status.flags, false, 4);
    lsx_fp_apply_daz(src2_low, status.mxcsr, status.flags, false, 4);
    translate_avx_dpps_lane_lsx(result_low, src1_low, src2_low, imm);
    lsx_fp_apply_fz(result_low, status.mxcsr, status.flags, false, 4);
    if (ymm) {
        IR2_OPND result_high = ra_alloc_ftemp();

        lsx_fp_apply_daz(src1_high, status.mxcsr, status.flags, false, 4);
        lsx_fp_apply_daz(src2_high, status.mxcsr, status.flags, false, 4);
        translate_avx_dpps_lane_lsx(result_high, src1_high, src2_high, imm);
        lsx_fp_apply_fz(result_high, status.mxcsr, status.flags, false, 4);
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, result_low, result_high);
        ra_free_temp(result_high);
    } else {
        lsx_fp_status_finish(pir1, &status);
        store_avx_lsx_result(opnd0, result_low, result_low);
    }
    ra_free_temp(result_low);
    ra_free_temp(src1_low);
    ra_free_temp(src2_low);
    if (ymm) {
        ra_free_temp(src1_high);
        ra_free_temp(src2_high);
    }
    return true;
}

static void translate_avx_gather_lane_lsx(IR2_OPND dest,
                                          IR2_OPND mask_values,
                                          IR2_OPND mask_store,
                                          IR2_OPND index_values,
                                          IR2_OPND base_addr,
                                          IR2_OPND address,
                                          int scale,
                                          int lane,
                                          int index_lane,
                                          bool index64,
                                          bool value64,
                                          bool high_mask,
                                          int mask_index,
                                          bool high_dest,
                                          int dest_index)
{
    IR2_OPND mask_value = ra_alloc_itemp();
    IR2_OPND index_value = ra_alloc_itemp();
    IR2_OPND loaded = ra_alloc_itemp();
    IR2_OPND load = ra_alloc_label();
    IR2_OPND done = ra_alloc_label();

    if (index64) {
        la_vpickve2gr_d(index_value, index_values, index_lane);
        la_vpickve2gr_d(mask_value, mask_values, lane);
    } else {
        la_vpickve2gr_w(index_value, index_values, index_lane);
        la_vpickve2gr_w(mask_value, mask_values, lane);
    }

    /* A clear mask lane must not issue a memory access. */
    la_blt(mask_value, zero_ir2_opnd, load);
    if (value64) {
        la_vinsgr2vr_d(mask_store, zero_ir2_opnd, lane);
    } else {
        la_vinsgr2vr_w(mask_store, zero_ir2_opnd, lane);
    }
    if (high_mask) {
        store_ymm_high128_shadow(mask_store, mask_index);
    }
    la_b(done);

    la_label(load);
    adjust_vsib_index(address, base_addr, index_value, scale);
    gen_test_page_flag_force_range(address, 0, value64 ? 8 : 4,
                                   PAGE_READ);
    if (value64) {
        la_ld_d(loaded, address, 0);
        la_vinsgr2vr_d(dest, loaded, lane);
    } else {
        la_ld_w(loaded, address, 0);
        la_vinsgr2vr_w(dest, loaded, lane);
    }
    if (high_dest) {
        store_ymm_high128_shadow(dest, dest_index);
    }
    if (value64) {
        la_vinsgr2vr_d(mask_store, zero_ir2_opnd, lane);
    } else {
        la_vinsgr2vr_w(mask_store, zero_ir2_opnd, lane);
    }
    if (high_mask) {
        store_ymm_high128_shadow(mask_store, mask_index);
    }

    la_label(done);
    ra_free_temp(loaded);
    ra_free_temp(index_value);
    ra_free_temp(mask_value);
}

static bool translate_avx_gather_lsx(IR1_INST *pir1,
                                     bool index64,
                                     bool value64,
                                     bool ymm_allowed)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    bool ymm = ir1_opnd_is_ymm(opnd0);
    int dest_index = ir1_opnd_base_reg_num(opnd0);
    int mask_index = ir1_opnd_base_reg_num(opnd2);
    int index_index = ir1_opnd_vsib_index_reg_num(opnd1);
    bool has_base;
    longx offset;
    IR2_OPND base_addr = ra_alloc_itemp();
    IR2_OPND address = ra_alloc_itemp();
    IR2_OPND index_low = ra_alloc_ftemp();
    IR2_OPND index_high_values = ra_alloc_ftemp();
    IR2_OPND mask_low_values = ra_alloc_ftemp();
    IR2_OPND mask_high_values = ra_alloc_ftemp();
    IR2_OPND mask_low_store = ra_alloc_xmm(mask_index);
    IR2_OPND dest_low = ra_alloc_xmm(dest_index);
    bool index_ymm;

    lsassert(ir1_opnd_is_mem(opnd1) && ir1_opnd_has_index(opnd1));
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd2)) ||
             (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd2)));
    lsassert(!ymm || ymm_allowed);
    if (ymm) {
        lsassert(ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd2));
    }
    index_ymm = ir1_index_reg_is_ymm(opnd1);

    has_base = ir1_opnd_has_base(opnd1);
    offset = ir1_opnd_simm(opnd1);
    li_guest_addr(base_addr, offset);
    if (has_base) {
        IR2_OPND base = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd1));

        la_add(base_addr, base_addr, base);
    }
    la_vori_b(index_low, ra_alloc_xmm(index_index), 0);
    la_vori_b(mask_low_values, mask_low_store, 0);
    if (ymm) {
        IR2_OPND index_high = load_ymm_high128_shadow(index_index);
        IR2_OPND mask_high = load_ymm_high128_shadow(mask_index);

        la_vori_b(index_high_values, index_high, 0);
        la_vori_b(mask_high_values, mask_high, 0);
        ra_free_temp(index_high);
        ra_free_temp(mask_high);
    }

    int lanes_per_half = value64 ? 2 : 4;
    int half_count = ymm ? 2 : 1;
    for (int half = 0; half < half_count; ++half) {
        IR2_OPND index_values = index_low;
        IR2_OPND mask_values = mask_low_values;
        IR2_OPND mask_store = mask_low_store;
        IR2_OPND dest = dest_low;
        bool high = half != 0;

        if (high) {
            index_values = index_ymm ? index_high_values : index_low;
            mask_values = mask_high_values;
            mask_store = mask_values;
            dest = load_ymm_high128_shadow(dest_index);
        }
        for (int lane = 0; lane < lanes_per_half; ++lane) {
            int index_lane = lane;

            if (high && !index_ymm) {
                index_lane += lanes_per_half;
            }
            translate_avx_gather_lane_lsx(
                dest, mask_values, mask_store, index_values, base_addr,
                address, ir1_opnd_scale(opnd1), lane, index_lane,
                index64, value64,
                high, mask_index, high, dest_index);
        }
        if (high) {
            ra_free_temp(dest);
        }
    }

    if (ymm) {
        clear_ymm_high128_shadow(mask_index);
        ra_free_temp(index_high_values);
        ra_free_temp(mask_high_values);
    } else {
        clear_ymm_high128_shadow(mask_index);
        clear_ymm_high128_shadow(dest_index);
    }
    ra_free_temp(mask_low_values);
    ra_free_temp(index_low);
    ra_free_temp(address);
    ra_free_temp(base_addr);
    return true;
}

bool translate_vpgatherdd_lsx(IR1_INST *pir1)
{
    return translate_avx_gather_lsx(pir1, false, false, true);
}

bool translate_vpgatherqd_lsx(IR1_INST *pir1)
{
    return translate_avx_gather_lsx(pir1, true, false, false);
}

bool translate_vpgatherdq_lsx(IR1_INST *pir1)
{
    return translate_avx_gather_lsx(pir1, false, true, true);
}

bool translate_vpgatherqq_lsx(IR1_INST *pir1)
{
    return translate_avx_gather_lsx(pir1, true, true, true);
}

bool translate_vgatherdps_lsx(IR1_INST *pir1)
{
    return translate_vpgatherdd_lsx(pir1);
}

bool translate_vgatherqps_lsx(IR1_INST *pir1)
{
    return translate_vpgatherqd_lsx(pir1);
}

bool translate_vgatherdpd_lsx(IR1_INST *pir1)
{
    return translate_vpgatherdq_lsx(pir1);
}

bool translate_vgatherqpd_lsx(IR1_INST *pir1)
{
    return translate_vpgatherqq_lsx(pir1);
}
#endif
