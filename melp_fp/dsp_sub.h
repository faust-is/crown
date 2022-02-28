/*

2.4 kbps MELP Proposed Federal Standard speech coder

version 1.2

Copyright (c) 1996, Texas Instruments, Inc.  

Texas Instruments has intellectual property rights on the MELP
algorithm.  The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).

*/

/*

  dsp_sub.h: include file 
  
*/
#ifndef __DSP_SUB_H
#define __DSP_SUB_H

#include <stdint.h>
#include "random.h"

/* External function definitions */
void autocorr(float input[], float r[], int16_t order, int16_t npts);
void envelope(float input[], float prev_in, float output[], int16_t npts);
void fill(float output[], float fillval, int16_t npts);
void interp_array(float prev[],float curr[],float out[],float ifact,int16_t size);
float median(float input[], int16_t npts);
void  pack_code(int16_t code, unsigned char **p_ch_beg,int16_t *p_ch_bit,int16_t numbits,int16_t size);
float peakiness(float input[], int16_t npts);
void polflt(float input[], const float* coeff, float output[], int16_t order,int16_t npts);
void quant_u(float *p_data, int16_t *p_index, float qmin, float qmax, int16_t nlev);
void quant_u_dec(int16_t index, float *p_data,float qmin, float qmax, int16_t nlev);
void rand_vec(float output[],float amplitude, int16_t npts, rand_context* p_rnd);
int16_t unpack_code(unsigned char **p_ch_beg,int16_t *p_ch_bit,int16_t *p_code,int16_t numbits,int16_t wsize,uint16_t erase_mask);
void window(float input[], const float* win_cof, float output[], int16_t npts);
void zerflt(float input[], const float* coeff, float output[], int16_t order,int16_t npts);

#endif
