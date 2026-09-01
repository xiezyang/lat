/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "common.h"
#include "reg-alloc.h"
#include "lsenv.h"
#include "latx-options.h"
#include "flag-lbt.h"
#include "translate.h"
char pf_table[256] = {
    4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4, 0, 4, 4, 0, 4, 0,
    0, 4, 4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4,
    0, 4, 4, 0, 4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,

    0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0, 4, 0, 0, 4, 0, 4,
    4, 0, 0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0,
    4, 0, 0, 4, 0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,

    0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0, 4, 0, 0, 4, 0, 4,
    4, 0, 0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0,
    4, 0, 0, 4, 0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,

    4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4, 0, 4, 4, 0, 4, 0,
    0, 4, 4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4,
    0, 4, 4, 0, 4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
};

static void generate_cf(IR2_OPND dest, IR2_OPND src0,
                        IR2_OPND src1, IR1_INST *pir1)
{
    /* IR2_OPND eflags_opnd = ra_alloc_eflags(); */

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_BT:
    case dt_X86_INS_BTS:
    case dt_X86_INS_BTR:
    case dt_X86_INS_BTC:{
        IR2_OPND eflag_opnd = ra_alloc_itemp();
        la_srl_d(eflag_opnd, src0, src1);
        la_x86mtflag(eflag_opnd, 0x1);
        ra_free_temp(eflag_opnd);
        return;
    }
    case dt_X86_INS_SHLD: {
        int mask = ((ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 64) ? 0x3f : 0x1f);
        if (ir2_opnd_is_imm(&src1)) {
            lsassertm((ir2_opnd_imm(&src1) & mask) == ir2_opnd_imm(&src1),
                        "The value cannot be 0x%"PRIx16, ir2_opnd_imm(&src1));
            IR2_OPND t_dest_opnd = ra_alloc_itemp();
            int count = ir2_opnd_imm(&src1);
            la_srli_d(t_dest_opnd, src0, mask + 1 - count);

            la_x86mtflag(t_dest_opnd, 0x1);
            ra_free_temp(t_dest_opnd);
        } else {

            IR2_OPND t_dest_opnd = ra_alloc_itemp();
            /* src0 >> (size - count) */
            la_addi_d(t_dest_opnd, zero_ir2_opnd,
                                    mask + 1);
            la_sub_d(t_dest_opnd, t_dest_opnd, src1);
            la_srl_d(t_dest_opnd, src0, t_dest_opnd);

            la_x86mtflag(t_dest_opnd, 0x1);
            ra_free_temp(t_dest_opnd);
        }
        return;
    }
    case dt_X86_INS_SHRD: {
        if (ir2_opnd_is_imm(&src1)) {
            lsassertm((ir2_opnd_imm(&src1) &
                ((ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 64) ? 0x3f : 0x1f))
                == ir2_opnd_imm(&src1), "The value cannot be 0x%"PRIx16, ir2_opnd_imm(&src1));
            IR2_OPND t_dest_opnd = ra_alloc_itemp();
            int count = ir2_opnd_imm(&src1) - 1;
            la_srli_d(t_dest_opnd, src0, count);

            la_x86mtflag(t_dest_opnd, 0x1);
            ra_free_temp(t_dest_opnd);
        } else {
            IR2_OPND t_dest_opnd = ra_alloc_itemp();
            la_addi_d(t_dest_opnd, src1, -1);
            la_srl_d(t_dest_opnd, src0, t_dest_opnd);

            la_x86mtflag(t_dest_opnd, 0x1);
            ra_free_temp(t_dest_opnd);
        }
        return;
    }
    case dt_X86_INS_DAA:
    case dt_X86_INS_DAS: {
        /* cf has been calculate in translate_das*/
        return;
    }
    case dt_X86_INS_BLSI: {
        IR2_OPND cf = ra_alloc_itemp();
        IR2_OPND tmp = ra_alloc_itemp();
        int opnd_size = ir1_opnd_size(ir1_get_opnd(pir1, 0));
        la_ori(tmp, zero_ir2_opnd, 0xfff);
        if (opnd_size == 64) {
            la_maskeqz(cf, tmp, src1);
        } else {
            la_bstrpick_d(cf, src1, opnd_size - 1, 0);
            la_maskeqz(cf, tmp, cf);
        }
        la_x86mtflag(cf, CF_USEDEF_BIT);

        ra_free_temp(cf);
        ra_free_temp(tmp);
        return;
    }
    case dt_X86_INS_BLSR:
    case dt_X86_INS_BLSMSK: {
        IR2_OPND cf = ra_alloc_itemp();
        IR2_OPND tmp = ra_alloc_itemp();
        int opnd_size = ir1_opnd_size(ir1_get_opnd(pir1, 0));
        la_ori(tmp, zero_ir2_opnd, 0xfff);
        if (opnd_size == 64) {
            la_masknez(cf, tmp, src1);
        } else {
            la_bstrpick_d(cf, src1, opnd_size - 1, 0);
            la_masknez(cf, tmp, cf);
        }
        la_x86mtflag(cf, CF_USEDEF_BIT);

        ra_free_temp(cf);
        ra_free_temp(tmp);
        return;
    }
    case dt_X86_INS_LZCNT: {
        IR2_OPND label_temp = ra_alloc_label();
        IR2_OPND label_exit = ra_alloc_label();
        IR2_OPND temp_opnd = ra_alloc_itemp();
        int opnd_size = ir1_opnd_size(ir1_get_opnd(pir1, 0));
        li_d(temp_opnd, opnd_size);
        la_sub_d(temp_opnd, dest, temp_opnd);
        la_beq(temp_opnd, zero_ir2_opnd, label_temp);
        la_x86mtflag(zero_ir2_opnd, 0x1);
        la_b(label_exit);
        la_label(label_temp);
        la_orn(temp_opnd, zero_ir2_opnd, zero_ir2_opnd);
        la_x86mtflag(temp_opnd, 0x1);
        la_label(label_exit);
        return;
    }
    default:
        break;
    }

    /* lsenv->tr_data->curr_tb->dump(); */
    lsassertm(0, "%s for %s is not implemented\n", __FUNCTION__,
              ir1_name(ir1_opcode(pir1)));
}

