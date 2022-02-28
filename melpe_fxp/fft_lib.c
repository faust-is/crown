/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

/* ================================================================== */
/*                                                                    */
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */
/* ================================================================== */

/* ===================================== */
/* fft_lib.c: Fourier series subroutines */
/* ===================================== */

/* compiler include files */
#include "sc1200.h"
#include "mathhalf.h"
#include "mat_lib.h"
#include "math_lib.h"
#include "constant.h"
#include "global.h"
#include "fft_lib.h"
#include <string.h>
#include <stdlib.h>

#define SWAP(a, b)		{int16_t temp1; temp1 = (a); (a) = (b); (b) = temp1;}

#if (!defined FIXED_COMPLEX_MULTIPLY) || defined POSIX_TI_EMULATION
/* Memory definition */
typedef struct _fft_context {
	int16_t wr_array[FFTLENGTH / 2];
	int16_t wi_array[FFTLENGTH / 2];
} fft_context;

static int16_t v_getmaxabs(const int16_t* data, int16_t n) 
{
	register int16_t i;
	int16_t data_max = 0;
	for (i = 0; i < n; i++) {
		if (melpe_sub(data_max, melpe_abs_s(data[i])) < 0)
			data_max = melpe_abs_s(data[i]);
	}
	return data_max;
}

static void v_shr(int16_t* data, int16_t n, int16_t shift_count) {
	register int16_t i;
	for (i = 0; i < n; i++)
		data[i] = melpe_shr(data[i], shift_count);
}


/* Radix-2, DIT, 2N-point Real FFT */
void rfft(void* fft_handle, int16_t data[], int16_t n)
{
	int16_t i, n_2, itemp;
	int16_t r1, r2, i1, i2;
	int16_t wr, wi, data_max;
	int16_t index;
	int32_t L_temp1, L_temp2;
#ifdef FIXED_COMPLEX_MULTIPLY
	long40_t ACC;
#endif
	fft_context* ff = (fft_context*) fft_handle;

	n_2 = n >> 1; 
	cfft(fft_handle, data, n_2);
	/* Check for overflow */
	data_max = v_getmaxabs(data, n);
	
	if (data_max > 16383) {
		v_shr(data, n, 1);
	}
	/* Checking ends */
	for (i = 2; i < n_2; i += 2) {
		r1 = add_shr(data[i], data[n - i]);	/* Xr(k) + Xr(N-k):RP */
		L_temp1 = melpe_L_sub(data[i + 1], data[n - i + 1]);	/* Xi(k) - Xi(N-k):IM */
		L_temp1 = melpe_L_shl(L_temp1, 16);
		r2 = add_shr(data[i + 1], data[n - i + 1]);	/* Xi(k) + Xi(N-k):IP */
		L_temp2 = melpe_L_sub(data[i], data[n - i]);	/* Xr(k) - Xr(N-k):RM */
		L_temp2 = melpe_L_shl(L_temp2, 16);
		data[i] = r1;
		data[n - i] = r1;
		data[2 * n - i + 1] = melpe_extract_h(melpe_L_shr(L_temp2, 1));
		L_temp2 = melpe_L_negate(L_temp2);
		data[n + i + 1] = melpe_extract_h(melpe_L_shr(L_temp2, 1));

		data[i + 1] = r2;
		data[n - i + 1] = r2;
		data[2 * n - i] = melpe_extract_h(melpe_L_shr(L_temp1, 1));
		L_temp1 = melpe_L_negate(L_temp1);
		data[n + i] = melpe_extract_h(melpe_L_shr(L_temp1, 1));
	}
	data[n + n_2] = 0;
	data[n + n_2 + 1] = 0;
	r1 = melpe_add(data[0], data[1]);
	i1 = melpe_sub(data[0], data[1]);
	data[0] = r1;
	data[1] = 0;
	data[n] = i1;
	data[n + 1] = 0;
	index = 1;
	wr = ff->wr_array[index];
	wi = ff->wi_array[index];
	for (i = 2; i < n; i += 2) {
		r1 = data[i];
		i1 = data[2 * n - i];
		r2 = data[i + 1];
		i2 = data[2 * n - i + 1];

		/* complex multiply (i2,r2)*(wi,wr) - Re part */
#ifdef FIXED_COMPLEX_MULTIPLY
		ACC = melpe_L40_mac(0, r2, wr);
		ACC = melpe_L40_msu(ACC, i2, wi);
		L_temp1 = melpe_L_sat32(ACC);
		itemp = melpe_r_ound(L_temp1);
		itemp = melpe_add(itemp, r1);
#else
		L_temp1 = melpe_L_deposit_h(r1);
		L_temp1 = melpe_L_add(L_temp1, melpe_L_mult(r2, wr));
		L_temp1 = melpe_L_deposit_h(melpe_r_ound(L_temp1));
		L_temp1 = melpe_L_sub(L_temp1, melpe_L_mult(i2, wi));
		itemp = melpe_r_ound(L_temp1);
#endif
		
		
		data[i] = itemp;
		data[2 * n - i] = itemp;

		/* complex multiply (i2,r2)*(wi,wr) - Im part */
#ifdef FIXED_COMPLEX_MULTIPLY
		ACC = melpe_L40_msu(0, r2, wi);
		ACC = melpe_L40_msu(ACC, i2, wr);
		L_temp2 = melpe_L_sat32(ACC);
		itemp = melpe_r_ound(L_temp2);
		itemp = melpe_add(itemp, i1);
		data[i + 1] = itemp;
		data[2 * n - i + 1] = melpe_negate(itemp);
#else
		L_temp2 = melpe_L_deposit_h(i1);
		L_temp2 = melpe_L_sub(L_temp2, melpe_L_mult(r2, wi));
		L_temp2 = melpe_L_deposit_h(melpe_r_ound(L_temp2));
		L_temp2 = melpe_L_sub(L_temp2, melpe_L_mult(i2, wr));
		L_temp2 = melpe_L_add(L_temp2, 0x8000);
		data[i + 1] = melpe_extract_h(L_temp2);
		data[2 * n - i + 1] = melpe_extract_h(melpe_L_negate(L_temp2));
#endif
		index += 1;
		if (!(index >= FFTLENGTH / 2)) {
			wr = ff->wr_array[index];
			wi = ff->wi_array[index];
		}
	}
}

