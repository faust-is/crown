/* ======================================================================= */
/* DSPF_sp_fftSPxSP_cn.c -- Forward FFT with Mixed Radix                   */
/*                 Natural C Implementation                                */
/*                                                                         */
/* Rev 0.0.3                                                               */
/*                                                                         */
/* Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/  */ 
/*                                                                         */
/*                                                                         */
/*  Redistribution and use in source and binary forms, with or without     */
/*  modification, are permitted provided that the following conditions     */
/*  are met:                                                               */
/*                                                                         */
/*    Redistributions of source code must retain the above copyright       */
/*    notice, this list of conditions and the following disclaimer.        */
/*                                                                         */
/*    Redistributions in binary form must reproduce the above copyright    */
/*    notice, this list of conditions and the following disclaimer in the  */
/*    documentation and/or other materials provided with the               */
/*    distribution.                                                        */
/*                                                                         */
/*    Neither the name of Texas Instruments Incorporated nor the names of  */
/*    its contributors may be used to endorse or promote products derived  */
/*    from this software without specific prior written permission.        */
/*                                                                         */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS    */
/*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  */
/*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT   */
/*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  */
/*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  */
/*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY  */
/*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   */
/*                                                                         */
/* ======================================================================= */


#include "../DSPF_sp_fftSPxSP.h"

static const int32_t bitrev_table[64] = {
0x00, 0x40, 0x20, 0x60, 0x10, 0x50, 0x30, 0x70, 0x08, 0x48, 0x28, 0x68, 0x18, 0x58, 0x38, 0x78,
0x04, 0x44, 0x24, 0x64, 0x14, 0x54, 0x34, 0x74, 0x0c, 0x4c, 0x2c, 0x6c, 0x1c, 0x5c, 0x3c, 0x7c,
0x02, 0x42, 0x22, 0x62, 0x12, 0x52, 0x32, 0x72, 0x0a, 0x4a, 0x2a, 0x6a, 0x1a, 0x5a, 0x3a, 0x7a,
0x06, 0x46, 0x26, 0x66, 0x16, 0x56, 0x36, 0x76, 0x0e, 0x4e, 0x2e, 0x6e, 0x1e, 0x5e, 0x3e, 0x7e,
};

