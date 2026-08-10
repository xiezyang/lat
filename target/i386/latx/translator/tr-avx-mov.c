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
#include "latx-smc.h"

#ifdef CONFIG_LATX_AVX_OPT

static bool translate_vmovaps_lasx(IR1_INST *pir1);

static void vmovaps_check_alignment(IR1_INST *pir1, IR1_OPND *mem,
                                    int alignment)
{
    IR2_OPND address = convert_mem_to_itemp(mem);
    IR2_OPND remainder = ra_alloc_itemp();
    IR2_OPND aligned = ra_alloc_label();

    la_andi(remainder, address, alignment - 1);
    la_beq(remainder, zero_ir2_opnd, aligned);

    IR2_OPND helper = ra_alloc_itemp();
    IR2_OPND eip = ra_alloc_dbt_arg2();
    li_d(eip, ir1_addr(pir1));
    la_store_addrx(eip, env_ir2_opnd, lsenv_offset_of_eip(lsenv));
    tr_save_registers_to_env(0xff, 0xff, option_save_xmm,
                             options_to_save());
#ifdef TARGET_X86_64
    tr_save_x64_8_registers_to_env(0xff, 0xff);
#endif
    if (!option_enable_lasx) {
        tr_save_ymm_to_env(UINT16_MAX);
    }
    aot_load_host_addr(helper, (ADDR)helper_raise_gpf,
                       LOAD_HELPER_RAISE_GPF, 0);
    la_jirl(zero_ir2_opnd, helper, 0);

    la_label(aligned);
    ra_free_temp(helper);
    ra_free_temp(remainder);
    ra_free_temp(address);
}

bool translate_vmovupd(IR1_INST * pir1) {
    translate_vmovaps_lasx(pir1);
    return true;
}

static bool translate_vmovdqa_dqu_lsx(IR1_INST *pir1, bool aligned)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src = ir1_get_opnd(pir1, 1);

    if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_mem(src)) {
        int dest_index = ir1_opnd_base_reg_num(dest);
        IR2_OPND low;
        IR2_OPND high;

        if (aligned) {
            vmovaps_check_alignment(pir1, src, 32);
        }
        load_v256_from_ir1_mem_exact(src, &low, &high);
        la_vori_b(ra_alloc_xmm(dest_index), low, 0);
        store_ymm_high128_shadow(high, dest_index);
    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_ymm(src)) {
        int src_index = ir1_opnd_base_reg_num(src);

        if (aligned) {
            vmovaps_check_alignment(pir1, dest, 32);
        }
        store_v256_to_ir1_mem_exact(
            ra_alloc_xmm(src_index), load_ymm_high128_shadow(src_index),
            dest);
    } else if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_ymm(src)) {
        int dest_index = ir1_opnd_base_reg_num(dest);
        int src_index = ir1_opnd_base_reg_num(src);
        IR2_OPND high = load_ymm_high128_shadow(src_index);

        la_vori_b(ra_alloc_xmm(dest_index), ra_alloc_xmm(src_index), 0);
        store_ymm_high128_shadow(high, dest_index);
    } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_mem(src)) {
        int dest_index = ir1_opnd_base_reg_num(dest);

        if (aligned) {
            vmovaps_check_alignment(pir1, src, 16);
        }
        la_vori_b(ra_alloc_xmm(dest_index),
                  load_v128_from_ir1_mem_exact(src), 0);
        clear_ymm_high128_shadow(dest_index);
    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
        if (aligned) {
            vmovaps_check_alignment(pir1, dest, 16);
        }
        store_v128_to_ir1_mem_exact(
            ra_alloc_xmm(ir1_opnd_base_reg_num(src)), dest);
    } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
        int dest_index = ir1_opnd_base_reg_num(dest);
        IR2_OPND temp = ra_alloc_ftemp();

        la_vori_b(temp, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
        la_vori_b(ra_alloc_xmm(dest_index), temp, 0);
        clear_ymm_high128_shadow(dest_index);
    } else {
#ifdef CONFIG_LATX_TS
        return false;
#endif
        lsassert(0);
    }
    return true;
}

bool translate_vmovdqa_lsx(IR1_INST *pir1)
{
    return translate_vmovdqa_dqu_lsx(pir1, true);
}

bool translate_vmovdqa(IR1_INST * pir1) {
    return translate_vmovaps_lasx(pir1);
}

bool translate_vmovdqu_lsx(IR1_INST *pir1)
{
    return translate_vmovdqa_dqu_lsx(pir1, false);
}

bool translate_vmovdqu(IR1_INST * pir1) {
    return translate_vmovaps_lasx(pir1);
}

bool translate_vmovups_lsx(IR1_INST * pir1) {
    {
        /* LSX-only path */
        IR1_OPND *dest = ir1_get_opnd(pir1, 0);
        IR1_OPND *src = ir1_get_opnd(pir1, 1);

        if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_mem(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND low;
            IR2_OPND high;

            load_v256_from_ir1_mem_exact(src, &low, &high);
            la_vori_b(ra_alloc_xmm(dest_index), low, 0);
            store_ymm_high128_shadow(high, dest_index);
        } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_ymm(src)) {
            int src_index = ir1_opnd_base_reg_num(src);

            store_v256_to_ir1_mem_exact(
                ra_alloc_xmm(src_index),
                load_ymm_high128_shadow(src_index), dest);
        } else if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_ymm(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            int src_index = ir1_opnd_base_reg_num(src);
            IR2_OPND high = load_ymm_high128_shadow(src_index);

            la_vori_b(ra_alloc_xmm(dest_index), ra_alloc_xmm(src_index), 0);
            store_ymm_high128_shadow(high, dest_index);
        } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_mem(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND value = load_v128_from_ir1_mem_exact(src);

            la_vori_b(ra_alloc_xmm(dest_index), value, 0);
            clear_ymm_high128_shadow(dest_index);
        } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
            store_v128_to_ir1_mem_exact(
                ra_alloc_xmm(ir1_opnd_base_reg_num(src)), dest);
        } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND value = ra_alloc_ftemp();

            la_vori_b(value, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
            la_vori_b(ra_alloc_xmm(dest_index), value, 0);
            clear_ymm_high128_shadow(dest_index);
        } else {
#ifdef CONFIG_LATX_TS
            return false;
#endif
            lsassert(0);
        }
    }
    return true;
}

