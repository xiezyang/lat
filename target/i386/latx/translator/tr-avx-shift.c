/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "common.h"
#include "reg-alloc.h"
#include "latx-options.h"
#include "translate.h"

#ifdef CONFIG_LATX_AVX_OPT
bool translate_vpslldq(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_is_imm(opnd2));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    uint8_t imm = ir1_opnd_uimm(opnd2);
    if (imm > 15) {
        la_xvandi_b(dest, src, 0);
        return true;
    }
    la_xvbsll_v(dest, src, imm);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpsllx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_INST * ( * rep_inst)(IR2_OPND, IR2_OPND);
    IR2_INST * ( * tr_inst_i)(IR2_OPND, IR2_OPND, int);
    IR2_INST * ( * tr_inst_r)(IR2_OPND, IR2_OPND, IR2_OPND);
    int max_count;
    switch (ir1_opcode(pir1)) {
        case dt_X86_INS_VPSLLW:
            rep_inst = la_xvreplve0_h;
            tr_inst_i = la_xvslli_h;
            tr_inst_r = la_xvsll_h;
            max_count = 15;
            break;
        case dt_X86_INS_VPSLLD:
            rep_inst = la_xvreplve0_w;
            tr_inst_i = la_xvslli_w;
            tr_inst_r = la_xvsll_w;
            max_count = 31;
            break;
        case dt_X86_INS_VPSLLQ:
            rep_inst = la_xvreplve0_d;
            tr_inst_i = la_xvslli_d;
            tr_inst_r = la_xvsll_d;
            max_count = 63;
            break;
        default:
            rep_inst = NULL;
            tr_inst_i = NULL;
            tr_inst_r = NULL;
            max_count = 0;
            lsassert(0);
            break;
    }
    if (ir1_opnd_is_imm(opnd2)) {
        uint8_t imm = ir1_opnd_uimm(opnd2);
        if (imm > max_count) {
            la_xvxor_v(dest, dest, dest);
        } else {
            tr_inst_i(dest, src, imm);
        }
    } else {
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND mask = ra_alloc_ftemp();
        IR2_OPND temp = ra_alloc_ftemp();
        if (max_count == 63) {
            la_xvreplve0_d(mask, src2);
            la_xvldi(temp, VLDI_IMM_TYPE0(3, 63));
            la_xvsle_du(mask, mask, temp);
        } else {
            la_xvreplve0_d(mask, src2);
            la_xvslei_du(mask, mask, max_count);
        }
        rep_inst(temp, src2);
        tr_inst_r(dest, src, temp);
        la_xvand_v(dest, dest, mask);
    }
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

bool translate_vpsrldq(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    lsassert(ir1_opnd_is_imm(opnd2));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    uint8_t imm = ir1_opnd_uimm(opnd2);
    if (imm > 15) {
        la_xvxor_v(dest, dest, dest);
        return true;
    }
    la_xvbsrl_v(dest, src, imm);
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}

typedef IR2_INST *(*latx_avx_shift_imm_fn)(IR2_OPND, IR2_OPND, int);
typedef IR2_INST *(*latx_avx_shift_var_fn)(IR2_OPND, IR2_OPND, IR2_OPND);

static void load_avx_shift_operand_lsx(IR1_OPND *opnd, bool is_ymm,
                                       IR2_OPND *low, IR2_OPND *high)
{
    *high = (IR2_OPND){ 0 };
    if (ir1_opnd_is_mem(opnd)) {
        tr_save_ymm_to_env(UINT16_MAX);
        if (is_ymm) {
            load_v256_from_ir1_mem_exact(opnd, low, high);
        } else {
            *low = load_v128_from_ir1_mem_exact(opnd);
        }
        return;
    }

    lsassert(ir1_opnd_is_xmm(opnd) || ir1_opnd_is_ymm(opnd));
    *low = ra_alloc_ftemp();
    la_vori_b(*low, ra_alloc_xmm(ir1_opnd_base_reg_num(opnd)), 0);
    if (is_ymm) {
        *high = load_ymm_high128_shadow(ir1_opnd_base_reg_num(opnd));
    }
}

