/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "common.h"
#include "reg-alloc.h"
#include "lsenv.h"
#include "latx-options.h"
#include "translate.h"

#ifdef CONFIG_LATX_AVX_OPT

static bool avx_lsx_vex_256(IR1_INST *pir1)
{
    const uint8_t *bytes = pir1->info->bytes;

    if (bytes[0] == 0xc5) {
        return bytes[1] & 0x04;
    }
    if (bytes[0] == 0xc4) {
        return bytes[2] & 0x04;
    }
    return false;
}

/* Keep a 256-bit guest value as two LSX registers.  The high half is kept in
 * ymmh[] while LASX is disabled, so never use load_freg256_from_ir1() here. */
static void load_avx_cvt_value_lsx(IR1_OPND *opnd, bool ymm,
                                   IR2_OPND *low, IR2_OPND *high)
{
    *high = (IR2_OPND){ 0 };
    if (ir1_opnd_is_mem(opnd)) {
        if (ymm) {
            /* VEX.L carries the width for these destination-narrowing
             * conversions, while IR1 may retain a 128-bit memory size. */
            IR1_OPND wide = *opnd;

            wide.size = 32;
            load_v256_from_ir1_mem_exact(&wide, low, high);
        } else {
            *low = load_v128_from_ir1_mem_exact(opnd);
        }
        return;
    }

    lsassert(ir1_opnd_is_xmm(opnd) || ir1_opnd_is_ymm(opnd));
    *low = ra_alloc_ftemp();
    la_vori_b(*low, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd)), 0);
    if (ymm) {
        *high = load_ymm_high128_shadow(ir1_opnd_base_reg_num(opnd));
    }
}

static void store_avx_cvt_value_lsx(IR2_OPND low, IR2_OPND high, int dest,
                                    bool ymm)
{
    la_vori_b(ra_alloc_xmm(dest), low, 0);
    if (ymm) {
        store_ymm_high128_shadow(high, dest);
    } else {
        clear_ymm_high128_shadow(dest);
    }
}

static void pack_avx_cvt_low64_lsx(IR2_OPND dest, IR2_OPND low,
                                   IR2_OPND high, bool have_high)
{
    IR2_OPND value = ra_alloc_itemp();

    la_vxor_v(dest, dest, dest);
    la_vpickve2gr_du(value, low, 0);
    la_vinsgr2vr_d(dest, value, 0);
    if (have_high) {
        la_vpickve2gr_du(value, high, 0);
        la_vinsgr2vr_d(dest, value, 1);
    }
    ra_free_temp(value);
}

bool translate_vcvtdq2ps_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    IR2_OPND result_high = ymm ? ra_alloc_ftemp() : (IR2_OPND){ 0 };
    IR2_OPND fcsr = set_fpu_fcsr_rounding_field_by_x86();

    load_avx_cvt_value_lsx(ir1_get_opnd(pir1, 1), ymm, &src_low, &src_high);
    la_vffint_s_w(result_low, src_low);
    if (ymm) {
        la_vffint_s_w(result_high, src_high);
    }
    store_avx_cvt_value_lsx(result_low, result_high,
                             ir1_opnd_base_reg_num(dest_opnd), ymm);
    set_fpu_rounding_mode(fcsr);
    ra_free_temp_auto(fcsr);
    ra_free_temp(result_low);
    if (ymm) {
        ra_free_temp(result_high);
        ra_free_temp(src_high);
    }
    ra_free_temp(src_low);
    return true;
}

bool translate_vcvtdq2pd_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    IR2_OPND src;
    IR2_OPND low = ra_alloc_ftemp();
    IR2_OPND high = ymm ? ra_alloc_ftemp() : (IR2_OPND){ 0 };
    IR2_OPND fcsr = set_fpu_fcsr_rounding_field_by_x86();

    load_avx_cvt_value_lsx(ir1_get_opnd(pir1, 1), false, &src,
                           &(IR2_OPND){ 0 });
    la_vffintl_d_w(low, src);
    if (ymm) {
        la_vffinth_d_w(high, src);
    }
    store_avx_cvt_value_lsx(low, high, ir1_opnd_base_reg_num(dest_opnd), ymm);
    set_fpu_rounding_mode(fcsr);
    ra_free_temp_auto(fcsr);
    ra_free_temp(src);
    ra_free_temp(low);
    if (ymm) {
        ra_free_temp(high);
    }
    return true;
}

