
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
#define HW_OVERFLOW_BIT_SUPPORTED

#define BRANCH_PREDICTION

#ifdef BRANCH_PREDICTION
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

/*-----------------------*
 * Operators prototypes  *
 *-----------------------*/

INLINE_PREFIX Word32 L_abs(Word32 L_var1) {
	__asm__("cmp %0, #0\n\t"
		"sublt %0, #1\n\t"
		"eor %0, %0, %0, asr #31\n\t" : "+r" (L_var1));
   return L_var1;
}

INLINE_PREFIX Word16 abs_s(Word16 var1) {
	return L_abs(var1 << 16) >> 16;
}


INLINE_PREFIX Word32 L_add(Word32 L_var1, Word32 L_var2) {
	register int32_t out;
	__asm__("qadd %0, %1, %2" : "=r" (out) : "r" (L_var1), "r" (L_var2));
	return out;
}

INLINE_PREFIX Word32 L_sub(Word32 L_var1, Word32 L_var2) {
	register int32_t out;
	__asm__("qsub %0, %1, %2" : "=r" (out) : "r" (L_var1), "r" (L_var2));
	return out;
}

INLINE_PREFIX Word16 extract_h(Word32 L_var1) {
	/* extract upper 16 bits  */
	return (uint32_t) (L_var1)>>16;
}

INLINE_PREFIX Word16 extract_l(Word32 L_var1) {
	/* extract lower 16 bits  */
	return (L_var1 & 0xffff);
}

INLINE_PREFIX Word16 add(Word16 var1, Word16 var2) {
	return L_add(var1 << 16, var2 << 16) >> 16;
}

INLINE_PREFIX Word16 sub(Word16 var1, Word16 var2) {
	return L_sub(var1 << 16, var2 << 16) >> 16;
}

INLINE_PREFIX Word16 negate(Word16 var1) {
	return  (L_sub(0, ((var1)<<16)) >> 16);
}

INLINE_PREFIX Word16 _normalize(Word32 L_var1) {
	if (L_var1 < 0) L_var1 = ~L_var1;
	return  __builtin_clz(L_var1) - 1;
	/*
	__asm__ ("cmp %0, #0\n\t"
		"mvnlt %0, %0\n\t"
		"clz %0, %0\n\t"
		"sub %0, %0, #1\n\t"
		: "+r" (L_var1)
		::);
	return L_var1;
	*/
}

INLINE_PREFIX Word16 norm_l(Word32 L_var1) {
	if (unlikely(L_var1==0)) return 0; else return _normalize(L_var1);
}


INLINE_PREFIX Word16 norm_s(Word16 var1) {
	if (unlikely(var1==0)) return 0; else return _normalize(var1)-16;
}

INLINE_PREFIX Word32 L_negate(Word32 L_var1) {
	return L_sub(0, L_var1);
}

INLINE_PREFIX Word32 L_deposit_h(Word16 var1) {
	/* put short in upper 16  */
	return (var1)<<16;
}

INLINE_PREFIX Word32 L_deposit_l(Word16 var1) {
	/* put short in lower 16  */
	return (Word32) var1;
}

INLINE_PREFIX Word32 L_mult(Word16 var1, Word16 var2) {
	register int32_t out;
	__asm__("smulbb %0, %1, %2\n\t"
		"qadd %0,%0,%0" : "=r" (out) : "r" (var1), "r" (var2));
	return out;
}

INLINE_PREFIX Word16 mult(Word16 var1, Word16 var2) {
	return L_mult(var1,var2) >> 16;
}

INLINE_PREFIX Word16 r_ound(Word32 L_var1) {
	return (uint32_t) L_add(L_var1, 0x8000) >> 16;
}

/* arithmetic shifts */
INLINE_PREFIX int16_t S_shl_s_positive(int16_t var1, int16_t var2)
{
	int16_t swOut;
	int32_t L_Out;

	if (unlikely((var2 == 0 || var1 == 0))) {
		swOut = var1;
	} else  {
		if (likely(var2 < 15)) {
			L_Out = (int32_t) var1 << var2;
			swOut = (int16_t) L_Out;        /* copy low portion to swOut */
			/* overflow could have happened */
			if (swOut != L_Out) {   /* overflowed */
				swOut = (int16_t) ((var1 > 0) ? MAX_16 : MIN_16);
				/* saturate */
			}
		} else {
			swOut = (int16_t) ((var1 > 0) ? MAX_16 : MIN_16); /* saturate */
		}
	}
	return (swOut);
}