static void generate_pf(IR2_OPND dest, IR2_OPND src0, IR2_OPND src1)
{
    
    IR2_OPND pf_opnd = ra_alloc_itemp();
    IR2_OPND low_byte = ra_alloc_itemp();

    TranslationBlock *tb __attribute__((unused)) = NULL;
    if (option_aot) {
        tb = (TranslationBlock *)lsenv->tr_data->curr_tb;
    }
    aot_load_host_addr(pf_opnd, (ADDR)pf_table, LOAD_HOST_PFTABLE, 0);
    la_andi(low_byte, dest, 0xff);
    la_add_d(low_byte, pf_opnd, low_byte);
    la_ld_bu(pf_opnd, low_byte, 0);
    la_x86mtflag(pf_opnd, 0x2);
    ra_free_temp(pf_opnd);
    ra_free_temp(low_byte);
}

static void generate_af(IR2_OPND dest, IR2_OPND src0,
                        IR2_OPND src1, IR1_INST *pir1)
{
    if (ir1_opcode(pir1) == dt_X86_INS_SHLD ||
        ir1_opcode(pir1) == dt_X86_INS_SHRD) {
        la_x86mtflag(zero_ir2_opnd, 0x4);
        return;
    }
    IR2_OPND af_opnd = ra_alloc_itemp();
    if (ir2_opnd_is_imm(&src1)) {
        la_xori(af_opnd, src0, ir2_opnd_imm(&src1));
    }
    else
        la_xor(af_opnd, src0, src1);
    la_xor(af_opnd, af_opnd, dest);
    la_andi(af_opnd, af_opnd, 0x10);
    la_x86mtflag(af_opnd, 0x4);
    ra_free_temp(af_opnd);

}

static void generate_zf(IR2_OPND dest, IR2_OPND src0,
                        IR2_OPND src1, IR1_INST *pir1)
{
    IR2_OPND zf = ra_alloc_itemp();
    IR2_OPND tmp = ra_alloc_itemp();
    int opnd_size = ir1_opnd_size(ir1_get_opnd(pir1, 0));

    lsassert(opnd_size == 0 || opnd_size == 8 || opnd_size == 16 ||
            opnd_size == 32 || opnd_size == 64 || opnd_size == 128);

    la_ori(tmp, zero_ir2_opnd, 0xfff);
    if (opnd_size == 64) {
        la_masknez(zf, tmp, dest);
    } else {
        la_bstrpick_d(zf, dest, opnd_size - 1, 0);
        la_masknez(zf, tmp, zf);
    }

    la_x86mtflag(zf, ZF_USEDEF_BIT);
    ra_free_temp(zf);
    ra_free_temp(tmp);
}

static void generate_sf(IR2_OPND dest, IR2_OPND src0, IR2_OPND src1)
{
    IR1_INST *pir1 = lsenv->tr_data->curr_ir1_inst;
    int operation_size = ir1_opnd_size(ir1_get_opnd(pir1, 0));
    IR2_OPND sf_opnd = ra_alloc_itemp();
    if (operation_size > 8) {
        la_srli_d(sf_opnd, dest, operation_size - 8);
        la_andi(sf_opnd, sf_opnd, 0x80);
    } else
        la_andi(sf_opnd, dest, 0x80);

    la_x86mtflag(sf_opnd, 0x10);
    ra_free_temp(sf_opnd);
}