bool translate_vcvtps2pd_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    IR2_OPND src;
    IR2_OPND low = ra_alloc_ftemp();
    IR2_OPND high = ymm ? ra_alloc_ftemp() : (IR2_OPND){ 0 };
    IR2_OPND fcsr = set_fpu_fcsr_rounding_field_by_x86();

    load_avx_cvt_value_lsx(ir1_get_opnd(pir1, 1), false, &src,
                           &(IR2_OPND){ 0 });
    la_vfcvtl_d_s(low, src);
    if (ymm) {
        la_vfcvth_d_s(high, src);
    }
    store_avx_cvt_value_lsx(low, high, ir1_opnd_base_reg_num(dest_opnd), ymm);
    set_fpu_rounding_mode(fcsr);
    ra_free_temp_auto(fcsr);
    ra_free_temp(src);
    ra_free_temp(low);
    if (ymm) {
        ra_free_temp(high);
    }
    return true;
}

static bool translate_vcvtpd2x_lsx(IR1_INST *pir1, bool truncate)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    bool src_ymm = ir1_opnd_is_ymm(src_opnd) || avx_lsx_vex_256(pir1);
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    IR2_OPND result_high = src_ymm ? ra_alloc_ftemp() : (IR2_OPND){ 0 };
    IR2_OPND dest = ra_alloc_ftemp();
    IR2_OPND fcsr = set_fpu_fcsr_rounding_field_by_x86();

    load_avx_cvt_value_lsx(src_opnd, src_ymm, &src_low, &src_high);
    if (truncate) {
        la_vftintrz_w_d(result_low, src_low, src_low);
        if (src_ymm) {
            la_vftintrz_w_d(result_high, src_high, src_high);
        }
    } else {
        la_vftint_w_d(result_low, src_low, src_low);
        if (src_ymm) {
            la_vftint_w_d(result_high, src_high, src_high);
        }
    }
    pack_avx_cvt_low64_lsx(dest, result_low, result_high, src_ymm);
    store_avx_cvt_value_lsx(dest, (IR2_OPND){ 0 },
                             ir1_opnd_base_reg_num(dest_opnd), false);
    set_fpu_rounding_mode(fcsr);
    ra_free_temp_auto(fcsr);
    ra_free_temp(dest);
    ra_free_temp(result_low);
    ra_free_temp(src_low);
    if (src_ymm) {
        ra_free_temp(result_high);
        ra_free_temp(src_high);
    }
    return true;
}

bool translate_vcvtpd2dq_lsx(IR1_INST *pir1)
{
    return translate_vcvtpd2x_lsx(pir1, false);
}

bool translate_vcvttpd2dq_lsx(IR1_INST *pir1)
{
    return translate_vcvtpd2x_lsx(pir1, true);
}

bool translate_vcvtpd2ps_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    bool src_ymm = ir1_opnd_is_ymm(src_opnd) || avx_lsx_vex_256(pir1);
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    IR2_OPND result_high = src_ymm ? ra_alloc_ftemp() : (IR2_OPND){ 0 };
    IR2_OPND dest = ra_alloc_ftemp();
    IR2_OPND fcsr = set_fpu_fcsr_rounding_field_by_x86();

    load_avx_cvt_value_lsx(src_opnd, src_ymm, &src_low, &src_high);
    la_vfcvt_s_d(result_low, src_low, src_low);
    if (src_ymm) {
        la_vfcvt_s_d(result_high, src_high, src_high);
    }
    pack_avx_cvt_low64_lsx(dest, result_low, result_high, src_ymm);
    store_avx_cvt_value_lsx(dest, (IR2_OPND){ 0 },
                             ir1_opnd_base_reg_num(dest_opnd), false);
    set_fpu_rounding_mode(fcsr);
    ra_free_temp_auto(fcsr);
    ra_free_temp(dest);
    ra_free_temp(result_low);
    ra_free_temp(src_low);
    if (src_ymm) {
        ra_free_temp(result_high);
        ra_free_temp(src_high);
    }
    return true;
}

