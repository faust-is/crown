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
   lpc.h     LPC include file.
*/

#ifndef __LPC_H
#define __LPC_H

/* better names */
#define lpc_bw_expand          lpc_bwex
#define lpc_synthesis          lpc_syn
#define lpc_clamp              lpc_clmp

/* bandwidth expansion function */
int16_t lpc_bwex(float *a, float *aw, float gamma, int16_t p);
/* lpc synthesis filter */
int16_t lpc_syn(float *x,float *y,float *a,int16_t p,int16_t n);

/* sort LSPs and ensure minimum separation */
int16_t lpc_clmp(float *w, float delta, int16_t p);

/* lpc conversion routines */
/* convert predictor parameters to LSPs */
int16_t lpc_pred2lsp(float *a, float *w, float** c);
/* convert predictor parameters to reflection coefficients */
int16_t lpc_pred2refl(float *a, float *k);
/* convert LSPs to predictor parameters */
int16_t lpc_lsp2pred(float *w, float *a, float** f);
/* convert reflection coefficients to predictor parameters */
int16_t lpc_refl2pred(float *k, float *a);

/* schur recursion */
float lpc_schr(float *r, float *a, float *k, float *y1, float* y2);

/* evaluation of |A(e^jw)|^2 at a single point (using Horner's method) */
float lpc_aejw(float *a,float w,int16_t p);

#endif