/* Subroutine FFT: Fast Fourier Transform */
int16_t cfft(void* fft_handle, int16_t data[], int16_t nn)
{
	fft_context* ff = (fft_context*) fft_handle;
	register int16_t i, j;
	int16_t n, mmax, m, istep;
	int16_t sPR, sQR, sPI, sQI;
	int16_t wr, wi, data_max;
	int32_t L_temp1, L_temp2;
	int16_t index, index_step;
	int16_t guard;
#ifdef FIXED_COMPLEX_MULTIPLY
	int16_t itemp;
	long40_t ACC;
#else
	register int32_t L_tempr, L_tempi;
#endif
	
	guard = 0;

	n = nn << 1;
	j = 0;
	for (i = 0; i < n - 1; i += 2) {
		if (j > i) {
			SWAP(data[j], data[i]);
			SWAP(data[j + 1], data[i + 1]);
		}
		m = nn;
		while (m >= 2 && j >= m) {
			j-=m; 
			m=m>>1;
		}
		j+=m;
	}

	/* Stage 1 */
	/* Check for Overflow */
	data_max = v_getmaxabs(data, n);
	if (data_max > 16383) {
		guard += 1;
		v_shr(data, n, 1);
	}
	/* End of checking */
	for (i = 0; i < n; i += 4) {
		sPR = data[i];
		sQR = data[i + 2];
		sPI = data[i + 1];
		sQI = data[i + 2 + 1];
		data[i] = melpe_add(sPR, sQR);	/* PR' */
		data[i + 2] = melpe_sub(sPR, sQR);	/* QR' */
		data[i + 1] = melpe_add(sPI, sQI);	/* PI' */
		data[i + 2 + 1] = melpe_sub(sPI, sQI);	/* QI' */
	}
	/* Stage 2 */
	/* Check for Overflow */
	data_max = v_getmaxabs(data, n);
	if (data_max > 16383) {
		guard += 1;
		v_shr(data, n, 1);
	}
	/* End of checking */
	for (i = 0; i < n; i += 8) {
		/* Butterfly 1 */
		sPR = data[i];
		sQR = data[i + 4];
		sPI = data[i + 1];
		sQI = data[i + 4 + 1];
		data[i] = melpe_add(sPR, sQR);	/* PR' */
		data[i + 4] = melpe_sub(sPR, sQR);	/* QR' */
		data[i + 1] = melpe_add(sPI, sQI);	/* PI' */
		data[i + 4 + 1] = melpe_sub(sPI, sQI);	/* QI' */
		/* Butterfly 2 */
		sPR = data[i + 2];
		sQR = data[i + 2 + 4];
		sPI = data[i + 2 + 1];
		sQI = data[i + 4 + 2 + 1];
		data[i + 2] = melpe_add(sPR, sQI);	/* PR' */
		data[i + 4 + 2] = melpe_sub(sPR, sQI);	/* QR' */
		data[i + 2 + 1] = melpe_sub(sPI, sQR);	/* PI' */
		data[i + 4 + 2 + 1] = melpe_add(sPI, sQR);	/* QI' */
	}
	/* Stages 3... */
	mmax = 8;

	/* initialize index step */
	index_step = nn >> 1;

	while (n > mmax) {
		/* Overflow checking */
		data_max = v_getmaxabs(data, n);
		
		if (data_max > 16383) {
			guard += 2;
			v_shr(data, n, 2);
		} else if (data_max > 8191) {
			guard += 1;
			v_shr(data, n, 1);
		}
		/* Checking ends */
		istep = mmax << 1;	/* istep = 2 * mmax */

		index = 0;
		index_step = index_step >> 1;

		wr = ONE_Q15;
		wi = 0;
		for (m = 0; m < mmax - 1; m += 2) {
			for (i = m; i < n; i+=istep) {
				j = i+mmax;
				
				/* complex multiply (i2,r2)*(wi,wr) - Im part */
				sPR = data[i];
				sQR = data[j];
#ifdef FIXED_COMPLEX_MULTIPLY
				ACC = melpe_L40_mac(0, wr, data[j]);
				ACC = melpe_L40_mac(ACC, wi, data[j + 1]);
				L_temp1 = melpe_L_sat32(ACC);
				itemp = melpe_r_ound(L_temp1);
				data[i] = melpe_add(sPR, itemp);
				data[j] = melpe_sub(sPR, itemp);
#else
				L_temp1 = melpe_L_mult(wr, data[j]);
				L_temp2 = melpe_L_mult(wi, data[j + 1]);
				L_tempr = melpe_L_add(L_temp1, L_temp2);
				L_tempr = melpe_L_deposit_h(melpe_r_ound(L_tempr));
				data[i] = melpe_extract_h(melpe_L_add(melpe_L_deposit_h(sPR), L_tempr));
				data[j] = melpe_extract_h(melpe_L_sub(melpe_L_deposit_h(sPR), L_tempr));
#endif
				
				
				/* data[i]= data[i]+ Im (data[j], data[j+1]) * (wi, wr) */
				/* data[j]= data[i]- Im (data[j], data[j+1]) * (wi, wr) */
#ifdef FIXED_COMPLEX_MULTIPLY				
				ACC = melpe_L40_mac(0, wi, sQR);
				ACC = melpe_L40_msu(ACC, wr, data[j + 1]);
				L_temp2 = melpe_L_sat32(ACC);
				itemp = melpe_r_ound(L_temp2);
				sPI = data[i + 1];
				data[i + 1] = melpe_sub(sPI, itemp);
				data[j + 1] = melpe_add(sPI, itemp);
#else
				L_temp1 = melpe_L_mult(wi, sQR);
				L_temp2 = melpe_L_mult(wr, data[j + 1]);
				L_tempi = melpe_L_sub(L_temp1, L_temp2);
				L_tempi = melpe_L_deposit_h(melpe_r_ound(L_tempi));
				sPI = data[i + 1];
				data[i + 1] = melpe_extract_h(melpe_L_sub(melpe_L_deposit_h(sPI), L_tempi));
				data[j + 1] = melpe_extract_h(melpe_L_add(melpe_L_deposit_h(sPI), L_tempi));
#endif
				
				/* data[i+1]= data[i+1]- Re (data[j], data[j+1]) * (wi, wr) */
				/* data[j+1]= data[i+1]+ Re (data[j], data[j+1]) * (wi, wr) */
			}
			index += index_step;
			if (!(index >= FFTLENGTH / 2)) {
				wr = ff->wr_array[index];
				wi = ff->wi_array[index];
			}
		}
		mmax = istep;
	}
	return guard;
}