static void apply_avx_shift_imm_lsx(IR2_OPND dest, IR2_OPND src,
                                    latx_avx_shift_imm_fn shift, int imm,
                                    int max_count, bool arithmetic)
{
    if (arithmetic) {
        if (imm > max_count) {
            imm = max_count;
        }
        shift(dest, src, imm);
    } else if (imm > max_count) {
        la_vxor_v(dest, dest, dest);
    } else {
        shift(dest, src, imm);
    }
}

static void apply_avx_shift_var_lsx(
    IR2_OPND dest, IR2_OPND src, IR2_OPND count,
    latx_avx_shift_var_fn shift, latx_avx_shift_imm_fn valid,
    latx_avx_shift_imm_fn sign, int max_count, bool arithmetic)
{
    IR2_OPND mask = ra_alloc_ftemp();

    valid(mask, count, max_count);
    shift(dest, src, count);
    if (arithmetic) {
        IR2_OPND sign_fill = ra_alloc_ftemp();

        sign(sign_fill, src, max_count);
        la_vbitsel_v(dest, sign_fill, dest, mask);
        ra_free_temp(sign_fill);
    } else {
        la_vand_v(dest, dest, mask);
    }
    ra_free_temp(mask);
}

static bool translate_avx_integer_shift_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int dest_index = ir1_opnd_base_reg_num(opnd0);
    bool is_ymm = ir1_opnd_is_ymm(opnd0);
    bool arithmetic = false;
    bool scalar_count = false;
    int max_count = 0;
    latx_avx_shift_imm_fn shift_imm = NULL;
    latx_avx_shift_var_fn shift_var = NULL;
    latx_avx_shift_imm_fn valid = NULL;
    latx_avx_shift_imm_fn sign = NULL;

    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
             (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VPSLLW:
        shift_imm = la_vslli_h;
        shift_var = la_vsll_h;
        valid = la_vslei_hu;
        max_count = 15;
        scalar_count = true;
        break;
    case dt_X86_INS_VPSLLD:
        shift_imm = la_vslli_w;
        shift_var = la_vsll_w;
        valid = la_vslei_wu;
        max_count = 31;
        scalar_count = true;
        break;
    case dt_X86_INS_VPSLLQ:
        shift_imm = la_vslli_d;
        shift_var = la_vsll_d;
        valid = la_vslei_du;
        max_count = 63;
        scalar_count = true;
        break;
    case dt_X86_INS_VPSRLW:
        shift_imm = la_vsrli_h;
        shift_var = la_vsrl_h;
        valid = la_vslei_hu;
        max_count = 15;
        scalar_count = true;
        break;
    case dt_X86_INS_VPSRLD:
        shift_imm = la_vsrli_w;
        shift_var = la_vsrl_w;
        valid = la_vslei_wu;
        max_count = 31;
        scalar_count = true;
        break;
    case dt_X86_INS_VPSRLQ:
        shift_imm = la_vsrli_d;
        shift_var = la_vsrl_d;
        valid = la_vslei_du;
        max_count = 63;
        scalar_count = true;
        break;
    case dt_X86_INS_VPSRAW:
        shift_imm = la_vsrai_h;
        shift_var = la_vsra_h;
        valid = la_vslei_hu;
        sign = la_vsrai_h;
        max_count = 15;
        scalar_count = true;
        arithmetic = true;
        break;
    case dt_X86_INS_VPSRAD:
        shift_imm = la_vsrai_w;
        shift_var = la_vsra_w;
        valid = la_vslei_wu;
        sign = la_vsrai_w;
        max_count = 31;
        scalar_count = true;
        arithmetic = true;
        break;
    case dt_X86_INS_VPSLLVD:
        shift_var = la_vsll_w;
        valid = la_vslei_wu;
        max_count = 31;
        break;
    case dt_X86_INS_VPSLLVQ:
        shift_var = la_vsll_d;
        valid = la_vslei_du;
        max_count = 63;
        break;
    case dt_X86_INS_VPSRLVD:
        shift_var = la_vsrl_w;
        valid = la_vslei_wu;
        max_count = 31;
        break;
    case dt_X86_INS_VPSRLVQ:
        shift_var = la_vsrl_d;
        valid = la_vslei_du;
        max_count = 63;
        break;
    case dt_X86_INS_VPSRAVD:
        shift_var = la_vsra_w;
        valid = la_vslei_wu;
        sign = la_vsrai_w;
        max_count = 31;
        arithmetic = true;
        break;
    case dt_X86_INS_VPSLLDQ:
        lsassert(ir1_opnd_is_imm(opnd2));
        shift_imm = la_vbsll_v;
        max_count = 15;
        break;
    case dt_X86_INS_VPSRLDQ:
        lsassert(ir1_opnd_is_imm(opnd2));
        shift_imm = la_vbsrl_v;
        max_count = 15;
        break;
    default:
        lsassert(0);
        return false;
    }

    IR2_OPND src_low;
    IR2_OPND src_high;
    load_avx_shift_operand_lsx(opnd1, is_ymm, &src_low, &src_high);

    if (ir1_opnd_is_imm(opnd2)) {
        int imm = ir1_opnd_uimm(opnd2);

        apply_avx_shift_imm_lsx(src_low, src_low, shift_imm, imm,
                                max_count, arithmetic);
        if (is_ymm) {
            apply_avx_shift_imm_lsx(src_high, src_high, shift_imm, imm,
                                    max_count, arithmetic);
        }
    } else {
        IR2_OPND count_low;
        IR2_OPND count_high;
        bool count_is_ymm = is_ymm && !scalar_count;

        lsassert(shift_var != NULL);
        load_avx_shift_operand_lsx(opnd2, count_is_ymm,
                                   &count_low, &count_high);
        if (scalar_count) {
            IR2_OPND scalar = ra_alloc_ftemp();

            la_vreplvei_d(scalar, count_low, 0);
            apply_avx_shift_var_lsx(src_low, src_low, scalar, shift_var,
                                    valid, sign, max_count, arithmetic);
            if (is_ymm) {
                apply_avx_shift_var_lsx(src_high, src_high, scalar, shift_var,
                                        valid, sign, max_count, arithmetic);
            }
            ra_free_temp(scalar);
        } else {
            apply_avx_shift_var_lsx(src_low, src_low, count_low, shift_var,
                                    valid, sign, max_count, arithmetic);
            if (is_ymm) {
                apply_avx_shift_var_lsx(src_high, src_high, count_high,
                                        shift_var, valid, sign, max_count,
                                        arithmetic);
            }
        }
        ra_free_temp(count_low);
        if (count_is_ymm) {
            ra_free_temp(count_high);
        }
    }

    la_vori_b(ra_alloc_xmm(dest_index), src_low, 0);
    if (is_ymm) {
        store_ymm_high128_shadow(src_high, dest_index);
    } else {
        clear_ymm_high128_shadow(dest_index);
    }
    ra_free_temp(src_low);
    if (is_ymm) {
        ra_free_temp(src_high);
    }
    return true;
}

