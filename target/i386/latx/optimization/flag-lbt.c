/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#define LATX_INTERNAL_RAW_LBT
#include "env.h"
#include "reg-alloc.h"
#include "latx-options.h"
#include "translate.h"
#include "flag-lbt.h"

bool generate_eflag_by_lbt(IR2_OPND dest, IR2_OPND src0, IR2_OPND src1,
                           IR1_INST *pir1, bool is_imm)
{
    if (!option_enable_lbt) {
        return false;
    }

    switch (ir1_opcode(pir1)) {
    case dt_X86_INS_XADD:
    case dt_X86_INS_ADD: {
        GENERATE_EFLAG_IR2_2(la_x86add);
        break;
    }
    case dt_X86_INS_ADC: {
        GENERATE_EFLAG_IR2_2(la_x86adc);
        break;
    }
    case dt_X86_INS_INC: {
        GENERATE_EFLAG_IR2_1(la_x86inc);
        break;
    }
    case dt_X86_INS_DEC: {
        GENERATE_EFLAG_IR2_1(la_x86dec);
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
    case dt_X86_INS_SUB: {
        GENERATE_EFLAG_IR2_2(la_x86sub);
        break;
    }
    case dt_X86_INS_SBB: {
        GENERATE_EFLAG_IR2_2(la_x86sbc);
        break;
    }
    case dt_X86_INS_BLSMSK:
    case dt_X86_INS_XOR: {
        GENERATE_EFLAG_IR2_2(la_x86xor);
        break;
    }
    case dt_X86_INS_TEST:
    case dt_X86_INS_BLSI:
    case dt_X86_INS_BLSR:
    case dt_X86_INS_BZHI:
    case dt_X86_INS_AND:
    case dt_X86_INS_ANDN: {
        GENERATE_EFLAG_IR2_2(la_x86and);
        break;
    }
    case dt_X86_INS_OR: {
        GENERATE_EFLAG_IR2_2(la_x86or);
        break;
    }
    case dt_X86_INS_SAL:
    case dt_X86_INS_SHL: {
        if (is_imm) {
            GENERATE_EFLAG_IR2_2_I(la_x86slli);
        } else {
            GENERATE_EFLAG_IR2_2(la_x86sll);
        }
        break;
    }
    case dt_X86_INS_SHR: {
        if (is_imm) {
            GENERATE_EFLAG_IR2_2_I(la_x86srli);
        } else {
            GENERATE_EFLAG_IR2_2(la_x86srl);
        }
        break;
    }
    case dt_X86_INS_SAR: {
        if (is_imm) {
            GENERATE_EFLAG_IR2_2_I(la_x86srai);
        } else {
            GENERATE_EFLAG_IR2_2(la_x86sra);
        }
        break;
    }
    case dt_X86_INS_RCL: {
        GENERATE_EFLAG_IR2_2(la_x86rcl);
        break;
    }
    case dt_X86_INS_RCR: {
        GENERATE_EFLAG_IR2_2(la_x86rcr);
        break;
    }
    case dt_X86_INS_MUL: {
        /*
         * 3A500 has new MUL insn which could leverage directly.
         */
        GENERATE_EFLAG_IR2_2_U(la_x86mul);
        break;
    }
    case dt_X86_INS_IMUL: {
        GENERATE_EFLAG_IR2_2(la_x86mul);
        break;
    }
    case dt_X86_INS_ROR:
    case dt_X86_INS_ROL: {
	    break;
	}
#ifdef CONFIG_LATX_XCOMISX_OPT
    case dt_X86_INS_COMISS:
    case dt_X86_INS_COMISD:
    case dt_X86_INS_UCOMISS:
    case dt_X86_INS_UCOMISD:
#endif
    case dt_X86_INS_AAM:
    case dt_X86_INS_AAD:
    case dt_X86_INS_AAA:
    case dt_X86_INS_DAA:
    case dt_X86_INS_DAS:
    case dt_X86_INS_BSF:
    case dt_X86_INS_BSR:
    case dt_X86_INS_BT:
    case dt_X86_INS_BTS:
    case dt_X86_INS_BTR:
    case dt_X86_INS_BTC:
    case dt_X86_INS_SHLD:
    case dt_X86_INS_SHRD:
    case dt_X86_INS_CMPXCHG8B:
    case dt_X86_INS_CMPXCHG16B:
    case dt_X86_INS_LZCNT: {
	    return false;
        }
        default:
            lsassertm(0, "%s (%x) is not implemented in %s\n",
                      pir1->info->mnemonic, ir1_opcode(pir1), __func__);
            return false;
        }
    return true;
}

static uint16_t usedef_to_eflags_mask(uint8_t mask)
{
    uint16_t eflags_mask = 0;

    eflags_mask |= (mask & CF_USEDEF_BIT) ? CF_BIT : 0;
    eflags_mask |= (mask & PF_USEDEF_BIT) ? PF_BIT : 0;
    eflags_mask |= (mask & AF_USEDEF_BIT) ? AF_BIT : 0;
    eflags_mask |= (mask & ZF_USEDEF_BIT) ? ZF_BIT : 0;
    eflags_mask |= (mask & SF_USEDEF_BIT) ? SF_BIT : 0;
    eflags_mask |= (mask & OF_USEDEF_BIT) ? OF_BIT : 0;
    return eflags_mask;
}

void latx_write_eflags(IR2_OPND value, uint8_t mask)
{
    IR2_OPND eflags;
    IR2_OPND selected;
    uint16_t eflags_mask;

    if (option_enable_lbt) {
        la_x86mtflag(value, mask);
        return;
    }

    eflags_mask = usedef_to_eflags_mask(mask);
    eflags = ra_alloc_eflags();
    selected = ra_alloc_itemp();
    if (eflags_mask & CF_BIT) {
        la_bstrins_d(eflags, zero_ir2_opnd, CF_BIT_INDEX, CF_BIT_INDEX);
    }
    if (eflags_mask & PF_BIT) {
        la_bstrins_d(eflags, zero_ir2_opnd, PF_BIT_INDEX, PF_BIT_INDEX);
    }
    if (eflags_mask & AF_BIT) {
        la_bstrins_d(eflags, zero_ir2_opnd, AF_BIT_INDEX, AF_BIT_INDEX);
    }
    if (eflags_mask & ZF_BIT) {
        la_bstrins_d(eflags, zero_ir2_opnd, ZF_BIT_INDEX, ZF_BIT_INDEX);
    }
    if (eflags_mask & SF_BIT) {
        la_bstrins_d(eflags, zero_ir2_opnd, SF_BIT_INDEX, SF_BIT_INDEX);
    }
    if (eflags_mask & OF_BIT) {
        la_bstrins_d(eflags, zero_ir2_opnd, OF_BIT_INDEX, OF_BIT_INDEX);
    }
    la_andi(selected, value, eflags_mask);
    la_or(eflags, eflags, selected);
    ra_free_temp(selected);
}

void latx_read_eflags(IR2_OPND value, uint8_t mask)
{
    if (option_enable_lbt) {
        la_x86mfflag(value, mask);
        return;
    }

    la_andi(value, ra_alloc_eflags(), usedef_to_eflags_mask(mask));
}

static void soft_eflag_bit(IR2_OPND dest, int bit)
{
    la_bstrpick_d(dest, ra_alloc_eflags(), bit, bit);
}

void latx_set_eflag_condition(IR2_OPND dest, int condition)
{
    IR2_OPND lhs;
    IR2_OPND rhs;

    if (option_enable_lbt) {
        la_setx86j(dest, condition);
        return;
    }

    lhs = ra_alloc_itemp();
    rhs = ra_alloc_itemp();
    switch (condition) {
    case COND_AE:
        soft_eflag_bit(dest, CF_BIT_INDEX);
        la_xori(dest, dest, 1);
        break;
    case COND_B:
        soft_eflag_bit(dest, CF_BIT_INDEX);
        break;
    case COND_PO:
        soft_eflag_bit(dest, PF_BIT_INDEX);
        la_xori(dest, dest, 1);
        break;
    case COND_PE:
        soft_eflag_bit(dest, PF_BIT_INDEX);
        break;
    case COND_E:
        soft_eflag_bit(dest, ZF_BIT_INDEX);
        break;
    case COND_NE:
        soft_eflag_bit(dest, ZF_BIT_INDEX);
        la_xori(dest, dest, 1);
        break;
    case COND_S:
        soft_eflag_bit(dest, SF_BIT_INDEX);
        break;
    case COND_NS:
        soft_eflag_bit(dest, SF_BIT_INDEX);
        la_xori(dest, dest, 1);
        break;
    case COND_O:
        soft_eflag_bit(dest, OF_BIT_INDEX);
        break;
    case COND_NO:
        soft_eflag_bit(dest, OF_BIT_INDEX);
        la_xori(dest, dest, 1);
        break;
    case COND_BE:
    case COND_A:
        soft_eflag_bit(lhs, CF_BIT_INDEX);
        soft_eflag_bit(rhs, ZF_BIT_INDEX);
        la_or(dest, lhs, rhs);
        if (condition == COND_A) {
            la_xori(dest, dest, 1);
        }
        break;
    case COND_L:
    case COND_GE:
        soft_eflag_bit(lhs, SF_BIT_INDEX);
        soft_eflag_bit(rhs, OF_BIT_INDEX);
        la_xor(dest, lhs, rhs);
        if (condition == COND_GE) {
            la_xori(dest, dest, 1);
        }
        break;
    case COND_LE:
    case COND_G:
        soft_eflag_bit(lhs, SF_BIT_INDEX);
        soft_eflag_bit(rhs, OF_BIT_INDEX);
        la_xor(lhs, lhs, rhs);
        soft_eflag_bit(rhs, ZF_BIT_INDEX);
        la_or(dest, lhs, rhs);
        if (condition == COND_G) {
            la_xori(dest, dest, 1);
        }
        break;
    default:
        lsassertm(0, "unsupported software eflags condition %d\n", condition);
    }
    ra_free_temp(lhs);
    ra_free_temp(rhs);
}

void latx_write_top(IR2_OPND value)
{
    int top_offset = lsenv_offset_of_top(lsenv);

    lsassert(!option_enable_lbt);
    lsassert(top_offset >= -2048 && top_offset <= 2047);
    la_andi(value, value, 0x7);
    la_st_w(value, env_ir2_opnd, top_offset);
}

void latx_write_top_const(int value)
{
    if (option_enable_lbt) {
        la_x86mttop(value);
        return;
    }

    IR2_OPND top = ra_alloc_itemp();

    li_d(top, value);
    latx_write_top(top);
    ra_free_temp(top);
}

void latx_read_top(IR2_OPND value)
{
    int top_offset = lsenv_offset_of_top(lsenv);

    if (option_enable_lbt) {
        la_x86mftop(value);
        return;
    }

    lsassert(top_offset >= -2048 && top_offset <= 2047);
    la_ld_wu(value, env_ir2_opnd, top_offset);
}

static void latx_adjust_top(int delta)
{
    IR2_OPND top = ra_alloc_itemp();

    latx_read_top(top);
    la_addi_w(top, top, delta);
    la_andi(top, top, 0x7);
    latx_write_top(top);
    ra_free_temp(top);
}

void latx_inc_top(void)
{
    if (option_enable_lbt) {
        la_x86inctop();
        return;
    }
    latx_adjust_top(1);
}

void latx_dec_top(void)
{
    if (option_enable_lbt) {
        la_x86dectop();
        return;
    }
    latx_adjust_top(-1);
}

void get_eflag_condition(IR2_OPND *cond, IR1_INST *pir1) {
    switch(ir1_opcode(pir1)) {
        /* CF */
        case dt_X86_INS_SETAE:
        case dt_X86_INS_CMOVAE:
        case dt_X86_INS_FCMOVNB:
        case dt_X86_INS_JAE:
        {
            latx_set_eflag_condition(*cond, COND_AE);
            break;
        }
        case dt_X86_INS_SETB:
        case dt_X86_INS_CMOVB: 
        case dt_X86_INS_FCMOVB:
        case dt_X86_INS_JB:
        {
            latx_set_eflag_condition(*cond, COND_B);
            break;
        }
        case dt_X86_INS_RCL:
        case dt_X86_INS_RCR: {
            latx_read_eflags(*cond, 0x1);
            break;
        }
        /* PF */
        case dt_X86_INS_SETNP:
        case dt_X86_INS_CMOVNP:
        case dt_X86_INS_FCMOVNU:
        case dt_X86_INS_JNP:
        {
            latx_set_eflag_condition(*cond, COND_PO);
            break;
        }
        case dt_X86_INS_SETP:
        case dt_X86_INS_CMOVP:
        case dt_X86_INS_FCMOVU:
        case dt_X86_INS_JP:
        {
            latx_set_eflag_condition(*cond, COND_PE);
            break;
        }
        /* ZF */
        case dt_X86_INS_SETE:
        case dt_X86_INS_CMOVE:
        case dt_X86_INS_FCMOVE:
        case dt_X86_INS_JE:
        {
            latx_set_eflag_condition(*cond, COND_E);
            break;
        }
        case dt_X86_INS_SETNE:
        case dt_X86_INS_CMOVNE:
        case dt_X86_INS_FCMOVNE:
        case dt_X86_INS_JNE:
        {
            latx_set_eflag_condition(*cond, COND_NE);
            break;
        }
        /* SF */
        case dt_X86_INS_SETS:
        case dt_X86_INS_CMOVS:
        case dt_X86_INS_JS:
        {
            latx_set_eflag_condition(*cond, COND_S);
            break;
        }
        case dt_X86_INS_SETNS:
        case dt_X86_INS_CMOVNS:
        case dt_X86_INS_JNS:
        {
            latx_set_eflag_condition(*cond, COND_NS);
            break;
        }
        /* OF */
        case dt_X86_INS_SETO:
        case dt_X86_INS_CMOVO:
        case dt_X86_INS_JO:
        {
            latx_set_eflag_condition(*cond, COND_O);
            break;
        }
        case dt_X86_INS_SETNO:
        case dt_X86_INS_CMOVNO:
        case dt_X86_INS_JNO:
        {
            latx_set_eflag_condition(*cond, COND_NO);
            break;
        }
        /* CF ZF */
        case dt_X86_INS_SETBE:
        case dt_X86_INS_CMOVBE:
        case dt_X86_INS_FCMOVBE:
        case dt_X86_INS_JBE:
        {
            latx_set_eflag_condition(*cond, COND_BE);
            break;
        }
        case dt_X86_INS_SETA:
        case dt_X86_INS_CMOVA:
        case dt_X86_INS_FCMOVNBE:
        case dt_X86_INS_JA:
        {
            latx_set_eflag_condition(*cond, COND_A);
            break;
        }
        case dt_X86_INS_LOOPE:
        case dt_X86_INS_LOOPNE: {
            latx_read_eflags(*cond, 0x8);
            la_slli_d(*cond, *cond, 63 - ZF_BIT_INDEX);
            la_srai_d(*cond, *cond, 63);
            break;
        }

        /* SF OF */
        case dt_X86_INS_SETL:
        case dt_X86_INS_CMOVL:
        case dt_X86_INS_JL:
        {
            latx_set_eflag_condition(*cond, COND_L);
            break;
        }
        case dt_X86_INS_SETGE:
        case dt_X86_INS_CMOVGE:
        case dt_X86_INS_JGE:
        {
            latx_set_eflag_condition(*cond, COND_GE);
            break;
        }
        /* ZF SF OF */
        case dt_X86_INS_SETLE:
        case dt_X86_INS_CMOVLE:
        case dt_X86_INS_JLE:
        {
            latx_set_eflag_condition(*cond, COND_LE);
            break;
        }
        case dt_X86_INS_SETG:
        case dt_X86_INS_CMOVG:
        case dt_X86_INS_JG:
        {
            latx_set_eflag_condition(*cond, COND_G);
            break;
        }
    default: {
        lsassertm(0, "%s for %s is not implemented\n", __func__,
                  ir1_name(ir1_opcode(pir1)));
    }
    }
}
