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

bool translate_vcvtps2pd(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtps2pd_lsx(pir1);
    }

    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vfcvth_d_s(temp, src);
        la_vfcvtl_d_s(dest, src);
        la_xvpermi_q(dest, temp, XVPERMI_Q_4_0(0, 2));
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vfcvtl_d_s(temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vcvtpd2ps(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtpd2ps_lsx(pir1);
    }

    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)));

    if (ir1_opnd_size(ir1_get_opnd(pir1, 1)) == 128) {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));

        la_vfcvt_s_d(dest, src, src);
        la_xvpickve_d(dest, dest, 0);
    } else if (ir1_opnd_size(ir1_get_opnd(pir1, 1)) == 256) {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvpermi_q(temp, src, XVPERMI_Q_4_0(1, 1));
        la_vfcvt_s_d(temp, temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    } else {
        lsassert(0);
    }
    return true;
}

bool translate_vcvtdq2ps(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtdq2ps_lsx(pir1);
    }

    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));

        la_xvffint_s_w(dest, src);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vffint_s_w(temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

static bool translate_vcvtps2dq_opt(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    lsassert(ir1_opnd_is_xmm(opnd0) ||
        ir1_opnd_is_ymm(opnd0));

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND temp_f = ra_alloc_ftemp();
    IR2_OPND sse_invalid = ra_alloc_ftemp();
    IR2_OPND overflow = ra_alloc_ftemp();
    IR2_OPND comp_mask = ra_alloc_ftemp();

    if (ir1_opnd_is_xmm(opnd0)) {
        la_vftint_w_s(temp_f, src);
        la_vldi(sse_invalid, 0b1001110000000); // broadcast 0x80000000 to all 0x1380
        la_vldi(overflow, (0b10011 << 8) | 0x4f); //0x134f
        la_vfcmp_cond_s(comp_mask, overflow, src, 0xE); // get Nan mark 0xE=cULE
        la_vbitsel_v(temp_f, temp_f, sse_invalid, comp_mask);
        la_vand_v(dest, temp_f, temp_f);
        set_high128_xreg_to_zero(dest);
    } else {
        la_xvftint_w_s(temp_f, src);
        la_xvldi(sse_invalid, 0b1001110000000); // broadcast 0x80000000 to all 0x1380
        la_xvldi(overflow, (0b10011 << 8) | 0x4f); //0x134f
        la_xvfcmp_cond_s(comp_mask, overflow, src, 0xE); // get Nan mark 0xE=cULE
        la_xvbitsel_v(temp_f, temp_f, sse_invalid, comp_mask);
        la_xvand_v(dest, temp_f, temp_f);
    }
    return true;
}

bool translate_vcvtps2dq(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtps2dq_lsx(pir1);
    }

    if (option_cvt_opt) {
        return translate_vcvtps2dq_opt(pir1);
    }
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    lsassert(ir1_opnd_is_xmm(opnd0) ||
        ir1_opnd_is_ymm(opnd0));

    bool is_same_reg = ir1_opnd_is_same_reg(opnd0, opnd1);

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND temp_fcsr = ra_alloc_itemp();
        IR2_OPND temp_int = ra_alloc_itemp();
        IR2_OPND temp_operand_count = ra_alloc_itemp();
        IR2_OPND label_over = ra_alloc_label();
        IR2_OPND label_second_operand = ra_alloc_label();
        IR2_OPND label_third_operand = ra_alloc_label();
        IR2_OPND label_fourth_operand = ra_alloc_label();
        IR2_OPND label_fifth_operand = ra_alloc_label();
        IR2_OPND label_sixth_operand = ra_alloc_label();
        IR2_OPND label_seventh_operand = ra_alloc_label();
        IR2_OPND label_eighth_operand = ra_alloc_label();
        IR2_OPND dest = is_same_reg ? ra_alloc_ftemp() : load_freg256_from_ir1(opnd0);
        IR2_OPND src = load_freg256_from_ir1(opnd1);

        la_xvftint_w_s(dest, src);

        /* check if INVALID bit in fcsr is set */
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);

        /* if no INVALID exception happend, convertion done */
        la_beqz(temp_fcsr, label_over);

        /* if INVALID exception did happen, check the four operands separately */
        li_wu(temp_int, 0x80000000);
        IR2_OPND ftemp_src_temp1 = ra_alloc_ftemp();
        IR2_OPND ftemp_src_temp2 = ra_alloc_ftemp();
        IR2_OPND src_h128 = ra_alloc_ftemp();
        la_xvpermi_q(src_h128, src, XVPERMI_Q_4_0(1, 1));

        /* check the first single operand */
        la_xvreplve_w(ftemp_src_temp1, src, zero_ir2_opnd);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_second_operand);
        la_xvinsgr2vr_w(dest, temp_int, 0);

        /* check the second single operand */
        la_label(label_second_operand);
        li_wu(temp_operand_count, 0x1);
        la_xvreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_third_operand);
        la_xvinsgr2vr_w(dest, temp_int, 1);

        /* check the third single operand */
        la_label(label_third_operand);
        li_wu(temp_operand_count, 0x2);
        la_xvreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_fourth_operand);
        la_xvinsgr2vr_w(dest, temp_int, 2);

        /* check the fourth single operand */
        la_label(label_fourth_operand);
        li_wu(temp_operand_count, 0x3);
        la_xvreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_fifth_operand);
        la_xvinsgr2vr_w(dest, temp_int, 3);

        /* check the fifth single operand */
        la_label(label_fifth_operand);
        la_xvreplve_w(ftemp_src_temp1, src_h128, zero_ir2_opnd);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_sixth_operand);
        la_xvinsgr2vr_w(dest, temp_int, 4);

        /* check the sixth single operand */
        la_label(label_sixth_operand);
        li_wu(temp_operand_count, 0x1);
        la_xvreplve_w(ftemp_src_temp1, src_h128, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_seventh_operand);
        la_xvinsgr2vr_w(dest, temp_int, 5);

        /* check the seventh single operand */
        la_label(label_seventh_operand);
        li_wu(temp_operand_count, 0x2);
        la_xvreplve_w(ftemp_src_temp1, src_h128, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_eighth_operand);
        la_xvinsgr2vr_w(dest, temp_int, 6);

        /* check the eighth single operand */
        la_label(label_eighth_operand);
        li_wu(temp_operand_count, 0x3);
        la_xvreplve_w(ftemp_src_temp1, src_h128, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_over);
        la_xvinsgr2vr_w(dest, temp_int, 7);

        la_label(label_over);
        if (is_same_reg) {
            la_xvori_b(load_freg256_from_ir1(opnd0), dest, 0);
        }
        ra_free_temp(temp_fcsr);
        ra_free_temp(temp_int);
        ra_free_temp(temp_operand_count);
        ra_free_temp(ftemp_src_temp1);
        ra_free_temp(ftemp_src_temp2);
    } else {
        IR2_OPND temp_fcsr = ra_alloc_itemp();
        IR2_OPND temp_int = ra_alloc_itemp();
        IR2_OPND temp_operand_count = ra_alloc_itemp();
        IR2_OPND label_over = ra_alloc_label();
        IR2_OPND label_second_operand = ra_alloc_label();
        IR2_OPND label_third_operand = ra_alloc_label();
        IR2_OPND label_fourth_operand = ra_alloc_label();
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND src = load_freg128_from_ir1(opnd1);
        IR2_OPND temp = ra_alloc_ftemp();

        la_vftint_w_s(temp, src);

        /* check if INVALID bit in fcsr is set */
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);

        /* if no INVALID exception happend, convertion done */
        la_beqz(temp_fcsr, label_over);

        /* if INVALID exception did happen, check the four operands separately */
        li_wu(temp_int, 0x80000000);
        IR2_OPND ftemp_src_temp1 = ra_alloc_ftemp();
        IR2_OPND ftemp_src_temp2 = ra_alloc_ftemp();

        /* check the first single operand */
        la_vreplve_w(ftemp_src_temp1, src, zero_ir2_opnd);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_second_operand);
        la_vinsgr2vr_w(temp, temp_int, 0);

        /* check the second single operand */
        la_label(label_second_operand);
        li_wu(temp_operand_count, 0x1);
        la_vreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_third_operand);
        la_vinsgr2vr_w(temp, temp_int, 1);

        /* check the third single operand */
        la_label(label_third_operand);
        li_wu(temp_operand_count, 0x2);
        la_vreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_fourth_operand);
        la_vinsgr2vr_w(temp, temp_int, 2);

        /* check the fourth single operand */
        la_label(label_fourth_operand);
        li_wu(temp_operand_count, 0x3);
        la_vreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftint_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_over);
        la_vinsgr2vr_w(temp, temp_int, 3);

        la_label(label_over);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
        ra_free_temp(temp_fcsr);
        ra_free_temp(temp_int);
        ra_free_temp(temp_operand_count);
        ra_free_temp(ftemp_src_temp1);
        ra_free_temp(ftemp_src_temp2);
    }
    return true;
}