/* Initialization of ff->wr_array and ff->wi_array */
void* fft_init(void* context)
{
	register int16_t i;
	int16_t fft_len2, shift, step, theta;
	fft_context* ff = (fft_context*) context;
	if (!ff) ff = (fft_context*) malloc(sizeof(fft_context));
	if (!ff) return 0;
	memset(ff, 0x00, sizeof(fft_context));
	

	fft_len2 = melpe_shr(FFTLENGTH, 1);
	shift = melpe_norm_s(fft_len2);
	step = melpe_shl(2, shift);
	theta = 0;

	for (i = 0; i < fft_len2; i++) {
		ff->wr_array[i] = cos_fxp(theta);
		ff->wi_array[i] = sin_fxp(theta);
		if (i == (fft_len2 - 1))
			theta = ONE_Q15;
		else
			theta = melpe_add(theta, step);
	}
	return (void*) ff;
}
#else /* !POSIX_TI_EMULATION && FIXED_COMPLEX_MULTIPLY */

/* MELPe Vocoder FFT implementation optimized for C674X */
typedef struct _fft_context {
	unsigned w_array[FFTLENGTH / 2];
} fft_context;

  
static inline int16_t v_getmaxabs2(unsigned* d2, int16_t n2) 
{
	register int16_t i;
	int32_t m2 = 0;
	_nassert ((int) d2 & 3 == 0);
	
	#pragma MUST_ITERATE(256,256)
	for (i = 0; i < n2; i++, d2++) {
		m2 = _max2(m2, _abs2(_amem4_const(d2)));
	}
	return _max2(m2 >> 16, m2 & 0xFFFF);
}