static bool translate_avx_byte_shift_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpsllx_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpsrlx_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpsrax_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpsllvd_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpsllvq_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpsrlvd_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpsrlvq_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpsravd_lsx(IR1_INST *pir1)
{
    return translate_avx_integer_shift_lsx(pir1);
}

bool translate_vpslldq_lsx(IR1_INST *pir1)
{
    return translate_avx_byte_shift_lsx(pir1);
}

bool translate_vpsrldq_lsx(IR1_INST *pir1)
{
    return translate_avx_byte_shift_lsx(pir1);
}

static bool translate_vpsrlq_lasx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *opnd2 = ir1_get_opnd(pir1, 2);
    int dest_index = ir1_opnd_base_reg_num(opnd0);
    int src_index = ir1_opnd_base_reg_num(opnd1);
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND label_exit = ra_alloc_label();
    IR2_OPND label_shift = ra_alloc_label();

    if (ir1_opnd_is_ymm(opnd1)) {
        IR2_OPND src_high = load_ymm_high128_shadow(src_index);
        IR2_OPND high = ra_alloc_itemp();

        src = ra_alloc_ftemp();
        la_xvori_b(src, ra_alloc_xmm(src_index), 0);
        la_xvpickve2gr_d(high, src_high, 0);
        la_xvinsgr2vr_d(src, high, 2);
        la_xvpickve2gr_d(high, src_high, 1);
        la_xvinsgr2vr_d(src, high, 3);
        ra_free_temp(high);
        ra_free_temp(src_high);
    }

    if (ir1_opnd_is_imm(opnd2)) {
        uint8_t imm = ir1_opnd_uimm(opnd2);
        if (imm > 63) {
            la_xvxor_v(dest, dest, dest);
        } else {
            la_xvsrli_d(dest, src, imm);
        }
    } else {
        IR2_OPND src2;
        IR2_OPND mask = ra_alloc_ftemp();
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND count = ra_alloc_itemp();
        IR2_OPND max = ra_alloc_itemp();

        if (ir1_opnd_is_mem(opnd2)) {
            tr_save_ymm_to_env(UINT16_MAX);
            src2 = load_v128_from_ir1_mem_exact(opnd2);
        } else {
            src2 = load_freg256_from_ir1(opnd2);
        }

        la_addi_d(max, zero_ir2_opnd, 64);
        la_vpickve2gr_d(count, src2, 0);
        la_bltu(count, max, label_shift);
        la_xvxor_v(dest, dest, dest);
        la_b(label_exit);
        la_label(label_shift);
        la_xvreplve0_d(mask, src2);
        la_xvldi(temp, VLDI_IMM_TYPE0(3, 63));
        la_xvsle_du(mask, mask, temp);
        la_xvreplve0_d(temp, src2);
        la_xvsrl_d(dest, src, temp);
        la_xvand_v(dest, dest, mask);
    }
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    la_label(label_exit);
    if (ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest_high = ra_alloc_ftemp();
        IR2_OPND high = ra_alloc_itemp();

        la_xvxor_v(dest_high, dest_high, dest_high);
        la_xvpickve2gr_d(high, dest, 2);
        la_xvinsgr2vr_d(dest_high, high, 0);
        la_xvpickve2gr_d(high, dest, 3);
        la_xvinsgr2vr_d(dest_high, high, 1);
        store_ymm_high128_shadow(dest_high, dest_index);
        ra_free_temp(high);
        ra_free_temp(dest_high);
    } else {
        clear_ymm_high128_shadow(dest_index);
    }
    tr_save_ymm_to_env(UINT16_MAX);
    return true;
}