bool translate_vcvtdq2pd(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtdq2pd_lsx(pir1);
    }

    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));
    IR2_OPND fcsr_opnd = set_fpu_fcsr_rounding_field_by_x86();
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_vffintl_d_w(temp1, src);
        la_vffinth_d_w(temp2, src);
        la_xvpermi_q(temp1, temp2, XVPERMI_Q_4_0(0, 2));
        la_xvori_b(dest, temp1, 0);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vffintl_d_w(temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    set_fpu_rounding_mode(fcsr_opnd);
    return true;
}

static bool translate_vcvtpd2dq_opt(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND temp_f = ra_alloc_ftemp();
    IR2_OPND mxcsr = ra_alloc_itemp();
    IR2_OPND temp_i = ra_alloc_itemp();
    int offset = lsenv_offset_of_mxcsr(lsenv);
    IR2_OPND label_rd_rz = ra_alloc_label();
    IR2_OPND label_ru = ra_alloc_label();
    IR2_OPND label_exit = ra_alloc_label();
    IR2_OPND sse_invalid = ra_alloc_ftemp();
    IR2_OPND overflow = ra_alloc_ftemp();
    IR2_OPND comp_mask = ra_alloc_ftemp();

    la_ld_w(mxcsr, env_ir2_opnd, offset);
    la_bstrpick_d(temp_i, mxcsr, 13, 13);
    la_bnez(temp_i, label_rd_rz);

    la_bstrpick_d(temp_i, mxcsr, 14, 14);
    la_bnez(temp_i, label_ru);
    /*Round to nearest(00B)*/
    li_d(temp_i, 0x41dfffffffe00000); //2147483647.5
    la_b(label_exit);
    la_label(label_ru);
    /*Round up (10B)*/
    li_d(temp_i, 0x41DFFFFFFFC00001); //2147483647.0000002
    la_b(label_exit);
    la_label(label_rd_rz);
    /*Round down(01B) or Round toward zero(11B)*/
    li_d(temp_i, 0x41E0000000000000); //2147483648.0
    la_label(label_exit);

    if (ir1_opnd_size(opnd1) == 128) {
        la_vftint_w_d(temp_f, src, src);
        la_vldi(sse_invalid, 0b1001110000000); // broadcast 0x80000000 to all 0x1380
        la_vreplgr2vr_d(overflow, temp_i);
        la_vfcmp_cond_d(comp_mask, overflow, src, 0xE); // get Nan mark 0xE=cULE
        la_vshuf4i_w(comp_mask, comp_mask, 0x88);
        la_vbitsel_v(temp_f, temp_f, sse_invalid, comp_mask);
        la_xvpickve_d(dest, temp_f, 0);
    } else {
        IR2_OPND src_h128 = ra_alloc_ftemp();
        la_xvpermi_q(src_h128, src, XVPERMI_Q_4_0(1, 1));
        la_vftint_w_d(temp_f, src_h128, src);
        la_xvldi(sse_invalid, 0b1001110000000); // broadcast 0x80000000 to all 0x1380
        la_xvreplgr2vr_d(overflow, temp_i);
        la_xvfcmp_cond_d(comp_mask, overflow, src, 0xE); // get Nan mark 0xE=cULE
        la_xvshuf4i_w(comp_mask, comp_mask, 0x88);
        la_xvpermi_d(comp_mask, comp_mask, 0x88);
        // la_xvshuf4i_d(comp_mask, comp_mask, 0x88);
        la_vbitsel_v(temp_f, temp_f, sse_invalid, comp_mask);
        la_vand_v(dest, temp_f, temp_f);
        set_high128_xreg_to_zero(dest);
    }
    return true;
}
bool translate_vcvtpd2dq(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtpd2dq_lsx(pir1);
    }

    if (option_cvt_opt) {
        return translate_vcvtpd2dq_opt(pir1);
    }

    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));

    if (ir1_opnd_size(ir1_get_opnd(pir1, 1)) == 256) {
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src_h128 = ra_alloc_ftemp();
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp_fcsr = ra_alloc_itemp();
        IR2_OPND temp_int = ra_alloc_itemp();
        IR2_OPND label_over = ra_alloc_label();
        IR2_OPND label_second_operand = ra_alloc_label();
        IR2_OPND label_third_operand = ra_alloc_label();
        IR2_OPND label_fourth_operand = ra_alloc_label();

        la_xvpermi_q(src_h128, src, XVPERMI_Q_4_0(1, 1));
        la_vftint_w_d(temp, src_h128, src);

        /* check if INVALID bit in fcsr is set */
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);

        /* if no INVALID exception happend, convertion done */
        la_beqz(temp_fcsr, label_over);

        /* if INVALID exception did happen, check the four operands separately */
        li_wu(temp_int, 0x80000000);
        IR2_OPND ftemp_src_temp1 = ra_alloc_ftemp();
        IR2_OPND ftemp_src_temp2 = ra_alloc_ftemp();

        /* check the first single operand */
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(0, 0, 0, 0));
        la_vftint_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_second_operand);
        la_vinsgr2vr_w(temp, temp_int, 0);

        /* check the second single operand */
        la_label(label_second_operand);
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(1, 1, 1, 1));
        la_vftint_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_third_operand);
        la_vinsgr2vr_w(temp, temp_int, 1);

        /* check the third single operand */
        la_label(label_third_operand);
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(2, 2, 2, 2));
        la_vftint_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_fourth_operand);
        la_vinsgr2vr_w(temp, temp_int, 2);

        /* check the fourth single operand */
        la_label(label_fourth_operand);
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(3, 3, 3, 3));
        la_vftint_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_over);
        la_vinsgr2vr_w(temp, temp_int, 3);

        la_label(label_over);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
        ra_free_temp(temp_fcsr);
        ra_free_temp(temp_int);
        ra_free_temp(ftemp_src_temp1);
        ra_free_temp(ftemp_src_temp2);
    } else if (ir1_opnd_size(ir1_get_opnd(pir1, 1)) == 128) {
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp_fcsr = ra_alloc_itemp();
        IR2_OPND temp_int = ra_alloc_itemp();
        IR2_OPND label_over = ra_alloc_label();
        IR2_OPND label_second_operand = ra_alloc_label();

        la_vftint_w_d(temp, src, src);

        /* check if INVALID bit in fcsr is set */
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);

        /* if no INVALID exception happend, convertion done */
        la_beqz(temp_fcsr, label_over);

        /* if INVALID exception did happen, check the four operands separately */
        li_wu(temp_int, 0x80000000);
        IR2_OPND ftemp_src_temp1 = ra_alloc_ftemp();
        IR2_OPND ftemp_src_temp2 = ra_alloc_ftemp();

        /* check the first single operand */
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(0, 0, 0, 0));
        la_vftint_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_second_operand);
        la_vinsgr2vr_w(temp, temp_int, 0);

        /* check the second single operand */
        la_label(label_second_operand);
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(1, 1, 1, 1));
        la_vftint_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_over);
        la_vinsgr2vr_w(temp, temp_int, 1);

        la_label(label_over);
        la_xvpickve_d(dest, temp, 0);
        ra_free_temp(temp_fcsr);
        ra_free_temp(temp_int);
        ra_free_temp(ftemp_src_temp1);
        ra_free_temp(ftemp_src_temp2);
    } else {
        lsassert(0);
    }

    return true;
}