static void generate_of(IR2_OPND dest, IR2_OPND src0,
                        IR2_OPND src1, IR1_INST *pir1)
{
    /* IR2_OPND eflags_opnd = ra_alloc_eflags(); */
    switch (ir1_opcode(pir1)) {
        case dt_X86_INS_SHLD: {
            IR2_OPND count_opnd;
            if (ir2_opnd_is_imm(&src1)) {
                count_opnd = ra_alloc_itemp();
                la_addi_d(count_opnd, zero_ir2_opnd, ir2_opnd_imm(&src1));
            } else {
                count_opnd = src1;
            }
            IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
            IR2_OPND dest_old = ra_alloc_itemp();
            IR2_OPND label_temp = ra_alloc_label();
            IR2_OPND label_exit = ra_alloc_label();
            int opnd_size = ir1_opnd_size(opnd0);
            if (opnd_size == 16) {
                IR2_OPND size = ra_alloc_itemp();
                li_d(size, opnd_size + 1);
                la_bge(count_opnd, size, label_temp);
                ra_free_temp(size);
            }
            load_ireg_from_ir1_2(dest_old, opnd0, ZERO_EXTENSION, false);
            IR2_OPND shl_count = ra_alloc_itemp();
            la_addi_d(shl_count, count_opnd, -1);
            la_sll_d(dest_old, dest_old, shl_count);
            ra_free_temp(shl_count);
            if (opnd_size == 16) {
                la_b(label_exit);

                la_label(label_temp);
                IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
                shl_count = ra_alloc_itemp();
                load_ireg_from_ir1_2(dest_old, opnd1, ZERO_EXTENSION, false);
                la_addi_d(shl_count, count_opnd, -17);
                la_sll_d(dest_old, dest_old, shl_count);
                ra_free_temp(shl_count);

                la_label(label_exit);
            }
            if (ir2_opnd_is_imm(&src1)) {
                ra_free_temp(count_opnd);
            }
            /*
            * In x86 cpu, the undefined OF when shifting bits > 1
            * is designed the same behavior with shifting bit == 1
            */
            IR2_OPND t_of_opnd = ra_alloc_itemp();
            la_xor(t_of_opnd, dest_old, dest);
            la_srli_d(t_of_opnd, t_of_opnd, ir1_opnd_size(ir1_get_opnd(pir1, 0)) - OF_BIT_INDEX - 1);

            la_x86mtflag(t_of_opnd, 0x20);
            ra_free_temp(dest_old);
            ra_free_temp(t_of_opnd);
            return;
        }
        case dt_X86_INS_SHRD: {
            IR2_OPND count_opnd = ra_alloc_itemp();
            if (ir2_opnd_is_imm(&src1)) {
                la_addi_d(count_opnd, zero_ir2_opnd, ir2_opnd_imm(&src1));
            } else {
                la_add_d(count_opnd, zero_ir2_opnd, src1);
            }
            IR1_OPND *opnd0 = ir1_get_opnd(pir1, 0);
            int opnd_size = ir1_opnd_size(opnd0);
            IR2_OPND dest_old = ra_alloc_itemp();

            if (opnd_size == 16) {
                IR1_OPND *opnd1 = ir1_get_opnd(pir1, 1);
                IR2_OPND src0_old = ra_alloc_itemp();
                load_ireg_from_ir1_2(dest_old, opnd0, ZERO_EXTENSION, false);
                load_ireg_from_ir1_2(src0_old, opnd1, ZERO_EXTENSION, false);
                la_bstrins_d(dest_old, src0_old, 31, 16);
                la_bstrins_d(dest_old, dest_old, 47, 32);

                ra_free_temp(src0_old);
                la_addi_d(count_opnd, count_opnd, -1);
                la_srl_d(dest_old, dest_old, count_opnd);
            } else {
                la_slli_d(dest_old, dest, 1);
            }
            if (ir2_opnd_is_imm(&src1)) {
                ra_free_temp(count_opnd);
            }
            /*
            * In x86 cpu, the undefined OF when shifting bits > 1
            * is designed the same behavior with shifting bit == 1
            */
            IR2_OPND t_of_opnd = ra_alloc_itemp();
            la_xor(t_of_opnd, dest_old, dest);
            la_srli_d(t_of_opnd, t_of_opnd, ir1_opnd_size(ir1_get_opnd(pir1, 0)) - OF_BIT_INDEX - 1);

            la_x86mtflag(t_of_opnd, 0x20);
            ra_free_temp(dest_old);
            ra_free_temp(t_of_opnd);
            return;
        }

    default:
        break;
    }

    /* lsenv->tr_data->curr_tb->dump(); */
    lsassertm(0, "%s for %s is not implemented\n", __FUNCTION__,
              ir1_name(ir1_opcode(pir1)));
}

static IR2_OPND soft_flag_operand(IR2_OPND value, IR1_INST *pir1, int index,
                                  int size, bool *allocated)
{
    IR2_OPND result = value;

    *allocated = false;
    if (ir2_opnd_is_imm(&value)) {
        result = ra_alloc_itemp();
        load_ireg_from_ir1_2(result, ir1_get_opnd(pir1, index),
                             ZERO_EXTENSION, false);
        *allocated = true;
    } else if (size < 64) {
        result = ra_alloc_itemp();
        la_bstrpick_d(result, value, size - 1, 0);
        *allocated = true;
    }
    return result;
}

