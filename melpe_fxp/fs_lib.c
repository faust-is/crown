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

/* ==================================== */
/* fs_lib.c: Fourier series subroutines */
/* ==================================== */

/*	compiler include files	*/

#include <assert.h>

#include "sc1200.h"
#include "mathhalf.h"
#include "mat_lib.h"
#include "math_lib.h"
#include "fs_lib.h"
#include "constant.h"
#include "global.h"
#include "dsp_sub.h"
#include "macro.h"
#include "fft_lib.h"
#include "string.h"


/*      Subroutine FIND_HARM: find Fourier coefficients using   */
/*      FFT of input signal divided into pitch dependent bins.  */
/*                                                              */
/*  Q values:                                                   */
/*  input - Q0                                                  */
/*  fsmag - Q13                                                 */
/*  pitch - Q7                                                  */

void find_harm(void* fft_handle, int16_t input[], int16_t fsmag[], int16_t pitch,
	       int16_t num_harm, int16_t length)
{
	register int16_t i, j, k, i2;
	int32_t find_hbuf_32[FFTLENGTH]; // To ensure that find_hbuf is 4-byte alligned
	int16_t iwidth;
	int16_t fwidth, mult_fwidth, shift, max;
	int32_t *L_fsmag;
	int32_t L_temp, L_max;
	long40_t avg;
	int16_t temp1, temp2;
	int16_t* find_hbuf = (int16_t*) find_hbuf_32;
	L_fsmag = L_v_get(num_harm);

	/* Find normalization factor of frame and scale input to maximum          */
	/* precision. */
	max = 0;
	for (i = 0; i < length; i++) {
		temp1 = melpe_abs_s(input[i]);
		if (temp1 > max)
			max = temp1;
	}
	shift = melpe_norm_s(max);

	/* initialize fsmag */
	fill(fsmag, ONE_Q13, num_harm);

	/* Perform peak-picking on FFT of input signal */
	/* Calculate FFT of complex signal in scratch buffer */
	v_zap(find_hbuf, 2 * FFTLENGTH);
	for (i = 0; i < length; i++) {
		find_hbuf[i] = melpe_shl(input[i], shift);
	}
	rfft(fft_handle, find_hbuf, FFTLENGTH);

	/* Implement pitch dependent staircase function */
	/* Harmonic bin width fwidth = Q6 */

	fwidth = melpe_shr(melpe_divide_s(FFTLENGTH, pitch), 2);
	/* iwidth = (int) fwidth */
	iwidth = melpe_shr(fwidth, 6);

	/* Originally here we have a check for setting iwidth to be at least 2.   */
	/* This is not necessary because FFTLENGTH (== 512) divided by PITCHMAX   */
	/* (== 160) will be at least 3.                                           */

	i2 = melpe_shr(iwidth, 1);

	/* if (num_harm > 0.25*pitch) num_harm = 0.25*pitch */
	/* temp1 = 0.25*pitch in Q0 */

	temp1 = melpe_shr(pitch, 9);
	if (num_harm > temp1)
		num_harm = temp1;

	/* initialize avg to make sure that it is non-zero */
	mult_fwidth = fwidth;	/* (k + 1)*fwidth, Q6 */
	for (k = 0; k < num_harm; k++) {
		/*      i = ((k + 1) * fwidth) - i2 + 0.5 *//* Start at peak-i2 */
		i = melpe_sub(melpe_shr(melpe_add(mult_fwidth, X05_Q6), 6), i2);

		/* Calculate magnitude squared of coefficients */
		L_max = 0;
		for (j = 0; j < iwidth; j++) {
			/*      temp1 = find_hbuf[2*(j + i)]; */
			/*      temp2 = find_hbuf[2*(j + i) + 1]; */
			temp2 = melpe_add(i, j);
			temp1 = find_hbuf[2 * temp2];
			temp2 = find_hbuf[2 * temp2 + 1];
			L_temp =
			    melpe_L_add(melpe_L_mult(temp1, temp1), melpe_L_mult(temp2, temp2));
			L_max = Max(L_max, L_temp);
		}

		L_fsmag[k] = L_max;
		mult_fwidth = melpe_add(mult_fwidth, fwidth);
	}

	/* Normalize Fourier series values to average magnitude */
	avg = 1;
	for (k = 0; k < num_harm; k++)
		avg = melpe_L40_add(avg, L_fsmag[k]);
	temp1 = melpe_norm32(avg);
	L_temp = (int32_t) melpe_L40_shl(avg, temp1);	/* man. of avg */
	temp1 = melpe_sub(31, temp1);	/* exp. of avg */
	/* now compute num_harm/avg. avg = L_temp(Q31) x 2^temp1 */
	temp2 = melpe_shl(num_harm, 10);
	/* num_harm = temp2(Q15) x 2^5 */
	temp2 = melpe_divide_s(temp2, melpe_extract_h(L_temp));
	/* now think fs as Q15 x 2^31. The constant below should be 31 */
	/* but consider Q5 for num_harm and fsmag before sqrt Q11, and */
	/* add two guard bits  30 = 31 + 5 - 4 - 2                     */
	shift = melpe_sub(30, temp1);
	/* the sentence above is just for clarity. temp1 = sub(31, temp1) */
	/* and shift = sub(30, temp1) can be shift = sub(temp1, 1)        */

	for (i = 0; i < num_harm; i++) {
		/*      temp1 = num_harm/(avg + 0.0001) */
		/*      fsmag[i] = sqrt(temp1*fsmag[i]) */
		temp1 = melpe_extract_h(melpe_L_shl(L_fsmag[i], shift));
		temp1 = melpe_extract_h(melpe_L_shl(melpe_L_mult(temp1, temp2), 2));	/* Q11 */
		fsmag[i] = sqrt_Q15(temp1);
	}

	v_free(L_fsmag);
}