bool translate_vcvtsd2ss(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtsd2ss_lsx(pir1);
    }

    IR2_OPND fcsr_opnd = set_fpu_fcsr_rounding_field_by_x86();
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND fcsr = ra_alloc_itemp();
    IR2_OPND mxcsr = ra_alloc_itemp();
    IR2_OPND flags = ra_alloc_itemp();
    IR2_OPND bit = ra_alloc_itemp();
    IR2_OPND masks = ra_alloc_itemp();
    IR2_OPND unmasked = ra_alloc_itemp();
    IR2_OPND keep_denormal = ra_alloc_label();
    IR2_OPND daz_done = ra_alloc_label();
    IR2_OPND check_overflow = ra_alloc_label();
    IR2_OPND check_underflow = ra_alloc_label();
    IR2_OPND check_precision = ra_alloc_label();
    IR2_OPND exception_ready = ra_alloc_label();
    IR2_OPND no_exception = ra_alloc_label();
    IR2_OPND temp = ra_alloc_ftemp();
    IR2_OPND dest_temp = ra_alloc_ftemp();
    la_or(flags, zero_ir2_opnd, zero_ir2_opnd);
    la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    la_vpickve2gr_du(fcsr, src2, 0);
    li_d(masks, UINT64_C(0x7ff0000000000000));
    la_and(bit, fcsr, masks);
    la_bne(bit, zero_ir2_opnd, daz_done);
    li_d(masks, UINT64_C(0x000fffffffffffff));
    la_and(bit, fcsr, masks);
    la_beq(bit, zero_ir2_opnd, daz_done);
    la_andi(bit, mxcsr, 0x40);
    la_beq(bit, zero_ir2_opnd, keep_denormal);
    li_d(masks, UINT64_C(0x8000000000000000));
    la_and(fcsr, fcsr, masks);
    la_vinsgr2vr_d(src2, fcsr, 0);
    la_b(daz_done);
    la_label(keep_denormal);
    la_ori(flags, flags, 0x2);
    la_label(daz_done);
    la_vinsgr2vr_d(src2, zero_ir2_opnd, 1);
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_bstrins_w(fcsr, zero_ir2_opnd,
                 FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_I);
    la_bstrins_w(fcsr, zero_ir2_opnd,
                 FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_I);
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
    la_fcvt_s_d(temp, src2);
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    set_fpu_rounding_mode(fcsr_opnd);
    ra_free_temp_auto(fcsr_opnd);

    la_bstrpick_w(bit, fcsr, FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_V);
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
    ra_free_temp(fcsr);
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
    ra_free_temp(flags);
    ra_free_temp(mxcsr);
    la_vori_b(dest_temp, src1, 0);
    la_xvinsve0_w(dest_temp, temp, 0);
    set_high128_xreg_to_zero(dest_temp);
    la_xvori_b(dest, dest_temp, 0);
    return true;
}

