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

#include <assert.h>

#include "sc1200.h"
#include "mathhalf.h"
#include "math_lib.h"
#include "constant.h"
#include "global.h"
#include "macro.h"

/* log_table[] is Q13, and log_table[i] = log(i+1) * 2^11. */
static const int16_t log_table[256] = {
	0, 2466, 3908, 4932, 5725, 6374, 6923, 7398, 7817, 8192,
	8531, 8840, 9125, 9389, 9634, 9864, 10079, 10283, 10475, 10658,
	10831, 10997, 11155, 11306, 11451, 11591, 11725, 11855, 11979, 12100,
	12217, 12330, 12439, 12545, 12649, 12749, 12846, 12941, 13034, 13124,
	13211, 13297, 13381, 13463, 13543, 13621, 13697, 13772, 13846, 13917,
	13988, 14057, 14125, 14191, 14257, 14321, 14384, 14446, 14506, 14566,
	14625, 14683, 14740, 14796, 14851, 14905, 14959, 15011, 15063, 15115,
	15165, 15215, 15264, 15312, 15360, 15407, 15454, 15500, 15545, 15590,
	15634, 15677, 15721, 15763, 15805, 15847, 15888, 15929, 15969, 16009,
	16048, 16087, 16125, 16163, 16201, 16238, 16275, 16312, 16348, 16384,
	16419, 16454, 16489, 16523, 16557, 16591, 16624, 16657, 16690, 16723,
	16755, 16787, 16818, 16850, 16881, 16912, 16942, 16972, 17002, 17032,
	17062, 17091, 17120, 17149, 17177, 17206, 17234, 17262, 17289, 17317,
	17344, 17371, 17398, 17425, 17451, 17477, 17504, 17529, 17555, 17581,
	17606, 17631, 17656, 17681, 17705, 17730, 17754, 17778, 17802, 17826,
	17850, 17873, 17896, 17920, 17943, 17966, 17988, 18011, 18033, 18056,
	18078, 18100, 18122, 18144, 18165, 18187, 18208, 18229, 18250, 18271,
	18292, 18313, 18334, 18354, 18374, 18395, 18415, 18435, 18455, 18475,
	18494, 18514, 18533, 18553, 18572, 18591, 18610, 18629, 18648, 18667,
	18686, 18704, 18723, 18741, 18759, 18778, 18796, 18814, 18832, 18850,
	18867, 18885, 18903, 18920, 18937, 18955, 18972, 18989, 19006, 19023,
	19040, 19057, 19074, 19090, 19107, 19123, 19140, 19156, 19172, 19189,
	19205, 19221, 19237, 19253, 19269, 19284, 19300, 19316, 19331, 19347,
	19362, 19378, 19393, 19408, 19423, 19438, 19453, 19468, 19483, 19498,
	19513, 19528, 19542, 19557, 19572, 19586, 19600, 19615, 19629, 19643,
	19658, 19672, 19686, 19700, 19714, 19728
};

/***************************************************************************
 *
 *	 FUNCTION NAME: L_divider2
 *
 *	 PURPOSE:
 *
 *	   Divide numer by denom.  If numer is bigger than denom, a warning
 *	   is given and a zero is returned.
 *
 *
 *	 INPUTS:
 *
 *	   numer
 *					   32 bit long signed integer (int32_t).
 *	   denom
 *					   32 bit long signed integer (int32_t).
 *	   numer_shift
 *					   16 bit short signed integer (int16_t) represents
 *					   number of right shifts for numer.
 *	   denom_shift
 *					   16 bit short signed integer (int16_t) represents
 *					   number of left shifts for denom.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   result
 *					   16 bit short signed integer (int16_t).
 *
 *************************************************************************/
