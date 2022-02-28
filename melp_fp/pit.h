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

  pit.h: include file for pitch subroutines
  
*/

#ifndef __PIT_H
#define __PIT_H

/* External function definitions */
float double_chk(float sig_in[], float *pcorr, float pitch, float pdouble, int16_t pmin, int16_t pmax, int16_t lmin);
void double_ver(float sig_in[], float *pcorr, float pitch, int16_t pmin, int16_t pmax, int16_t lmin);
float find_pitch(float sig_in[],float *pcorr,int16_t lower,int16_t upper,int16_t length);
float frac_pch(float sig_in[], float *corr, float pitch, int16_t range, int16_t pmin, int16_t pmax, int16_t lmin);
float pitch_ana(void* pitch_handle, float speech[], float resid[], float pitch_est, float pitch_avg, float *pcorr2);
void* pitch_ana_init();
void pitch_ana_free(void* pitch_handle);
float p_avg_update(void* pitch_handle, float pitch, float pcorr, float pthresh);

#endif