bool translate_vcvtsi2sd(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtsi2sd_lsx(pir1);
    }

    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2;
    IR2_OPND fcsr_opnd = set_fpu_fcsr_rounding_field_by_x86();
    IR2_OPND fcsr = ra_alloc_itemp();
    IR2_OPND mxcsr = ra_alloc_itemp();
    IR2_OPND precision = ra_alloc_itemp();
    IR2_OPND no_exception = ra_alloc_label();

    if (ir1_opnd_is_mem(opnd2)) {
        if (ir1_opnd_size(opnd2) == 32) {
            src2 = load_u32_from_ir1_mem_exact(opnd2);
            la_mov32_sx(src2, src2);
        } else {
            src2 = load_u64_from_ir1_mem_exact(opnd2);
        }
    } else {
        src2 = load_ireg_from_ir1(opnd2, UNKNOWN_EXTENSION, false);
    }
    IR2_OPND temp = ra_alloc_ftemp();

    /* Clear stale FCSR flags before conversion and preserve MXCSR rounding. */
    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    la_bstrins_w(fcsr, zero_ir2_opnd,
                 FCSR_OFF_FLAGS_V, FCSR_OFF_FLAGS_I);
    la_bstrins_w(fcsr, zero_ir2_opnd,
                 FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_I);
    la_movgr2fcsr(fcsr_ir2_opnd, fcsr);
    la_movgr2fr_d(temp, src2);
    if (ir1_opnd_size(ir1_get_opnd(pir1, 2)) == 32) {
        la_vffintl_d_w(temp, temp);
    } else {
        la_vffint_d_l(temp, temp);
    }
    la_vshuf4i_d(temp, src1, 0xc);
    set_high128_xreg_to_zero(temp);

    la_movfcsr2gr(fcsr, fcsr_ir2_opnd);
    set_fpu_rounding_mode(fcsr_opnd);
    ra_free_temp_auto(fcsr_opnd);

    /* Map LoongArch inexact to MXCSR precision and raise #XM when unmasked. */
    la_bstrpick_w(precision, fcsr, FCSR_OFF_FLAGS_I, FCSR_OFF_FLAGS_I);
    la_slli_w(precision, precision, 5);
    la_ld_wu(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    la_or(mxcsr, mxcsr, precision);
    la_bstrpick_w(fcsr, mxcsr, 12, 12);
    la_beq(precision, zero_ir2_opnd, no_exception);
    la_bne(fcsr, zero_ir2_opnd, no_exception);
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
    aot_load_host_addr(helper, (ADDR)helper_raise_simd_exception,
                       LOAD_HELPER_RAISE_SIMD_EXCEPTION, 0);
    la_mov64(a0_ir2_opnd, precision);
    la_jirl(zero_ir2_opnd, helper, 0);

    la_label(no_exception);
    la_st_w(mxcsr, env_ir2_opnd, lsenv_offset_of_mxcsr(lsenv));
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vcvtss2sd(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtss2sd_lsx(pir1);
    }

    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND temp = ra_alloc_ftemp();
    la_fcvt_d_s(temp, src2);
    la_vshuf4i_d(temp, src1, 0xc);
    set_high128_xreg_to_zero(temp);
    la_xvori_b(dest, temp, 0);
    return true;
}

