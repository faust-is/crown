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

#include <stdlib.h>
#include <stdio.h>

#include "sc1200.h"
#include "constant.h"
#include "global.h"
#include "macro.h"

/***************************************************************************
 *
 *	 File Name:  mathhalf.c
 *
 *	 Purpose:  Contains functions which implement the primitive
 *	   arithmetic operations.
 *
 *		The functions in this file are listed below.  Some of them are
 *		defined in terms of other basic operations.  One of the
 *		routines, saturate() is static.  This is not a basic
 *		operation, and is not reference outside the scope of this
 *		file.
 *
 *
 *		 abs_s()
 *		 add()
 *		 divide_s()
 *		 extract_h()
 *		 extract_l()
 *		 L_abs()
 *		 L_add()
 *		 L_deposit_h()
 *		 L_deposit_l()
 *		 L_mac()
 *		 L_msu()
 *		 L_mult()
 *		 L_negate()
 *		 L_shift_r()
 *		 L_shl()
 *		 L_shr()
 *		 L_sub()
 *		 mac_r()
 *		 msu_r()
 *		 mult()
 *		 negate()
 *		 norm_l()
 *		 norm_s()
 *		 r_ound()
 *		 saturate()
 *		 shift_r()
 *		 shl()
 *		 shr()
 *		 sub()
 *
 **************************************************************************/

/***************************************************************************
 *
 *	 FUNCTION NAME: saturate
 *
 *	 PURPOSE:
 *
 *	   Limit the 32 bit input to the range of a 16 bit word.
 *
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 KEYWORDS: saturation, limiting, limit, saturate, 16 bits
 *
 *************************************************************************/

#if OVERFLOW_CHECK
static inline void inc_saturation()
{
	saturation++;
}
#else
#define inc_saturation()
#endif