bool translate_vmovupd_lsx(IR1_INST *pir1)
{
    return translate_vmovups_lsx(pir1);
}

bool translate_vmovups(IR1_INST * pir1) {
    return translate_vmovaps_lasx(pir1);
}

bool translate_vmovapd(IR1_INST * pir1) {
    translate_vmovaps_lasx(pir1);
    return true;
}

bool translate_vlddqu(IR1_INST * pir1) {
    translate_vmovaps_lasx(pir1);
    return true;
}

static bool translate_vmovaps_lasx(IR1_INST *pir1)
{
    IR1_OPND * dest = ir1_get_opnd(pir1, 0);
    IR1_OPND * src = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_mem(src)) {
        vmovaps_check_alignment(pir1, src, 32);
        load_freg256_from_ir1_mem(ra_alloc_xmm(ir1_opnd_base_reg_num(dest)),
            src);
    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_ymm(src)) {
        vmovaps_check_alignment(pir1, dest, 32);
        store_freg256_to_ir1_mem(ra_alloc_xmm(ir1_opnd_base_reg_num(src)),
            dest);
    } else if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_ymm(src)) {
        la_xvori_b(ra_alloc_xmm(ir1_opnd_base_reg_num(dest)),
            ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
    } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_mem(src)) {
        IR2_OPND temp = ra_alloc_ftemp();

        vmovaps_check_alignment(pir1, src, 16);
        load_freg128_from_ir1_mem(temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(ra_alloc_xmm(ir1_opnd_base_reg_num(dest)), temp, 0);
    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
        vmovaps_check_alignment(pir1, dest, 16);
        store_freg128_to_ir1_mem(ra_alloc_xmm(ir1_opnd_base_reg_num(src)),
            dest);
    } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
        IR2_OPND temp = ra_alloc_ftemp();

        la_vori_b(temp, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(ra_alloc_xmm(ir1_opnd_base_reg_num(dest)), temp, 0);
    } else {
#ifdef CONFIG_LATX_TS
        return false;
#endif
        lsassert(0);
    }
    return true;
}

bool translate_vmovaps_lsx(IR1_INST *pir1)
{
    {
        /* LSX-only path */
        IR1_OPND *dest = ir1_get_opnd(pir1, 0);
        IR1_OPND *src = ir1_get_opnd(pir1, 1);

        if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_mem(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND address;
            IR2_OPND low = ra_alloc_ftemp();
            IR2_OPND high = ra_alloc_ftemp();

            vmovaps_check_alignment(pir1, src, 32);
            address = convert_mem_to_itemp(src);
            gen_test_page_flag_force(address, 0, PAGE_READ);
            gen_test_page_flag_force(address, 16, PAGE_READ);
            la_vld(low, address, 0);
            la_vld(high, address, 16);
            la_vori_b(ra_alloc_xmm(dest_index), low, 0);
            store_ymm_high128_shadow(high, dest_index);
            ra_free_temp(address);
        } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_ymm(src)) {
            int src_index = ir1_opnd_base_reg_num(src);
            IR2_OPND address;
            IR2_OPND low = ra_alloc_xmm(src_index);
            IR2_OPND high = load_ymm_high128_shadow(src_index);

            vmovaps_check_alignment(pir1, dest, 32);
            address = convert_mem_to_itemp(dest);
            gen_test_page_flag_force(address, 0,
                                     PAGE_WRITE | PAGE_WRITE_ORG);
            gen_test_page_flag_force(address, 16,
                                     PAGE_WRITE | PAGE_WRITE_ORG);
            la_vst(low, address, 0);
            la_vst(high, address, 16);
            ra_free_temp(address);
        } else if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_ymm(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            int src_index = ir1_opnd_base_reg_num(src);
            IR2_OPND high = load_ymm_high128_shadow(src_index);

            la_vori_b(ra_alloc_xmm(dest_index), ra_alloc_xmm(src_index), 0);
            store_ymm_high128_shadow(high, dest_index);
        } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_mem(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND address;
            IR2_OPND value = ra_alloc_ftemp();

            vmovaps_check_alignment(pir1, src, 16);
            address = convert_mem_to_itemp(src);
            gen_test_page_flag_force(address, 0, PAGE_READ);
            la_vld(value, address, 0);
            la_vori_b(ra_alloc_xmm(dest_index), value, 0);
            clear_ymm_high128_shadow(dest_index);
            ra_free_temp(address);
        } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
            IR2_OPND address;

            vmovaps_check_alignment(pir1, dest, 16);
            address = convert_mem_to_itemp(dest);
            gen_test_page_flag_force(address, 0,
                                     PAGE_WRITE | PAGE_WRITE_ORG);
            la_vst(ra_alloc_xmm(ir1_opnd_base_reg_num(src)),
                   address, 0);
            ra_free_temp(address);
        } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND value = ra_alloc_ftemp();

            la_vori_b(value, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
            la_vori_b(ra_alloc_xmm(dest_index), value, 0);
            clear_ymm_high128_shadow(dest_index);
        } else {
#ifdef CONFIG_LATX_TS
            return false;
#endif
            lsassert(0);
        }
    }
    return true;
}

bool translate_vmovapd_lsx(IR1_INST *pir1)
{
    return translate_vmovaps_lsx(pir1);
}

bool translate_vlddqu_lsx(IR1_INST *pir1)
{
    return translate_vmovups_lsx(pir1);
}

bool translate_vmovaps(IR1_INST *pir1)
{
    return translate_vmovaps_lasx(pir1);
}

bool translate_vmovmskps(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_gpr(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) || ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)));

    IR2_OPND dest = ra_alloc_gpr(ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)));
    if (ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) {
        IR2_OPND temp = ra_alloc_ftemp();
        la_vmskltz_w(temp,
            ra_alloc_xmm(ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))));
        la_movfr2gr_d(dest, temp);
    } else {
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_xvmskltz_w(temp1,
            ra_alloc_xmm(ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))));
        la_xvpermi_q(temp2, temp1, VEXTRINS_IMM_4_0(1, 1));
        la_vslli_b(temp2, temp2, 4);
        la_vor_v(temp1, temp1, temp2);
        la_movfr2gr_d(dest, temp1);
    }
    return true;
}