static IR2_OPND generate_common_result(IR2_OPND src0, IR2_OPND src1,
                                       IR1_INST *pir1, bool *handled)
{
    IR2_OPND lhs;
    IR2_OPND rhs;
    IR2_OPND result;
    bool lhs_allocated;
    bool rhs_allocated;
    int size = ir1_opnd_size(ir1_get_opnd(pir1, 0));

    *handled = true;
    lhs = soft_flag_operand(src0, pir1, 0, size, &lhs_allocated);
    rhs = soft_flag_operand(src1, pir1, 1, size, &rhs_allocated);
    if (ir1_opcode(pir1) == dt_X86_INS_INC ||
        ir1_opcode(pir1) == dt_X86_INS_DEC) {
        if (rhs_allocated) {
            ra_free_temp(rhs);
        }
        rhs = ra_alloc_itemp();
        li_d(rhs, 1);
        rhs_allocated = true;
    }
    result = ra_alloc_itemp();

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_XADD:
    case dt_X86_INS_ADD:
    case dt_X86_INS_INC:
        la_add_d(result, lhs, rhs);
        break;
    case dt_X86_INS_ADC: {
        IR2_OPND carry = ra_alloc_itemp();
        latx_read_eflags(carry, CF_USEDEF_BIT);
        la_andi(carry, carry, CF_BIT);
        la_add_d(result, lhs, rhs);
        la_add_d(result, result, carry);
        ra_free_temp(carry);
        break;
    }
    case dt_X86_INS_CMPSB:
    case dt_X86_INS_CMPSW:
    case dt_X86_INS_CMPSD:
    case dt_X86_INS_CMPSQ:
    case dt_X86_INS_SCASB:
    case dt_X86_INS_SCASW:
    case dt_X86_INS_SCASD:
    case dt_X86_INS_SCASQ:
    case dt_X86_INS_CMPXCHG:
    case dt_X86_INS_NEG:
    case dt_X86_INS_CMP:
    case dt_X86_INS_SUB:
    case dt_X86_INS_DEC:
        la_sub_d(result, lhs, rhs);
        break;
    case dt_X86_INS_SBB: {
        IR2_OPND carry = ra_alloc_itemp();
        latx_read_eflags(carry, CF_USEDEF_BIT);
        la_andi(carry, carry, CF_BIT);
        la_sub_d(result, lhs, rhs);
        la_sub_d(result, result, carry);
        ra_free_temp(carry);
        break;
    }
    case dt_X86_INS_TEST:
    case dt_X86_INS_AND:
    case dt_X86_INS_ANDN:
        la_and(result, lhs, rhs);
        break;
    case dt_X86_INS_XOR:
        la_xor(result, lhs, rhs);
        break;
    case dt_X86_INS_OR:
        la_or(result, lhs, rhs);
        break;
    case dt_X86_INS_SAL:
    case dt_X86_INS_SHL:
    case dt_X86_INS_SHR:
    case dt_X86_INS_SAR: {
        IR2_OPND count = ra_alloc_itemp();
        la_andi(count, rhs, size == 64 ? 0x3f : 0x1f);
        if (ir1_opcode(pir1) == dt_X86_INS_SAL ||
            ir1_opcode(pir1) == dt_X86_INS_SHL) {
            la_sll_d(result, lhs, count);
        } else if (ir1_opcode(pir1) == dt_X86_INS_SHR) {
            la_srl_d(result, lhs, count);
        } else {
            if (size < 64) {
                la_slli_d(result, lhs, 64 - size);
                la_srai_d(result, result, 64 - size);
                la_sra_d(result, result, count);
            } else {
                la_sra_d(result, lhs, count);
            }
        }
        ra_free_temp(count);
        break;
    }
    default:
        *handled = false;
        la_or(result, zero_ir2_opnd, zero_ir2_opnd);
        break;
    }
    if (size < 64) {
        la_bstrpick_d(result, result, size - 1, 0);
    }
    if (rhs_allocated) {
        ra_free_temp(rhs);
    }
    if (lhs_allocated) {
        ra_free_temp(lhs);
    }
    return result;
}

static void generate_multiply_overflow(IR2_OPND overflow, IR2_OPND src0,
                                       IR2_OPND src1, IR1_INST *pir1)
{
    IR2_OPND lhs = ra_alloc_itemp();
    IR2_OPND rhs = ra_alloc_itemp();
    IR2_OPND product = ra_alloc_itemp();
    IR2_OPND check = ra_alloc_itemp();
    int size = ir1_opnd_size(ir1_get_opnd(pir1, 0));
    bool is_signed = ir1_opcode(pir1) == dt_X86_INS_IMUL;

    la_or(lhs, src0, zero_ir2_opnd);
    if (ir2_opnd_is_imm(&src1)) {
        int src1_index = ir1_opnd_num(pir1) == 3 ? 2 : 1;
        load_ireg_from_ir1_2(rhs, ir1_get_opnd(pir1, src1_index),
                             is_signed ? SIGN_EXTENSION : ZERO_EXTENSION,
                             false);
    } else {
        la_or(rhs, src1, zero_ir2_opnd);
    }

    if (size < 64) {
        la_bstrpick_d(lhs, lhs, size - 1, 0);
        la_bstrpick_d(rhs, rhs, size - 1, 0);
        if (is_signed) {
            la_slli_d(lhs, lhs, 64 - size);
            la_srai_d(lhs, lhs, 64 - size);
            la_slli_d(rhs, rhs, 64 - size);
            la_srai_d(rhs, rhs, 64 - size);
        }
        la_mul_d(product, lhs, rhs);
        if (is_signed) {
            la_bstrpick_d(check, product, size - 1, 0);
            la_slli_d(check, check, 64 - size);
            la_srai_d(check, check, 64 - size);
            la_xor(check, check, product);
        } else {
            la_srli_d(check, product, size);
        }
    } else if (is_signed) {
        la_mul_d(product, lhs, rhs);
        la_mulh_d(check, lhs, rhs);
        la_srai_d(product, product, 63);
        la_xor(check, check, product);
    } else {
        la_mulh_du(check, lhs, rhs);
    }
    la_sltu(overflow, zero_ir2_opnd, check);

    ra_free_temp(check);
    ra_free_temp(product);
    ra_free_temp(rhs);
    ra_free_temp(lhs);
}

