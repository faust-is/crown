/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

/*

2.4 kbps MELP Proposed Federal Standard speech coder

Fixed-point C code, version 1.0

Copyright (c) 1998, Texas Instruments, Inc.

Texas Instruments has intellectual property rights on the MELP
algorithm.	The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).

The fixed-point version of the voice codec Mixed Excitation Linear
Prediction (MELP) is based on specifications on the C-language software
simulation contained in GSM 06.06 which is protected by copyright and
is the property of the European Telecommunications Standards Institute
(ETSI). This standard is available from the ETSI publication office
tel. +33 (0)4 92 94 42 58. ETSI has granted a license to United States
Department of Defense to use the C-language software simulation contained
in GSM 06.06 for the purposes of the development of a fixed-point
version of the voice codec Mixed Excitation Linear Prediction (MELP).
Requests for authorization to make other use of the GSM 06.06 or
otherwise distribute or modify them need to be addressed to the ETSI
Secretariat fax: +33 493 65 47 16.

*/

/* mat_lib.c: Matrix and vector manipulation library                          */

#include <assert.h>

#include "sc1200.h"
#include "mathhalf.h"
#include "mat_lib.h"
#include <string.h>
#include <stdlib.h>

/***************************************************************************
 *
 *	 FUNCTION NAME: v_add
 *
 *	 PURPOSE:
 *
 *	   Perform the addition of the two 16 bit input vector with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   vec2 		   16 bit short signed integer (int16_t) vector whose
 *					   values falls in the range
 *					   0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the addition of the two 16 bit input vectors with
 *	   saturation.
 *
 *	   vec1 = vec1 + vec2
 *
 *	   vec1[] is set to 0x7fff if the operation results in an
 *	   overflow.  vec1[] is set to 0x8000 if the operation results
 *	   in an underflow.
 *
 *	 KEYWORDS: add, addition
 *
 *************************************************************************/

