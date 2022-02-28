
/***************************************************************************
 *
 *       File Name:  mathhalf.h
 *
 *       Purpose:  Contains functions which implement the primitive
 *         arithmetic operations with GCC optimizations (platform independent code).
 ****************************************************************************/

#ifndef _MATHHALF_H_
#define _MATHHALF_H_

/* addition */

static inline int16_t melpe_add(int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int16_t melpe_sub(int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int32_t melpe_L_add(int32_t L_var1, int32_t L_var2) __attribute__((always_inline));

static inline int32_t melpe_L_sub(int32_t L_var1, int32_t L_var2) __attribute__((always_inline));

/* multiplication */

static inline int16_t melpe_mult(int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int32_t melpe_L_mult(int16_t var1, int16_t var2) __attribute__((always_inline));

static inline uint32_t L_mpyu(uint16_t var1, uint16_t var2) __attribute__((always_inline));

static inline int32_t L_mpy_ls(int32_t L_var2, int16_t var1) __attribute__((always_inline));

/* arithmetic shifts */

static inline int16_t melpe_shr(int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int16_t melpe_shl(int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int32_t melpe_L_shr(int32_t L_var1, int16_t var2) __attribute__((always_inline));

static inline int32_t melpe_L_shl(int32_t L_var1, int16_t var2) __attribute__((always_inline));

static inline int16_t melpe_shift_r(int16_t var, int16_t var2) __attribute__((always_inline));

/* absolute value  */

static inline int16_t melpe_abs_s(int16_t var1) __attribute__((always_inline));

static inline int32_t melpe_L_abs(int32_t var1) __attribute__((always_inline));

/* multiply accumulate	*/

static inline int32_t melpe_L_mac(int32_t L_var3, int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int32_t melpe_L_msu(int32_t L_var3, int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int16_t melpe_msu_r(int32_t L_var3, int16_t var1, int16_t var2) __attribute__((always_inline));

/* negation  */

static inline int16_t melpe_negate(int16_t var1) __attribute__((always_inline));

static inline int32_t melpe_L_negate(int32_t L_var1) __attribute__((always_inline));

/* Accumulator manipulation */

static inline int32_t melpe_L_deposit_l(int16_t var1) __attribute__((always_inline));

static inline int32_t melpe_L_deposit_h(int16_t var1) __attribute__((always_inline));

static inline int16_t melpe_extract_l(int32_t L_var1) __attribute__((always_inline));

static inline int16_t melpe_extract_h(int32_t L_var1) __attribute__((always_inline));

/* r_ound */

static inline int16_t melpe_r_ound(int32_t L_var1) __attribute__((always_inline));

/* Normalization */

static inline int16_t melpe_norm_l(int32_t L_var1) __attribute__((always_inline));

static inline int16_t melpe_norm_s(int16_t var1) __attribute__((always_inline));

/* Division */

static inline int16_t melpe_divide_s(int16_t var1, int16_t var2) __attribute__((always_inline));

/* 40-Bit Routines */
static inline int64_t melpe_L40_add(int64_t acc, int32_t L_var1) __attribute__((always_inline));

static inline int64_t melpe_L40_sub(int64_t acc, int32_t L_var1) __attribute__((always_inline));

static inline int64_t melpe_L40_mac(int64_t acc, int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int64_t melpe_L40_msu(int64_t acc, int16_t var1, int16_t var2) __attribute__((always_inline));

static inline int64_t melpe_L40_shl(int64_t acc, int16_t var1)  __attribute__((always_inline));

static inline int64_t melpe_L40_shr(int64_t acc, int16_t var1) __attribute__((always_inline));

static inline int64_t melpe_L40_negate(int64_t acc) __attribute__((always_inline));

static inline int16_t melpe_norm32(int64_t acc) __attribute__((always_inline));
static inline int32_t melpe_L_sat32(int64_t acc) __attribute__((always_inline));


#include "mathhalf_i.h"

#endif
