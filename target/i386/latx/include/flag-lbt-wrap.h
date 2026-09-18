/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LATX_FLAG_LBT_WRAP_H
#define LATX_FLAG_LBT_WRAP_H

/* Include only after la-append.h has declared the raw emitters. */
#ifndef LATX_INTERNAL_RAW_LBT
#define la_x86mtflag(value, mask) latx_write_eflags((value), (mask))
#define la_x86mfflag(value, mask) latx_read_eflags((value), (mask))
#define la_setx86j(dest, condition) \
    latx_set_eflag_condition((dest), (condition))
#define la_x86mftop(value) latx_read_top((value))
#define la_x86inctop() latx_inc_top()
#define la_x86dectop() latx_dec_top()
#endif

#endif