bool translate_vmovmskpd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_gpr(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) || ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)));

    IR2_OPND dest = ra_alloc_gpr(ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 0)));
    if (ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1))) {
        IR2_OPND temp = ra_alloc_ftemp();
        la_vmskltz_d(temp,
            ra_alloc_xmm(ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))));
        la_movfr2gr_d(dest, temp);
    } else {
        IR2_OPND temp1 = ra_alloc_ftemp();
        IR2_OPND temp2 = ra_alloc_ftemp();

        la_xvmskltz_d(temp1,
            ra_alloc_xmm(ir1_opnd_base_reg_num(ir1_get_opnd(pir1, 1))));
        la_xvpermi_q(temp2, temp1, VEXTRINS_IMM_4_0(1, 1));
        la_vslli_b(temp2, temp2, 2);
        la_vor_v(temp1, temp1, temp2);
        la_movfr2gr_d(dest, temp1);
    }
    return true;
}

static bool translate_vmovmsk_lsx(IR1_INST *pir1, bool is_pd)
{
    IR1_OPND *dest_opnd = ir1_get_opnd(pir1, 0);
    IR1_OPND *src_opnd = ir1_get_opnd(pir1, 1);
    int src_index;
    IR2_OPND result = ra_alloc_itemp();
    IR2_OPND low_mask = ra_alloc_ftemp();

    lsassert(ir1_opnd_is_gpr(dest_opnd));
    lsassert(ir1_opnd_is_xmm(src_opnd) || ir1_opnd_is_ymm(src_opnd));

    src_index = ir1_opnd_base_reg_num(src_opnd);
    if (is_pd) {
        la_vmskltz_d(low_mask, ra_alloc_xmm(src_index));
    } else {
        la_vmskltz_w(low_mask, ra_alloc_xmm(src_index));
    }
    la_movfr2gr_d(result, low_mask);

    if (ir1_opnd_is_ymm(src_opnd)) {
        IR2_OPND high_mask = ra_alloc_ftemp();
        IR2_OPND high_bits = ra_alloc_itemp();

        if (is_pd) {
            la_vmskltz_d(high_mask, load_ymm_high128_shadow(src_index));
            la_movfr2gr_d(high_bits, high_mask);
            la_slli_d(high_bits, high_bits, 2);
        } else {
            la_vmskltz_w(high_mask, load_ymm_high128_shadow(src_index));
            la_movfr2gr_d(high_bits, high_mask);
            la_slli_d(high_bits, high_bits, 4);
        }
        la_or(result, result, high_bits);
    }

    store_ireg_to_ir1(result, dest_opnd, false);
    return true;
}

bool translate_vmovmskpd_lsx(IR1_INST *pir1)
{
    return translate_vmovmsk_lsx(pir1, true);
}

bool translate_vmovmskps_lsx(IR1_INST *pir1)
{
    return translate_vmovmsk_lsx(pir1, false);
}

bool translate_vmovntps(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_mem(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) || ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)));

    translate_vmovaps(pir1);
    return true;
}

bool translate_vmovntpd(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_mem(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) || ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)));

    translate_vmovaps(pir1);
    return true;
}

bool translate_vmovntdq(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_mem(ir1_get_opnd(pir1, 0)));
#if (defined CONFIG_LATX_TS) || (defined CONFIG_LATX_TU)
    if (!(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) ||
                ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)))) {
        return false;
    }
#else
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) || ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)));
#endif

    translate_vmovaps(pir1);
    return true;
}

static bool translate_vmovsdup_lsx(IR1_INST *pir1, bool odd)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src = ir1_get_opnd(pir1, 1);
    int dest_index;

    lsassert(ir1_opnd_is_xmm(dest) || ir1_opnd_is_ymm(dest));
    dest_index = ir1_opnd_base_reg_num(dest);

    if (ir1_opnd_is_ymm(dest)) {
        IR2_OPND src_low;
        IR2_OPND src_high;
        IR2_OPND result_low = ra_alloc_ftemp();
        IR2_OPND result_high = ra_alloc_ftemp();

        lsassert(ir1_opnd_is_ymm(src) ||
                 (ir1_opnd_is_mem(src) && ir1_opnd_size(src) == 256));
        if (ir1_opnd_is_mem(src)) {
            load_v256_from_ir1_mem_exact(src, &src_low, &src_high);
        } else {
            int src_index = ir1_opnd_base_reg_num(src);

            src_low = ra_alloc_ftemp();
            la_vori_b(src_low, ra_alloc_xmm(src_index), 0);
            src_high = load_ymm_high128_shadow(src_index);
        }

        if (odd) {
            la_vpackod_w(result_low, src_low, src_low);
            la_vpackod_w(result_high, src_high, src_high);
        } else {
            la_vpackev_w(result_low, src_low, src_low);
            la_vpackev_w(result_high, src_high, src_high);
        }
        la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
        store_ymm_high128_shadow(result_high, dest_index);
    } else {
        IR2_OPND src_value;
        IR2_OPND result = ra_alloc_ftemp();

        lsassert(ir1_opnd_is_xmm(src) ||
                 (ir1_opnd_is_mem(src) && ir1_opnd_size(src) == 128));
        if (ir1_opnd_is_mem(src)) {
            src_value = load_v128_from_ir1_mem_exact(src);
        } else {
            src_value = ra_alloc_ftemp();
            la_vori_b(src_value,
                      ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
        }

        if (odd) {
            la_vpackod_w(result, src_value, src_value);
        } else {
            la_vpackev_w(result, src_value, src_value);
        }
        la_vori_b(ra_alloc_xmm(dest_index), result, 0);
        clear_ymm_high128_shadow(dest_index);
    }
    return true;
}

bool translate_vmovshdup_lsx(IR1_INST *pir1)
{
    return translate_vmovsdup_lsx(pir1, true);
}

bool translate_vmovsldup_lsx(IR1_INST *pir1)
{
    return translate_vmovsdup_lsx(pir1, false);
}