void DSPF_sp_fftSPxSP (int32_t N, float *ptr_x, const float *ptr_w, float *ptr_y,
    const unsigned char *brev, int32_t n_min, int32_t offset, int32_t n_max)
{

    int32_t i, j, k, l1, l2, h2, predj;
    int32_t tw_offset, stride, fft_jmp;

    float x0, x1, x2, x3, x4, x5, x6, x7;
    float xt0, yt0, xt1, yt1, xt2, yt2, yt3;
    float yt4, yt5, yt6, yt7;
    float si1, si2, si3, co1, co2, co3;
    float xh0, xh1, xh20, xh21, xl0, xl1, xl20, xl21;
    float x_0, x_1, x_l1, x_l1p1, x_h2, x_h2p1, x_l2, x_l2p1;
    float xl0_0, xl1_0, xl0_1, xl1_1;
    float xh0_0, xh1_0, xh0_1, xh1_1;
    float *x;
    const float * w;
    int32_t radix;
    float *y0, *ptr_x0, *ptr_x2;

    radix = n_min;

    stride = N;                                                /* n is the number of complex samples */
    tw_offset = 0;
    while (stride > radix)
    {
        j = 0;
        fft_jmp = stride + (stride >> 1);
        h2 = stride >> 1;
        l1 = stride;
        l2 = stride + (stride >> 1);
        x = ptr_x;
        w = ptr_w + tw_offset;

        for (i = 0; i < N; i += 4)
        {
            co1 = w[j];
            si1 = w[j + 1];
            co2 = w[j + 2];
            si2 = w[j + 3];
            co3 = w[j + 4];
            si3 = w[j + 5];

            x_0 = x[0];
            x_1 = x[1];
            x_h2 = x[h2];
            x_h2p1 = x[h2 + 1];
            x_l1 = x[l1];
            x_l1p1 = x[l1 + 1];
            x_l2 = x[l2];
            x_l2p1 = x[l2 + 1];

            xh0 = x_0 + x_l1;
            xh1 = x_1 + x_l1p1;
            xl0 = x_0 - x_l1;
            xl1 = x_1 - x_l1p1;

            xh20 = x_h2 + x_l2;
            xh21 = x_h2p1 + x_l2p1;
            xl20 = x_h2 - x_l2;
            xl21 = x_h2p1 - x_l2p1;

            ptr_x0 = x;
            ptr_x0[0] = xh0 + xh20;
            ptr_x0[1] = xh1 + xh21;

            ptr_x2 = ptr_x0;
            x += 2;
            j += 6;
            predj = (j - fft_jmp);
            if (!predj)
                x += fft_jmp;
            if (!predj)
                j = 0;

            xt0 = xh0 - xh20;
            yt0 = xh1 - xh21;
            xt1 = xl0 + xl21;
            yt2 = xl1 + xl20;
            xt2 = xl0 - xl21;
            yt1 = xl1 - xl20;

            ptr_x2[l1] = xt1 * co1 + yt1 * si1;
            ptr_x2[l1 + 1] = yt1 * co1 - xt1 * si1;
            ptr_x2[h2] = xt0 * co2 + yt0 * si2;
            ptr_x2[h2 + 1] = yt0 * co2 - xt0 * si2;
            ptr_x2[l2] = xt2 * co3 + yt2 * si3;
            ptr_x2[l2 + 1] = yt2 * co3 - xt2 * si3;
        }
        tw_offset += fft_jmp;
        stride = stride >> 2;
    }                                                        /* end while */

    j = offset >> 2;

    ptr_x0 = ptr_x;
    y0 = ptr_y;

    if (radix <= 4)
        for (i = 0; i < N; i += 4)
        {
            /* reversal computation */
            k = bitrev_table[j];
            j++;                                            /* multiple of 4 index */

            x0 = ptr_x0[0];
            x1 = ptr_x0[1];
            x2 = ptr_x0[2];
            x3 = ptr_x0[3];
            x4 = ptr_x0[4];
            x5 = ptr_x0[5];
            x6 = ptr_x0[6];
            x7 = ptr_x0[7];
            ptr_x0 += 8;

            xh0_0 = x0 + x4;
            xh1_0 = x1 + x5;
            xh0_1 = x2 + x6;
            xh1_1 = x3 + x7;


            yt0 = xh0_0 + xh0_1;
            yt1 = xh1_0 + xh1_1;
            yt4 = xh0_0 - xh0_1;
            yt5 = xh1_0 - xh1_1;

            xl0_0 = x0 - x4;
            xl1_0 = x1 - x5;
            xl0_1 = x2 - x6;
            xl1_1 = x3 - x7;


            yt2 = xl0_0 + xl1_1;
            yt3 = xl1_0 - xl0_1;
            yt6 = xl0_0 - xl1_1;
            yt7 = xl1_0 + xl0_1;


            y0[k] = yt0;
            y0[k + 1] = yt1;
            k += n_max >> 1;
            y0[k] = yt2;
            y0[k + 1] = yt3;
            k += n_max >> 1;
            y0[k] = yt4;
            y0[k + 1] = yt5;
            k += n_max >> 1;
            y0[k] = yt6;
            y0[k + 1] = yt7;
        }
}