static bool generate_common_cf(IR2_OPND dest, IR2_OPND src0, IR2_OPND src1,
                               IR1_INST *pir1)
{
    IR2_OPND result;
    IR2_OPND lhs;
    IR2_OPND rhs;
    IR2_OPND cf;
    bool result_allocated;
    bool lhs_allocated;
    bool rhs_allocated;
    int size = ir1_opnd_size(ir1_get_opnd(pir1, 0));
    bool handled = true;

    result = soft_flag_operand(dest, pir1, 0, size, &result_allocated);
    lhs = soft_flag_operand(src0, pir1, 0, size, &lhs_allocated);
    rhs = soft_flag_operand(src1, pir1, 1, size, &rhs_allocated);
    if (ir1_opcode(pir1) == dt_X86_INS_INC ||
        ir1_opcode(pir1) == dt_X86_INS_DEC) {
        if (rhs_allocated) {
            ra_free_temp(rhs);
        }
        rhs = ra_alloc_itemp();
        li_d(rhs, 1);
        rhs_allocated = true;
    }
    cf = ra_alloc_itemp();

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_XADD:
    case dt_X86_INS_ADD:
        la_sltu(cf, result, lhs);
        break;
    case dt_X86_INS_ADC: {
        IR2_OPND partial = ra_alloc_itemp();
        IR2_OPND first = ra_alloc_itemp();
        IR2_OPND second = ra_alloc_itemp();
        la_add_d(partial, lhs, rhs);
        if (size < 64) {
            la_bstrpick_d(partial, partial, size - 1, 0);
        }
        la_sltu(first, partial, lhs);
        la_sltu(second, result, partial);
        la_or(cf, first, second);
        ra_free_temp(second);
        ra_free_temp(first);
        ra_free_temp(partial);
        break;
    }
    case dt_X86_INS_CMPSB:
    case dt_X86_INS_CMPSW:
    case dt_X86_INS_CMPSD:
    case dt_X86_INS_CMPSQ:
    case dt_X86_INS_SCASB:
    case dt_X86_INS_SCASW:
    case dt_X86_INS_SCASD:
    case dt_X86_INS_SCASQ:
    case dt_X86_INS_CMPXCHG:
    case dt_X86_INS_NEG:
    case dt_X86_INS_CMP:
    case dt_X86_INS_SUB:
        la_sltu(cf, lhs, rhs);
        break;
    case dt_X86_INS_SBB: {
        IR2_OPND partial = ra_alloc_itemp();
        IR2_OPND first = ra_alloc_itemp();
        IR2_OPND second = ra_alloc_itemp();
        IR2_OPND carry = ra_alloc_itemp();
        la_sub_d(partial, lhs, rhs);
        if (size < 64) {
            la_bstrpick_d(partial, partial, size - 1, 0);
        }
        la_sltu(first, lhs, rhs);
        latx_read_eflags(carry, CF_USEDEF_BIT);
        la_andi(carry, carry, CF_BIT);
        la_sltu(second, partial, carry);
        la_or(cf, first, second);
        ra_free_temp(carry);
        ra_free_temp(second);
        ra_free_temp(first);
        ra_free_temp(partial);
        break;
    }
    case dt_X86_INS_TEST:
    case dt_X86_INS_XOR:
    case dt_X86_INS_AND:
    case dt_X86_INS_ANDN:
    case dt_X86_INS_OR:
        la_or(cf, zero_ir2_opnd, zero_ir2_opnd);
        break;
    case dt_X86_INS_SAL:
    case dt_X86_INS_SHL:
    case dt_X86_INS_SHR:
    case dt_X86_INS_SAR: {
        IR2_OPND count = ra_alloc_itemp();
        IR2_OPND shift = ra_alloc_itemp();
        la_andi(count, rhs, size == 64 ? 0x3f : 0x1f);
        if (ir1_opcode(pir1) == dt_X86_INS_SAL ||
            ir1_opcode(pir1) == dt_X86_INS_SHL) {
            li_d(shift, size);
            la_sub_d(shift, shift, count);
        } else {
            la_addi_d(shift, count, -1);
        }
        la_srl_d(cf, lhs, shift);
        la_andi(cf, cf, 1);
        ra_free_temp(shift);
        ra_free_temp(count);
        break;
    }
    case dt_X86_INS_MUL:
    case dt_X86_INS_IMUL:
        generate_multiply_overflow(cf, src0, src1, pir1);
        break;
    case dt_X86_INS_INC:
    case dt_X86_INS_DEC:
        handled = true; /* INC/DEC preserve CF. */
        goto out;
    default:
        handled = false;
        goto out;
    }
    latx_write_eflags(cf, CF_USEDEF_BIT);

