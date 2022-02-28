

#ifndef _MATHHALF_H_
#define _MATHHALF_H_

#include "macro.h"
#include <stdlib.h>

/***************************************************************************************
 *
 *	File Name:  mathhalf.h
 *
 *	Purpose:  Contains inlined functions which implement the primitive
 *	arithmetic operations, optimized for Texas Instruments C674X. 
 *	Originally mathhalf.h was developed in GSM Half-rate speech coder,
 *	(GSM 06.06) which is protected by copyright and is the property of 
 *	the European Telecommunications Standards Institute (ETSI). 
 *	NOTE: TI compiler has own implementation of some GSM functions (gsm.h),
 *	but some of gsm.h macro are not compatible with old standard of GSM (06.06) 
 *	and we need C674X specific implementation.
 *	
 *
 *	The functions in this file are listed below.
 *
 *	COMPATIBILITY NOTES: 
 *	MELPe for C674X must be binary compatible with reference
 *	implementation of MELPe for POSIX, so these functions shoud guarantee
 *	the same result as corresponding mathhalf.h reference MELPe in the 
 *	scope of MELPe implementation, but not always guarantee the same result 
 *	in the full range of input data due to optimization reasons. Functions 
 *	that are known to be incompatible with the reference implementation 
 *	in the full range of input dataare marked "ETSI compatibility issue"
 *	
 *	When you change implementation of primitive(s) in this file, please ensure that 
 *	the binary compatibility of full MELPe coder is not broken for the full 
 *	range of input voice.
 *
 *	See also: original mathhalf.h from reference implementation.
 * ************************************************************************************/

/* addition and subtraction */
static inline int32_t melpe_L_add(int32_t L_var1, int32_t L_var2) {
	return _sadd(L_var1, L_var2);
}


static inline int16_t melpe_add(int16_t var1, int16_t var2) {
	return (int16_t) _sadd2((int)var1, (int)var2);
}

static inline int32_t melpe_L_sub(int32_t L_var1, int32_t L_var2) {
	return _ssub(L_var1, L_var2);
}

static inline int16_t melpe_sub(int16_t var1, int16_t var2) {
	return (int16_t) _ssub2((int) var1, (int) var2);
}

/* multiplication */
static inline int32_t melpe_L_mult(int16_t var1, int16_t var2) {
	return _smpy(var1,var2);
}

static inline int16_t melpe_mult(int16_t var1, int16_t var2) {
	return _smpy(var1,var2) >> 16;
}

/* arithmetic shifts */
static inline int32_t melpe_L_shr(int32_t L_var1, int16_t var2) {
	/*   NOTE that C operators << and >> have restrictions on shift count.
	 *   So use we should sshvr/sshvl intrinisics as var2 is unbounded.
	*/
	return _sshvr(L_var1, var2);
}

static inline int32_t melpe_L_shl(int32_t L_var1, int16_t var2) {
	/*   NOTE that C operators << and >> have restrictions on shift count.
	 *   So use we should sshvr/sshvl intrinisics as var2 is unbounded.
	*/
	return _sshvl(L_var1, var2);
}

static inline int16_t melpe_shr(int16_t var1, int16_t var2) {
	/*   NOTE that C operators << and >> have restrictions on shift count.
	 *   So use we should sshvr/sshvl intrinisics as var2 is unbounded.
	*/
	return _spack2(0, _sshvr(var1, var2)); 
}

static inline int16_t melpe_shl(int16_t var1, int16_t var2) {
	/*   NOTE that C operators << and >> have restrictions on shift count.
	 *   So use we should sshvr/sshvl intrinisics as var2 is unbounded.
	*/
	return  _spack2(0, _sshvl(var1, var2));
}

/* Accumulator manipulation */
static inline int16_t melpe_extract_h(int32_t L_var1) {
	/* extract upper 16 bits  */
	return (unsigned) (L_var1)>>16;
}

static inline int16_t melpe_extract_l(int32_t L_var1) {
	/* extract lower 16 bits  */
	return (L_var1 & 0xffff);
}

static inline int32_t melpe_L_deposit_h(int16_t var1) {
	/* put short in upper 16  */
	return (var1)<<16;
}

static inline int32_t melpe_L_deposit_l(int16_t var1) {
	/* put short in lower 16  */
	return (int32_t) var1;
}

/* Rounding */
static inline int16_t melpe_r_ound(int32_t L_var1) {
	return (unsigned) _sadd(L_var1, 0x8000) >> 16;
}

