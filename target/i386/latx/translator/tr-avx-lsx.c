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

static bool translate_avx_fp_scalar_lsx(IR1_INST *pir1,
                                        IR2_INST *(*tr_inst)(IR2_OPND,
                                                             IR2_OPND,
                                                             IR2_OPND),
                                        bool is_double,
                                        bool track_fp_status);

bool translate_vmulsd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_lsx(pir1, la_vfmul_d, true, false);
}

bool translate_vdivsd_lsx(IR1_INST *pir1)
{
    return translate_avx_fp_scalar_lsx(pir1, la_vfdiv_d, true, false);
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
            src2_low = load_v128_from_ir1_mem_exact(opnd2);
            lsx_op(result_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
            ra_free_temp(result_low);
            ra_free_temp(src1_low);
            ra_free_temp(src2_low);

            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high;
            src2_high = load_v256_high_from_ir1_mem_exact(opnd2);
            IR2_OPND result_high = ra_alloc_ftemp();
            lsx_op(result_high, src1_high, src2_high);
            store_ymm_high128_shadow(result_high, dest_index);
            ra_free_temp(src1_high);
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
            la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
            ra_free_temp(result_low);
            ra_free_temp(src1_low);
            ra_free_temp(src2_low);

            IR2_OPND src1_high = load_ymm_high128_shadow(src1_index);
            IR2_OPND src2_high = load_ymm_high128_shadow(src2_index);
            IR2_OPND result_high = ra_alloc_ftemp();
            lsx_op(result_high, src1_high, src2_high);
            store_ymm_high128_shadow(result_high, dest_index);
            ra_free_temp(src1_high);
            ra_free_temp(src2_high);
            ra_free_temp(result_high);
        } else {
            lsx_op(result_low, src1_low, src2_low);
            la_vori_b(ra_alloc_xmm(dest_index), result_low, 0);
            clear_ymm_high128_shadow(dest_index);
        }
        if (!ir1_opnd_is_ymm(opnd0)) {
            ra_free_temp(src2_low);
        }
    }
    if (!ir1_opnd_is_ymm(opnd0)) {
        ra_free_temp(result_low);
        ra_free_temp(src1_low);
    }
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
        ra_free_temp(zero_low);
        return false;
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
    IR2_OPND flags = ra_alloc_itemp();
    IR2_OPND n4095_opnd = ra_alloc_num_4095();
    IR2_OPND zf_done = ra_alloc_label();
    IR2_OPND cf_done = ra_alloc_label();

    lsassert(ir1_opnd_is_xmm(dest_opnd) || ymm);
    lsassert(ir1_opnd_is_xmm(src_opnd) == !ymm ||
             ir1_opnd_is_ymm(src_opnd) == ymm ||
             ir1_opnd_is_mem(src_opnd));
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
    la_x86mfflag(flags, 0x3f);
    la_bstrins_w(flags, zero_ir2_opnd, CF_BIT_INDEX, CF_BIT_INDEX);
    la_bstrins_w(flags, zero_ir2_opnd, ZF_BIT_INDEX, ZF_BIT_INDEX);
    la_vseteqz_v(fcc0_ir2_opnd, and_result);
    la_bceqz(fcc0_ir2_opnd, zf_done);
    la_bstrins_w(flags, n4095_opnd, ZF_BIT_INDEX, ZF_BIT_INDEX);
    la_label(zf_done);

    la_vseteqz_v(fcc0_ir2_opnd, andn_result);
    la_bceqz(fcc0_ir2_opnd, cf_done);
    la_bstrins_w(flags, n4095_opnd, CF_BIT_INDEX, CF_BIT_INDEX);
    la_label(cf_done);

    la_x86mtflag(flags, 0x3f);
    ra_free_temp(flags);
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

static bool translate_avx_vtest_lsx(IR1_INST *pir1, bool double_precision)
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
    load_avx_lsx_operand(dest_opnd, ymm, &dest_low, &dest_high);
    load_avx_lsx_operand(src_opnd, ymm, &src_low, &src_high);

    la_vand_v(and_result, dest_low, src_low);
    la_vandn_v(andn_result, dest_low, src_low);
    if (double_precision) {
        la_vsrli_d(and_result, and_result, 0x3f);
        la_vsrli_d(andn_result, andn_result, 0x3f);
    } else {
        la_vsrli_w(and_result, and_result, 0x1f);
        la_vsrli_w(andn_result, andn_result, 0x1f);
    }
    if (ymm) {
        la_vand_v(half_result, dest_high, src_high);
        if (double_precision) {
            la_vsrli_d(half_result, half_result, 0x3f);
        } else {
            la_vsrli_w(half_result, half_result, 0x1f);
        }
        la_vor_v(and_result, and_result, half_result);

        la_vandn_v(half_result, dest_high, src_high);
        if (double_precision) {
            la_vsrli_d(half_result, half_result, 0x3f);
        } else {
            la_vsrli_w(half_result, half_result, 0x1f);
        }
        la_vor_v(andn_result, andn_result, half_result);
    }

    la_x86mtflag(zero_ir2_opnd, 0x3f);
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

bool translate_vtestps_lsx(IR1_INST *pir1)
{
    return translate_avx_vtest_lsx(pir1, false);
}

bool translate_vtestpd_lsx(IR1_INST *pir1)
{
    return translate_avx_vtest_lsx(pir1, true);
}

typedef IR2_INST *(*avx_lsx_lane_3op_fn)(IR2_OPND, IR2_OPND, IR2_OPND);
typedef IR2_INST *(*avx_lsx_narrow_fn)(IR2_OPND, IR2_OPND, int);
