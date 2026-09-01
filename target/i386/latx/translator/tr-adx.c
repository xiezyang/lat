/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "env.h"
#include "reg-alloc.h"
#include "latx-options.h"
#include "flag-lbt.h"
#include "translate.h"

static void latx_adx_write_overflow_bit(IR2_OPND flags, IR2_OPND bit, int mask)
{
    IR2_OPND shifted = ra_alloc_itemp();

    if (mask == OF_USEDEF_BIT) {
        la_slli_w(shifted, bit, 11);
        la_bstrins_d(flags, shifted, 11, 11);
    } else {
        la_bstrins_d(flags, bit, 0, 0);
    }
    la_x86mtflag(flags, mask);
    ra_free_temp(shifted);
}

static void latx_adx_compute_carry(IR2_OPND carry, IR2_OPND lhs, IR2_OPND rhs,
                                   IR2_OPND cin, int size)
{
    IR2_OPND partial = ra_alloc_itemp();
    IR2_OPND carry0 = ra_alloc_itemp();
    IR2_OPND carry1 = ra_alloc_itemp();
    IR2_OPND lhs_cmp = lhs;
    IR2_OPND partial_cmp = partial;
    IR2_OPND mask = ir2_opnd_new_none();

    if (size == 32) {
        lhs_cmp = ra_alloc_itemp();
        partial_cmp = ra_alloc_itemp();
        mask = ra_alloc_itemp();
        li_wu(mask, UINT32_MAX);
        la_and(lhs_cmp, lhs, mask);
    }

    la_add_d(partial, lhs, rhs);
    if (size == 32) {
        la_and(partial_cmp, partial, mask);
    }
    la_sltu(carry0, partial_cmp, lhs_cmp);
    la_add_d(partial, partial, cin);
    if (size == 32) {
        la_and(partial_cmp, partial, mask);
    }
    la_sltu(carry1, partial_cmp, cin);
    la_or(carry, carry0, carry1);

    if (size == 32) {
        ra_free_temp(mask);
        ra_free_temp(partial_cmp);
        ra_free_temp(lhs_cmp);
    }
    ra_free_temp(carry1);
    ra_free_temp(carry0);
    ra_free_temp(partial);
}

bool translate_adcx(IR1_INST *pir1)
{
    IR2_OPND src_opnd = load_ireg_from_ir1(ir1_get_opnd(pir1, 1), ZERO_EXTENSION, false);
    IR2_OPND dest_opnd = load_ireg_from_ir1(ir1_get_opnd(pir1, 0), ZERO_EXTENSION, false);
    IR2_OPND flag_opnd = ra_alloc_itemp();
    IR2_OPND cflag_opnd = ra_alloc_itemp();
    IR2_OPND temp1_opnd = ra_alloc_itemp();
    int size = ir1_opnd_size(ir1_get_opnd(pir1, 0));

    la_x86mfflag(flag_opnd, 0x3f);
    la_bstrpick_d(cflag_opnd, flag_opnd, 0, 0);

    if (!option_enable_lbt) {
        latx_adx_compute_carry(cflag_opnd, dest_opnd, src_opnd, cflag_opnd, size);
        if (size == 32) {
            la_add_w(temp1_opnd, dest_opnd, src_opnd);
            la_add_w(temp1_opnd, temp1_opnd, cflag_opnd);
        } else {
            la_add_d(temp1_opnd, dest_opnd, src_opnd);
            la_add_d(temp1_opnd, temp1_opnd, cflag_opnd);
        }
        latx_adx_write_overflow_bit(flag_opnd, cflag_opnd, CF_USEDEF_BIT);
    } else if (size == 32) {
        la_add_w(temp1_opnd, dest_opnd, src_opnd);
        la_add_w(temp1_opnd, temp1_opnd, cflag_opnd);
        la_x86adc_w(dest_opnd, src_opnd);
    } else {
        la_add_d(temp1_opnd, dest_opnd, src_opnd);
        la_add_d(temp1_opnd, temp1_opnd, cflag_opnd);
        la_x86adc_d(dest_opnd, src_opnd);
    }
    la_x86mtflag(flag_opnd, 0x3e); //cf is not recovered

    store_ireg_to_ir1(temp1_opnd, ir1_get_opnd(pir1, 0), false);
    return true;
}

bool translate_adox(IR1_INST *pir1)
{
    IR2_OPND src_opnd = load_ireg_from_ir1(ir1_get_opnd(pir1, 1), ZERO_EXTENSION, false);
    IR2_OPND dest_opnd = load_ireg_from_ir1(ir1_get_opnd(pir1, 0), ZERO_EXTENSION, false);
    IR2_OPND flag_opnd = ra_alloc_itemp();
    IR2_OPND oflag_opnd = ra_alloc_itemp();
    IR2_OPND temp1_opnd = ra_alloc_itemp();
    int size = ir1_opnd_size(ir1_get_opnd(pir1, 0));

    /* save eflags and set of -> cf */
    la_x86mfflag(flag_opnd, 0x3f);
    la_bstrpick_d(oflag_opnd, flag_opnd, 11, 11);
    if (option_enable_lbt) {
        la_x86mtflag(oflag_opnd, 0x1);
    }

    if (!option_enable_lbt) {
        latx_adx_compute_carry(oflag_opnd, dest_opnd, src_opnd, oflag_opnd, size);
        if (size == 32) {
            la_add_w(temp1_opnd, dest_opnd, src_opnd);
            la_add_w(temp1_opnd, temp1_opnd, oflag_opnd);
        } else {
            la_add_d(temp1_opnd, dest_opnd, src_opnd);
            la_add_d(temp1_opnd, temp1_opnd, oflag_opnd);
        }
        latx_adx_write_overflow_bit(flag_opnd, oflag_opnd, OF_USEDEF_BIT);
    } else if (size == 32) {
        la_add_w(temp1_opnd, dest_opnd, src_opnd);
        la_add_w(temp1_opnd, temp1_opnd, oflag_opnd);
        la_x86adc_w(dest_opnd, src_opnd);
    } else {
        la_add_d(temp1_opnd, dest_opnd, src_opnd);
        la_add_d(temp1_opnd, temp1_opnd, oflag_opnd);
            la_x86adc_d(dest_opnd, src_opnd);
    }
    if (option_enable_lbt) {
        /* set cf -> of */
        la_x86mfflag(oflag_opnd, 0x1);
        la_bstrins_d(flag_opnd, oflag_opnd, 11, 11);
        la_x86mtflag(flag_opnd, 0x3f);
    }

    store_ireg_to_ir1(temp1_opnd, ir1_get_opnd(pir1, 0), false);
    return true;
}