bool translate_vcvtsi2ss(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvtsi2ss_lsx(pir1);
    }

    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) &&
        ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    IR2_OPND fcsr_opnd = set_fpu_fcsr_rounding_field_by_x86();
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_ireg_from_ir1(opnd2, UNKNOWN_EXTENSION, false);
    IR2_OPND temp = ra_alloc_ftemp();
    la_movgr2fr_d(temp, src2);
    if (ir1_opnd_size(opnd2) == 64) {
        la_ffint_s_l(temp, temp);
    } else {
        la_ffint_s_w(temp, temp);
    }
    if (ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)) != ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))) {
        la_xvori_b(dest, src1, 0);
    }
    la_xvinsve0_w(dest, temp, 0);
    set_high128_xreg_to_zero(dest);
    set_fpu_rounding_mode(fcsr_opnd);
    return true;
}

static bool translate_vcvttpd2dq_opt(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);

    IR2_OPND temp_f = ra_alloc_ftemp();
    IR2_OPND temp_i = ra_alloc_itemp();
    IR2_OPND sse_invalid = ra_alloc_ftemp();
    IR2_OPND overflow = ra_alloc_ftemp();
    IR2_OPND comp_mask = ra_alloc_ftemp();

    li_d(temp_i, 0x41E0000000000000); //0x41E0000000000000 2^31

    if (ir1_opnd_size(opnd1) == 128) {
        la_vftintrz_w_d(temp_f, src, src);
        la_vldi(sse_invalid, 0b1001110000000); // broadcast 0x80000000 to all 0x1380
        la_vreplgr2vr_d(overflow, temp_i);
        la_vfcmp_cond_d(comp_mask, overflow, src, 0xE); // get Nan mark 0xE=cULE
        la_vshuf4i_w(comp_mask, comp_mask, 0x88);
        la_vbitsel_v(temp_f, temp_f, sse_invalid, comp_mask);
        la_xvpickve_d(dest, temp_f, 0);
    } else {
        IR2_OPND src_h128 = ra_alloc_ftemp();
        la_xvpermi_q(src_h128, src, XVPERMI_Q_4_0(1, 1));
        la_xvftintrz_w_d(temp_f, src_h128, src);
        la_xvldi(sse_invalid, 0b1001110000000); // broadcast 0x80000000 to all 0x1380
        la_xvreplgr2vr_d(overflow, temp_i);
        la_xvfcmp_cond_d(comp_mask, overflow, src, 0xE); // get Nan mark 0xE=cULE
        la_xvshuf4i_w(comp_mask, comp_mask, 0x88);
        la_xvpermi_d(comp_mask, comp_mask, 0x88);
        la_vbitsel_v(temp_f, temp_f, sse_invalid, comp_mask);
        la_vand_v(dest, temp_f, temp_f);
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vcvttpd2dq(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvttpd2dq_lsx(pir1);
    }

    if (option_cvt_opt) {
        return translate_vcvttpd2dq_opt(pir1);
    }

    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));

    if (ir1_opnd_size(ir1_get_opnd(pir1, 1)) == 256) {
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND src_h128 = ra_alloc_ftemp();
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp_fcsr = ra_alloc_itemp();
        IR2_OPND temp_int = ra_alloc_itemp();
        IR2_OPND label_over = ra_alloc_label();
        IR2_OPND label_second_operand = ra_alloc_label();
        IR2_OPND label_third_operand = ra_alloc_label();
        IR2_OPND label_fourth_operand = ra_alloc_label();

        la_xvpermi_q(src_h128, src, XVPERMI_Q_4_0(1, 1));
        la_vftintrz_w_d(temp, src_h128, src);

        /* check if INVALID bit in fcsr is set */
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);

        /* if no INVALID exception happend, convertion done */
        la_beqz(temp_fcsr, label_over);

        /* if INVALID exception did happen, check the four operands separately */
        li_wu(temp_int, 0x80000000);
        IR2_OPND ftemp_src_temp1 = ra_alloc_ftemp();
        IR2_OPND ftemp_src_temp2 = ra_alloc_ftemp();

        /* check the first single operand */
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(0, 0, 0, 0));
        la_vftintrz_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_second_operand);
        la_vinsgr2vr_w(temp, temp_int, 0);

        /* check the second single operand */
        la_label(label_second_operand);
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(1, 1, 1, 1));
        la_vftintrz_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_third_operand);
        la_vinsgr2vr_w(temp, temp_int, 1);

        /* check the third single operand */
        la_label(label_third_operand);
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(2, 2, 2, 2));
        la_vftintrz_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_fourth_operand);
        la_vinsgr2vr_w(temp, temp_int, 2);

        /* check the fourth single operand */
        la_label(label_fourth_operand);
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(3, 3, 3, 3));
        la_vftintrz_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_over);
        la_vinsgr2vr_w(temp, temp_int, 3);

        la_label(label_over);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
        ra_free_temp(temp_fcsr);
        ra_free_temp(temp_int);
        ra_free_temp(ftemp_src_temp1);
        ra_free_temp(ftemp_src_temp2);
    } else if (ir1_opnd_size(ir1_get_opnd(pir1, 1)) == 128) {
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp_fcsr = ra_alloc_itemp();
        IR2_OPND temp_int = ra_alloc_itemp();
        IR2_OPND label_over = ra_alloc_label();
        IR2_OPND label_second_operand = ra_alloc_label();

        la_vftintrz_w_d(temp, src, src);

        /* check if INVALID bit in fcsr is set */
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);

        /* if no INVALID exception happend, convertion done */
        la_beqz(temp_fcsr, label_over);

        /* if INVALID exception did happen, check the four operands separately */
        li_wu(temp_int, 0x80000000);
        IR2_OPND ftemp_src_temp1 = ra_alloc_ftemp();
        IR2_OPND ftemp_src_temp2 = ra_alloc_ftemp();

        /* check the first single operand */
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(0, 0, 0, 0));
        la_vftintrz_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_second_operand);
        la_vinsgr2vr_w(temp, temp_int, 0);

        /* check the second single operand */
        la_label(label_second_operand);
        la_xvpermi_d(ftemp_src_temp1, src, XVPERMI_D_2_2_2_2(1, 1, 1, 1));
        la_vftintrz_w_d(ftemp_src_temp2, ftemp_src_temp1, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_over);
        la_vinsgr2vr_w(temp, temp_int, 1);

        la_label(label_over);
        la_xvpickve_d(dest, temp, 0);
        ra_free_temp(temp_fcsr);
        ra_free_temp(temp_int);
        ra_free_temp(ftemp_src_temp1);
        ra_free_temp(ftemp_src_temp2);
    } else {
        lsassert(0);
    }

    return true;
}