static inline void v_shr2(unsigned* d2, int16_t n2, int16_t shift_count) {
	register int16_t i;
	_nassert ((int) d2 & 3 == 0);
	#pragma MUST_ITERATE(256,256)
	for (i = 0; i < n2; i++, d2++) {
		_amem4(d2) = _shr2(_amem4_const(d2), shift_count);
	}
}


/* Radix-2, DIT, 2N-point Real FFT */
void rfft(void* fft_handle, int16_t data[], int16_t n)
{
	int16_t i, n_2, n_4;
	int16_t r1, i1;
	int16_t data_max;
	unsigned* data32;
	unsigned R12, L12, C1, C2, W;
	
	fft_context* ff = (fft_context*) fft_handle;
	data32 = (unsigned*) data;
	n_2 = n >> 1; 
	n_4 = n_2 >> 1;
	
	cfft(fft_handle, data, n_2);
	/* Check for overflow */
	data_max = v_getmaxabs2(data32, n_2);
	
	if (data_max > 16383) {
		v_shr2(data32, n_2, 1);
	}
	
	for (i = 1; i < n_4; i++) {
		R12 = _shr2(_sadd2(data32[i], data32[n_2 - i]), 1); /* r1, r2 */
		C1 = _shr2(_ssub2(data32[i], data32[n_2 - i]), 1); /* L_temp2, L_temp1 */
		L12 = _rotl(C1, 16); /* L_temp1, L_temp2 */
		
		data32[i] = R12; /* r1, r2 */
		data32[n_2 - i] = R12; /* r1, r2 */
		data32[n - i] = L12; /* L1, L2 */
		data32[n_2 + i] = _ssub2(0, L12); /* -L1, -L2 */
	}
	data32[n_2 + n_4] = 0;
	
	
	r1 = melpe_add(data[0], data[1]);
	i1 = melpe_sub(data[0], data[1]);
	
	data32[0] = _pack2(0, r1);
	data32[n_2] = _pack2(0, i1);
	

	for (i = 1; i < n_2; i ++) 
	{
		if (i < FFTLENGTH / 2) {
			W = ff->w_array[i];
		}
		/* W=(wi, wr) */
		R12 = data32[i]; /* (r2, r1) */
		L12 = data32[n - i];  /* (i2, i1) */
		
		C1 = _pack2(R12, L12); /* (r1, i1) */
		C2 = _packh2(L12, R12); /* (i2, r2) */
		C2 = _ssub2(0, C2);  /* (-i2, -r2) */
		C2 = _cmpyr1(W, C2); /* (r2*wr - i2*wi, -r2*wi - i2*wr) */
		
		C1 = _rotl(_sadd2(C1, C2), 16) ; /* (r1 + r2*wr - i2*wi, i1 -r2*wi - i2*wr) */
		data32[i] = C1;  /* (data[i+1], data[i]) = (i1-r2*wi-i2*wr,  r1 + r2*wr-i2*wi)*/
		data32[n - i] = _packhl2(_ssub2(0, C1), C1);  /* (data[2*n-i+1], data[2*n-i]) */
	}
}

