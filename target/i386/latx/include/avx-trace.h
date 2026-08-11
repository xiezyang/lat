/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LATX_AVX_TRACE_H
#define LATX_AVX_TRACE_H

#include "ir1.h"

void latx_avx_trace_init(void);
void latx_avx_trace_flush(void);
void latx_avx_trace_instrument(IR1_INST *ir1);
void latx_avx_trace_record_cpuid(uint32_t leaf, uint32_t subleaf,
                                uint32_t eax, uint32_t ebx,
                                uint32_t ecx, uint32_t edx);
void latx_avx_trace_record_xgetbv(uint32_t index, uint64_t value,
                                 bool allowed);
void latx_avx_trace_hit(uint64_t guest_pc, uint64_t metadata,
                        uint64_t opcode, uint64_t bytes_lo,
                        uint64_t bytes_hi, uint64_t insn_size);

#endif