static bool translate_vcvttps2dq_opt(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    lsassert(ir1_opnd_is_xmm(opnd0) ||
        ir1_opnd_is_ymm(opnd0));

    IR2_OPND dest =  load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND temp_f = ra_alloc_ftemp();
    IR2_OPND sse_invalid = ra_alloc_ftemp();
    IR2_OPND overflow = ra_alloc_ftemp();
    IR2_OPND comp_mask = ra_alloc_ftemp();

    if (ir1_opnd_is_xmm(opnd0)) {
        la_vftintrz_w_s(temp_f, src);
        la_vldi(sse_invalid, 0b1001110000000); // broadcast 0x80000000 to all 0x1380
        la_vldi(overflow, (0b10011 << 8) | 0x4f); //0x134f 2^31
        la_vfcmp_cond_s(comp_mask, overflow, src, 0xE); // get Nan mark 0xE=cULE
        la_vbitsel_v(temp_f, temp_f, sse_invalid, comp_mask);
        la_vand_v(dest, temp_f, temp_f);
        set_high128_xreg_to_zero(dest);
    } else {
        la_xvftintrz_w_s(temp_f, src);
        la_xvldi(sse_invalid, 0b1001110000000); // broadcast 0x80000000 to all 0x1380
        la_xvldi(overflow, (0b10011 << 8) | 0x4f); //0x134f 2^31
        la_xvfcmp_cond_s(comp_mask, overflow, src, 0xE); // get Nan mark 0xE=cULE
        la_xvbitsel_v(temp_f, temp_f, sse_invalid, comp_mask);
        la_xvand_v(dest, temp_f, temp_f);
    }
    return true;
}