/*  Shift and r_ound */
static inline int16_t melpe_shift_r(int16_t var1, int16_t var2) {
	/*  ETSI compatibility issue:
	 *  1 <= var2  <=15 
	 *  (restricted by MELPe implementation) 
	 */
	return (var1+(1<<(var2-1))) >> var2;
}

/* absolute value  */
static inline int16_t melpe_abs_s(int16_t var1) {
	return (int16_t) _abs2(var1);
}

static inline int32_t melpe_L_abs(int32_t L_var1) {
	return _abs(L_var1);
}

/* multiply accumulate	*/
static inline int32_t melpe_L_mac(int32_t L_var3, int16_t var1, int16_t var2) {
	return _sadd(L_var3,_smpy(var1,var2));
}

static inline int32_t melpe_L_msu(int32_t L_var3, int16_t var1, int16_t var2) {
	/* saturated mpy & subtraction from accum  */
	return _ssub(L_var3,_smpy(var1,var2));
}

static inline int16_t melpe_msu_r(int32_t L_var3, int16_t var1, int16_t var2) {
	return melpe_r_ound(_ssub(L_var3,_smpy(var1,var2)));
}

/* negation  */
static inline int16_t melpe_negate(int16_t var1) {
	return (int16_t) _ssub2(0, (int) var1);
}

static inline int32_t melpe_L_negate(int32_t L_var1) {
	return _ssub(0, L_var1);
}

/* Normalization */
static inline int16_t melpe_norm_l(int32_t L_var1) {
	return ((L_var1) == 0 ? 0 : _norm(L_var1)); 
}

static inline int16_t melpe_norm_s(int16_t var1) {
	return ((var1) == 0 ? 0 : _norm(var1)-16);
}

/* Division */
static inline int16_t melpe_divide_s(int16_t var1, int16_t var2)
{
	register unsigned int var1int;
	register int          var2int;

	if (var1 <= 0 || var2 < 0 || var1 > var2)  return 0;
	if (var1 == var2) return 0x7fff;
	
	var1int = var1 << 16;
	var2int = var2 << 16;
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	var1int = _subc(var1int,var2int);
	return _subc(var1int,var2int) & 0xffff;
}

/* 40 bit basic operators (__int40_t or long) */

static inline long40_t melpe_L40_add(long40_t acc, int32_t L_var1) {
	return _lsadd(L_var1, acc);
}

static inline long40_t melpe_L40_mac(long40_t acc, int16_t var1, int16_t var2) {
	return _lsadd(_smpy(var1, var2), acc);
}

static inline long40_t melpe_L40_msu(long40_t acc, int16_t var1, int16_t var2) {
	return -_lssub(_smpy(var1, var2), acc);
}

static inline int16_t melpe_norm32(long40_t acc) {
	return (acc) == 0 ? 0 : _lnorm(acc)-8;
}

static inline int32_t melpe_L_sat32(long40_t acc) {
	return _sat(acc);
}

static inline long40_t melpe_L40_shl(long40_t acc, int16_t var1)
{
	/* ETSI compatibility issue:
	* not necessary to use 40-bit shift with saturation
	* as it is not hardware implemented in C6X and
	* we always check the norm of ACC before shift
	*/
	if (var1 > 0) return acc << var1; /* compiles to 40 bit SHL */ else
	if (var1 < 0) return acc >> (-var1); /* compiles to 40 bit SHR */ else
	return acc;
}

static inline long40_t melpe_L40_shr(long40_t acc, int16_t var1)
{
	/* ETSI compatibility issue:
	* not necessary to use 40-bit shift with saturation
	* as it is not hardware implemented in C6X and
	* we always check the norm of ACC before shift
	*/
	if (var1 > 0) return acc >> var1; /* compiles to 40 bit SHR */ else
	if (var1 < 0) return acc << (-var1); /* compiles to 40 bit SHL */ else
	return acc;
}

/* 16 bit x 16 bit unsigned multipy */
static inline uint32_t L_mpyu(uint16_t var1, uint16_t var2) 
{
	return _mpyu(var1, var2);
}

/* 32 bit x 16 bit signed multiply */
static inline int32_t L_mpy_ls(int32_t L_var2, int16_t var1) 
{
	return _sadd(_smpylh(var1, L_var2), _smpy(var1, _shru2(L_var2, 1)) >> 15);
}

/* Debugging */
#if OVERFLOW_CHECK
static inline void inc_saturation()
{
	saturation++;
}
#else
#define inc_saturation()
#endif

#endif