static inline int16_t saturate(int32_t L_var1)
{
	int16_t swOut;

	if (L_var1 > SW_MAX) {
		swOut = SW_MAX;
		inc_saturation();
	} else if (L_var1 < SW_MIN) {
		swOut = SW_MIN;
		inc_saturation();
	} else
		swOut = (int16_t) L_var1;	/* automatic type conversion */
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: divide_s
 *
 *	 PURPOSE:
 *
 *	   Divide var1 by var2.  Note that both must be positive, and
 *	   var1 >= var2.  The output is set to 0 if invalid input is
 *	   provided.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   In the case where var1==var2 the function returns 0x7fff.  The output
 *	   is undefined for invalid inputs.  This implementation returns zero
 *	   and issues a warning via stdio if invalid input is presented.
 *
 *	 KEYWORDS: divide
 *
 *************************************************************************/

static inline int16_t melpe_divide_s(int16_t var1, int16_t var2)
{
	int32_t L_div;
	int16_t swOut;

	if (var1 < 0 || var2 < 0 || var1 > var2) {
		/* undefined output for invalid input into divide_s() */
		return ((int16_t) 0);
	}

	if (var1 == var2)
		return ((int16_t) 0x7fff);

	L_div = ((0x00008000L * (int32_t) var1) / (int32_t) var2);
	swOut = saturate(L_div);
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_deposit_l
 *
 *	 PURPOSE:
 *
 *	   Put the 16 bit input into the 16 LSB's of the output int32_t with
 *	   sign extension i.e. the top 16 bits are set to either 0 or 0xffff.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= L_var1 <= 0x0000 7fff.
 *
 *	 KEYWORDS: deposit, assign
 *
 *************************************************************************/

static inline int32_t melpe_L_deposit_l(int16_t var1)
{
	int32_t L_Out;

	L_Out = var1;
	return (L_Out);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_deposit_h
 *
 *	 PURPOSE:
 *
 *	   Put the 16 bit input into the 16 MSB's of the output int32_t.  The
 *	   LS 16 bits are zeroed.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff 0000.
 *
 *
 *	 KEYWORDS: deposit, assign, fractional assign
 *
 *************************************************************************/

static inline int32_t melpe_L_deposit_h(int16_t var1)
{
	int32_t L_var2;

	L_var2 = (int32_t) var1 << 16;
	return (L_var2);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: extract_l
 *
 *	 PURPOSE:
 *
 *	   Extract the 16 LS bits of a 32 bit int32_t.  Return the 16 bit
 *	   number as a int16_t.  The upper portion of the input int32_t
 *	   has no impact whatsoever on the output.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *
 *	 KEYWORDS: extract, assign
 *
 *************************************************************************/

static inline int16_t melpe_extract_l(int32_t L_var1)
{
	int16_t var2;

	var2 = (int16_t) (0x0000ffffL & L_var1);
	return (var2);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: extract_h
 *
 *	 PURPOSE:
 *
 *	   Extract the 16 MS bits of a 32 bit int32_t.  Return the 16 bit
 *	   number as a int16_t.  This is used as a "truncation" of a fractional
 *	   number.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	 KEYWORDS: assign, truncate
 *
 *************************************************************************/

static inline int16_t melpe_extract_h(int32_t L_var1)
{
	int16_t var2;

	var2 = (int16_t) (0x0000ffffL & (L_var1 >> 16));
	return (var2);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: r_ound
 *
 *	 PURPOSE:
 *
 *	   r_ound the 32 bit int32_t into a 16 bit shortword with saturation.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform a two's complement r_ound on the input int32_t with
 *	   saturation.
 *
 *	   This is equivalent to adding 0x0000 8000 to the input.  The
 *	   result may overflow due to the add.	If so, the result is
 *	   saturated.  The 32 bit r_ounded number is then shifted down
 *	   16 bits and returned as a int16_t.
 *
 *
 *	 KEYWORDS: r_ound
 *
 *************************************************************************/

static inline int16_t melpe_r_ound(int32_t L_var1)
{
	int32_t L_Prod;
	int16_t var2;

    L_Prod = melpe_L_add(L_var1, 0x00008000L);	/* r_ound MSP */
    var2 = melpe_extract_h(L_Prod);
	return (var2);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: negate
 *
 *	 PURPOSE:
 *
 *	   Negate the 16 bit input. 0x8000's negated value is 0x7fff.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8001 <= swOut <= 0x0000 7fff.
 *
 *	 KEYWORDS: negate, negative, invert
 *
 *************************************************************************/

static inline int16_t melpe_negate(int16_t var1)
{
	int16_t swOut;

	if (var1 == SW_MIN) {
		inc_saturation();
		swOut = SW_MAX;
	} else
		swOut = (int16_t) (-var1);
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_negate
 *
 *	 PURPOSE:
 *
 *	   Negate the 32 bit input. 0x8000 0000's negated value is
 *	   0x7fff ffff.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0001 <= L_var1 <= 0x7fff ffff.
 *
 *	 KEYWORDS: negate, negative
 *
 *************************************************************************/

static inline int32_t melpe_L_negate(int32_t L_var1)
{
	int32_t L_Out;

	if (L_var1 == LW_MIN) {
		inc_saturation();
		L_Out = LW_MAX;
	} else
		L_Out = -L_var1;
	return (L_Out);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: add
 *
 *	 PURPOSE:
 *
 *	   Perform the addition of the two 16 bit input variable with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the addition of the two 16 bit input variable with
 *	   saturation.
 *
 *	   swOut = var1 + var2
 *
 *	   swOut is set to 0x7fff if the operation results in an
 *	   overflow.  swOut is set to 0x8000 if the operation results
 *	   in an underflow.
 *
 *	 KEYWORDS: add, addition
 *
 *************************************************************************/

static inline int16_t melpe_add(int16_t var1, int16_t var2)
{
	int32_t L_sum;
	int16_t swOut;

	L_sum = (int32_t) var1 + var2;
	swOut = saturate(L_sum);
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_add
 *
 *	 PURPOSE:
 *
 *	   Perform the addition of the two 32 bit input variables with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *	   L_var2
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the addition of the two 32 bit input variables with
 *	   saturation.
 *
 *	   L_Out = L_var1 + L_var2
 *
 *	   L_Out is set to 0x7fff ffff if the operation results in an
 *	   overflow.  L_Out is set to 0x8000 0000 if the operation
 *	   results in an underflow.
 *
 *	 KEYWORDS: add, addition
 *
 *************************************************************************/
static inline int32_t melpe_L_add(int32_t L_var1, int32_t L_var2)
{
	int32_t L_Sum, L_SumLow, L_SumHigh;

	L_Sum = L_var1 + L_var2;

	if ((L_var1 > 0 && L_var2 > 0) || (L_var1 < 0 && L_var2 < 0)) {

		/* an overflow is possible */
		L_SumLow =
		    (L_var1 & (int32_t) 0xffff) + (L_var2 & (int32_t) 0xffff);
		L_SumHigh =
		    ((L_var1 >> 16) & (int32_t) 0xffff) +
		    ((L_var2 >> 16) & (int32_t) 0xffff);
		if (L_SumLow & (int32_t) 0x10000) {
			/* carry into high word is set */
			L_SumHigh++;
		}

		/* Update sum only if there is an overflow or underflow */
		if (((int32_t) 0x10000 & L_SumHigh) &&
		    !((int32_t) 0x8000 & L_SumHigh)) {
			inc_saturation();
			L_Sum = LW_MIN;	/* underflow */
		} else if (!((int32_t) 0x10000 & L_SumHigh) &&
			   ((int32_t) 0x8000 & L_SumHigh)) {
			inc_saturation();
			L_Sum = LW_MAX;	/* overflow */
		}
	}

	return (L_Sum);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: sub
 *
 *	 PURPOSE:
 *
 *	   Perform the subtraction of the two 16 bit input variable with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the subtraction of the two 16 bit input variable with
 *	   saturation.
 *
 *	   swOut = var1 - var2
 *
 *	   swOut is set to 0x7fff if the operation results in an
 *	   overflow.  swOut is set to 0x8000 if the operation results
 *	   in an underflow.
 *
 *	 KEYWORDS: sub, subtraction
 *
 *************************************************************************/
static inline int16_t melpe_sub(int16_t var1, int16_t var2)
{
	int32_t L_diff;
	int16_t swOut;

	L_diff = (int32_t) var1 - var2;
	swOut = saturate(L_diff);

	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_sub
 *
 *	 PURPOSE:
 *
 *	   Perform the subtraction of the two 32 bit input variables with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *	   L_var2
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the subtraction of the two 32 bit input variables with
 *	   saturation.
 *
 *	   L_Out = L_var1 - L_var2
 *
 *	   L_Out is set to 0x7fff ffff if the operation results in an
 *	   overflow.  L_Out is set to 0x8000 0000 if the operation
 *	   results in an underflow.
 *
 *	 KEYWORDS: sub, subtraction
 *
 *************************************************************************/
static inline int32_t melpe_L_sub(int32_t L_var1, int32_t L_var2)
{
	int32_t L_Sum;

	/* check for overflow */
	if ((L_var1 > 0 && L_var2 < 0) || (L_var1 < 0 && L_var2 > 0)) {
		if (L_var2 == LW_MIN) {
            L_Sum = melpe_L_add(L_var1, LW_MAX);
            L_Sum = melpe_L_add(L_Sum, 1);
		} else
            L_Sum = melpe_L_add(L_var1, -L_var2);
	} else			/* no overflow possible */
		L_Sum = L_var1 - L_var2;
	return (L_Sum);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: shr
 *
 *	 PURPOSE:
 *
 *	   Arithmetic shift right (or left).
 *	   Arithmetically shift the input right by var2.   If var2 is
 *	   negative then an arithmetic shift left (shl) of var1 by
 *	   -var2 is performed.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Arithmetically shift the input right by var2.  This
 *	   operation maintains the sign of the input number. If var2 is
 *	   negative then an arithmetic shift left (shl) of var1 by
 *	   -var2 is performed.	See description of shl for details.
 *
 *	   Equivalent to the Full-Rate GSM ">> n" operation.  Note that
 *	   ANSI-C does not guarantee operation of the C ">>" or "<<"
 *	   operator for negative numbers.
 *
 *	 KEYWORDS: shift, arithmetic shift right,
 *
 *************************************************************************/

static inline int16_t melpe_shr(int16_t var1, int16_t var2)
{
	int16_t swMask, swOut;

	if (var2 == 0 || var1 == 0)
		swOut = var1;
	else if (var2 < 0) {	/* perform an arithmetic left shift */
		if (var2 <= -15) {
			swOut = (int16_t) ((var1 > 0) ? SW_MAX : SW_MIN);
			/* saturate */
			inc_saturation();
		} else
            swOut = melpe_shl(var1, (int16_t) - var2);
	} else {		/* positive shift count */
		if (var2 >= 15)
			swOut = (int16_t) ((var1 < 0) ? -1 : 0);
		else {		/* take care of sign extension */
			swMask = 0;
			if (var1 < 0)
				swMask = (int16_t) (~swMask << (16 - var2));

			var1 >>= var2;
			swOut = (int16_t) (swMask | var1);
		}
	}
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: shl
 *
 *	 PURPOSE:
 *
 *	   Arithmetically shift the input left by var2.
 *
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   If Arithmetically shift the input left by var2.	If var2 is
 *	   negative then an arithmetic shift right (shr) of var1 by
 *	   -var2 is performed.	See description of shr for details.
 *	   When an arithmetic shift left is performed the var2 LS bits
 *	   are zero filled.
 *
 *	   The only exception is if the left shift causes an overflow
 *	   or underflow.  In this case the LS bits are not modified.
 *	   The number returned is 0x8000 in the case of an underflow or
 *	   0x7fff in the case of an overflow.
 *
 *	   The shl is equivalent to the Full-Rate GSM "<< n" operation.
 *	   Note that ANSI-C does not guarantee operation of the C ">>"
 *	   or "<<" operator for negative numbers - it is not specified
 *	   whether this shift is an arithmetic or logical shift.
 *
 *	 KEYWORDS: asl, arithmetic shift left, shift
 *
 *************************************************************************/

static inline int16_t melpe_shl(int16_t var1, int16_t var2)
{
	int16_t swOut;
	int32_t L_Out;

	if (var2 == 0 || var1 == 0) {
		swOut = var1;
	} else if (var2 < 0) {	/* perform a right shift */
		if (var2 <= -15)
			swOut = (int16_t) ((var1 < 0) ? -1 : 0);
		else
            swOut = melpe_shr(var1, melpe_negate(var2));

	} else {		/* var2 > 0 */
		if (var2 >= 15) {
			swOut = (int16_t) ((var1 > 0) ? SW_MAX : SW_MIN);
			/* saturate */
			inc_saturation();
		} else {
			L_Out = (int32_t) var1 *(1 << var2);
			swOut = (int16_t) L_Out;	/* copy low portion to swOut */
			/* overflow could have happened */
			if (swOut != L_Out) {	/* overflowed */
				swOut =
				    (int16_t) ((var1 > 0) ? SW_MAX : SW_MIN);
				/* saturate */
				inc_saturation();
			}
		}
	}
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_shr
 *
 *	 PURPOSE:
 *
 *	   Arithmetic shift right (or left).
 *	   Arithmetically shift the input right by var2.   If var2 is
 *	   negative then an arithmetic shift left (shl) of var1 by
 *	   -var2 is performed.
 *
 *	 INPUTS:
 *
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *
 *	 IMPLEMENTATION:
 *
 *	   Arithmetically shift the input right by var2.  This
 *	   operation maintains the sign of the input number. If var2 is
 *	   negative then an arithmetic shift left (shl) of L_var1 by
 *	   -var2 is performed.	See description of L_shl for details.
 *
 *	   The input is a 32 bit number, as is the output.
 *
 *	   Equivalent to the Full-Rate GSM ">> n" operation.  Note that
 *	   ANSI-C does not guarantee operation of the C ">>" or "<<"
 *	   operator for negative numbers.
 *
 *	 KEYWORDS: shift, arithmetic shift right,
 *
 *************************************************************************/

static inline int32_t melpe_L_shr(int32_t L_var1, int16_t var2)
{
	int32_t L_Mask, L_Out;

	if (var2 == 0 || L_var1 == 0)
		L_Out = L_var1;
	else if (var2 < 0) {	/* perform a left shift */
		if (var2 <= -31) {
			L_Out = (L_var1 > 0) ? LW_MAX : LW_MIN;	/* saturate */
			inc_saturation();
		} else
            L_Out = melpe_L_shl(L_var1, (int16_t) - var2);
	} else {
		if (var2 >= 31)
			L_Out = (L_var1 > 0) ? 0 : 0xffffffffL;
		else {
			L_Mask = 0;
			if (L_var1 < 0)
				L_Mask = ~L_Mask << (32 - var2);

			L_var1 >>= var2;
			L_Out = L_Mask | L_var1;
		}
	}
	return (L_Out);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_shl
 *
 *	 PURPOSE:
 *
 *	   Arithmetic shift left (or right).
 *	   Arithmetically shift the input left by var2.   If var2 is
 *	   negative then an arithmetic shift right (L_shr) of L_var1 by
 *	   -var2 is performed.
 *
 *	 INPUTS:
 *
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *
 *	 IMPLEMENTATION:
 *
 *	   Arithmetically shift the 32 bit input left by var2.	This
 *	   operation maintains the sign of the input number. If var2 is
 *	   negative then an arithmetic shift right (L_shr) of L_var1 by
 *	   -var2 is performed.	See description of L_shr for details.
 *
 *	   Equivalent to the Full-Rate GSM ">> n" operation.  Note that
 *	   ANSI-C does not guarantee operation of the C ">>" or "<<"
 *	   operator for negative numbers.
 *
 *	 KEYWORDS: shift, arithmetic shift left,
 *
 *************************************************************************/

static inline int32_t melpe_L_shl(int32_t L_var1, int16_t var2)
{
	int32_t L_Mask, L_Out = 0;
	int i, iOverflow = 0;

	if (var2 == 0 || L_var1 == 0)
		L_Out = L_var1;
	else if (var2 < 0) {
		if (var2 <= -31)
			L_Out = (L_var1 > 0) ? 0 : 0xffffffffL;
		else
            L_Out = melpe_L_shr(L_var1, (int16_t) - var2);
	} else {
		if (var2 >= 31)
			iOverflow = 1;
		else {
			if (L_var1 < 0)
				L_Mask = LW_SIGN;	/* sign bit mask */
			else
				L_Mask = 0x0;
			L_Out = L_var1;
			for (i = 0; i < var2 && !iOverflow; i++) {	/* check the sign bit */
				L_Out = (L_Out & 0x7fffffffL) << 1;
				if ((L_Mask ^ L_Out) & LW_SIGN)
					iOverflow = 1;
			}
		}

		if (iOverflow) {
			L_Out = (L_var1 > 0) ? LW_MAX : LW_MIN;	/* saturate */
			inc_saturation();
		}
	}

	return (L_Out);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: shift_r
 *
 *	 PURPOSE:
 *
 *	   Shift and r_ound.  Perform a shift right. After shifting, use
 *	   the last bit shifted out of the LSB to r_ound the result up
 *	   or down.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *
 *	 IMPLEMENTATION:
 *
 *	   Shift and r_ound.  Perform a shift right. After shifting, use
 *	   the last bit shifted out of the LSB to r_ound the result up
 *	   or down.
 *
 *	   If var2 is negative perform a arithmetic left shift
 *	   with saturation (see shl() above).
 *
 *	   If var2 is zero simply return var1.
 *
 *	   If var2 is positive perform a arithmetic right shift (shr)
 *	   of var1 by var2-1.  Add the LS bit of the result to var1
 *	   shifted right (shr) by var2.
 *
 *	   Note that there is no constraint on var2, so if var2 is
 *	   -0xffff 8000 then -var2 is 0x0000 8000, not 0x0000 7fff.
 *	   This is the reason the shl function is used.
 *
 *
 *	 KEYWORDS:
 *
 *************************************************************************/

static inline int16_t melpe_shift_r(int16_t var1, int16_t var2)
{
	int16_t swOut, swRnd;
	/* differ from initial implementation (negate var2):
		we always call shift_r with constant 0<= var2 <=15 */
	if ((var2 <=0) || (var2 > 15)) {
		fprintf(stderr, "shift_r wrong shift value, var2=%d\n", var2);
		exit(0);
	} else {
		swRnd = (int16_t) (melpe_shr(var1, (int16_t) (var2 - 1)) & (int16_t) 0x1);
		swOut = melpe_add(melpe_shr(var1, var2), swRnd);
	}
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_shift_r
 *
 *	 PURPOSE:
 *
 *	   Shift and r_ound.  Perform a shift right. After shifting, use
 *	   the last bit shifted out of the LSB to r_ound the result up
 *	   or down.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *
 *	 IMPLEMENTATION:
 *
 *	   Shift and r_ound.  Perform a shift right. After shifting, use
 *	   the last bit shifted out of the LSB to r_ound the result up
 *	   or down.  This is just like shift_r above except that the
 *	   input/output is 32 bits as opposed to 16.
 *
 *	   if var2 is positve perform a arithmetic left shift
 *	   with saturation (see L_shl() above).
 *
 *	   If var2 is zero simply return L_var1.
 *
 *	   If var2 is negative perform a arithmetic right shift (L_shr)
 *	   of L_var1 by (-var2)+1.	Add the LS bit of the result to
 *	   L_var1 shifted right (L_shr) by -var2.
 *
 *	   Note that there is no constraint on var2, so if var2 is
 *	   -0xffff 8000 then -var2 is 0x0000 8000, not 0x0000 7fff.
 *	   This is the reason the L_shl function is used.
 *
 *
 *	 KEYWORDS:
 *
 *************************************************************************/

static inline int32_t melpe_L_shift_r(int32_t L_var1, int16_t var2)
{
	int32_t L_Out, L_rnd;

	if (var2 < -31)
		L_Out = 0;
	else if (var2 < 0) {	/* right shift */
        L_rnd = melpe_L_shl(L_var1, (int16_t) (var2 + 1)) & (int32_t) 0x1;
        L_Out = melpe_L_add(melpe_L_shl(L_var1, var2), L_rnd);
	} else
        L_Out = melpe_L_shl(L_var1, var2);

	return (L_Out);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: norm_l
 *
 *	 PURPOSE:
 *
 *	   Get normalize shift count:
 *
 *	   A 32 bit number is input (possiblly unnormalized).  Output
 *	   the positive (or zero) shift count required to normalize the
 *	   input.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0 <= swOut <= 31
 *
 *
 *
 *	 IMPLEMENTATION:
 *
 *	   Get normalize shift count:
 *
 *	   A 32 bit number is input (possiblly unnormalized).  Output
 *	   the positive (or zero) shift count required to normalize the
 *	   input.
 *
 *	   If zero in input, return 0 as the shift count.
 *
 *	   For non-zero numbers, count the number of left shift
 *	   required to get the number to fall into the range:
 *
 *	   0x4000 0000 >= normlzd number >= 0x7fff ffff (positive number)
 *	   or
 *	   0x8000 0000 <= normlzd number < 0xc000 0000 (negative number)
 *
 *	   Return the number of shifts.
 *
 *	   This instruction corresponds exactly to the Full-Rate "norm"
 *	   instruction.
 *
 *	 KEYWORDS: norm, normalization
 *
 *************************************************************************/

static inline int16_t melpe_norm_l(int32_t L_var1)
{
	int16_t swShiftCnt;

	if (L_var1 != 0) {
		if (!(L_var1 & LW_SIGN)) {	/* positive input */
			for (swShiftCnt = 0;
			     !(L_var1 <= LW_MAX && L_var1 >= 0x40000000L);
			     swShiftCnt++) {
				L_var1 = L_var1 << 1;
			}
		} else {	/* negative input */
			for (swShiftCnt = 0;
			     !(L_var1 >= LW_MIN
			       && L_var1 < (int32_t) 0xc0000000L);
			     swShiftCnt++) {
				L_var1 = L_var1 << 1;
			}
		}
	} else
		swShiftCnt = 0;
	return (swShiftCnt);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: norm_s
 *
 *	 PURPOSE:
 *
 *	   Get normalize shift count:
 *
 *	   A 16 bit number is input (possiblly unnormalized).  Output
 *	   the positive (or zero) shift count required to normalize the
 *	   input.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0 <= swOut <= 15
 *
 *
 *
 *	 IMPLEMENTATION:
 *
 *	   Get normalize shift count:
 *
 *	   A 16 bit number is input (possiblly unnormalized).  Output
 *	   the positive (or zero) shift count required to normalize the
 *	   input.
 *
 *	   If zero in input, return 0 as the shift count.
 *
 *	   For non-zero numbers, count the number of left shift
 *	   required to get the number to fall into the range:
 *
 *	   0x4000 >= normlzd number >= 0x7fff (positive number)
 *	   or
 *	   0x8000 <= normlzd number <  0xc000 (negative number)
 *
 *	   Return the number of shifts.
 *
 *	   This instruction corresponds exactly to the Full-Rate "norm"
 *	   instruction.
 *
 *	 KEYWORDS: norm, normalization
 *
 *************************************************************************/

static inline int16_t melpe_norm_s(int16_t var1)
{
	short swShiftCnt;
	int32_t L_var1;

    L_var1 = melpe_L_deposit_h(var1);
    swShiftCnt = melpe_norm_l(L_var1);
	return (swShiftCnt);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_mult
 *
 *	 PURPOSE:
 *
 *	   Perform a fractional multipy of the two 16 bit input numbers
 *	   with saturation.  Output a 32 bit number.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Multiply the two the two 16 bit input numbers. If the
 *	   result is within this range, left shift the result by one
 *	   and output the 32 bit number.  The only possible overflow
 *	   occurs when var1==var2==-0x8000.  In this case output
 *	   0x7fff ffff.
 *
 *	 KEYWORDS: multiply, mult, mpy
 *
 *************************************************************************/

static inline int32_t melpe_L_mult(int16_t var1, int16_t var2)
{
	int32_t L_product;

	if (var1 == SW_MIN && var2 == SW_MIN) {	/* overflow */
		inc_saturation();
		L_product = LW_MAX;
	} else {
		L_product = (int32_t) var1 *var2;	/* integer multiply */
		L_product <<= 1;
	}
	return (L_product);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: mult
 *
 *	 PURPOSE:
 *
 *	   Perform a fractional multipy of the two 16 bit input numbers
 *	   with saturation and truncation.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform a fractional multipy of the two 16 bit input
 *	   numbers.  If var1 == var2 == -0x8000, output 0x7fff.
 *	   Otherwise output var1*var2 >> 15.  The output is a
 *	   16 bit number.
 *
 *	 KEYWORDS: mult, mulitply, mpy
 *
 *************************************************************************/

static inline int16_t melpe_mult(int16_t var1, int16_t var2)
{
	int32_t L_product;
	int16_t swOut;

    L_product = melpe_L_mult(var1, var2);
    swOut = melpe_extract_h(L_product);
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_mac
 *
 *	 PURPOSE:
 *
 *	   Multiply accumulate.  Fractionally multiply two 16 bit
 *	   numbers together with saturation.  Add to that result to the
 *	   32 bit input with saturation.  Return the 32 bit result.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *	   L_var3
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Fractionally multiply two 16 bit numbers together with
 *	   saturation.	The only numbers which will cause saturation on
 *	   the multiply are 0x8000 * 0x8000.
 *
 *	   Add to that result to the 32 bit input with saturation.
 *	   Return the 32 bit result.
 *
 *	   Please note that this is not a true multiply accumulate as
 *	   most processors would implement it.	The 0x8000*0x8000
 *	   causes and overflow for this instruction.  On an most
 *	   processors this would cause an overflow only if the 32 bit
 *	   input added to it were positive or zero.
 *
 *	 KEYWORDS: mac, multiply accumulate
 *
 *************************************************************************/

static inline int32_t melpe_L_mac(int32_t L_var3, int16_t var1, int16_t var2)
{
	int32_t L_Out;

    L_Out = melpe_L_add(L_var3, melpe_L_mult(var1, var2));
	return (L_Out);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_msu
 *
 *	 PURPOSE:
 *
 *	   Multiply and subtract.  Fractionally multiply two 16 bit
 *	   numbers together with saturation.  Subtract from that result the
 *	   32 bit input with saturation.  Return the 32 bit result.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *	   L_var3
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Fractionally multiply two 16 bit numbers together with
 *	   saturation.	The only numbers which will cause saturation on
 *	   the multiply are 0x8000 * 0x8000.
 *
 *	   Subtract from that result to the 32 bit input with saturation.
 *	   Return the 32 bit result.
 *
 *	   Please note that this is not a true multiply accumulate as
 *	   most processors would implement it.	The 0x8000*0x8000
 *	   causes and overflow for this instruction.  On an most
 *	   processors this would cause an overflow only if the 32 bit
 *	   input added to it were negative or zero.
 *
 *	 KEYWORDS: mac, multiply accumulate, msu
 *
 *************************************************************************/

static inline int32_t melpe_L_msu(int32_t L_var3, int16_t var1, int16_t var2)
{
	int32_t L_Out;

    L_Out = melpe_L_sub(L_var3, melpe_L_mult(var1, var2));
	return (L_Out);
}

/***************************************************************************
 *
 *	 FUNCTION NAME:  msu_r
 *
 *	 PURPOSE:
 *
 *	   Multiply subtract and r_ound.  Fractionally multiply two 16
 *	   bit numbers together with saturation.  Subtract from that result
 *	   the 32 bit input with saturation.  Finally r_ound the result
 *	   into a 16 bit number.
 *
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *	   var2
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *	   L_var3
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Fractionally multiply two 16 bit numbers together with
 *	   saturation.	The only numbers which will cause saturation on
 *	   the multiply are 0x8000 * 0x8000.
 *
 *	   Subtract from that result to the 32 bit input with saturation.
 *	   r_ound the 32 bit result by adding 0x0000 8000 to the input.
 *	   The result may overflow due to the add.	If so, the result
 *	   is saturated.  The 32 bit r_ounded number is then shifted
 *	   down 16 bits and returned as a int16_t.
 *
 *	   Please note that this is not a true multiply accumulate as
 *	   most processors would implement it.	The 0x8000*0x8000
 *	   causes and overflow for this instruction.  On an most
 *	   processors this would cause an overflow only if the 32 bit
 *	   input added to it were positive or zero.
 *
 *	 KEYWORDS: mac, multiply accumulate, macr
 *
 *************************************************************************/

static inline int16_t melpe_msu_r(int32_t L_var3, int16_t var1, int16_t var2)
{
	int16_t swOut;

    swOut = melpe_r_ound(melpe_L_sub(L_var3, melpe_L_mult(var1, var2)));
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: abs_s
 *
 *	 PURPOSE:
 *
 *	   Take the absolute value of the 16 bit input.  An input of
 *	   -0x8000 results in a return value of 0x7fff.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (int16_t) whose value
 *					   falls in the range
 *					   0x0000 0000 <= swOut <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Take the absolute value of the 16 bit input.  An input of
 *	   -0x8000 results in a return value of 0x7fff.
 *
 *	 KEYWORDS: absolute value, abs
 *
 *************************************************************************/

static inline int16_t melpe_abs_s(int16_t var1)
{
	int16_t swOut;

	if (var1 == SW_MIN)
		swOut = SW_MAX;
	else {
		if (var1 < 0)
            swOut = melpe_negate(var1);
		else
			swOut = var1;
	}
	return (swOut);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_abs
 *
 *	 PURPOSE:
 *
 *	   Take the absolute value of the 32 bit input.  An input of
 *	   -0x8000 0000 results in a return value of 0x7fff ffff.
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long signed integer (int32_t) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *
 *
 *	 KEYWORDS: absolute value, abs
 *
 *************************************************************************/
static inline int32_t melpe_L_abs(int32_t L_var1)
{
	int32_t L_Out;

	if (L_var1 == LW_MIN)
		L_Out = LW_MAX;
	else {
		if (L_var1 < 0)
			L_Out = -L_var1;
		else
			L_Out = L_var1;
	}
	return (L_Out);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : L40_add                                                 |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   40 bits addition of 40 bits accumulator with 32 bits variable (L_var1)  |
 |   with overflow control and saturation; the result is set at MAX40        |
 |   when overflow occurs or at MIN40 when underflow occurs.                 |
 |                                                                           |
 |   Complexity weight : 2                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |    L_var1   32 bit long signed integer (int32_t) whose value falls in the  |
 |             range : 0x8000 0000 <= L_var1 <= 0x7fff ffff.                 |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |___________________________________________________________________________|
*/

static inline int64_t melpe_L40_add(int64_t acc, int32_t L_var1)
{
	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	acc = acc + (int64_t) L_var1;

	if (acc > MAX_40) {
		acc = MAX_40;
		/* Overflow = 1; */
	}
	if (acc < MIN_40) {
		acc = MIN_40;
		/* Overflow = 1; */
	}
	return (acc);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : L40_sub                                                 |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   40 bits subtraction of 40 bits accumulator with 32 bits variable        |
 |   (L_var1) with overflow control and saturation; the result is set at     |
 |   MAX40 when overflow occurs or at MIN40 when underflow occurs.           |
 |                                                                           |
 |   Complexity weight : 2                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |    L_var1   32 bit long signed integer (int32_t) whose value falls in the  |
 |             range : 0x8000 0000 <= L_var1 <= 0x7fff ffff.                 |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |___________________________________________________________________________|
*/

static inline int64_t melpe_L40_sub(int64_t acc, int32_t L_var1)
{
	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	acc = acc - (int64_t) L_var1;

	if (acc > MAX_40) {
		acc = MAX_40;
		/* Overflow = 1; */
	}
	if (acc < MIN_40) {
		acc = MIN_40;
		/* Overflow = 1; */
	}
	return (acc);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : L40_mac                                                 |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Multiply var1 by var2 and shift the result left by 1. Add the result    |
 |   to 40 bits accumulator with overflow control and saturation; the result |
 |   is set at MAX40 when overflow occurs or at MIN40 when underflow occurs. |
 |                                                                           |
 |   Complexity weight : 1                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |    var1     16 bit short signed integer (int16_t) whose value falls in the |
 |             range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   |
 |                                                                           |
 |    var2     16 bit short signed integer (int16_t) whose value falls in the |
 |             range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |___________________________________________________________________________|
*/

static inline int64_t melpe_L40_mac(int64_t acc, int16_t var1, int16_t var2)
{
	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	acc = acc + ((int64_t) var1 * (int64_t) var2 << 1);

	if (acc > MAX_40) {
		acc = MAX_40;
		/* Overflow = 1; */
	}
	if (acc < MIN_40) {
		acc = MIN_40;
		/* Overflow = 1; */
	}
	return (acc);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : L40_msu                                                 |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Multiply var1 by var2 and shift the result left by 1. Subtract the      |
 |   result to 40 bits accumulator with overflow control and saturation;     |
 |   the result is set at MAX40 when overflow occurs or at MIN40 when        |
 |   underflow occurs.                                                       |
 |                                                                           |
 |   Complexity weight : 1                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |    var1     16 bit short signed integer (int16_t) whose value falls in the |
 |             range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   |
 |                                                                           |
 |    var2     16 bit short signed integer (int16_t) whose value falls in the |
 |             range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |___________________________________________________________________________|
*/

static inline int64_t melpe_L40_msu(int64_t acc, int16_t var1, int16_t var2)
{
	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	acc = acc - ((int64_t) var1 * (int64_t) var2 << 1);

	if (acc > MAX_40) {
		acc = MAX_40;
		/* Overflow = 1; */
	}
	if (acc < MIN_40) {
		acc = MIN_40;
		/* Overflow = 1; */
	}
	return (acc);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : L40_shl                                                 |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Arithmetically shift the 40 bits accumulator left var1 positions. Zero  |
 |   fill the var1 LSB of the result. If var1 is negative, arithmetically    |
 |   shift 40 bits accumulator right by -var1 with sign extension. Saturate  |
 |   the result in case of underflows or overflows.                          |              |
 |                                                                           |
 |   Complexity weight : 2                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |    var1     16 bit short signed integer (int16_t) whose value falls in the |
 |             range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |___________________________________________________________________________|
*/

static inline int64_t melpe_L40_shl(int64_t acc, int16_t var1)
{

	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	if (var1 < 0) {
        acc = melpe_L40_shr(acc, (int16_t) (-var1));
	} else {
		for (; var1 > 0; var1--) {
			acc = acc << 1;
			if (acc > MAX_40) {
				acc = MAX_40;
				fprintf(stderr, "BASIC_OP: SHL 40 bit overflow.\n");
				break;
			} else if (acc < MIN_40) {
				acc = MIN_40;
				fprintf(stderr, "BASIC_OP: SHL 40 bit overflow.\n");
				break;
			}
		}
	}
	return (acc);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : L40_shr                                                 |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Arithmetically shift the 40 bits accumulator right var1 positions with  |
 |   sign extension. If var1 is negative, arithmetically shift 40 bits       |
 |   accumulator left by -var1 and zero fill the var1 LSB of the result.     |
 |   Saturate the result in case of underflows or overflows.                 |
 |                                                                           |
 |   Complexity weight : 2                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |    var1     16 bit short signed integer (int16_t) whose value falls in the |
 |             range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |___________________________________________________________________________|
*/

static inline int64_t melpe_L40_shr(int64_t acc, int16_t var1)
{

	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	if (var1 < 0) {
        acc = melpe_L40_shl(acc, (int16_t) - var1);
	} else {
		for (; var1 > 0; var1--) {
			acc = acc >> 1;
			if ((acc == 0) || (acc == -1))
				break;
		}
	}
	return (acc);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : L40_negate                                              |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Negate the 40 bits accumulator with saturation; saturate to MAX40 in    |
 |   the case where input is MIN40.                                          |
 |                                                                           |
 |   Complexity weight : 2                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |___________________________________________________________________________|
*/

static inline int64_t melpe_L40_negate(int64_t acc)
{
	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	acc = -acc;

	if (acc > MAX_40) {
		acc = MAX_40;
	}
	return (acc);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : norm32                                                  |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Produces the number of left shift needed to normalize the 40 bits       |
 |   accumulator in 32 bits format (for positive value on the interval with  |
 |   minimum of 1073741824 and maximum of 2147483647, and for negative value |
 |   on the interval with minimum of -2147483648 and maximum of -1073741824);|
 |   in order to normalize the result, the following operation must be done: |
 |                   norm32_acc = L40_shl(acc, norm32(acc)).                 |
 |                                                                           |
 |   Complexity weight : 2                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    var1     16 bit short signed integer (int16_t) whose value falls in the |
 |             range : -8 <= var_out <= 31.                                  |
 |___________________________________________________________________________|
*/

static inline int16_t melpe_norm32(int64_t acc)
{
	int16_t var1;

	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	var1 = 0;

	if (acc > 0) {
		while (acc > (int64_t) MAX_32) {
			acc = acc >> 1;
			var1--;
		}
		while (acc <= ((int64_t) MAX_32 >> 1)) {
			acc = acc << 1;
			var1++;
		}
	} else if (acc < 0) {
		while (acc < (int64_t) MIN_32) {
			acc = acc >> 1;
			var1--;
		}
		while (acc >= ((int64_t) MIN_32 >> 1)) {
			acc = acc << 1;
			var1++;
		}
	}
	return (var1);
}

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : L_sat32                                                 |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |    Saturate the 40 bits accumulator to the range of a 32 bits word.       |
 |                                                                           |
 |   Complexity weight : 2                                                   |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    acc      40 bits accumulator (int64_t) whose value falls in the         |
 |             range : MIN40 <= acc <= MAX40.                                |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    L_var_out                                                              |
 |             32 bit long signed integer (int32_t) whose value falls in the  |
 |             range : 0x8000 0000 <= var_out <= 0x7fff ffff.                |
 |___________________________________________________________________________|
*/

static inline int32_t melpe_L_sat32(int64_t acc)
{
	int32_t L_var_out;

	if ((acc > MAX_40) || (acc < MIN_40)) {
		fprintf(stderr, "BASIC_OP: Error in 40 bits format.\n");
		exit(0);
	}

	if (acc > (int64_t) MAX_32) {
		acc = (int64_t) MAX_32;
	}
	if (acc < (int64_t) MIN_32) {
		acc = (int64_t) MIN_32;
	}
	L_var_out = (int32_t) acc;
	return (L_var_out);
}

/***************************************************************************
 *
 *       FUNCTION NAME: L_mpyu
 *
 *       PURPOSE:
 *
 *         Perform an unsigned fractional multipy of the two unsigned 16 bit
 *         input numbers with saturation.  Output a 32 bit unsigned number.
 *
 *       INPUTS:
 *
 *         var1
 *                                         16 bit short unsigned integer (int16_t) whose value
 *                                         falls in the range 0xffff 0000 <= var1 <= 0x0000 ffff.
 *         var2
 *                                         16 bit short unsigned integer (int16_t) whose value
 *                                         falls in the range 0xffff 0000 <= var2 <= 0x0000 ffff.
 *
 *       OUTPUTS:
 *
 *         none
 *
 *       RETURN VALUE:
 *
 *         L_Out
 *                                         32 bit long unsigned integer (int32_t) whose value
 *                                         falls in the range
 *                                         0x0000 0000 <= L_var1 <= 0xffff ffff.
 *
 *       IMPLEMENTATION:
 *
 *         Multiply the two unsigned 16 bit input numbers.
 *
 *       KEYWORDS: multiply, mult, mpy
 *
 *************************************************************************/
static inline uint32_t L_mpyu(uint16_t var1, uint16_t var2)
{
	return (uint32_t) var1 * var2; /* integer multiply */
}

/****************************************************************************
 *
 *         FUNCTION NAME: L_mpy_ls
 *
 *         PURPOSE:    Multiply a 32 bit number (L_var2) and a 16 bit
 *                                 number (var1) returning a 32 bit result. L_var2
 *                                 is truncated to 31 bits prior to executing the
 *                                 multiply.
 *
 *         INPUTS:
 *
 *               L_var2                         A int32_t input variable
 *
 *               var1                           A int16_t input variable
 *
 *         OUTPUTS:                     none
 *
 *         RETURN VALUE:                A int32_t value
 *
 *         KEYWORDS: mult,mpy,multiplication
 *
 ***************************************************************************/

static inline int32_t L_mpy_ls(int32_t L_var2, int16_t var1)
{
	int32_t L_varOut;
	int16_t swtemp;

	swtemp = melpe_shr(melpe_extract_l(L_var2), 1);
	swtemp = (int16_t) ((short)32767 & (short)swtemp);

	L_varOut = melpe_L_mult(var1, swtemp);
	L_varOut = melpe_L_shr(L_varOut, 15);
	L_varOut = melpe_L_mac(L_varOut, var1, melpe_extract_h(L_var2));
	return (L_varOut);
}