bool translate_vmovshdup(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        la_xvpackod_w(dest, src, src);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvpackod_w(temp, src, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vmovsldup(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
        la_xvpackev_w(dest, src, src);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvpackev_w(temp, src, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}

bool translate_vmovddup_lsx(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)) ||
        ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0)));

    {
        /* LSX-only path */
        IR1_OPND *dest = ir1_get_opnd(pir1, 0);
        IR1_OPND *src = ir1_get_opnd(pir1, 1);
        int dest_index = ir1_opnd_base_reg_num(dest);

        if (ir1_opnd_is_ymm(dest)) {
            IR2_OPND src_low;
            IR2_OPND src_high;

            lsassert(ir1_opnd_is_ymm(src) ||
                     (ir1_opnd_is_mem(src) && ir1_opnd_size(src) == 256));
            if (ir1_opnd_is_mem(src)) {
                load_v256_from_ir1_mem_exact(src, &src_low, &src_high);
            } else {
                int src_index = ir1_opnd_base_reg_num(src);

                src_low = ra_alloc_ftemp();
                la_vori_b(src_low, ra_alloc_xmm(src_index), 0);
                src_high = load_ymm_high128_shadow(src_index);
            }

            la_vreplvei_d(src_low, src_low, 0);
            la_vreplvei_d(src_high, src_high, 0);
            la_vori_b(ra_alloc_xmm(dest_index), src_low, 0);
            store_ymm_high128_shadow(src_high, dest_index);
        } else {
            IR2_OPND result = ra_alloc_ftemp();

            lsassert(ir1_opnd_is_xmm(src) ||
                     (ir1_opnd_is_mem(src) && ir1_opnd_size(src) == 64));
            if (ir1_opnd_is_mem(src)) {
                IR2_OPND value = load_u64_from_ir1_mem_exact(src);

                la_vreplgr2vr_d(result, value);
            } else {
                IR2_OPND src_low = ra_alloc_ftemp();

                la_vori_b(src_low,
                          ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
                la_vreplvei_d(result, src_low, 0);
            }

            la_vori_b(ra_alloc_xmm(dest_index), result, 0);
            clear_ymm_high128_shadow(dest_index);
        }
    }
    return true;
}

bool translate_vmovddup(IR1_INST * pir1) {
    if (ir1_opnd_is_ymm(ir1_get_opnd(pir1, 0))) {
        IR2_OPND dest = load_freg256_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));

        la_xvrepl128vei_d(dest, src, 0);
    } else {
        IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
        IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
        IR2_OPND temp = ra_alloc_ftemp();

        la_xvrepl128vei_d(temp, src, 0);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    }
    return true;
}
#if 0
bool translate_vmovsd_lsx(IR1_INST *pir1)
{
    /*
     * vmovsd xmm1, m64   dest[255:64] = 0
     * vmovsd m64, xmm1
     *   dest[63:0] = src[63:0]
     * vmovsd xmm1, xmm2, xmm3
     *   dest[255:128] = 0
     *   dest[127: 64] = src1[127: 64]
     *   dest[63 : 0 ] = src2[63:0]
     */
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_mem(opnd1)) {
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
        IR2_OPND temp = load_freg_from_ir1_1(opnd1, false, IS_INTEGER);
        la_xvpickve_d(dest, temp, 0);
    } else if (ir1_opnd_is_mem(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        store_freg_to_ir1(src1, opnd0, false, false);
    } else if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
        IR2_OPND src1 = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        if (dest._val == src1._val) {
            la_xvinsve0_d(dest, src2, 0);
            set_high128_xreg_to_zero(dest);
        } else {
            la_xvpickve_d(dest, src2, 0);
            la_vextrins_d(dest, src1, VEXTRINS_IMM_4_0(1, 1));
            set_high128_xreg_to_zero(dest);
        }
        /* dest[255:64] = 0 */
    } else {
        lsassert(0);
    }
    return true;
}
#endif
bool translate_vmovss_lsx(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    {
        /* LSX-only path */
        IR2_OPND src1 = load_freg128_from_ir1(opnd1);

        if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) {
            int dest_index = ir1_opnd_base_reg_num(opnd0);
            IR2_OPND dest = ra_alloc_xmm(dest_index);
            IR2_OPND temp = ra_alloc_ftemp();
            IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));

            la_vori_b(temp, src1, 0);
            la_vextrins_w(temp, src2, VEXTRINS_IMM_4_0(0, 0));
            la_vori_b(dest, temp, 0);
            clear_ymm_high128_shadow(dest_index);
        } else if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_mem(opnd1)) {
            int dest_index = ir1_opnd_base_reg_num(opnd0);
            IR2_OPND dest = ra_alloc_xmm(dest_index);

            la_vxor_v(dest, dest, dest);
            la_vextrins_w(dest, src1, VEXTRINS_IMM_4_0(0, 0));
            clear_ymm_high128_shadow(dest_index);
        } else if (ir1_opnd_is_mem(opnd0) && ir1_opnd_is_xmm(opnd1)) {
            int little_disp;
            IR2_OPND mem_opnd = convert_mem(opnd0, &little_disp);

            la_fst_s(src1, mem_opnd, little_disp);
        } else {
            lsassert(0);
        }
    }
    return true;
}

bool translate_vmovss(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    IR2_OPND src1 = load_freg128_from_ir1(opnd1);

    if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        IR2_OPND temp = ra_alloc_ftemp();
        IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
        la_xvori_b(temp, src1, 0);
        la_xvinsve0_w(temp, src2, 0);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    } else if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_mem(opnd1)) {
        IR2_OPND dest = load_freg128_from_ir1(opnd0);
        la_xvpickve_w(dest, src1, 0);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_mem(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        int little_disp;
        IR2_OPND mem_opnd = convert_mem(opnd0, &little_disp);
        la_fst_s(src1, mem_opnd, little_disp);
    } else {
        lsassert(0);
    }
    return true;
}

bool translate_vmovd_lsx(IR1_INST *pir1)
{
    /*
     * vmovd r/m32, xmm1
     * vmovd xmm1, r/m32
     *   dest[255:32] = 0
     */
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    {
        /* LSX-only path */
        if (ir1_opnd_is_xmm(opnd0) &&
            (ir1_opnd_is_mem(opnd1) || ir1_opnd_is_gpr(opnd1))) {
            int dest_index = ir1_opnd_base_reg_num(opnd0);
            IR2_OPND value = ir1_opnd_is_mem(opnd1) ?
                load_u32_from_ir1_mem_exact(opnd1) :
                load_ireg_from_ir1(opnd1, UNKNOWN_EXTENSION, false);
            IR2_OPND dest = ra_alloc_xmm(dest_index);

            la_vxor_v(dest, dest, dest);
            la_vinsgr2vr_w(dest, value, 0);
            clear_ymm_high128_shadow(dest_index);
        } else if (ir1_opnd_is_mem(opnd0) && ir1_opnd_is_xmm(opnd1)) {
            IR2_OPND value = ra_alloc_itemp();
            IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));

            la_vpickve2gr_wu(value, src, 0);
            store_u32_to_ir1_mem_exact(value, opnd0);
        } else if (ir1_opnd_is_gpr(opnd0) && ir1_opnd_is_xmm(opnd1)) {
            IR2_OPND dest = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd0));
            IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));

            la_vpickve2gr_wu(dest, src, 0);
        } else {
            lsassert(0);
        }
    }
    return true;
}