/* Subroutine IDFT_REAL: take inverse discrete Fourier transform of real      */
/*                       input coefficients.  Assume real time signal, so     */
/*                       reduce computation using symmetry between lower and  */
/*                       upper DFT coefficients.                              */
/*                                                                            */
/* Q values:                                                                  */
/*      real - Q13, signal - Q15                                              */

void idft_real(int16_t real[], int16_t signal[], int16_t length, int16_t fslib_idftc[])
{
	register int16_t i, j, k;

	int16_t k_inc, length2;
	int16_t w, w2;
	int16_t temp;
	int32_t L_temp;

	assert(length <= DFTMAX);
	length2 = melpe_add(melpe_shr(length, 1), 1);
	/*      w = TWOPI / length; */
	w = melpe_divide_s(TWO_Q3, length);	/* w = 2/length in Q18 */

	for (i = 0; i < length; i++) {
		L_temp = melpe_L_mult(w, i);	/* L_temp in Q19 */

		/* make sure argument for cos function is less than 1 */
		if (L_temp > (int32_t) ONE_Q19) {
			/*      cos(pi+x) = cos(pi-x) */
			L_temp = melpe_L_sub((int32_t) TWO_Q19, L_temp);
		} else if (L_temp == (int32_t) ONE_Q19)
			L_temp = melpe_L_sub(L_temp, 1);

		L_temp = melpe_L_shr(L_temp, 4);	/* L_temp in Q15 */
		temp = melpe_extract_l(L_temp);
		fslib_idftc[i] = cos_fxp(temp);	/* fslib_idftc in Q15 */
	}

	w = melpe_shr(w, 1);		/* w = 2/length in Q17 */
	w2 = melpe_shr(w, 1);		/* w2 = 1/length in Q17 */
	real[0] = melpe_mult(real[0], w2);	/* real in Q15 */
	temp = melpe_sub(length2, 1);
	for (i = 1; i < temp; i++) {
		/*      real[i] *= (2.0/length); */
		real[i] = melpe_mult(real[i], w);
	}
	temp = melpe_shl(i, 1);
	if (temp == length)	/* real[i] *= (1.0/length); */
		real[i] = melpe_mult(real[i], w2);
	else			/* real[i] *= (2.0/length); */
		real[i] = melpe_mult(real[i], w);

	for (i = 0; i < length; i++) {
		L_temp = melpe_L_deposit_h(real[0]);	/* L_temp in Q31 */
		k_inc = i;
		k = k_inc;
		for (j = 1; j < length2; j++) {
			L_temp = melpe_L_mac(L_temp, real[j], fslib_idftc[k]);
			k = melpe_add(k, k_inc);
			if (k >= length)
				k = melpe_sub(k, length);
		}
		signal[i] = melpe_r_ound(L_temp);
	}
}