static bool translate_vcvtps2dq_lsx_common(IR1_INST *pir1, bool truncate)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    bool ymm = ir1_opnd_is_ymm(dest_opnd);
    IR2_OPND src_low;
    IR2_OPND src_high;
    IR2_OPND result_low = ra_alloc_ftemp();
    IR2_OPND result_high = ymm ? ra_alloc_ftemp() : (IR2_OPND){ 0 };
    IR2_OPND fcsr = set_fpu_fcsr_rounding_field_by_x86();

    load_avx_cvt_value_lsx(ir1_get_opnd(pir1, 1), ymm, &src_low, &src_high);
    if (truncate) {
        la_vftintrz_w_s(result_low, src_low);
        if (ymm) {
            la_vftintrz_w_s(result_high, src_high);
        }
    } else {
        la_vftint_w_s(result_low, src_low);
        if (ymm) {
            la_vftint_w_s(result_high, src_high);
        }
    }
    {
        IR2_OPND invalid = ra_alloc_ftemp();
        IR2_OPND overflow = ra_alloc_ftemp();
        IR2_OPND mask = ra_alloc_ftemp();
        IR2_OPND value = ra_alloc_itemp();

        li_d(value, UINT64_C(0x0000000080000000));
        la_vreplgr2vr_w(invalid, value);
        li_d(value, UINT64_C(0x000000004f000000));
        la_vreplgr2vr_w(overflow, value);
        la_vfcmp_cond_s(mask, overflow, src_low, FCMP_COND_CULE);
        la_vbitsel_v(result_low, result_low, invalid, mask);
        if (ymm) {
            la_vfcmp_cond_s(mask, overflow, src_high, FCMP_COND_CULE);
            la_vbitsel_v(result_high, result_high, invalid, mask);
        }
        ra_free_temp(value);
        ra_free_temp(mask);
        ra_free_temp(overflow);
        ra_free_temp(invalid);
    }
    store_avx_cvt_value_lsx(result_low, result_high,
                             ir1_opnd_base_reg_num(dest_opnd), ymm);
    set_fpu_rounding_mode(fcsr);
    ra_free_temp_auto(fcsr);
    ra_free_temp(result_low);
    ra_free_temp(src_low);
    if (ymm) {
        ra_free_temp(result_high);
        ra_free_temp(src_high);
    }
    return true;
}

bool translate_vcvtps2dq_lsx(IR1_INST *pir1)
{
    return translate_vcvtps2dq_lsx_common(pir1, false);
}

bool translate_vcvttps2dq_lsx(IR1_INST *pir1)
{
    return translate_vcvtps2dq_lsx_common(pir1, true);
}