/* Subroutine FFT: Fast Fourier Transform */
int16_t cfft(void* fft_handle, int16_t data[], int16_t nn)
{
	fft_context* ff = (fft_context*) fft_handle;
	register int16_t i, j, m;
	register unsigned temp;
	int16_t mmax, istep, n_4;
	int16_t data_max;
	int16_t index, index_step;
	int16_t guard;
	unsigned C1, C2, W, WC;
	unsigned* data32 = (unsigned*) data;
	
	guard = 0;

	n_4 = nn >> 1;
	j = 0;
	for (i = 0; i < nn; i++) {
		if (j > i) {
			temp = data32[j]; data32[j] = data32[i]; data32[i]=temp;
		}
		m = n_4;
		while (m >= 1 && j >= m) {
			j-=m; 
			m=m>>1;
		}
		j+=m;
	}

	/* Stage 1 */
	/* Check for Overflow */
	data_max = v_getmaxabs2(data32, nn);
	if (data_max > 16383) {
		guard += 1;
		v_shr2(data32, nn, 1);
	}
	/* End of checking */
	for (i = 0; i < nn; i += 2) {
		C1 = data32[i];
		C2 = data32[i + 1];
		data32[i] = _sadd2(C1, C2);
		data32[i + 1] = _ssub2(C1, C2);
	}
	/* Stage 2 */
	/* Check for Overflow */
	data_max = v_getmaxabs2(data32, nn);
	if (data_max > 16383) {
		guard += 1;
		v_shr2(data32, nn, 1);
	}
	/* End of checking */
	for (i = 0; i < nn; i += 4) {
		/* Butterfly 1 */
		C1 = data32[i];
		C2 = data32[i + 2];
		data32[i] = _sadd2(C1, C2);
		data32[i + 2] = _ssub2(C1, C2);
		
		/* Butterfly 2 */
		W = data32[i + 1]; /*(sPI, sPR) */
		WC = _rotl(data32[i + 3], 16) ; /*(sQR, sQI) */
		C1 = _ssub2(W, WC);
		C2 = _sadd2(W, WC);

		data32[i + 1] = _packhl2(C1, C2); /* (sPI-sQR, sPR+sQI) */
		data32[i + 3] = _packhl2(C2, C1); /* (sPI+sQR, sPR-sQI) */
		
	}
	/* Stages 3... */
	mmax = 4;

	/* initialize index step */
	index_step = nn >> 1;

	while (nn > mmax) {
		/* Overflow checking */
		data_max = v_getmaxabs2(data32, nn);
		
		if (data_max > 16383) {
			guard += 2;
			v_shr2(data32, nn, 2);
		} else if (data_max > 8191) {
			guard += 1;
			v_shr2(data32, nn, 1);
		}
		/* Checking ends */
		istep = mmax << 1;	/* istep = mmax*2 */

		index = 0;
		index_step = index_step >> 1;

		W = _pack2(0, ONE_Q15);
		for (m = 0; m < mmax; m++) {
			for (i = m; i < nn; i+=istep) {
				j = i + mmax;
				
				/* W=(wi, wr) */
				C1 = data32[i]; /* C1 = (data[i + 1], data[i]) */
				C2 = _rotl(data32[j], 16); /* C2 = (data[j], data[j + 1]) */
				
				WC = _cmpyr1(W, C2);
				
				/* W = (wr*data[j + 1]-wi*data[j]), (wr*data[j]+ wi*data[j + 1]) */
				C2 = _sadd2(C1, WC); /*(data[i + 1]+ Re(C1*W), data[i] + Im(C1*W)) */
				C1 = _ssub2(C1, WC); /*(data[i + 1]- Re(C1*W), data[i] - Im(C1*W)) */
				
				data32[i] = _packhl2(C1, C2);  /* (data[i + 1], data[i]) = (extract_h(C1), extract_l(C2)) */ 
				data32[j] = _packhl2(C2, C1);  /* (data[j + 1], data[j]) = (extract_h(C2), extract_l(C1)) */ 
				
				
			}
			index += index_step;
			if (!(index >= FFTLENGTH / 2)) {
				W = ff->w_array[index];
			}
		}
		mmax = istep;
	}
	return guard;
}