out:
    ra_free_temp(cf);
    if (rhs_allocated) {
        ra_free_temp(rhs);
    }
    if (lhs_allocated) {
        ra_free_temp(lhs);
    }
    if (result_allocated) {
        ra_free_temp(result);
    }
    return handled;
}

static bool generate_common_of(IR2_OPND dest, IR2_OPND src0, IR2_OPND src1,
                               IR1_INST *pir1)
{
    IR2_OPND result;
    IR2_OPND lhs;
    IR2_OPND rhs;
    IR2_OPND first;
    IR2_OPND second;
    IR2_OPND of;
    bool result_allocated;
    bool lhs_allocated;
    bool rhs_allocated;
    int size = ir1_opnd_size(ir1_get_opnd(pir1, 0));
    bool handled = true;

    result = soft_flag_operand(dest, pir1, 0, size, &result_allocated);
    lhs = soft_flag_operand(src0, pir1, 0, size, &lhs_allocated);
    rhs = soft_flag_operand(src1, pir1, 1, size, &rhs_allocated);
    if (ir1_opcode(pir1) == dt_X86_INS_INC ||
        ir1_opcode(pir1) == dt_X86_INS_DEC) {
        if (rhs_allocated) {
            ra_free_temp(rhs);
        }
        rhs = ra_alloc_itemp();
        li_d(rhs, 1);
        rhs_allocated = true;
    }
    first = ra_alloc_itemp();
    second = ra_alloc_itemp();
    of = ra_alloc_itemp();

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_XADD:
    case dt_X86_INS_ADD:
    case dt_X86_INS_ADC:
    case dt_X86_INS_INC:
        la_xor(first, lhs, rhs);
        la_nor(first, first, zero_ir2_opnd);
        la_xor(second, lhs, result);
        la_and(of, first, second);
        break;
    case dt_X86_INS_CMPSB:
    case dt_X86_INS_CMPSW:
    case dt_X86_INS_CMPSD:
    case dt_X86_INS_CMPSQ:
    case dt_X86_INS_SCASB:
    case dt_X86_INS_SCASW:
    case dt_X86_INS_SCASD:
    case dt_X86_INS_SCASQ:
    case dt_X86_INS_CMPXCHG:
    case dt_X86_INS_NEG:
    case dt_X86_INS_CMP:
    case dt_X86_INS_SUB:
    case dt_X86_INS_SBB:
    case dt_X86_INS_DEC:
        la_xor(first, lhs, rhs);
        la_xor(second, lhs, result);
        la_and(of, first, second);
        break;
    case dt_X86_INS_TEST:
    case dt_X86_INS_XOR:
    case dt_X86_INS_AND:
    case dt_X86_INS_ANDN:
    case dt_X86_INS_OR:
        la_or(of, zero_ir2_opnd, zero_ir2_opnd);
        break;
    case dt_X86_INS_SAL:
    case dt_X86_INS_SHL:
    case dt_X86_INS_SHR:
    case dt_X86_INS_SAR: {
        IR2_OPND count = ra_alloc_itemp();
        IR2_OPND one = ra_alloc_itemp();
        IR2_OPND done = ra_alloc_label();
        la_andi(count, rhs, size == 64 ? 0x3f : 0x1f);
        li_d(one, 1);
        la_bne(count, one, done);
        if (ir1_opcode(pir1) == dt_X86_INS_SAL ||
            ir1_opcode(pir1) == dt_X86_INS_SHL) {
            IR2_OPND cf = ra_alloc_itemp();
            IR2_OPND msb = ra_alloc_itemp();
            latx_read_eflags(cf, CF_USEDEF_BIT);
            la_andi(cf, cf, CF_BIT);
            la_bstrpick_d(msb, result, size - 1, size - 1);
            la_xor(of, msb, cf);
            ra_free_temp(msb);
            ra_free_temp(cf);
        } else if (ir1_opcode(pir1) == dt_X86_INS_SHR) {
            la_bstrpick_d(of, lhs, size - 1, size - 1);
        } else {
            la_or(of, zero_ir2_opnd, zero_ir2_opnd);
        }
        la_slli_d(of, of, OF_BIT_INDEX);
        latx_write_eflags(of, OF_USEDEF_BIT);
        la_label(done);
        ra_free_temp(one);
        ra_free_temp(count);
        handled = true;
        goto out;
    }
    case dt_X86_INS_MUL:
    case dt_X86_INS_IMUL:
        generate_multiply_overflow(of, src0, src1, pir1);
        la_slli_d(of, of, OF_BIT_INDEX);
        latx_write_eflags(of, OF_USEDEF_BIT);
        handled = true;
        goto out;
    default:
        handled = false;
        goto out;
    }
    la_bstrpick_d(of, of, size - 1, size - 1);
    la_slli_d(of, of, OF_BIT_INDEX);
    latx_write_eflags(of, OF_USEDEF_BIT);