bool translate_vcvttps2dq(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vcvttps2dq_lsx(pir1);
    }

    if (option_cvt_opt) {
        return translate_vcvttps2dq_opt(pir1);
    }

    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    lsassert(ir1_opnd_is_xmm(opnd0) ||
        ir1_opnd_is_ymm(opnd0));

    bool is_same_reg = ir1_opnd_is_same_reg(opnd0, opnd1);

    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND temp_fcsr = ra_alloc_itemp();
        IR2_OPND temp_int = ra_alloc_itemp();
        IR2_OPND temp_operand_count = ra_alloc_itemp();
        IR2_OPND label_over = ra_alloc_label();
        IR2_OPND label_second_operand = ra_alloc_label();
        IR2_OPND label_third_operand = ra_alloc_label();
        IR2_OPND label_fourth_operand = ra_alloc_label();
        IR2_OPND label_fifth_operand = ra_alloc_label();
        IR2_OPND label_sixth_operand = ra_alloc_label();
        IR2_OPND label_seventh_operand = ra_alloc_label();
        IR2_OPND label_eighth_operand = ra_alloc_label();
        IR2_OPND dest = is_same_reg ? ra_alloc_ftemp() : load_freg256_from_ir1(opnd0);
        IR2_OPND src = load_freg256_from_ir1(opnd1);

        la_xvftintrz_w_s(dest, src);

        /* check if INVALID bit in fcsr is set */
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);

        /* if no INVALID exception happend, convertion done */
        la_beqz(temp_fcsr, label_over);

        /* if INVALID exception did happen, check the four operands separately */
        li_wu(temp_int, 0x80000000);
        IR2_OPND ftemp_src_temp1 = ra_alloc_ftemp();
        IR2_OPND ftemp_src_temp2 = ra_alloc_ftemp();
        IR2_OPND src_h128 = ra_alloc_ftemp();
        la_xvpermi_q(src_h128, src, XVPERMI_Q_4_0(1, 1));

        /* check the first single operand */
        la_xvreplve_w(ftemp_src_temp1, src, zero_ir2_opnd);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_second_operand);
        la_xvinsgr2vr_w(dest, temp_int, 0);

        /* check the second single operand */
        la_label(label_second_operand);
        li_wu(temp_operand_count, 0x1);
        la_xvreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_third_operand);
        la_xvinsgr2vr_w(dest, temp_int, 1);

        /* check the third single operand */
        la_label(label_third_operand);
        li_wu(temp_operand_count, 0x2);
        la_xvreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_fourth_operand);
        la_xvinsgr2vr_w(dest, temp_int, 2);

        /* check the fourth single operand */
        la_label(label_fourth_operand);
        li_wu(temp_operand_count, 0x3);
        la_xvreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_fifth_operand);
        la_xvinsgr2vr_w(dest, temp_int, 3);

        /* check the fifth single operand */
        la_label(label_fifth_operand);
        la_xvreplve_w(ftemp_src_temp1, src_h128, zero_ir2_opnd);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_sixth_operand);
        la_xvinsgr2vr_w(dest, temp_int, 4);

        /* check the sixth single operand */
        la_label(label_sixth_operand);
        li_wu(temp_operand_count, 0x1);
        la_xvreplve_w(ftemp_src_temp1, src_h128, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_seventh_operand);
        la_xvinsgr2vr_w(dest, temp_int, 5);

        /* check the seventh single operand */
        la_label(label_seventh_operand);
        li_wu(temp_operand_count, 0x2);
        la_xvreplve_w(ftemp_src_temp1, src_h128, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_eighth_operand);
        la_xvinsgr2vr_w(dest, temp_int, 6);

        /* check the eighth single operand */
        la_label(label_eighth_operand);
        li_wu(temp_operand_count, 0x3);
        la_xvreplve_w(ftemp_src_temp1, src_h128, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_over);
        la_xvinsgr2vr_w(dest, temp_int, 7);

        la_label(label_over);
        if (is_same_reg) {
            la_xvori_b(load_freg256_from_ir1(opnd0), dest, 0);
        }
        ra_free_temp(temp_fcsr);
        ra_free_temp(temp_int);
        ra_free_temp(temp_operand_count);
        ra_free_temp(ftemp_src_temp1);
        ra_free_temp(ftemp_src_temp2);
    } else {
        IR2_OPND temp_fcsr = ra_alloc_itemp();
        IR2_OPND temp_int = ra_alloc_itemp();
        IR2_OPND temp_operand_count = ra_alloc_itemp();
        IR2_OPND label_over = ra_alloc_label();
        IR2_OPND label_second_operand = ra_alloc_label();
        IR2_OPND label_third_operand = ra_alloc_label();
        IR2_OPND label_fourth_operand = ra_alloc_label();
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_vftintrz_w_s(temp, src);

        /* check if INVALID bit in fcsr is set */
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);

        /* if no INVALID exception happend, convertion done */
        la_beqz(temp_fcsr, label_over);

        /* if INVALID exception did happen, check the four operands separately */
        li_wu(temp_int, 0x80000000);
        IR2_OPND ftemp_src_temp1 = ra_alloc_ftemp();
        IR2_OPND ftemp_src_temp2 = ra_alloc_ftemp();

        /* check the first single operand */
        la_vreplve_w(ftemp_src_temp1, src, zero_ir2_opnd);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_second_operand);
        la_vinsgr2vr_w(temp, temp_int, 0);

        /* check the second single operand */
        la_label(label_second_operand);
        li_wu(temp_operand_count, 0x1);
        la_vreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_third_operand);
        la_vinsgr2vr_w(temp, temp_int, 1);

        /* check the third single operand */
        la_label(label_third_operand);
        li_wu(temp_operand_count, 0x2);
        la_vreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_fourth_operand);
        la_vinsgr2vr_w(temp, temp_int, 2);

        /* check the fourth single operand */
        la_label(label_fourth_operand);
        li_wu(temp_operand_count, 0x3);
        la_vreplve_w(ftemp_src_temp1, src, temp_operand_count);
        la_vftintrz_w_s(ftemp_src_temp2, ftemp_src_temp1);
        la_movfcsr2gr(temp_fcsr, fcsr_ir2_opnd);
        la_bstrpick_w(temp_fcsr, temp_fcsr, FCSR_OFF_CAUSE_V, FCSR_OFF_CAUSE_V);
        la_beqz(temp_fcsr, label_over);
        la_vinsgr2vr_w(temp, temp_int, 3);

        la_label(label_over);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
        ra_free_temp(temp_fcsr);
        ra_free_temp(temp_int);
        ra_free_temp(temp_operand_count);
        ra_free_temp(ftemp_src_temp1);
        ra_free_temp(ftemp_src_temp2);
    }
    return true;
}
#endif
