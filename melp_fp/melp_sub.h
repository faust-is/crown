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

  melp_sub.h: include file for MELP subroutines

*/

#ifndef __MELP_SUB_H
#define __MELP_SUB_H

void dc_rmv(float sigin[], float sigout[], float dcdel[], float* tmp_buf);
void bpvc_ana(float speech[], float fpitch[], float bpvc[], float sub_pitch[], float **bpfdel, float **envdel,  float *envdel2, float *sigbuf);
float gain_ana(float sigin[], float pitch, int16_t minlength, int16_t maxlength);
void q_gain(float *gain,int16_t *gain_index, float* prev_gain);
void q_gain_dec(float *gain,int16_t *gain_index, float* prev_gain, int16_t* p_err);
int16_t q_bpvc(float *bpvc,int16_t *bpvc_index,float bpthresh);
void q_bpvc_dec(float *bpvc,int16_t *bpvc_index,int16_t uv_flag);
void scale_adj(float *speech, float gain, float *prev_scale, int16_t length);
float lin_int_bnd(float x,float xmin,float xmax,float ymin,float ymax);
void noise_est(float gain,float *noise_gain,float up,float down,float min,float max);
void noise_sup(float *gain,float noise_gain,float max_noise,float max_atten,float nfact);

#endif