out:
    ra_free_temp(of);
    ra_free_temp(second);
    ra_free_temp(first);
    if (rhs_allocated) {
        ra_free_temp(rhs);
    }
    if (lhs_allocated) {
        ra_free_temp(lhs);
    }
    if (result_allocated) {
        ra_free_temp(result);
    }
    return handled;
}

static void generate_cf_not_sx(IR2_OPND dest, IR2_OPND src0, IR2_OPND src1)
{
#if 0
    IR1_INST *pir1 = lsenv->tr_data->curr_ir1_inst;
    IR2_OPND eflags_opnd = ra_alloc_eflags();

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_OR:
    case dt_X86_INS_AND:
    case dt_X86_INS_XOR:
    case dt_X86_INS_TEST:
        la_append_ir2_opnd2i(mips_andi, eflags_opnd, eflags_opnd, ~CF_BIT);
        return;
    case dt_X86_INS_ADD:
    case dt_X86_INS_ADC:
    case dt_X86_INS_CMP:
    case dt_X86_INS_SUB: {
        lsassert(ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 8 ||
                 ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 16 ||
                 ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 32);
        IR2_OPND cf_opnd = ra_alloc_itemp();
        if (ir1_opnd_size(ir1_get_opnd(pir1, 0)) != 32)
            append_ir2_opnd2i(mips_dsra, cf_opnd, dest,
                              ir1_opnd_size(ir1_get_opnd(pir1, 0)));
        else
            append_ir2_opnd2i(mips_dsra32, cf_opnd, dest, 0);
        if (!ir2_opnd_is_zx(&cf_opnd, 1))
            append_ir2_opnd2i(mips_andi, cf_opnd, cf_opnd, CF_BIT);
        append_ir2_opnd2i(mips_andi, eflags_opnd, eflags_opnd, ~CF_BIT);
        append_ir2_opnd3(mips_or, eflags_opnd, eflags_opnd, cf_opnd);
        ra_free_temp(cf_opnd);
        return;
    }
    case dt_X86_INS_SHR: {
        lsassert(ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 8 ||
                 ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 16 ||
                 ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 32);
        IR2_OPND cf_opnd = ra_alloc_itemp();
        IR2_OPND ir2_opnd_tmp;
        ir2_opnd_build(&ir2_opnd_tmp, IR2_OPND_IMM, src1._imm16 - 1);
        append_ir2_opnd3(mips_srl, cf_opnd, src0, ir2_opnd_tmp);
        append_ir2_opnd2i(mips_andi, cf_opnd, cf_opnd, CF_BIT);
        append_ir2_opnd2i(mips_andi, eflags_opnd, eflags_opnd, ~CF_BIT);
        append_ir2_opnd3(mips_or, eflags_opnd, eflags_opnd, cf_opnd);
        ra_free_temp(cf_opnd);
        return;
    }
    case dt_X86_INS_SHL: {
        lsassert(ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 8 ||
                 ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 16 ||
                 ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 32);
        IR2_OPND cf_opnd = ra_alloc_itemp();
        IR2_OPND ir2_opnd_tmp;
        ir2_opnd_build(&ir2_opnd_tmp, IR2_OPND_IMM, 32 - src1._imm16);
        append_ir2_opnd3(mips_srl, cf_opnd, src0, ir2_opnd_tmp);
        append_ir2_opnd2i(mips_andi, cf_opnd, cf_opnd, CF_BIT);
        append_ir2_opnd2i(mips_andi, eflags_opnd, eflags_opnd, ~CF_BIT);
        append_ir2_opnd3(mips_or, eflags_opnd, eflags_opnd, cf_opnd);
        ra_free_temp(cf_opnd);
        return;
    }
    default:
        break;
    }
#endif
    /* lsenv->tr_data->curr_tb->dump(); */
    lsassertm(0, "%s for %s is not implemented\n", __FUNCTION__,
              ir1_name(ir1_opcode(lsenv->tr_data->curr_ir1_inst)));
}

static void generate_of_not_sx(IR2_OPND dest, IR2_OPND src0, IR2_OPND src1)
{
#if 0
    IR1_INST *pir1 = lsenv->tr_data->curr_ir1_inst;
    IR2_OPND eflags_opnd = ra_alloc_eflags();

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_OR:
    case dt_X86_INS_AND:
    case dt_X86_INS_XOR:
    case dt_X86_INS_TEST:
        append_ir2_opnd2i(mips_andi, eflags_opnd, eflags_opnd, ~OF_BIT);
        return;
    case dt_X86_INS_ADD:
    case dt_X86_INS_SUB: {
        lsassert(ir1_opnd_size(ir1_get_opnd(pir1, 0)) == 32);
        IR2_OPND of_opnd = ra_alloc_itemp();
        /* since it is unsigned add/sub, so set OF = bit32 */
        append_ir2_opnd2i(mips_dsrl, of_opnd, dest, 32 - OF_BIT_INDEX);
        append_ir2_opnd2i(mips_andi, of_opnd, of_opnd, OF_BIT);
        append_ir2_opnd2i(mips_andi, eflags_opnd, eflags_opnd, ~OF_BIT);
        append_ir2_opnd3(mips_or, eflags_opnd, eflags_opnd, of_opnd);
        ra_free_temp(of_opnd);
        return;
    }
    default:
        break;
    }
