/* LSX fallback implementation split from tr-simd-mov.c. */
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


bool translate_maskmovdqu_lsx(IR1_INST *pir1)
{
    IR2_OPND src = load_freg128_from_ir1(ir1_get_opnd(pir1, 0));
    IR2_OPND mask = load_freg128_from_ir1(ir1_get_opnd(pir1, 1));
    IR2_OPND zero = ra_alloc_ftemp();
    IR2_OPND base_opnd = ra_alloc_gpr(edi_index);
    IR2_OPND selected_mask = ra_alloc_ftemp();
    IR2_OPND unselected_mask = ra_alloc_ftemp();
    IR2_OPND mem_data;
    IR2_OPND selected_data = ra_alloc_ftemp();

    la_vxor_v(zero, zero, zero);
    la_vandi_b(selected_mask, mask, 0x80);
    la_vseq_b(unselected_mask, selected_mask, zero);
    la_vnor_v(selected_mask, unselected_mask, zero);
#ifndef TARGET_X86_64
    la_bstrpick_d(base_opnd, base_opnd, 31, 0);
#else
    if (!CODEIS64) {
        la_bstrpick_d(base_opnd, base_opnd, 31, 0);
    }
#endif
    mem_data = load_v128_from_guest_addr_exact(base_opnd);
    la_vand_v(selected_data, src, selected_mask);
    la_vand_v(mem_data, mem_data, unselected_mask);
    la_vor_v(mem_data, mem_data, selected_data);
    store_v128_to_guest_addr_exact(mem_data, base_opnd);
    return true;
}
