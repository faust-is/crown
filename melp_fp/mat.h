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
   mat.h     Matrix include file.
             (Low level matrix and vector functions.)  

   Copyright (c) 1995 by Texas Instruments, Inc.  All rights reserved.
*/

#ifndef __MAT_H
#define __MAT_H

float v_inner(const float *v1, const float *v2,int16_t n);
float v_magsq(const float *v, int16_t n);
float *v_zap(float *v, int16_t n);
float *v_equ(float *v1, const float *v2, int16_t n);
float *v_sub(float *v1, const float *v2, int16_t n);
float *v_add(float *v1, const float *v2, int16_t n);
float *v_scale(float *v, float scale, int16_t n);
int16_t *v_zap_int(int16_t *v, int16_t n);
int16_t *v_equ_int(int16_t *v1, const int16_t *v2, int16_t n);

#endif