bool translate_vcvtsd2ss_lsx(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    lsassert(ir1_opnd_is_xmm(opnd2) ||
             (ir1_opnd_is_mem(opnd2) && ir1_opnd_size(opnd2) == 64));
    {
        /* LSX-only path */
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        IR2_OPND src1 = ra_alloc_ftemp();
        IR2_OPND src2;
        bool src2_is_temp = false;

        /* Read all sources before changing FCSR or an aliased dest. */
        la_vori_b(src1, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1)), 0);
        if (ir1_opnd_is_mem(opnd2)) {
            IR2_OPND memory_value = load_u64_from_ir1_mem_exact(opnd2);

            src2 = ra_alloc_ftemp();
            src2_is_temp = true;
            la_vxor_v(src2, src2, src2);
            la_vinsgr2vr_d(src2, memory_value, 0);
            ra_free_temp(memory_value);
        } else {
            src2 = load_freg128_from_ir1(opnd2);
        }
        IR2_OPND fcsr_opnd;
        IR2_OPND converted = ra_alloc_ftemp();
        IR2_OPND converted_low = ra_alloc_itemp();
        IR2_OPND mxcsr = ra_alloc_itemp();
        IR2_OPND flags = ra_alloc_itemp();
        IR2_OPND bit = ra_alloc_itemp();
        IR2_OPND masks = ra_alloc_itemp();
        IR2_OPND unmasked = ra_alloc_itemp();
        IR2_OPND old_mxcsr = ra_alloc_itemp();
        IR2_OPND keep_denormal = ra_alloc_label();
        IR2_OPND daz_done = ra_alloc_label();
        IR2_OPND check_overflow = ra_alloc_label();
        IR2_OPND check_underflow = ra_alloc_label();
        IR2_OPND check_precision = ra_alloc_label();
        IR2_OPND exception_ready = ra_alloc_label();
        IR2_OPND keep_precision = ra_alloc_label();
        IR2_OPND no_exception = ra_alloc_label();
        fcsr_opnd = set_fpu_fcsr_rounding_field_by_x86();
        la_or(flags, zero_ir2_opnd, zero_ir2_opnd);
        la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
        la_or(old_mxcsr, mxcsr, zero_ir2_opnd);
        la_vpickve2gr_du(converted_low, src2, 0);
        li_d(masks, UINT64_C(0x7ff0000000000000));
        la_and(bit, converted_low, masks);
        la_bne(bit, zero_ir2_opnd, daz_done);
        li_d(masks, UINT64_C(0x000fffffffffffff));
        la_and(bit, converted_low, masks);
        la_beq(bit, zero_ir2_opnd, daz_done);
        la_andi(bit, mxcsr, 0x40);
        la_beq(bit, zero_ir2_opnd, keep_denormal);
        li_d(masks, UINT64_C(0x8000000000000000));
        la_and(converted_low, converted_low, masks);
        la_vinsgr2vr_d(src2, converted_low, 0);
        la_b(daz_done);
        la_label(keep_denormal);
        la_ori(flags, flags, 0x2);
        la_label(daz_done);
        la_vinsgr2vr_d(src2, zero_ir2_opnd, 1);
        la_movfcsr2gr(converted_low, fcsr_ir2_opnd);
        la_bstrins_w(converted_low, zero_ir2_opnd,
                     FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_I);
        la_bstrins_w(converted_low, zero_ir2_opnd,
                     FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_I);
        la_movgr2fcsr(fcsr_ir2_opnd, converted_low);
        la_fcvt_s_d(converted, src2);
        if (src2_is_temp) {
            ra_free_temp(src2);
        }
        la_movfcsr2gr(converted_low, fcsr_ir2_opnd);
        set_fpu_rounding_mode(fcsr_opnd);
        ra_free_temp_auto(fcsr_opnd);

        la_bstrpick_w(bit, converted_low, FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_V);
        la_or(flags, flags, bit);
        la_bstrpick_w(bit, converted_low, FCSR_OFF_FLAGS_O, FCSR_OFF_FLAGS_O);
        la_slli_w(bit, bit, 3);
        la_or(flags, flags, bit);
        la_bstrpick_w(bit, converted_low, FCSR_OFF_FLAGS_U, FCSR_OFF_FLAGS_U);
        la_slli_w(bit, bit, 4);
        la_or(flags, flags, bit);
        la_bstrpick_w(bit, converted_low, FCSR_OFF_FLAGS_I, FCSR_OFF_FLAGS_I);
        la_slli_w(bit, bit, 5);
        la_or(flags, flags, bit);
        la_vpickve2gr_w(converted_low, converted, 0);
        ra_free_temp(converted);
        la_or(mxcsr, mxcsr, flags);
        la_srli_w(masks, mxcsr, 7);
        la_xori(masks, masks, 0x3f);
        la_and(unmasked, flags, masks);
        la_beq(unmasked, zero_ir2_opnd, no_exception);

        /* Keep only the first x86 exception class for the signal helper. */
        la_andi(bit, unmasked, 0x1);
        la_beq(bit, zero_ir2_opnd, check_overflow);
        la_or(unmasked, bit, zero_ir2_opnd);
        la_b(exception_ready);
        la_label(check_overflow);
        la_andi(bit, unmasked, 0x8);
        la_beq(bit, zero_ir2_opnd, check_underflow);
        la_or(unmasked, bit, zero_ir2_opnd);
        la_b(exception_ready);
        la_label(check_underflow);
        la_andi(bit, unmasked, 0x10);
        la_beq(bit, zero_ir2_opnd, check_precision);
        la_or(unmasked, bit, zero_ir2_opnd);
        la_b(exception_ready);
        la_label(check_precision);
        la_andi(unmasked, unmasked, 0x20);
        la_label(exception_ready);

        /* An unmasked underflow is delivered before a new inexact flag. */
        la_andi(bit, unmasked, 0x10);
        la_beq(bit, zero_ir2_opnd, keep_precision);
        la_andi(bit, flags, 0x20);
        la_beq(bit, zero_ir2_opnd, keep_precision);
        la_andi(bit, old_mxcsr, 0x20);
        la_bne(bit, zero_ir2_opnd, keep_precision);
        la_bstrins_w(mxcsr, zero_ir2_opnd, 5, 5);
        la_label(keep_precision);
        ra_free_temp(bit);
        ra_free_temp(masks);
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
        ra_free_temp(unmasked);

        la_label(no_exception);
        la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
        ra_free_temp(old_mxcsr);
        ra_free_temp(flags);
        ra_free_temp(mxcsr);
        la_vinsgr2vr_w(src1, converted_low, 0);
        la_vori_b(ra_alloc_xmm(dest_index), src1, 0);
        clear_ymm_high128_shadow(dest_index);
        ra_free_temp(converted_low);
        ra_free_temp(src1);
    }
    return true;
}