bool translate_vmovd(IR1_INST *pir1)
{
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_mem(opnd1)) {
        IR2_OPND src = load_u32_from_ir1_mem_exact(opnd1);
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));

        la_xvandi_b(dest, dest, 0);
        la_xvinsgr2vr_w(dest, src, 0);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_mem(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        IR2_OPND value = ra_alloc_itemp();

        la_vpickve2gr_wu(value, src, 0);
        store_u32_to_ir1_mem_exact(value, opnd0);
    } else if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_gpr(opnd1)) {
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
        IR2_OPND src = load_ireg_from_ir1(opnd1, UNKNOWN_EXTENSION, false);
        la_xvandi_b(dest, dest, 0);
        la_xvinsgr2vr_w(dest, src, 0);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_gpr(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND dest = load_ireg_from_ir1(opnd0, UNKNOWN_EXTENSION, false);
        IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        la_vpickve2gr_wu(dest, src, 0);
    } else {
        lsassert(0);
    }
    return true;
}
#if 0
bool translate_vmovq(IR1_INST *pir1)
{
    /*
     * vmovd r/m64, xmm1
     * vmovd xmm1, r/m64
     * dest[255:64] = 0
     */
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);

    if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_mem(opnd1)) {
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
        IR2_OPND src = load_freg_from_ir1_1(opnd1, false, IS_INTEGER);
        la_xvandi_b(dest, dest, 0);
        la_xvpickve_d(dest, src, 0);
    } else if (ir1_opnd_is_mem(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        store_freg_to_ir1(src, opnd0, false, false);
    } else if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_gpr(opnd1)) {
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
        IR2_OPND src = load_ireg_from_ir1(opnd1, UNKNOWN_EXTENSION, false);
        la_xvandi_b(dest, dest, 0);
        la_vinsgr2vr_d(dest, src, 0);
    } else if (ir1_opnd_is_gpr(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND dest = load_ireg_from_ir1(opnd0, UNKNOWN_EXTENSION, false);
        IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        la_vpickve2gr_du(dest, src, 0);
    } else if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
        IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        la_xvandi_b(dest, dest, 0);
        la_vextrins_d(dest, src, VEXTRINS_IMM_4_0(0, 0));
        set_high128_xreg_to_zero(dest);
    } else {
        lsassert(0);
    }

    return true;
}
#endif

bool translate_vpmovmskb(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND * opnd1 = ir1_get_opnd(pir1, 1);
    lsassert(ir1_opnd_is_gpr(opnd0));
    lsassert(ir1_opnd_is_xmm(opnd1) || ir1_opnd_is_ymm(opnd1));
    IR2_OPND dest = ra_alloc_gpr(ir1_opnd_base_reg_num(opnd0));
    IR2_OPND src = load_freg256_from_ir1(opnd1);
    IR2_OPND ftemp = ra_alloc_ftemp();
    la_xvmskltz_b(ftemp, src);
    if (ir1_opnd_is_ymm(opnd1)) {
        la_xvpermi_d(ftemp, ftemp, 0x8);
        la_vextrins_h(ftemp, ftemp, 0x14);
    }
    la_vpickve2gr_d(dest, ftemp, 0);
    return true;
}

bool translate_vmaskmovpx(IR1_INST * pir1) {
    IR1_OPND * opnd0 = ir1_get_opnd(pir1, 0);
    IR2_OPND src1 = load_freg256_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg256_from_ir1(ir1_get_opnd(pir1, 2));
    IR2_OPND mask = ra_alloc_ftemp();
    IR2_OPND temp;
    IR2_INST * ( * tr_slt)(IR2_OPND, IR2_OPND, int);
    IR1_OPCODE op = ir1_opcode(pir1);
    switch (op) {
        case dt_X86_INS_VMASKMOVPS:
            tr_slt = la_xvslti_w;
            break;
        case dt_X86_INS_VMASKMOVPD:
            tr_slt = la_xvslti_d;
            break;
        default:
            tr_slt = NULL;
            lsassert(0);
            break;
    }
    if (ir1_opnd_is_xmm(opnd0) || ir1_opnd_is_ymm(opnd0)) {
        IR2_OPND dest = load_freg256_from_ir1(opnd0);
        temp = ra_alloc_ftemp();
        la_xvandi_b(temp, temp, 0);
        tr_slt(mask, src1, 0);
        la_xvbitsel_v(temp, temp, src2, mask);
        if (ir1_opnd_is_xmm(opnd0))
            set_high128_xreg_to_zero(temp);
        la_xvori_b(dest, temp, 0);
    } else if (ir1_opnd_is_mem(opnd0)) {
        temp = load_freg256_from_ir1(opnd0);
        tr_slt(mask, src1, 0);
        la_xvbitsel_v(temp, temp, src2, mask);
        if (ir1_opnd_size(opnd0) == 128)
            store_freg128_to_ir1_mem(temp, opnd0);
        else if (ir1_opnd_size(opnd0) == 256)
            store_freg256_to_ir1_mem(temp, opnd0);
        else {
            lsassert(0);
        }
    } else {
        lsassert(0);
    }
    return true;
}