bool translate_vpsrlx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));

    if (ir1_opcode(pir1) == dt_X86_INS_VPSRLQ) {
        return translate_vpsrlq_lasx(pir1);
    }

    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_INST * ( * rep_inst)(IR2_OPND, IR2_OPND);
    IR2_INST * ( * tr_inst_i)(IR2_OPND, IR2_OPND, int);
    IR2_INST * ( * tr_inst_r)(IR2_OPND, IR2_OPND, IR2_OPND);
    IR2_OPND label_exit = ra_alloc_label();
    IR2_OPND label_shift = ra_alloc_label();
    int max_count;
    switch (ir1_opcode(pir1)) {
        case dt_X86_INS_VPSRLW:
            rep_inst = la_xvreplve0_h;
            tr_inst_i = la_xvsrli_h;
            tr_inst_r = la_xvsrl_h;
            max_count = 15;
            break;
        case dt_X86_INS_VPSRLD:
            rep_inst = la_xvreplve0_w;
            tr_inst_i = la_xvsrli_w;
            tr_inst_r = la_xvsrl_w;
            max_count = 31;
            break;
        default:
            rep_inst = NULL;
            tr_inst_i = NULL;
            tr_inst_r = NULL;
            max_count = 0;
            lsassert(0);
            break;
    }
    if (ir1_opnd_is_imm(opnd2)) {
        uint8_t imm = ir1_opnd_uimm(opnd2);
        if (imm > max_count) {
            la_xvxor_v(dest, dest, dest);
        } else {
            tr_inst_i(dest, src, imm);
        }
    } else {
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND mask = ra_alloc_ftemp();
        IR2_OPND temp = ra_alloc_ftemp();

        IR2_OPND count = ra_alloc_itemp();
        IR2_OPND max = ra_alloc_itemp();
        la_addi_d(max, zero_ir2_opnd, max_count);
        la_vpickve2gr_d(count, src2, 0);
        la_blt(count, max, label_shift);
        la_xvxor_v(dest, dest, dest);
        la_b(label_exit);

        la_label(label_shift);
        if (max_count == 63) {
            la_xvreplve0_d(mask, src2);
            la_xvldi(temp, VLDI_IMM_TYPE0(3, 63));
            la_xvsle_du(mask, mask, temp);
        } else {
            la_xvreplve0_d(mask, src2);
            la_xvslei_du(mask, mask, max_count);
        }
        rep_inst(temp, src2);
        tr_inst_r(dest, src, temp);
        la_xvand_v(dest, dest, mask);
    }
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    la_label(label_exit);
    return true;
}