#endif
    /* lsenv->tr_data->curr_tb->dump(); */
    lsassertm(0, "%s for %s is not implemented\n", __FUNCTION__,
              ir1_name(ir1_opcode(lsenv->tr_data->curr_ir1_inst)));
}

#define WRAP(ins) (dt_X86_INS_##ins)
void generate_eflag_calculation(IR2_OPND dest, IR2_OPND src0, IR2_OPND src1,
                                IR1_INST *pir1, bool flags)
{
    bool need_calc_flag = ir1_need_calculate_any_flag(pir1);
    bool use_lbt = true;
    if (need_calc_flag) {
        use_lbt = generate_eflag_by_lbt(dest, src0, src1, pir1, flags);
    }
#ifdef CONFIG_LATX_PROFILER
    IR1_INST *curr = lsenv->tr_data->curr_ir1_inst;
    if (curr->instptn.opc  != INSTPTN_OPC_NONE) {
    //if (!(curr->cflag & IR1_PATTERN_MASK)) {
        TranslationBlock *tb = (TranslationBlock *)lsenv->tr_data->curr_tb;
        /* want generated flag number */
        ADD_TB_PROFILE(tb, sta_generate, 1);
        /* eliminate flag number */
        ADD_TB_PROFILE(tb, sta_eliminate, !(need_calc_flag));
        /* profile used lbt number */
        ADD_TB_PROFILE(tb, sta_simulate, !(use_lbt));
    }
#endif

    if (!need_calc_flag) {
        return;
    }

    if (!option_enable_lbt) {
        bool soft_result_handled;
        IR2_OPND soft_result =
            generate_common_result(src0, src1, pir1, &soft_result_handled);
        IR2_OPND flag_result = soft_result_handled ? soft_result : dest;

        if (ir1_need_calculate_pf(pir1)) {
            generate_pf(flag_result, src0, src1);
        }
        if (ir1_need_calculate_af(pir1)) {
            generate_af(flag_result, src0, src1, pir1);
        }
        if (ir1_need_calculate_zf(pir1)) {
            generate_zf(flag_result, src0, src1, pir1);
        }
        if (ir1_need_calculate_sf(pir1)) {
            generate_sf(flag_result, src0, src1);
        }
        if (ir1_need_calculate_cf(pir1) &&
            !generate_common_cf(flag_result, src0, src1, pir1)) {
            generate_cf(flag_result, src0, src1, pir1);
        }
        if (ir1_need_calculate_of(pir1) &&
            !generate_common_of(flag_result, src0, src1, pir1)) {
            generate_of(flag_result, src0, src1, pir1);
        }
        ra_free_temp(soft_result);
        return;
    }

    if (use_lbt) {
        if (ir1_opcode(pir1) == dt_X86_INS_SBB) {
            generate_af(dest, src0, src1, pir1);
        } else if (ir1_opcode(pir1) == dt_X86_INS_BLSI ||
                   ir1_opcode(pir1) == dt_X86_INS_BLSR ||
                   ir1_opcode(pir1) == dt_X86_INS_BLSMSK) {
            generate_cf(dest, src0, src1, pir1);
        }
        return;
    }
#ifdef CONFIG_LATX_XCOMISX_OPT
    uint8_t use_flags =
        ir1_get_eflag_def(pir1) & (~(lsenv->tr_data->curr_ir1_skipped_eflags));
    switch (ir1_opcode(pir1)) {
    case WRAP(COMISS):
        generate_xcomisx(src0, src1, false, true, use_flags);
        return;
    case WRAP(COMISD):
        generate_xcomisx(src0, src1, true, true, use_flags);
        return;
    case WRAP(UCOMISS):
        generate_xcomisx(src0, src1, false, false, use_flags);
        return;
    case WRAP(UCOMISD):
        generate_xcomisx(src0, src1, true, false, use_flags);
        return;
    default:
        break;
    }
#endif
    /* extension mode does not affect pf, af and zf */
    if (ir1_need_calculate_pf(pir1))
        generate_pf(dest, src0, src1);
    if (ir1_need_calculate_af(pir1))
        generate_af(dest, src0, src1, pir1);
    if (ir1_need_calculate_zf(pir1))
        generate_zf(dest, src0, src1, pir1);
    if (ir1_need_calculate_sf(pir1))
        generate_sf(dest, src0, src1);

    /* calculate cf and of separately */
    if (flags) {
        if (ir1_need_calculate_cf(pir1))
            generate_cf(dest, src0, src1, pir1);
        if (ir1_need_calculate_of(pir1))
            generate_of(dest, src0, src1, pir1);
    } else {
        if (ir1_need_calculate_cf(pir1))
            generate_cf_not_sx(dest, src0, src1);
        if (ir1_need_calculate_of(pir1))
            generate_of_not_sx(dest, src0, src1);
    }
}
#undef WRAP