bool translate_vmaskmovpx_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *mask_opnd = ir1_get_opnd(pir1, 1);
    IR1_OPND *value_opnd = ir1_get_opnd(pir1, 2);
    IR2_INST *(*tr_slt)(IR2_OPND, IR2_OPND, int);
    bool is_ymm = ir1_opnd_is_ymm(dest) ||
                  (ir1_opnd_is_mem(dest) && ir1_opnd_size(dest) == 256);

    lsassert(ir1_opnd_is_xmm(mask_opnd) || ir1_opnd_is_ymm(mask_opnd));
    lsassert(ir1_opnd_is_xmm(value_opnd) || ir1_opnd_is_ymm(value_opnd) ||
             ir1_opnd_is_mem(value_opnd));
    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_VMASKMOVPS:
        tr_slt = la_vslti_w;
        break;
    case dt_X86_INS_VMASKMOVPD:
        tr_slt = la_vslti_d;
        break;
    default:
        lsassert(0);
        return false;
    }

    if (is_ymm) {
        int mask_index = ir1_opnd_base_reg_num(mask_opnd);
        IR2_OPND mask_low = ra_alloc_ftemp();
        IR2_OPND mask_high = load_ymm_high128_shadow(mask_index);
        IR2_OPND value_low;
        IR2_OPND value_high;

        lsassert(ir1_opnd_is_ymm(mask_opnd));
        la_vori_b(mask_low, ra_alloc_xmm(mask_index), 0);
        if (ir1_opnd_is_mem(value_opnd)) {
            load_v256_from_ir1_mem_exact(value_opnd, &value_low, &value_high);
        } else {
            int value_index = ir1_opnd_base_reg_num(value_opnd);

            lsassert(ir1_opnd_is_ymm(value_opnd));
            value_low = ra_alloc_ftemp();
            la_vori_b(value_low, ra_alloc_xmm(value_index), 0);
            value_high = load_ymm_high128_shadow(value_index);
        }

        tr_slt(mask_low, mask_low, 0);
        tr_slt(mask_high, mask_high, 0);
        if (ir1_opnd_is_ymm(dest)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND result_low = ra_alloc_ftemp();
            IR2_OPND result_high = ra_alloc_ftemp();

            la_vxor_v(result_low, result_low, result_low);
            la_vxor_v(result_high, result_high, result_high);
            la_vbitsel_v(result_low, result_low, value_low, mask_low);
            la_vbitsel_v(result_high, result_high, value_high, mask_high);
            la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
            store_ymm_high128_shadow(result_high, dest_index);
        } else {
            IR2_OPND old_low;
            IR2_OPND old_high;

            lsassert(ir1_opnd_is_mem(dest));
            load_v256_from_ir1_mem_exact(dest, &old_low, &old_high);
            la_vbitsel_v(old_low, old_low, value_low, mask_low);
            la_vbitsel_v(old_high, old_high, value_high, mask_high);
            store_v256_to_ir1_mem_exact(old_low, old_high, dest);
        }
    } else {
        int mask_index = ir1_opnd_base_reg_num(mask_opnd);
        IR2_OPND mask = ra_alloc_ftemp();
        IR2_OPND value;

        lsassert(ir1_opnd_is_xmm(mask_opnd));
        la_vori_b(mask, ra_alloc_xmm(mask_index), 0);
        if (ir1_opnd_is_mem(value_opnd)) {
            value = load_v128_from_ir1_mem_exact(value_opnd);
        } else {
            int value_index = ir1_opnd_base_reg_num(value_opnd);

            lsassert(ir1_opnd_is_xmm(value_opnd));
            value = ra_alloc_ftemp();
            la_vori_b(value, ra_alloc_xmm(value_index), 0);
        }

        tr_slt(mask, mask, 0);
        if (ir1_opnd_is_xmm(dest)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND result = ra_alloc_ftemp();

            la_vxor_v(result, result, result);
            la_vbitsel_v(result, result, value, mask);
            la_vori_b(ra_alloc_xmm(dest_index), result, 0);
            clear_ymm_high128_shadow(dest_index);
        } else {
            IR2_OPND old;

            lsassert(ir1_opnd_is_mem(dest));
            old = load_v128_from_ir1_mem_exact(dest);
            la_vbitsel_v(old, old, value, mask);
            store_v128_to_ir1_mem_exact(old, dest);
        }
    }
    return true;
}

bool translate_vmovq_lsx(IR1_INST * pir1) {
    {
        /* LSX-only path */
        IR1_OPND *dest = ir1_get_opnd(pir1, 0);
        IR1_OPND *src = ir1_get_opnd(pir1, 1);

        if (ir1_opnd_is_xmm(dest) &&
            (ir1_opnd_is_gpr(src) || ir1_opnd_is_xmm(src) ||
             ir1_opnd_is_mem(src))) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND value;

            if (ir1_opnd_is_xmm(src)) {
                value = ra_alloc_itemp();
                la_vpickve2gr_du(
                    value, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
            } else if (ir1_opnd_is_mem(src)) {
                value = load_u64_from_ir1_mem_exact(src);
            } else {
                value = load_ireg_from_ir1(src, UNKNOWN_EXTENSION, false);
            }

            IR2_OPND dest_reg = ra_alloc_xmm(dest_index);
            la_vxor_v(dest_reg, dest_reg, dest_reg);
            la_vinsgr2vr_d(dest_reg, value, 0);
            clear_ymm_high128_shadow(dest_index);
        } else if (ir1_opnd_is_gpr(dest) && ir1_opnd_is_xmm(src)) {
            IR2_OPND dest_reg = ra_alloc_gpr(ir1_opnd_base_reg_num(dest));
            IR2_OPND src_reg = ra_alloc_xmm(ir1_opnd_base_reg_num(src));

            la_vpickve2gr_du(dest_reg, src_reg, 0);
        } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
            IR2_OPND value = ra_alloc_itemp();

            la_vpickve2gr_du(
                value, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
            store_u64_to_ir1_mem_exact(value, dest);
        } else {
#ifdef CONFIG_LATX_TS
            return false;
#endif
            lsassert(0);
        }
    }
    return true;
}

bool translate_vmovq(IR1_INST * pir1) {
    IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
    IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_gpr(opnd0) || ir1_opnd_is_gpr(opnd1)) {
        if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_gpr(opnd1)) {
            IR2_OPND dest = load_freg128_from_ir1(opnd0);
            IR2_OPND src = load_ireg_from_ir1(opnd1, UNKNOWN_EXTENSION, false);
            la_xvandi_b(dest, dest, 0x0);
            la_xvinsgr2vr_d(dest, src, 0x0);
        } else if (ir1_opnd_is_gpr(opnd0) && ir1_opnd_is_xmm(opnd1)) {
            IR2_OPND dest = load_ireg_from_ir1(opnd0, UNKNOWN_EXTENSION, false);
            IR2_OPND src = load_freg128_from_ir1(opnd1);
            la_vpickve2gr_du(dest, src, 0x0);
        }
    } else if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
        IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        IR2_OPND value = ra_alloc_ftemp();

        la_vori_b(value, src, 0);
        la_xvandi_b(dest, dest, 0x0);
        la_xvpickve_d(dest, value, 0);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_xmm(opnd0) && ir1_opnd_is_mem(opnd1)) {
        IR2_OPND dest = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd0));
        IR2_OPND value = load_u64_from_ir1_mem_exact(opnd1);

        la_xvandi_b(dest, dest, 0x0);
        la_xvinsgr2vr_d(dest, value, 0x0);
        set_high128_xreg_to_zero(dest);
    } else if (ir1_opnd_is_mem(opnd0) && ir1_opnd_is_xmm(opnd1)) {
        IR2_OPND src = ra_alloc_xmm(ir1_opnd_base_reg_num(opnd1));
        IR2_OPND value = ra_alloc_itemp();

        la_vpickve2gr_du(value, src, 0);
        store_u64_to_ir1_mem_exact(value, opnd0);
    } else {
        lsassert(0);
    }
    return true;
}