/* shift right with saturation: Qx->Q(x-t)        */
INLINE_PREFIX int16_t S_shr_s_positive(int16_t var1, int16_t var2)
{
	if (likely(var2 < 15)) return var1 >> var2;
	return ((var1 < 0) ? -1 : 0);
}

INLINE_PREFIX Word16 shl(Word16 var1, Word16 var2) {
	if (var2 >= 0) return S_shl_s_positive(var1, var2);
	else return S_shr_s_positive(var1, -var2);
}

INLINE_PREFIX Word16 shr(Word16 var1, Word16 var2) {
	if (var2 >= 0) return S_shr_s_positive(var1, var2);
	else return S_shl_s_positive(var1, -var2);
}

/* shift left with saturation: Qx->Q(x-t)        */
INLINE_PREFIX int32_t L_shl_l_positive(int32_t L_var1, int16_t var2)
{
	int32_t L_Out;
	int64_t ACC;
	if (unlikely((var2 == 0 || L_var1 == 0))) {
		L_Out = L_var1;
	} else  {               /* var2 > 0 */
		if (unlikely(var2 >= 31)) {
			L_Out = (L_var1 > 0) ? MAX_32 : MIN_32; /* saturate */
		} else {
			ACC = (int64_t) L_var1 << var2;
			L_Out = (int32_t) ACC;        /* copy low portion to swOut */
			/* overflow could have happened */
			if (ACC != L_Out) {   /* overflowed */
				L_Out = (L_var1 > 0) ? MAX_32 : MIN_32;
				/* saturate */
			}
		}
	}
	return (L_Out);
}

/* shift right with saturation: Qx->Q(x-t)        */
INLINE_PREFIX int32_t L_shr_l_positive(int32_t L_var1, int16_t var2)
{
	if (likely(var2 < 31)) return L_var1 >> var2;
	return ((L_var1 < 0) ? -1 : 0);
}

INLINE_PREFIX int32_t L_shr(int32_t L_var1, int16_t var2)
{
	if (var2 >= 0) return L_shr_l_positive(L_var1, var2);
	else return L_shl_l_positive(L_var1, -var2);
}

INLINE_PREFIX int32_t L_shl(int32_t L_var1, int16_t var2)
{
	if (var2 >= 0) return L_shl_l_positive(L_var1, var2);
	else return L_shr_l_positive(L_var1, -var2);
}


INLINE_PREFIX int32_t L_mac(int32_t L_var3, int16_t var1, int16_t var2) {
	return L_add(L_var3,L_mult(var1,var2));
}

INLINE_PREFIX int32_t L_msu(int32_t L_var3, int16_t var1, int16_t var2) {
	/* saturated mpy & subtraction from accum  */
	return L_sub(L_var3,L_mult(var1,var2));
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

/************************************************************************
*
*	Function Name : mpy_mix
*
*	Purpose :
*
*		Multiply a 16 bit integer by a 32 bit (DPF). 
*		The result is divided by 2**16
*		L_32 = hi1*lo2 + (lo1*lo2)>>15
*
*	Complexity Weight : 4
*
*	Inputs :
*
*		hi1
*			hi part of 32 bit number
*
*		lo1
*			lo part of 32 bit number
*
*		lo2
*			16 bit number
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		L_var_out
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x0000 0000 <= L_var_out <= 0x7fff ffff.
*
************************************************************************/
INLINE_PREFIX Word32 mpy_mix(Word16 hi1, Word16 lo1, Word16 lo2) {
	register int32_t out;
	__asm__ ("smulbb %0, %2, %3\n\t"
		"asr %0, %0, #16\n\t"
		"lsl %0, %0, #1\n\t"
		"smlabb %0, %1, %3, %0\n\t" : "+r" (out) : "r" (hi1), "r" (lo1), "r" (lo2));
	return out;
}

/* Q bit (overflow) functions */
INLINE_PREFIX int32_t hw_reset_overflow() {
	register int32_t tmp;
	__asm__ volatile ("mrs %0, APSR\n\t"
		"bic %0, %0, #134217728\n\t"
		"msr APSR_nzcvq, %0"  : "=r" (tmp));
	return tmp;
}

INLINE_PREFIX int16_t hw_get_overflow() {
	register int32_t tmp;
	__asm__ volatile("mrs %0, APSR\n\t"
		"and %0, %0, #134217728\n\t"
		"lsr %0, %0, #27" : "=r" (tmp));
	return tmp;
}

#define mult_r(a,b)    (r_ound(L_mult(a,b)))
#define L_shr_r(a,b) (L_shr((a),(b)) + ((b)>0 && (((a) & (1<<((b)-1))) != 0)))

extern Word16 div_s(Word16 var1, Word16 var2); /* from tetra_op.s */

#endif