bool translate_vcvtsi2sd_lsx(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);

    lsassert(ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1));
    lsassert((ir1_opnd_is_gpr(opnd2) || ir1_opnd_is_mem(opnd2)) &&
             (ir1_opnd_size(opnd2) == 32 || ir1_opnd_size(opnd2) == 64));
    {
        /* LSX-only path */
        int dest_index = ir1_opnd_base_reg_num(opnd0);
        IR2_OPND fcsr_opnd;
        IR2_OPND src1 = ra_alloc_ftemp();
        IR2_OPND src2;
        IR2_OPND converted = ra_alloc_ftemp();
        IR2_OPND converted_low = ra_alloc_itemp();
        IR2_OPND fcsr = ra_alloc_itemp();
        IR2_OPND mxcsr = ra_alloc_itemp();
        IR2_OPND precision = ra_alloc_itemp();
        IR2_OPND no_exception = ra_alloc_label();

        /* Read all sources before changing FCSR or a possibly aliased dest. */
        la_vori_b(src1, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1)), 0);
        if (ir1_opnd_is_mem(opnd2)) {
            if (ir1_opnd_size(opnd2) == 32) {
                src2 = load_u32_from_ir1_mem_exact(opnd2);
                la_mov32_sx(src2, src2);
            } else {
                src2 = load_u64_from_ir1_mem_exact(opnd2);
            }
        } else {
            src2 = load_ireg_from_ir1(opnd2, SIGN_EXTENSION, false);
        }
        fcsr_opnd = set_fpu_fcsr_rounding_field_by_x86();
        la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
        la_bstrins_w(fcsr, zero_ir2_opnd,
                     FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_I);
        la_bstrins_w(fcsr, zero_ir2_opnd,
                     FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_I);
        la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
        /* xvffint operates both LSX lanes; make the inactive lane exact zero. */
        la_vxor_v(converted, converted, converted);
        la_vinsgr2vr_d(converted, src2, 0);
        if (ir1_opnd_size(opnd2) == 32) {
            la_vffintl_d_w(converted, converted);
        } else {
            la_vffint_d_l(converted, converted);
        }
        la_vpickve2gr_du(converted_low, converted, 0);
        ra_free_temp(converted);
        ra_free_temp_auto(src2);
        la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
        set_fpu_rounding_mode(fcsr_opnd);
        ra_free_temp_auto(fcsr_opnd);

        /* FCSR inexact is x86 MXCSR precision; raise #XM before writeback. */
        la_bstrpick_w(precision, fcsr, FCSR_OFF_FLAGS_I, FCSR_OFF_FLAGS_I);
        la_slli_w(precision, precision, 5);
        la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
        la_or(mxcsr, mxcsr, precision);
        la_bstrpick_w(fcsr, mxcsr, 12, 12);
        la_beq(precision, zero_ir2_opnd, no_exception);
        la_bne(fcsr, zero_ir2_opnd, no_exception);
        la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
        ra_free_temp(fcsr);

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
        la_mov64(a0_ir2_opnd, precision);
        la_jirl(zero_ir2_opnd, helper, 0);
        ra_free_temp(helper);
        ra_free_temp(precision);

        la_label(no_exception);
        la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
        la_vinsgr2vr_d(src1, converted_low, 0);
        la_vori_b(ra_alloc_xmm(dest_index), src1, 0);
        clear_ymm_high128_shadow(dest_index);
        ra_free_temp(mxcsr);
        ra_free_temp(converted_low);
        ra_free_temp(src1);
    }
    return true;
}


#endif
