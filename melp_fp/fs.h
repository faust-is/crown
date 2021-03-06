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
 
  fs.h: Fourier series functions include file

*/
#ifndef __FS_H
#define __FS_H

void fft(float *datam1,int16_t nn,int16_t isign);
void find_harm(float* input, float* fsmag, float* fftbuf, float pitch, int16_t num_harm, int16_t length);
int16_t findmax(float input[], int16_t npts);
void idft_real(float real[], float signal[], int16_t length);

#endif