void DSPF_sp_ifftSPxSP (int32_t N, float *ptr_x, const float *ptr_w, float *ptr_y, const unsigned char* brev,
    int32_t n_min, int32_t offset, int32_t n_max)
{

    int32_t i, j, k, l1, l2, h2, predj;
    int32_t tw_offset, stride, fft_jmp;

    float x0, x1, x2, x3, x4, x5, x6, x7;
    float xt0, yt0, xt1, yt1, xt2, yt2, yt3;
    float yt4, yt5, yt6, yt7;
    float si1, si2, si3, co1, co2, co3;
    float xh0, xh1, xh20, xh21, xl0, xl1, xl20, xl21;
    float x_0, x_1, x_l1, x_l1p1, x_h2, x_h2p1, x_l2, x_l2p1;
    float xl0_0, xl1_0, xl0_1, xl1_1;
    float xh0_0, xh1_0, xh0_1, xh1_1;
    float *x;
    const float *w;
    int32_t radix;
    float *y0, *ptr_x0, *ptr_x2;
    float scale;

    radix = n_min;

    stride = N;                                                /* n is the number of complex samples */
    tw_offset = 0;
    while (stride > radix)
    {
        j = 0;
        fft_jmp = stride + (stride >> 1);
        h2 = stride >> 1;
        l1 = stride;
        l2 = stride + (stride >> 1);
        x = ptr_x;
        w = ptr_w + tw_offset;

        for (i = 0; i < N; i += 4)
        {
            co1 = w[j];
            si1 = w[j + 1];
            co2 = w[j + 2];
            si2 = w[j + 3];
            co3 = w[j + 4];
            si3 = w[j + 5];

            x_0 = x[0];
            x_1 = x[1];
            x_h2 = x[h2];
            x_h2p1 = x[h2 + 1];
            x_l1 = x[l1];
            x_l1p1 = x[l1 + 1];
            x_l2 = x[l2];
            x_l2p1 = x[l2 + 1];

            xh0 = x_0 + x_l1;
            xh1 = x_1 + x_l1p1;
            xl0 = x_0 - x_l1;
            xl1 = x_1 - x_l1p1;

            xh20 = x_h2 + x_l2;
            xh21 = x_h2p1 + x_l2p1;
            xl20 = x_h2 - x_l2;
            xl21 = x_h2p1 - x_l2p1;

            ptr_x0 = x;
            ptr_x0[0] = xh0 + xh20;
            ptr_x0[1] = xh1 + xh21;

            ptr_x2 = ptr_x0;
            x += 2;
            j += 6;
            predj = (j - fft_jmp);
            if (!predj)
                x += fft_jmp;
            if (!predj)
                j = 0;

            xt0 = xh0 - xh20;
            yt0 = xh1 - xh21;
            xt1 = xl0 - xl21;                                // xt1 = xl0 + xl21;
            yt2 = xl1 - xl20;                                // yt2 = xl1 + xl20;
            xt2 = xl0 + xl21;                                // xt2 = xl0 - xl21;
            yt1 = xl1 + xl20;                                // yt1 = xl1 - xl20;

            ptr_x2[l1] = xt1 * co1 - yt1 * si1;                // ptr_x2[l1 ] = xt1 * co1 + yt1 * si1;
            ptr_x2[l1 + 1] = yt1 * co1 + xt1 * si1;            // ptr_x2[l1+1] = yt1 * co1 - xt1 * si1;
            ptr_x2[h2] = xt0 * co2 - yt0 * si2;                // ptr_x2[h2 ] = xt0 * co2 + yt0 * si2;
            ptr_x2[h2 + 1] = yt0 * co2 + xt0 * si2;            // ptr_x2[h2+1] = yt0 * co2 - xt0 * si2;
            ptr_x2[l2] = xt2 * co3 - yt2 * si3;                // ptr_x2[l2 ] = xt2 * co3 + yt2 * si3;
            ptr_x2[l2 + 1] = yt2 * co3 + xt2 * si3;            // ptr_x2[l2+1] = yt2 * co3 - xt2 * si3;
        }
        tw_offset += fft_jmp;
        stride = stride >> 2;
    }                                                        /* end while */

    j = offset >> 2;

    ptr_x0 = ptr_x;
    y0 = ptr_y;
    scale = 0.00390625f;
    if (radix <= 4)
        for (i = 0; i < N; i += 4)
        {
            /* reversal computation */

            k = bitrev_table[j];
            j++;                                            /* multiple of 4 index */

            x0 = ptr_x0[0];
            x1 = ptr_x0[1];
            x2 = ptr_x0[2];
            x3 = ptr_x0[3];
            x4 = ptr_x0[4];
            x5 = ptr_x0[5];
            x6 = ptr_x0[6];
            x7 = ptr_x0[7];
            ptr_x0 += 8;

            xh0_0 = x0 + x4;
            xh1_0 = x1 + x5;
            xh0_1 = x2 + x6;
            xh1_1 = x3 + x7;


            yt0 = xh0_0 + xh0_1;
            yt1 = xh1_0 + xh1_1;
            yt4 = xh0_0 - xh0_1;
            yt5 = xh1_0 - xh1_1;

            xl0_0 = x0 - x4;
            xl1_0 = x1 - x5;
            xl0_1 = x2 - x6;
            xl1_1 = x7 - x3;                                // xl1_1 = x3 - x7;


            yt2 = xl0_0 + xl1_1;
            yt3 = xl1_0 + xl0_1;                            // yt3 = xl1_0 + (- xl0_1);
            yt6 = xl0_0 - xl1_1;
            yt7 = xl1_0 - xl0_1;                            // yt7 = xl1_0 - (- xl0_1);

            y0[k] = yt0 * scale;
            y0[k + 1] = yt1 * scale;
            k += n_max >> 1;
            y0[k] = yt2 * scale;
            y0[k + 1] = yt3 * scale;
            k += n_max >> 1;
            y0[k] = yt4 * scale;
            y0[k + 1] = yt5 * scale;
            k += n_max >> 1;
            y0[k] = yt6 * scale;
            y0[k + 1] = yt7 * scale;
        }

}

/* ======================================================================= */
/*  End of file:  DSPF_sp_fftSPxSP_GCC.c                                    */
/* ----------------------------------------------------------------------- */
/*            Copyright (c) 2011 Texas Instruments, Incorporated.          */
/*                           All Rights Reserved.                          */
/* ======================================================================= */