bool translate_vpsrax(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    IR1_OPND * opnd2 = ir1_get_opnd(pir1, 2);
    lsassert((ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) ||
        (ir1_opnd_is_ymm(opnd0) && ir1_opnd_is_ymm(opnd1)));
    IR2_OPND dest = load_freg256_from_ir1(opnd0);
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_INST * ( * rep_inst)(IR2_OPND, IR2_OPND);
    IR2_INST * ( * tr_inst_i)(IR2_OPND, IR2_OPND, int);
    IR2_INST * ( * tr_inst_r)(IR2_OPND, IR2_OPND, IR2_OPND);
    int max_count;
    switch (ir1_opcode(pir1)) {
        case dt_X86_INS_VPSRAW:
            rep_inst = la_xvreplve0_h;
            tr_inst_i = la_xvsrai_h;
            tr_inst_r = la_xvsra_h;
            max_count = 15;
            break;
        case dt_X86_INS_VPSRAD:
            rep_inst = la_xvreplve0_w;
            tr_inst_i = la_xvsrai_w;
            tr_inst_r = la_xvsra_w;
            max_count = 31;
            break;
        default:
            rep_inst = NULL;
            tr_inst_i = NULL;
            tr_inst_r = NULL;
            max_count = 0;
            lsassert(0);
            break;
    }
    if (ir1_opnd_is_imm(opnd2)) {
        uint8_t imm = ir1_opnd_uimm(opnd2);
        if (imm > max_count)
            imm = max_count;
        tr_inst_i(dest, src, imm);
    } else {
        IR2_OPND src2 = load_freg256_from_ir1(opnd2);
        IR2_OPND mask = ra_alloc_ftemp();
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND temp_sign = ra_alloc_ftemp();
        if (max_count == 63) {
            la_xvreplve0_d(mask, src2);
            la_vldi(temp, VLDI_IMM_TYPE0(3, 63));
            la_xvsle_du(mask, mask, temp);
        } else {
            la_xvreplve0_d(mask, src2);
            la_xvslei_du(mask, mask, max_count);
        }
        tr_inst_i(temp_sign, src, max_count);
        rep_inst(temp, src2);
        tr_inst_r(temp, src, temp);
        la_xvbitsel_v(dest, temp_sign, temp, mask);
    }
    if (ir1_opnd_is_xmm(opnd0)) {
        set_high128_xreg_to_zero(dest);
    }
    return true;
}
#endif
