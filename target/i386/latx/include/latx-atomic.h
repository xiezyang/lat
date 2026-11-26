/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LATX_ATOMIC_H
#define LATX_ATOMIC_H

#include <stdint.h>

/* Explicit opt-in: AM* and LL/SC preserve atomicity at unaligned addresses
 * within a 64-byte cache line. Instruction availability is checked separately.
 */
extern int option_unaligned_atomic;

/* A deliberate software trap, not an address-alignment fault. */
#define LATX_CMPXCHG16B_BREAK_CODE 0x4c41
#define LATX_CMPXCHG16B_BREAK (0x002a0000 | LATX_CMPXCHG16B_BREAK_CODE)

static inline int latx_is_cmpxchg16b_break(const uint32_t *inst)
{
    return inst[0] == LATX_CMPXCHG16B_BREAK &&
           (inst[1] & 0xfffffc00) == 0x38608000;
}

#endif
