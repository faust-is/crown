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
   vq.h     VQ include file.
            (Search/decode/distortion/weighting functions for VQ)

   Copyright (c) 1995 by Texas Instruments Incorporated.  All rights reserved.
*/

#ifndef __VQ_H_
#define __VQ_H_

float vq_ms4(const float *cb, float *u, int16_t *levels, float *w, float *u_hat, int16_t *a_indices,
  int16_t *indices, float *errors, float *uhatw, float *d, int16_t *parents);

float *vq_msd2(const float *cb, float *u, int16_t *indices, int16_t *levels, int16_t stages, int16_t p);
float *vq_lspw(float *w,float *lsp,float *a,int16_t p);

/* Structure definition */
struct msvq_param {         /* Multistage VQ parameters */
    int16_t *num_levels;
    int16_t *num_bits;
    int16_t *indices;
    char *fname_cb;
    float *cb;
};

/* External function definitions */

void msvq_init(struct msvq_param *par);

float vq_enc(const float *cb, float *u, int16_t levels, int16_t p, float *u_hat, int16_t *indices);

void vq_fsw(float *w_fs, int16_t num_harm, float pitch);


#endif
