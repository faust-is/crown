
/************************************************************************
*
*	DESCRIPTION		:     OPERATOR PROTOTYPES for TETRA codec
*
************************************************************************/

#ifndef TETRA_OP_H
#define TETRA_OP_H


/*-----------------------*
 * Constants and Globals *
 *-----------------------*/


#define MAX_32 (Word32)0x7fffffff
#define MIN_32 (Word32)0x80000000

#define MAX_16 (Word16)0x7fff
#define MIN_16 (Word16)0x8000

#define INLINE_PREFIX static inline

/*-----------------------*
 * Operators prototypes  *
 *-----------------------*/
INLINE_PREFIX Word16 abs_s(Word16 var1) {
	/* Short abs,           1 */
	return (Word16) _abs2(var1);
}

INLINE_PREFIX Word16 add(Word16 var1, Word16 var2) {
	return (Word16) _sadd2((int)var1, (int)var2);
}

INLINE_PREFIX Word16 sub(Word16 var1, Word16 var2) {
	return (Word16) _ssub2((int) var1, (int) var2);
}

INLINE_PREFIX Word32 L_abs(Word32 L_var1) {
	return _abs(L_var1);
}

INLINE_PREFIX Word32 L_add(Word32 L_var1, Word32 L_var2) {
	return _sadd(L_var1, L_var2);
}

INLINE_PREFIX Word32 L_sub(Word32 L_var1, Word32 L_var2) {
	return _ssub(L_var1, L_var2);
}

INLINE_PREFIX Word16 extract_h(Word32 L_var1) {
	/* extract upper 16 bits  */
	return (unsigned) (L_var1)>>16;
}

INLINE_PREFIX Word16 extract_l(Word32 L_var1) {
	/* extract lower 16 bits  */
	return (L_var1 & 0xffff);
}

INLINE_PREFIX Word16 negate(Word16 var1) {
	return (Word16) _ssub2(0, (int) var1);
}

INLINE_PREFIX Word16 norm_l(Word32 L_var1) {
	return ((L_var1) == 0 ? 0 : _norm(L_var1)); 
}

INLINE_PREFIX Word16 norm_s(Word16 var1) {
	return ((var1) == 0 ? 0 : _norm(var1)-16);
}

INLINE_PREFIX Word32 L_negate(Word32 L_var1) {
	return _ssub(0, L_var1);
}

INLINE_PREFIX Word32 L_deposit_h(Word16 var1) {
	/* put short in upper 16  */
	return (var1)<<16;
}

INLINE_PREFIX Word32 L_deposit_l(Word16 var1) {
	/* put short in lower 16  */
	return (Word32) var1;
}

INLINE_PREFIX Word16 mult(Word16 var1, Word16 var2) {
	return _smpy(var1,var2) >> 16;
}

INLINE_PREFIX Word32 L_mult(Word16 var1, Word16 var2) {
	return _smpy(var1,var2);
}


INLINE_PREFIX Word16 div_s(Word16 var1, Word16 var2)
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


INLINE_PREFIX Word16 r_ound(Word32 L_var1) {
	return (unsigned) _sadd(L_var1, 0x8000) >> 16;
}

INLINE_PREFIX Word16 shl(Word16 var1, Word16 var2) {
	/*   NOTE that C operators << and >> have restrictions on shift count.
	*   So use we should use sshvr/sshvl intrinisics as var2 is unbounded.
	*/
	return  _spack2(0, _sshvl(var1, var2));
}

INLINE_PREFIX Word16 shr(Word16 var1, Word16 var2) {
	/*   NOTE that C operators << and >> have restrictions on shift count.
	*   So use we should sshvr/sshvl intrinisics as var2 is unbounded.
	*/
	return _spack2(0, _sshvr(var1, var2)); 
}


/* arithmetic shifts */
INLINE_PREFIX Word32 L_shr(Word32 L_var1, Word16 var2) {
	/*   NOTE that C operators << and >> have restrictions on shift count.
	*   So use we should sshvr/sshvl intrinisics as var2 is unbounded.
	*/
	return _sshvr(L_var1, var2);
}

INLINE_PREFIX Word32 L_shl(Word32 L_var1, Word16 var2) {
	/*   NOTE that C operators << and >> have restrictions on shift count.
	*   So use we should sshvr/sshvl intrinisics as var2 is unbounded.
	*/
	return _sshvl(L_var1, var2);
}


INLINE_PREFIX int32_t L_mac(int32_t L_var3, int16_t var1, int16_t var2) {
	return _sadd(L_var3,_smpy(var1,var2));
}

INLINE_PREFIX int32_t L_msu(int32_t L_var3, int16_t var1, int16_t var2) {
	/* saturated mpy & subtraction from accum  */
	return _ssub(L_var3,_smpy(var1,var2));
}

INLINE_PREFIX Word32 L_mult0(Word16 var1, Word16 var2)
{
	Word32 L_var_out;
	L_var_out = (Word32)var1 * (Word32)var2;
	return(L_var_out);
}

INLINE_PREFIX Word32 L_mac0(Word32 L_var3, Word16 var1, Word16 var2)
{
	return L_add(L_var3, L_mult0(var1,var2));
}

INLINE_PREFIX Word32 L_msu0(Word32 L_var3, Word16 var1, Word16 var2)
{
	return L_sub(L_var3,L_mult0(var1,var2));
}


INLINE_PREFIX Word32 L_add_overflow_test(Word32 L_var1, Word32 L_var2, Word16* p_overflow)
{
	Word32 L_var_out;
	L_var_out = L_var1 + L_var2;
	if (((L_var1 ^ L_var2) & MIN_32) == 0)
	{
		if ((L_var_out ^ L_var1) & MIN_32)
		{
			L_var_out = (L_var1 < 0) ? MIN_32 : MAX_32;
			*p_overflow = 1;
		}
	}
	return(L_var_out); /* TODO: use hw Overflow bit */
}


INLINE_PREFIX Word32 L_mac0_overflow_test(Word32 L_var3, Word16 var1, Word16 var2, Word16* p_overflow)
{
	return L_add_overflow_test(L_var3,L_mult0(var1,var2),p_overflow);
}


INLINE_PREFIX Word32 mpy_mix(Word16 hi1, Word16 lo1, Word16 lo2)
{
	return L_add(L_mult0(hi1, lo2), (L_mult0(lo1, lo2) >> 16) << 1);
}

#define L_shr_r(a,b) (L_shr((a),(b)) + ((b)>0 && (((a) & (1<<((b)-1))) != 0)))
#define mult_r(a,b)    (r_ound(L_mult(a,b)))


#endif