/* Initialization of ff->wr_array and ff->wi_array */
void* fft_init(void* context)
{
	int16_t i, wi, wr;
	
	int16_t fft_len2, shift, step, theta;
	fft_context* ff = (fft_context*) context;
	if (!ff) ff = (fft_context*) malloc(sizeof(fft_context));
	if (!ff) return 0;
	memset(ff, 0x00, sizeof(fft_context));
	

	fft_len2 = melpe_shr(FFTLENGTH, 1);
	shift = melpe_norm_s(fft_len2);
	step = melpe_shl(2, shift);
	theta = 0;

	for (i = 0; i < fft_len2; i++) {
		wr = cos_fxp(theta);
		wi = sin_fxp(theta);
		
		ff->w_array[i] = _pack2(wi, wr);
		
		if (i == (fft_len2 - 1))
			theta = ONE_Q15;
		else
			theta = melpe_add(theta, step);
	}
	return (void*) ff;
}

#endif /* #if (!defined FIXED_COMPLEX_MULTIPLY) || defined POSIX_TI_EMULATION */

#define FFT_NPP_SIZE 256
#define FFT_NPP_SIZE_DIV2 128

int16_t fft_npp(void* fft_handle, int16_t data[], int16_t dir)
{
	int16_t guard, n;
	int32_t * d0 = (int32_t*) data;
	int32_t * dn = d0 + (FFT_NPP_SIZE -1);
	int32_t temp;
	
	guard = cfft(fft_handle, data, FFT_NPP_SIZE);
	
	if (dir < 0) {		/* Reverse FFT */
		d0++;
		for (n = 1; n < FFT_NPP_SIZE_DIV2; n++) {
			/* reverse order of 32 bit complex data, counting from 1 */
			temp = *d0;
			*d0 = *dn;
			*dn = temp;
			d0++; dn--;
		}
	}
	return guard;
}