bool translate_vmovlps(IR1_INST * pir1) {
    translate_vmovlpd(pir1);
    return true;
}

static bool translate_vmovhpd_lpd_lsx(IR1_INST *pir1, bool high_lane)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src = ir1_get_opnd(pir1, 1);

    if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
        IR1_OPND *memory = ir1_get_opnd(pir1, 2);
        IR2_OPND value = load_u64_from_ir1_mem_exact(memory);
        IR2_OPND dest_reg = ra_alloc_xmm(ir1_opnd_base_reg_num(dest));
        IR2_OPND src_reg = ra_alloc_xmm(ir1_opnd_base_reg_num(src));

        la_vori_b(dest_reg, src_reg, 0);
        la_vinsgr2vr_d(dest_reg, value, high_lane ? 1 : 0);
        clear_ymm_high128_shadow(ir1_opnd_base_reg_num(dest));
        return true;
    }

    if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
        IR2_OPND value = ra_alloc_itemp();
        la_vpickve2gr_du(value, ra_alloc_xmm(ir1_opnd_base_reg_num(src)),
                         high_lane ? 1 : 0);
        store_u64_to_ir1_mem_exact(value, dest);
        return true;
    }

#ifdef CONFIG_LATX_TS
    return false;
#else
    lsassert(0);
    return true;
#endif
}

bool translate_vmovhpd_lsx(IR1_INST *pir1)
{
    return translate_vmovhpd_lpd_lsx(pir1, true);
}

bool translate_vmovlpd_lsx(IR1_INST *pir1)
{
    return translate_vmovhpd_lpd_lsx(pir1, false);
}

bool translate_vmovhps_lsx(IR1_INST *pir1)
{
    return translate_vmovhpd_lpd_lsx(pir1, true);
}

bool translate_vmovlps_lsx(IR1_INST *pir1)
{
    return translate_vmovhpd_lpd_lsx(pir1, false);
}

bool translate_vmovlpd(IR1_INST * pir1) {
    IR1_OPND * dest = ir1_get_opnd(pir1, 0);
    IR1_OPND * src = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
        IR1_OPND * src2 = ir1_get_opnd(pir1, 2);
        IR2_OPND temp_src2 = load_freg128_from_ir1(src2);
        IR2_OPND dest_temp = ra_alloc_xmm(ir1_opnd_base_reg_num(dest));
        IR2_OPND src_temp = ra_alloc_xmm(ir1_opnd_base_reg_num(src));
        la_vreplvei_d(dest_temp, src_temp, 1);
        la_xvinsve0_d(dest_temp, temp_src2, 0);
        set_high128_xreg_to_zero(dest_temp);
        return true;
    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
        IR2_OPND temp = ra_alloc_itemp();
        la_vpickve2gr_du(temp, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
        store_ireg_to_ir1(temp, dest, false);
        return true;
    }
    return true;

}

static void emit_vmovhlps_lsx_lane(IR2_OPND dest, IR2_OPND src1,
                                   IR2_OPND src2, bool high_to_low)
{
    if (high_to_low) {
        /* HLPS: src2.high -> dest.low, src1.high -> dest.high. */
        la_vilvh_d(dest, src1, src2);
    } else {
        /* LHPS: src1.low -> dest.low, src2.low -> dest.high. */
        la_vpickev_d(dest, src2, src1);
    }
}

static bool translate_vmovhlps_lhps_lsx(IR1_INST *pir1, bool high_to_low)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src1 = ir1_get_opnd(pir1, 1);
    IR1_OPND *src2 = ir1_get_opnd(pir1, 2);
    bool is_ymm = ir1_opnd_is_ymm(dest);
    int dest_index;
    IR2_OPND src1_low;
    IR2_OPND src2_low;
    IR2_OPND low_result;

    /* These instructions have register-only, same-width operands. */
    lsassert(ir1_opnd_is_xmm(dest) || is_ymm);
    lsassert(is_ymm ? ir1_opnd_is_ymm(src1) : ir1_opnd_is_xmm(src1));
    lsassert(is_ymm ? ir1_opnd_is_ymm(src2) : ir1_opnd_is_xmm(src2));

    dest_index = ir1_opnd_base_reg_num(dest);
    src1_low = ra_alloc_xmm(ir1_opnd_base_reg_num(src1));
    src2_low = ra_alloc_xmm(ir1_opnd_base_reg_num(src2));
    low_result = ra_alloc_ftemp();
    emit_vmovhlps_lsx_lane(low_result, src1_low, src2_low, high_to_low);

    if (is_ymm) {
        IR2_OPND src1_high = load_ymm_high128_shadow(
            ir1_opnd_base_reg_num(src1));
        IR2_OPND src2_high = load_ymm_high128_shadow(
            ir1_opnd_base_reg_num(src2));
        IR2_OPND high_result = ra_alloc_ftemp();

        emit_vmovhlps_lsx_lane(high_result, src1_high, src2_high,
                               high_to_low);
        la_vori_b(ra_alloc_xmm(dest_index), low_result, 0);
        store_ymm_high128_shadow(high_result, dest_index);
    } else {
        la_vori_b(ra_alloc_xmm(dest_index), low_result, 0);
        clear_ymm_high128_shadow(dest_index);
    }
    return true;
}

bool translate_vmovhlps_lsx(IR1_INST *pir1)
{
    return translate_vmovhlps_lhps_lsx(pir1, true);
}

bool translate_vmovlhps_lsx(IR1_INST *pir1)
{
    return translate_vmovhlps_lhps_lsx(pir1, false);
}