int16_t L_divider2(int32_t numer, int32_t denom, int16_t numer_shift,
		     int16_t denom_shift)
{
	int16_t result;
	int16_t sign = 0;
	int16_t short_shift = 0;
	int32_t L_temp;
	int16_t nn;

	assert(denom != 0);

	sign = ((numer < 0) ^ (denom < 0));

	L_temp = melpe_L_shl(denom, denom_shift);
	denom = melpe_L_abs(L_temp);
	L_temp = melpe_L_shr(numer, numer_shift);
	numer = melpe_L_abs(L_temp);

	nn = melpe_norm_l(denom);
	
	if (nn < 16) {
		short_shift = 16 - nn;
		denom = melpe_L_shr(denom, short_shift);
		numer = melpe_L_shr(numer, short_shift);
	}

	assert(numer <= denom);

	result = melpe_divide_s(melpe_extract_l(numer), melpe_extract_l(denom));

	if (sign) {
		result = melpe_negate(result);
	}

	return (result);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: log10_fxp
 *
 *	 PURPOSE:
 *
 *	   Compute the logarithm to the base 10 of x.
 *
 *
 *	 INPUTS:
 *
 *	   x
 *					   16 bit short signed integer (int16_t).
 *	   Q
 *					   16 bit short signed integer (int16_t) represents
 *					   Q value of x.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   y
 *					   16 bit short signed integer (int16_t) in Q12.
 *
 *************************************************************************/
int16_t log10_fxp(int16_t x, int16_t Q)
{
	int16_t y, interp_factor, interp_component;
	int16_t index1, index2;
	int16_t shift, nn;
	int16_t temp1, temp2;
	int32_t L_temp;

	/* Treat x as if it is a fixed-point number with Q7.  Use "shift" to      */
	/* record the exponent required for adjustment.                           */

	shift = melpe_sub(7, Q);

	/* If x is 0, stop and return minus infinity. */
	if (!x)
		return (-SW_MAX);

	/* Keep multiplying x by 2 until x is larger than 1 in Q7.  x in Q7 will  */
	/* now lie between the two integers index1 and index2.                    */

	nn = melpe_norm_s(x);
	if (nn > 7) {
		x = melpe_shl(x, nn - 7);
		shift = melpe_sub(shift, nn - 7);
	}
	index2 = melpe_shr(x, 7);
	index1 = melpe_sub(index2, 1);

	/* interpolation */

	interp_factor = melpe_shl((int16_t) (x & 127), 8);
	temp1 = melpe_sub(log_table[index2], log_table[index1]);
	interp_component = melpe_mult(temp1, interp_factor);

	/* return a Q12 */

	L_temp = melpe_L_mult(log_table[1], shift);
	L_temp = melpe_L_shr(L_temp, 2);
	temp1 = melpe_shr(log_table[index1], 1);
	temp1 = melpe_add(temp1, melpe_extract_l(L_temp));
	temp2 = melpe_shr(interp_component, 1);
	y = melpe_add(temp1, temp2);

	return (y);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_log10_fxp
 *
 *	 PURPOSE:
 *
 *	   Compute the logarithm to the base 10 of x.
 *
 *
 *	 INPUTS:
 *
 *	   x
 *					   32 bit long signed integer (int32_t).
 *	   Q
 *					   16 bit short signed integer (int16_t) represents
 *					   Q value of x.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   y
 *					   16 bit short signed integer (int16_t) in Q11.
 *
 *************************************************************************/
int16_t L_log10_fxp(int32_t x, int16_t Q)
{
	int16_t y, interp_component;
	int32_t interp_factor;
	int16_t index1, index2;
	int16_t shift, nn;
	int16_t temp1, temp2;
	int32_t L_temp;

	shift = melpe_sub(23, Q);
	if (!x)
		return ((int16_t) - SW_MAX);

	nn = melpe_norm_l(x) - 7;
	if (nn > 0) {
		x = melpe_L_shl(x, nn);
		shift = melpe_sub(shift, nn);
	}
	
	index2 = melpe_extract_l(melpe_L_shr(x, 23));
	index1 = melpe_sub(index2, 1);
	
	/* interpolation */

	interp_factor = melpe_L_shl(x & (int32_t) 0x7fffff, 8);
	temp1 = melpe_sub(log_table[index2], log_table[index1]);
	interp_component = melpe_extract_h(L_mpy_ls(interp_factor, temp1));

	/* return a Q11 */

	L_temp = melpe_L_mult(log_table[1], shift);	/* log10(2^shift) */
	L_temp = melpe_L_shr(L_temp, 3);
	temp1 = melpe_shr(log_table[index1], 2);
	temp1 = melpe_add(temp1, melpe_extract_l(L_temp));
	temp2 = melpe_shr(interp_component, 2);
	y = melpe_add(temp1, temp2);

	return (y);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: pow10_fxp
 *
 *	 PURPOSE:
 *
 *	   Compute 10 raised to the power x.
 *
 *
 *	 INPUTS:
 *
 *	   x
 *					   16 bit short signed integer (int16_t) in Q12.
 *	   Q
 *					   16 bit short signed integer (int16_t) represents
 *					   required Q value of returned result.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   y
 *					   16 bit short signed integer (int16_t).
 *
 *************************************************************************/
int16_t pow10_fxp(int16_t x, int16_t Q)
{
	/* table in Q11 */
	static const int16_t table[257] = {
		2048, 2066, 2085, 2104, 2123, 2142, 2161, 2181, 2200, 2220,
		2240, 2260, 2281, 2302, 2322, 2343, 2364, 2386, 2407, 2429,
		2451, 2473, 2496, 2518, 2541, 2564, 2587, 2610, 2634, 2658,
		2682, 2706, 2731, 2755, 2780, 2805, 2831, 2856, 2882, 2908,
		2934, 2961, 2988, 3015, 3042, 3069, 3097, 3125, 3153, 3182,
		3211, 3240, 3269, 3298, 3328, 3358, 3389, 3419, 3450, 3481,
		3513, 3544, 3576, 3609, 3641, 3674, 3708, 3741, 3775, 3809,
		3843, 3878, 3913, 3948, 3984, 4020, 4056, 4093, 4130, 4167,
		4205, 4243, 4281, 4320, 4359, 4399, 4438, 4478, 4519, 4560,
		4601, 4643, 4684, 4727, 4769, 4813, 4856, 4900, 4944, 4989,
		5034, 5079, 5125, 5172, 5218, 5266, 5313, 5361, 5410, 5458,
		5508, 5558, 5608, 5658, 5710, 5761, 5813, 5866, 5919, 5972,
		6026, 6081, 6136, 6191, 6247, 6303, 6360, 6418, 6476, 6534,
		6593, 6653, 6713, 6774, 6835, 6897, 6959, 7022, 7085, 7149,
		7214, 7279, 7345, 7411, 7478, 7546, 7614, 7683, 7752, 7822,
		7893, 7964, 8036, 8109, 8182, 8256, 8331, 8406, 8482, 8559,
		8636, 8714, 8793, 8872, 8952, 9033, 9115, 9197, 9280, 9364,
		9449, 9534, 9620, 9707, 9795, 9883, 9973, 10063, 10154, 10245,
		10338, 10431, 10526, 10621, 10717, 10813, 10911, 11010, 11109,
		    11210,
		11311, 11413, 11516, 11620, 11725, 11831, 11938, 12046, 12155,
		    12265,
		12375, 12487, 12600, 12714, 12829, 12945, 13062, 13180, 13299,
		    13419,
		13540, 13663, 13786, 13911, 14036, 14163, 14291, 14420, 14550,
		    14682,
		14815, 14948, 15084, 15220, 15357, 15496, 15636, 15777, 15920,
		    16064,
		16209, 16355, 16503, 16652, 16803, 16955, 17108, 17262, 17418,
		    17576,
		17734, 17895, 18056, 18220, 18384, 18550, 18718, 18887, 19058,
		    19230,
		19404, 19579, 19756, 19934, 20114, 20296, 20480
	};

	static const int16_t tens_table[9] = {
		26844, 16777, 20972, 26214, 1, 10, 100, 1000, 10000
	};

	static const int16_t Q_table[4] = {
		28, 24, 21, 18
	};

	int16_t y, interp_factor, interp_component;
	int16_t index1, index2;
	int16_t ten_multiple;
	int32_t L_y;
	int16_t temp1, temp2;

	ten_multiple = melpe_shr(x, 12);	/* ten_multiple is the integral part of x */
	if (ten_multiple < -4)
		return ((int16_t) 0);
	else if (ten_multiple > 4) {
		inc_saturation();
		return ((int16_t) SW_MAX);
	}

	index1 = melpe_shr((int16_t) (x & (int16_t) 0x0ff0), 4);
	/* index1 is the most significant 8 bits of the */
	/* fractional part of x, Q8 */
	index2 = melpe_add(index1, 1);

	/* interpolation */
	/* shift by 11 to make it a number between 0 & 1 in Q15 */

	interp_factor = melpe_shl((int16_t) (x & (int16_t) 0x000f), 11);
	/* interp_factor is the least significant 4 bits of the */
	/* fractional part of x, Q15 */
	temp1 = melpe_sub(table[index2], table[index1]);	/* Q0, at most 8 digits */
	interp_component = melpe_mult(temp1, interp_factor);	/* Q0 */

	/* L_y in Q12 if x >= 0 and in Q_table[temp2] + 12 if x < 0 */
	temp1 = melpe_add(table[index1], interp_component);	/* Q0 */
	temp2 = melpe_add(ten_multiple, 4);
	L_y = melpe_L_mult(tens_table[temp2], temp1);

	if (ten_multiple >= 0) {
		temp1 = melpe_sub(12, Q);
		L_y = melpe_L_shr(L_y, temp1);
		y = melpe_extract_l(L_y);
		if (melpe_extract_h(L_y)) {
			y = SW_MAX;
			inc_saturation();
		}
	} else {
		temp1 = melpe_add(Q_table[temp2], 12);
		temp1 = melpe_sub(temp1, Q);
		y = melpe_extract_l(melpe_L_shr(L_y, temp1));
	}

	return (y);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: sqrt_fxp
 *
 *	 PURPOSE:
 *
 *	   Compute the square root of x.
 *
 *
 *	 INPUTS:
 *
 *	   x
 *					   16 bit short signed integer (int16_t).
 *	   Q
 *					   16 bit short signed integer (int16_t) represents
 *					   Q value of x.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   temp
 *					   16 bit short signed integer (int16_t) in same Q.
 *
 *************************************************************************/
int16_t sqrt_fxp(int16_t x, int16_t Q)
{
	int16_t temp;

	if (!x)
		return ((int16_t) 0);

	temp = melpe_shr(log10_fxp(x, Q), 1);	/* temp is now Q12 */
	temp = pow10_fxp(temp, Q);
	return (temp);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_sqrt_fxp
 *
 *	 PURPOSE:
 *
 *	   Compute the square root of x.
 *
 *
 *	 INPUTS:
 *
 *	   x
 *					   32 bit long signed integer (int32_t).
 *	   Q
 *					   16 bit short signed integer (int16_t) represents
 *					   Q value of x.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   temp
 *					   16 bit short signed integer (int16_t) in same Q.
 *
 *************************************************************************/
int16_t L_sqrt_fxp(int32_t x, int16_t Q)
{
	int16_t temp;

	if (!x)
		return ((int16_t) 0);

	temp = L_log10_fxp(x, Q);
	/* temp in Q11, pow10 treat it as Q12, => no need to shr by 1. */
	temp = pow10_fxp(temp, Q);

	return (temp);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_pow_fxp
 *
 *	 PURPOSE:
 *
 *	   Compute the value of x raised to the power 'power'.
 *
 *
 *	 INPUTS:
 *
 *	   x
 *					   32 bit long signed integer (int32_t).
 *	   power
 *					   16 bit short signed integer (int16_t) in Q15.
 *	   Q_in
 *					   16 bit short signed integer (int16_t) represents
 *					   Q value of x.
 *	   Q_out
 *					   16 bit short signed integer (int16_t) represents
 *					   required Q value of returned result.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   temp
 *					   16 bit short signed integer (int16_t).
 *
 *************************************************************************/
int16_t L_pow_fxp(int32_t x, int16_t power, int16_t Q_in,
		    int16_t Q_out)
{
	int16_t temp;

	if (!x)
		return ((int16_t) 0);

	temp = L_log10_fxp(x, Q_in);	/* temp in Q11 */
	temp = melpe_mult(power, melpe_shl(temp, 1));	/* temp in Q12 */
	temp = pow10_fxp(temp, Q_out);
	return (temp);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: sin_fxp
 *
 *	 PURPOSE:
 *
 *	   Compute the sine of x whose value is expressed in radians/PI.
 *
 *
 *	 INPUTS:
 *
 *	   x
 *					   16 bit short signed integer (int16_t) in Q15.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   ty
 *					   16 bit short signed integer (int16_t) in Q15.
 *
 *************************************************************************/

int16_t sin_fxp(int16_t x)
{
	static const int16_t table[129] = {
		0, 402, 804, 1206, 1608, 2009, 2411, 2811, 3212,
		3612, 4011, 4410, 4808, 5205, 5602, 5998, 6393, 6787,
		7180, 7571, 7962, 8351, 8740, 9127, 9512, 9896, 10279,
		10660, 11039, 11417, 11793, 12167, 12540, 12910, 13279, 13646,
		14010, 14373, 14733, 15091, 15447, 15800, 16151, 16500, 16846,
		17190, 17531, 17869, 18205, 18538, 18868, 19195, 19520, 19841,
		20160, 20475, 20788, 21097, 21403, 21706, 22006, 22302, 22595,
		22884, 23170, 23453, 23732, 24008, 24279, 24548, 24812, 25073,
		25330, 25583, 25833, 26078, 26320, 26557, 26791, 27020, 27246,
		27467, 27684, 27897, 28106, 28311, 28511, 28707, 28899, 29086,
		29269, 29448, 29622, 29792, 29957, 30118, 30274, 30425, 30572,
		30715, 30853, 30986, 31114, 31238, 31357, 31471, 31581, 31686,
		31786, 31881, 31972, 32058, 32138, 32214, 32286, 32352, 32413,
		32470, 32522, 32568, 32610, 32647, 32679, 32706, 32729, 32746,
		32758, 32766, 32767
	};
	int16_t tx, ty;
	int16_t sign;
	int16_t index1, index2;
	int16_t m;
	int16_t temp;

	sign = 0;
	if (x < 0) {
		tx = melpe_negate(x);
		sign = -1;
	} else {
		tx = x;
	}

	/* if angle > pi/2, sin(angle) = sin(pi-angle) */
	if (tx > X05_Q15) {
		tx = melpe_sub(ONE_Q15, tx);
	}
	/* convert input to be within range 0-128 */
	index1 = melpe_shr(tx, 7);
	index2 = melpe_add(index1, 1);

	if (index1 == 128) {
		if (sign != 0)
			return (melpe_negate(table[index1]));
		else
			return (table[index1]);
	}

	m = melpe_sub(tx, melpe_shl(index1, 7));
	/* convert decimal part to Q15 */
	m = melpe_shl(m, 8);

	/* interpolate */
	temp = melpe_sub(table[index2], table[index1]);
	temp = melpe_mult(m, temp);
	ty = melpe_add(table[index1], temp);

	if (sign != 0)
		return (melpe_negate(ty));
	else
		return (ty);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: cos_fxp
 *
 *	 PURPOSE:
 *
 *	   Compute the cosine of x whose value is expressed in radians/PI.
 *
 *
 *	 INPUTS:
 *
 *	   x
 *					   16 bit short signed integer (int16_t) in Q15.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   ty
 *					   16 bit short signed integer (int16_t) in Q15.
 *
 *************************************************************************/
int16_t cos_fxp(int16_t x)
{
	static const int16_t table[129] = {
		32767, 32766, 32758, 32746, 32729, 32706, 32679, 32647, 32610,
		32568, 32522, 32470, 32413, 32352, 32286, 32214, 32138, 32058,
		31972, 31881, 31786, 31686, 31581, 31471, 31357, 31238, 31114,
		30986, 30853, 30715, 30572, 30425, 30274, 30118, 29957, 29792,
		29622, 29448, 29269, 29086, 28899, 28707, 28511, 28311, 28106,
		27897, 27684, 27467, 27246, 27020, 26791, 26557, 26320, 26078,
		25833, 25583, 25330, 25073, 24812, 24548, 24279, 24008, 23732,
		23453, 23170, 22884, 22595, 22302, 22006, 21706, 21403, 21097,
		20788, 20475, 20160, 19841, 19520, 19195, 18868, 18538, 18205,
		17869, 17531, 17190, 16846, 16500, 16151, 15800, 15447, 15091,
		14733, 14373, 14010, 13646, 13279, 12910, 12540, 12167, 11793,
		11417, 11039, 10660, 10279, 9896, 9512, 9127, 8740, 8351,
		7962, 7571, 7180, 6787, 6393, 5998, 5602, 5205, 4808,
		4410, 4011, 3612, 3212, 2811, 2411, 2009, 1608, 1206,
		804, 402, 0
	};
	int16_t tx, ty;
	int16_t sign;
	int16_t index1, index2;
	int16_t m;
	int16_t temp;

	sign = 0;
	if (x < 0) {
		tx = melpe_negate(x);
	} else {
		tx = x;
	}

	/* if angle > pi/2, cos(angle) = -cos(pi-angle) */
	if (tx > X05_Q15) {
		tx = melpe_sub(ONE_Q15, tx);
		sign = -1;
	}
	/* convert input to be within range 0-128 */
	index1 = melpe_shr(tx, 7);
	index2 = melpe_add(index1, 1);

	if (index1 == 128)
		return ((int16_t) 0);

	m = melpe_sub(tx, melpe_shl(index1, 7));
	/* convert decimal part to Q15 */
	m = melpe_shl(m, 8);

	temp = melpe_sub(table[index2], table[index1]);
	temp = melpe_mult(m, temp);
	ty = melpe_add(table[index1], temp);

	if (sign != 0)
		return (melpe_negate(ty));
	else
		return (ty);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: sqrt_Q15
 *
 *	 PURPOSE:
 *
 *	   Compute the Square Root of x Q15
 *		Based on 6-term Taylor-series
 *
 *	 INPUTS:
 *
 *	   x
 *					   16 bit short signed integer (int16_t) in Q15.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   A
 *					   16 bit short signed integer (int16_t) in Q15.
 *
 *************************************************************************/

/*************************************************************************/
/*  We calculate sqrt(y) = sqrt(x+1) :                                   */
/*  Approximated by:                                                     */
/*  1 + (x/2) - 0.5(x/2)^2 + 0.5(x/2)^3 - 0.625(x/2)^4 + 0.875(x/2)^5    */
/*  before this operation, y is normalized to [0.5; 1[                   */
/*************************************************************************/

int16_t sqrt_Q15(int16_t x)
{
	int16_t shift, odd;
	int16_t temp, x_2, x_24;
	int32_t L_temp, L_temp2;
	int32_t L_A;

	if (x == 0)
		return 0;	/* return zero */
	L_A = melpe_L_deposit_h(x);
	shift = melpe_norm_l(L_A);
	L_A = melpe_L_shl(L_A, shift);	/* Normalize */
	odd = 0;
	if (shift & 0x1)
		odd = 1;	/* Odd exponent */
	shift = melpe_shl(shift, -1);
	shift = melpe_negate(shift);
	L_A = melpe_L_shl(L_A, -1);
	L_A = melpe_L_sub(L_A, 0x40000000);
	temp = melpe_extract_h(L_A);
	x_2 = temp;
	L_A = melpe_L_add(L_A, 0x40000000);
	L_A = melpe_L_add(L_A, 0x40000000);
	L_temp = melpe_L_mult(temp, temp);
	L_temp = -L_temp;
	L_temp2 = melpe_L_shl(L_temp, -1); 
	L_A = melpe_L_add(L_A, L_temp2);
#ifdef POSIX_TI_EMULATION
	L_temp =
	    melpe_L_mult((int16_t) melpe_L_shl(L_temp, -16),
		   (int16_t) melpe_L_shl(L_temp, -16));
#else
	L_temp = _smpyh(L_temp, L_temp);
#endif
	x_24 = melpe_extract_h(L_temp);
	L_temp = melpe_L_mult(x_24, 0x5000);
	L_A = melpe_L_sub(L_A, L_temp);
	temp = melpe_mult(x_24, x_2);
	L_A = melpe_L_add(L_A, melpe_L_mult(0x7000, temp));
	temp = melpe_mult(x_2, x_2);
	L_temp2 = melpe_L_mult(temp, x_2);
	L_A = melpe_L_add(L_A, melpe_L_shl(L_temp2, -1));
	L_A = melpe_L_add(L_A, 0x8000);
	if (odd) {		/* Adjust, if odd exponent! */
#ifdef POSIX_TI_EMULATION
		L_A = melpe_L_mult((int16_t) melpe_L_shl(L_A, -16), 0x5A82);	/* L_A*=1/sqrt(2) */
#else
		L_A = _smpyhl(L_A, 0x5A82);
#endif
		L_A = melpe_L_add(L_A, 0x8000);	/* r_ound */
	}
	L_A = melpe_L_shl(L_A, shift);
	temp = melpe_extract_h(L_A);
	return temp;
}

int16_t add_shr(int16_t Var1, int16_t Var2)
{
	int16_t temp;
	int32_t L_1, L_2;

	L_1 = melpe_L_deposit_l(Var1);
	L_2 = melpe_L_deposit_l(Var2);
	temp = (int16_t) melpe_L_shr(melpe_L_add(L_1, L_2), 1);
	return temp;
}

int16_t sub_shr(int16_t Var1, int16_t Var2)
{
	int16_t temp;
	int32_t L_1, L_2;

	L_1 = melpe_L_deposit_l(Var1);
	L_2 = melpe_L_deposit_l(Var2);
	temp = (int16_t) melpe_L_shr(melpe_L_sub(L_1, L_2), 1);
	return temp;
}