int16_t *v_add(int16_t vec1[], const int16_t vec2[], int16_t n)
{
	register int16_t i;
#ifdef POSIX_TI_EMULATION
	for (i = 0; i < n; i++) {
	     *vec1 = melpe_add(*vec1, *vec2);
	     vec1++;
	     vec2++;
	}
#else
/* optimize to use 16-bit packed addition. Assume n=2k */
	register int32_t* v1;
	register int32_t* v2;
        int16_t count32 = (n >> 1);
	_nassert((n & 0x1) == 0);
	v1 = (int32_t*) vec1;
	v2 = (int32_t*) vec2;
	for (i = 0; i < count32; i++) {
		_mem4(v1) = _sadd2(_mem4(v1), _mem4_const(v2));
		v1++; v2++;
	}
#endif
	return (vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_add
 *
 *	 PURPOSE:
 *
 *	   Perform the addition of the two 32 bit input vector with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   L_vec1		   32 bit long signed integer (int32_t) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= L_vec1 <= 0x7fff ffff.
 *
 *	   L_vec2		   32 bit long signed integer (int32_t) vector whose
 *					   values falls in the range
 *					   0x8000 0000 <= L_vec2 <= 0x7fff ffff.
 *
 *	   n			   size of input vectors.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_vec1		   32 bit long signed integer (int32_t) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= L_vec1[] <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the addition of the two 32 bit input vectors with
 *	   saturation.
 *
 *	   L_vec1 = L_vec1 + L_vec2
 *
 *	   L_vec1[] is set to 0x7fff ffff if the operation results in an
 *	   overflow.  L_vec1[] is set to 0x8000 0000 if the operation results
 *	   in an underflow.
 *
 *	 KEYWORDS: add, addition
 *
 *************************************************************************/

int32_t *L_v_add(int32_t L_vec1[], int32_t L_vec2[], int16_t n)
{
	register int16_t i;

	for (i = 0; i < n; i++) {
		*L_vec1 = melpe_L_add(*L_vec1, *L_vec2);
		L_vec1++;
		L_vec2++;
	}
	return (L_vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_equ
 *
 *	 PURPOSE:
 *
 *	   Copy the contents of one 16 bit input vector to another
 *
 *	 INPUTS:
 *
 *	   vec2 		   16 bit short signed integer (int16_t) vector whose
 *					   values falls in the range
 *					   0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vector
 *
 *	 OUTPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Copy the contents of one 16 bit input vector to another
 *
 *	   vec1 = vec2
 *
 *	 KEYWORDS: equate, copy
 *
 *************************************************************************/
int16_t *v_equ(int16_t vec1[], const int16_t vec2[], int16_t n)
{
#ifdef MEMORY_OVERLAP_CHECK
	if (vec2 >= vec1) {
		if (vec2 < (vec1 + n))
			printf("V_EQU: memory overlap: v1=%p, v2=%p, n=%d\n", (void*) vec1, (void*) vec2, n);
	} else {
		if ((vec2 + n) > vec1)
			printf("V_EQU: memory overlap: v1=%p, v2=%p, n=%d\n", (void*) vec1, (void*) vec2, n);
	}
#endif
	memcpy(vec1, vec2, n << 1);
	return (vec1);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_equ_shr
 *
 *	 PURPOSE:
 *
 *	   Copy the contents of one 16 bit input vector to another with shift
 *
 *	 INPUTS:
 *
 *	   vec2 		   16 bit short signed integer (int16_t) vector whose
 *					   values falls in the range
 *					   0xffff 8000 <= vec2 <= 0x0000 7fff.
 *	   scale		   right shift factor
 *	   n			   size of input vector
 *
 *	 OUTPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Copy the contents of one 16 bit input vector to another with shift
 *
 *	   vec1 = vec2>>scale
 *
 *	 KEYWORDS: equate, copy
 *
 *************************************************************************/

int16_t *v_equ_shr(int16_t vec1[], int16_t vec2[], int16_t scale,
		     int16_t n)
{
	register int16_t i;

	for (i = 0; i < n; i++) {
		*vec1 = melpe_shr(*vec2, scale);
		vec1++;
		vec2++;
	}
	return (vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_equ
 *
 *	 PURPOSE:
 *
 *	   Copy the contents of one 32 bit input vector to another
 *
 *	 INPUTS:
 *
 *	   L_vec2		   32 bit long signed integer (int32_t) vector whose
 *					   values falls in the range
 *					   0x8000 0000 <= L_vec2 <= 0x7fff ffff.
 *
 *	   n			   size of input vector
 *
 *	 OUTPUTS:
 *
 *	   L_vec1		   32 bit long signed integer (int32_t) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= L_vec1 <= 0x7fff ffff.
 *
 *	 RETURN VALUE:
 *
 *	   L_vec1		   32 bit long signed integer (int32_t) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= L_vec1[] <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Copy the contents of one 32 bit input vector to another
 *
 *	   vec1 = vec2
 *
 *	 KEYWORDS: equate, copy
 *
 *************************************************************************/

int32_t *L_v_equ(int32_t L_vec1[], int32_t L_vec2[], int16_t n)
{
	memcpy(L_vec1, L_vec2, n << 2);
#ifdef MEMORY_OVERLAP_CHECK
	if (L_vec2 >= L_vec1) {
		if (L_vec2 < (L_vec1 + n))
			printf("V_EQU: memory overlap: v1=%p, v2=%p, n=%d\n", (void*) L_vec1, (void*) L_vec2, n);
	} else {
		if ((L_vec2 + n) > L_vec1)
			printf("V_EQU: memory overlap: v1=%p, v2=%p, n=%d\n", (void*) L_vec1, (void*) L_vec2, n);
	}
#endif
	return (L_vec1);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_inner
 *
 *	 PURPOSE:
 *
 *	   Compute the inner product of two 16 bit input vectors
 *	   with saturation and truncation.	Output is a 16 bit number.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   vec2 		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors
 *
 *	   qvec1		   Q value of vec1
 *
 *	   qvec2		   Q value of vec2
 *
 *	   qout 		   Q value of output
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   innerprod	   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= innerprod <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Compute the inner product of the two 16 bit input vectors.
 *	   The output is a 16 bit number.
 *
 *	 KEYWORDS: inner product
 *
 *************************************************************************/

int16_t v_inner(int16_t vec1[], int16_t vec2[], int16_t n,
		  int16_t qvec1, int16_t qvec2, int16_t qout)
{
	register int16_t i;
	int16_t innerprod;
	int32_t L_temp;

	L_temp = 0;
	for (i = 0; i < n; i++) {
		L_temp = melpe_L_mac(L_temp, *vec1, *vec2);
		vec1++;
		vec2++;
	}

	/* (qvec1 + qvec2 + 1) is the Q value from L_mult(vec1[i], vec2[i]), and  */
	/* also that for L_temp.  To make it Q qout, L_shl() it by                */
	/* (qout - (qvec1 + qvec2 + 1)).  To return only a int16_t, use         */
	/* extract_h() after L_shl() by 16.                                       */

	innerprod = melpe_extract_h(melpe_L_shl(L_temp,
				    (int16_t) (qout - ((qvec1 + qvec2 + 1) -
							 16))));
	return (innerprod);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_inner
 *
 *	 PURPOSE:
 *
 *	   Compute the inner product of two 16 bit input vectors
 *	   with saturation and truncation.	Output is a 32 bit number.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   vec2 		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors
 *
 *	   qvec1		   Q value of vec1
 *
 *	   qvec2		   Q value of vec2
 *
 *	   qout 		   Q value of output
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_innerprod	   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_innerprod <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Compute the inner product of the two 16 bit vectors
 *	   The output is a 32 bit number.
 *
 *	 KEYWORDS: inner product
 *
 *************************************************************************/

int32_t L_v_inner(int16_t vec1[], int16_t vec2[], int16_t n,
		   int16_t qvec1, int16_t qvec2, int16_t qout)
{
	register int16_t i;
	int16_t shift;
	int32_t L_innerprod, L_temp;

	L_temp = 0;
	for (i = 0; i < n; i++) {
		L_temp = melpe_L_mac(L_temp, *vec1, *vec2);
		vec1++;
		vec2++;
	}

	/* L_temp is now (qvec1 + qvec2 + 1) */
	shift = melpe_sub(qout, melpe_add(melpe_add(qvec1, qvec2), 1));
	L_innerprod = melpe_L_shl(L_temp, shift);
	return (L_innerprod);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_magsq
 *
 *	 PURPOSE:
 *
 *	   Compute the sum of square magnitude of a 16 bit input vector
 *	   with saturation and truncation.	Output is a 16 bit number.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors
 *
 *	   qvec1		   Q value of vec1
 *
 *	   qout 		   Q value of output
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   magsq		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= magsq <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Compute the sum of square magnitude of a 16 bit input vector.
 *	   The output is a 16 bit number.
 *
 *	 KEYWORDS: square magnitude
 *
 *************************************************************************/

int16_t v_magsq(int16_t vec1[], int16_t n, int16_t qvec1,
		  int16_t qout)
{
	register int16_t i;
	int16_t shift;
	int16_t magsq;
	int32_t L_temp;

	L_temp = 0;
	for (i = 0; i < n; i++) {
		L_temp = melpe_L_mac(L_temp, *vec1, *vec1);
		vec1++;
	}
	/* qout - ((2*qvec1 + 1) - 16) */
	shift = melpe_sub(qout, melpe_sub(melpe_add(melpe_shl(qvec1, 1), 1), 16));
	magsq = melpe_extract_h(melpe_L_shl(L_temp, shift));
	return (magsq);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_magsq
 *
 *	 PURPOSE:
 *
 *	   Compute the sum of square magnitude of a 16 bit input vector
 *	   with saturation and truncation.	Output is a 32 bit number.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors
 *
 *	   qvec1		   Q value of vec1
 *
 *	   qout 		   Q value of output
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_magsq		   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_magsq <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Compute the sum of square magnitude of a 16 bit input vector.
 *	   The output is a 32 bit number.
 *
 *	 KEYWORDS: square magnitude
 *
 *************************************************************************/

int32_t L_v_magsq(int16_t vec1[], int16_t n, int16_t qvec1,
		   int16_t qout)
{
	register int16_t i;
	int16_t shift;
	int32_t L_magsq, L_temp;

	L_temp = 0;
	for (i = 0; i < n; i++) {
		L_temp = melpe_L_mac(L_temp, *vec1, *vec1);
		vec1++;
	}
	/* ((qout-16)-((2*qvec1+1)-16)) */
	shift = melpe_sub(melpe_sub(qout, melpe_shl(qvec1, 1)), 1);
	L_magsq = melpe_L_shl(L_temp, shift);
	return (L_magsq);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_scale
 *
 *	 PURPOSE:
 *
 *	   Perform a multipy of the 16 bit input vector with a 16 bit input
 *	   scale with saturation and truncation.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *	   scale
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	   n			   size of vec1
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform a multipy of the 16 bit input vector with the 16 bit input
 *	   scale.  The output is a 16 bit vector.
 *
 *	 KEYWORDS: scale
 *
 *************************************************************************/

int16_t *v_scale(int16_t vec1[], int16_t scale, int16_t n)
{
	register int16_t i;

	for (i = 0; i < n; i++) {
		*vec1 = melpe_mult(*vec1, scale);
		vec1++;
	}
	return (vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_scale_shl
 *
 *	 PURPOSE:
 *
 *	   Perform a multipy of the 16 bit input vector with a 16 bit input
 *	   scale and shift to left with saturation and truncation.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   scale		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	   n			   size of vec1
 *
 *	   shift		   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0x0000 0000 <= var1 <= 0x0000 1f.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform a multipy of the 16 bit input vector with the 16 bit input
 *	   scale.  The output is a 16 bit vector.
 *
 *	 KEYWORDS: scale
 *
 *************************************************************************/

int16_t *v_scale_shl(int16_t vec1[], int16_t scale, int16_t n,
		       int16_t shift)
{
	register int16_t i;

	for (i = 0; i < n; i++) {
		*vec1 = melpe_extract_h(melpe_L_shl(melpe_L_mult(*vec1, scale), shift));
		vec1++;
	}
	return (vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_sub
 *
 *	 PURPOSE:
 *
 *	   Perform the subtraction of the two 16 bit input vector with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   vec2 		   16 bit short signed integer (int16_t) vector whose
 *					   values falls in the range
 *					   0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the subtraction of the two 16 bit input vectors with
 *	   saturation.
 *
 *	   vec1 = vec1 - vec2
 *
 *	   vec1[] is set to 0x7fff if the operation results in an
 *	   overflow.  vec1[] is set to 0x8000 if the operation results
 *	   in an underflow.
 *
 *	 KEYWORDS: sub, subtraction
 *
 *************************************************************************/

int16_t *v_sub(int16_t vec1[], const int16_t vec2[], int16_t n)
{
	register int16_t i;
#ifdef POSIX_TI_EMULATION
	for (i = 0; i < n; i++) {
	    *vec1 = melpe_sub(*vec1, *vec2);
	    vec1++;
	    vec2++;
	}
#else
/* optimize to use 16-bit packed subtraction */
	register int32_t* v1;
	register int32_t* v2;
	int16_t count32 = (n >> 1);

	v1 = (int32_t*) vec1;
	v2 = (int32_t*) vec2;
	_nassert((n & 0x1) == 0);
	for (i = 0; i < count32; i++) {
		_mem4(v1) = _ssub2(_mem4(v1), _mem4_const(v2));
		v1++;v2++;
	}
#endif
	return (vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_zap
 *
 *	 PURPOSE:
 *
 *	   Set the elements of a 16 bit input vector to zero.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   n			   size of vec1.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (int16_t) vector whose
 *					   values are equal to 0x0000 0000.
 *
 *	 IMPLEMENTATION:
 *
 *	   Set the elements of 16 bit input vector to zero.
 *
 *	   vec1 = 0
 *
 *	 KEYWORDS: zap, clear, reset
 *
 *************************************************************************/
int16_t *v_zap(int16_t vec1[], int16_t n)
{
	memset(vec1, 0x00, n << 1);
	return (vec1);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_zap
 *
 *	 PURPOSE:
 *
 *	   Set the elements of a 32 bit input vector to zero.
 *
 *	 INPUTS:
 *
 *	   L_vec1		   32 bit long signed integer (int32_t) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= vec1 <= 0x7fff ffff.
 *
 *	   n			   size of L_vec1.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_vec1		   32 bit long signed integer (int32_t) vector whose
 *					   values are equal to 0x0000 0000.
 *
 *	 IMPLEMENTATION:
 *
 *	   Set the elements of 32 bit input vector to zero.
 *
 *	   L_vec1 = 0
 *
 *	 KEYWORDS: zap, clear, reset
 *
 *************************************************************************/

int32_t *L_v_zap(int32_t L_vec1[], int16_t n)
{
	memset(L_vec1, 0x00, n << 2);
	return (L_vec1);
}

int16_t *v_get(int16_t n)
{
	int16_t *ptr;
	ptr = (int16_t *) malloc(n << 1);
	assert(ptr != NULL);
	return (ptr);
}

int32_t *L_v_get(int16_t n)
{
	int32_t *ptr;
	ptr = (int32_t *) malloc(n << 2);
	assert(ptr != NULL);
	return (ptr);
}

void v_free(void *v)
{
	if (v)
		free(v);
}