bool translate_vmovhlps(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 2)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));
    la_vilvh_d(dest, src1, src2);
    set_high128_xreg_to_zero(dest);
    return true;
}

bool translate_vmovhpd(IR1_INST * pir1) {
    IR1_OPND * dest = ir1_get_opnd(pir1, 0);
    IR1_OPND * src = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
        IR1_OPND * src2 = ir1_get_opnd(pir1, 2);
        IR2_OPND temp_src2 = load_freg128_from_ir1(src2);
        IR2_OPND dest_temp = ra_alloc_xmm(ir1_opnd_base_reg_num(dest));
        IR2_OPND src_temp = ra_alloc_xmm(ir1_opnd_base_reg_num(src));
        la_vilvl_d(dest_temp, temp_src2, src_temp);
        set_high128_xreg_to_zero(dest_temp);

    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
        IR2_OPND temp = ra_alloc_itemp();
        la_vpickve2gr_du(temp,ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 1);
        store_ireg_to_ir1(temp, dest, false);
    }
    return true;
}

bool translate_vmovhps(IR1_INST * pir1) {
    translate_vmovhpd(pir1);
    return true;
}


bool translate_vmovlhps(IR1_INST * pir1) {
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 2)));
    IR2_OPND dest = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND src1 = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND src2 = load_freg128_from_ir1(ir1_get_opnd(pir1, 2));

    la_vpickev_d(dest, src2, src1);
    set_high128_xreg_to_zero(dest);
    return true;
}


bool translate_vmovntdqa(IR1_INST * pir1) {
    IR1_OPND * dest = ir1_get_opnd(pir1, 0);
    IR1_OPND * src = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_mem(src)) {
        IR2_OPND temp = load_freg128_from_ir1(src);
        IR2_OPND temp_dest = ra_alloc_xmm(ir1_opnd_base_reg_num(dest));
        la_vand_v(temp_dest, temp, temp);
        set_high128_xreg_to_zero(temp_dest);

    } else if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_mem(src)) {
        IR2_OPND temp = load_freg256_from_ir1(src);
        la_xvand_v(ra_alloc_xmm(ir1_opnd_base_reg_num(dest)),
            temp, temp);
    }
    return true;
}

bool translate_vmovntdqa_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src = ir1_get_opnd(pir1, 1);
    int dest_index = ir1_opnd_base_reg_num(dest);

    lsassert(ir1_opnd_is_mem(src));
    if (ir1_opnd_is_xmm(dest)) {
        vmovaps_check_alignment(pir1, src, 16);
        IR2_OPND value = load_v128_from_ir1_mem_exact(src);

        la_vori_b(ra_alloc_xmm(dest_index), value, 0);
        clear_ymm_high128_shadow(dest_index);
    } else if (ir1_opnd_is_ymm(dest)) {
        IR2_OPND low;
        IR2_OPND high;

        vmovaps_check_alignment(pir1, src, 32);
        load_v256_from_ir1_mem_exact(src, &low, &high);
        la_vori_b(ra_alloc_xmm(dest_index), low, 0);
        store_ymm_high128_shadow(high, dest_index);
    } else {
        lsassert(0);
    }
    return true;
}


bool translate_vmovsd_lsx(IR1_INST *pir1)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src = ir1_get_opnd(pir1, 1);

    {
        /* LSX-only path */
        if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
            IR1_OPND *src2 = ir1_get_opnd(pir1, 2);
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND low;
            IR2_OPND high = ra_alloc_itemp();
            IR2_OPND result = ra_alloc_ftemp();

            lsassert(ir1_opnd_is_xmm(src2));
            low = ra_alloc_itemp();
            la_vpickve2gr_du(
                low, ra_alloc_xmm(ir1_opnd_base_reg_num(src2)), 0);
            la_vpickve2gr_du(
                high, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 1);
            la_vxor_v(result, result, result);
            la_vinsgr2vr_d(result, low, 0);
            la_vinsgr2vr_d(result, high, 1);
            la_vori_b(ra_alloc_xmm(dest_index), result, 0);
            clear_ymm_high128_shadow(dest_index);
        } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_mem(src)) {
            int dest_index = ir1_opnd_base_reg_num(dest);
            IR2_OPND low = load_u64_from_ir1_mem_exact(src);
            IR2_OPND result = ra_alloc_ftemp();

            la_vxor_v(result, result, result);
            la_vinsgr2vr_d(result, low, 0);
            la_vori_b(ra_alloc_xmm(dest_index), result, 0);
            clear_ymm_high128_shadow(dest_index);
        } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
            IR2_OPND low = ra_alloc_itemp();

            la_vpickve2gr_du(
                low, ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
            store_u64_to_ir1_mem_exact(low, dest);
        } else {
            lsassert(0);
        }
    }
    return true;
}

bool translate_vmovsd(IR1_INST *pir1)
{
    IR1_OPND *dest = ir1_get_opnd(pir1, 0);
    IR1_OPND *src = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_xmm(src)) {
        IR1_OPND *src2 = ir1_get_opnd(pir1, 2);
        int dest_index = ir1_opnd_base_reg_num(dest);
        IR2_OPND temp_dest = ra_alloc_xmm(dest_index);
        IR2_OPND temp_src1 = load_freg128_from_ir1(src);
        IR2_OPND temp_src2 = load_freg128_from_ir1(src2);
        IR2_OPND temp = ra_alloc_ftemp();
        la_xvori_b(temp, temp_src1, 0x0);
        la_vshuf4i_d(temp, temp_src2, 0b00000110);
        la_xvori_b(temp_dest, temp, 0x0);
        set_high128_xreg_to_zero(temp_dest);
        clear_ymm_high128_shadow(dest_index);
    } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_mem(src)) {
        int dest_index = ir1_opnd_base_reg_num(dest);
        IR2_OPND value = load_u64_from_ir1_mem_exact(src);
        IR2_OPND dest_reg = ra_alloc_xmm(dest_index);

        la_xvandi_b(dest_reg, dest_reg, 0);
        la_xvinsgr2vr_d(dest_reg, value, 0);
        set_high128_xreg_to_zero(dest_reg);
        clear_ymm_high128_shadow(dest_index);
    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
        IR2_OPND value = ra_alloc_itemp();

        la_vpickve2gr_du(value,
                         ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
        store_u64_to_ir1_mem_exact(value, dest);
    }
    return true;
}
#endif
