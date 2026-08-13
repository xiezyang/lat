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

static bool translate_vmovaps_lasx(IR1_INST *pir1, bool aligned);

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
    if (!option_enable_lasx) {
        return translate_vmovupd_lsx(pir1);
    }

    return translate_vmovaps_lasx(pir1, false);
}

bool translate_vmovdqa(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovdqa_lsx(pir1);
    }

    return translate_vmovaps_lasx(pir1, true);
}

bool translate_vmovdqu(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovdqu_lsx(pir1);
    }

    return translate_vmovaps_lasx(pir1, false);
}

bool translate_vmovups(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovups_lsx(pir1);
    }

    return translate_vmovaps_lasx(pir1, false);
}

bool translate_vmovapd(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovapd_lsx(pir1);
    }

    return translate_vmovaps_lasx(pir1, true);
}

bool translate_vlddqu(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vlddqu_lsx(pir1);
    }

    return translate_vmovaps_lasx(pir1, false);
}

static bool translate_vmovaps_lasx(IR1_INST *pir1, bool aligned)
{
    IR1_OPND * dest = ir1_get_opnd(pir1, 0);
    IR1_OPND * src = ir1_get_opnd(pir1, 1);
    if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_mem(src)) {
        if (aligned) {
            vmovaps_check_alignment(pir1, src, 32);
        }
        load_freg256_from_ir1_mem(ra_alloc_xmm(ir1_opnd_base_reg_num(dest)),
            src);
    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_ymm(src)) {
        if (aligned) {
            vmovaps_check_alignment(pir1, dest, 32);
        }
        store_freg256_to_ir1_mem(ra_alloc_xmm(ir1_opnd_base_reg_num(src)),
            dest);
    } else if (ir1_opnd_is_ymm(dest) && ir1_opnd_is_ymm(src)) {
        la_xvori_b(ra_alloc_xmm(ir1_opnd_base_reg_num(dest)),
            ra_alloc_xmm(ir1_opnd_base_reg_num(src)), 0);
    } else if (ir1_opnd_is_xmm(dest) && ir1_opnd_is_mem(src)) {
        IR2_OPND temp = ra_alloc_ftemp();

        if (aligned) {
            vmovaps_check_alignment(pir1, src, 16);
        }
        load_freg128_from_ir1_mem(temp, src);
        set_high128_xreg_to_zero(temp);
        la_xvori_b(ra_alloc_xmm(ir1_opnd_base_reg_num(dest)), temp, 0);
    } else if (ir1_opnd_is_mem(dest) && ir1_opnd_is_xmm(src)) {
        if (aligned) {
            vmovaps_check_alignment(pir1, dest, 16);
        }
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

bool translate_vmovaps(IR1_INST *pir1)
{
    if (!option_enable_lasx) {
        return translate_vmovaps_lsx(pir1);
    }

    return translate_vmovaps_lasx(pir1, true);
}

bool translate_vmovmskps(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovmskps_lsx(pir1);
    }

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
    if (!option_enable_lasx) {
        return translate_vmovmskpd_lsx(pir1);
    }

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

bool translate_vmovntps(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovntps_lsx(pir1);
    }

    lsassert(ir1_opnd_is_mem(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) || ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)));

    translate_vmovaps(pir1);
    return true;
}

bool translate_vmovntpd(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovntpd_lsx(pir1);
    }

    lsassert(ir1_opnd_is_mem(ir1_get_opnd(pir1, 0)));
    lsassert(ir1_opnd_is_xmm(ir1_get_opnd(pir1, 1)) || ir1_opnd_is_ymm(ir1_get_opnd(pir1, 1)));

    translate_vmovaps(pir1);
    return true;
}

bool translate_vmovntdq(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovntdq_lsx(pir1);
    }

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

bool translate_vmovshdup(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovshdup_lsx(pir1);
    }

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
    if (!option_enable_lasx) {
        return translate_vmovsldup_lsx(pir1);
    }

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

bool translate_vmovddup(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovddup_lsx(pir1);
    }

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

bool translate_vmovss(IR1_INST *pir1)
{
    if (!option_enable_lasx) {
        return translate_vmovss_lsx(pir1);
    }

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

bool translate_vmovd(IR1_INST *pir1)
{
    if (!option_enable_lasx) {
        return translate_vmovd_lsx(pir1);
    }

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
bool translate_vpmovmskb(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vpmovmskb_lsx(pir1);
    }

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
    if (!option_enable_lasx) {
        return translate_vmaskmovpx_lsx(pir1);
    }

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

bool translate_vmovq(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovq_lsx(pir1);
    }

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
    if (!option_enable_lasx) {
        return translate_vmovlps_lsx(pir1);
    }

    translate_vmovlpd(pir1);
    return true;
}

bool translate_vmovlpd(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovlpd_lsx(pir1);
    }

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

bool translate_vmovhlps(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovhlps_lsx(pir1);
    }

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
    if (!option_enable_lasx) {
        return translate_vmovhpd_lsx(pir1);
    }

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
    if (!option_enable_lasx) {
        return translate_vmovhps_lsx(pir1);
    }

    translate_vmovhpd(pir1);
    return true;
}

bool translate_vmovlhps(IR1_INST * pir1) {
    if (!option_enable_lasx) {
        return translate_vmovlhps_lsx(pir1);
    }

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
    if (!option_enable_lasx) {
        return translate_vmovntdqa_lsx(pir1);
    }

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

bool translate_vmovsd(IR1_INST *pir1)
{
    if (!option_enable_lasx) {
        return translate_vmovsd_lsx(pir1);
    }

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
