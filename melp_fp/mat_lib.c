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

  mat_lib.c: Matrix and vector manipulation library

*/

#include "spbstd.h"
#include "mat.h"

/* V_ADD- vector addition */
float *v_add(float *v1, const float *v2,int16_t n)
{
    int16_t i;

    for(i=0; i < n; i++)
        v1[i] += v2[i];
    return(v1);
}

/* V_EQU- vector equate */
float *v_equ(float *v1,const float *v2,int16_t n)
{
    int16_t i;

    for(i=0; i < n; i++)
        v1[i] = v2[i];
    return(v1);
}
int16_t *v_equ_int(int16_t *v1, const int16_t *v2, int16_t n)
{
    int16_t i;

    for(i=0; i < n; i++)
        v1[i] = v2[i];
    return(v1);
}

/* V_INNER- inner product */
float v_inner(const float *v1, const float *v2, int16_t n)
{
    int16_t i;
    float innerprod;

    for(i=0,innerprod=0.0; i < n; i++)
        innerprod += v1[i] * v2[i];
    return(innerprod);
}

/* v_magsq - sum of squares */

float v_magsq(const float *v,int16_t n)
{
    int16_t i;
    float magsq;

    for(i=0,magsq=0.0; i < n; i++)
        magsq += v[i] * v[i];
    return(magsq);
} /* V_MAGSQ */

/* V_SCALE- vector scale */
float *v_scale(float *v,float scale,int16_t n)
{
    int16_t i;

    for(i=0; i < n; i++)
        v[i] *= scale;
    return(v);
}

/* V_SUB- vector difference */
float *v_sub(float *v1, const float *v2, int16_t n)
{
    int16_t i;

    for(i=0; i < n; i++)
        v1[i] -= v2[i];
    return(v1);
}

/* v_zap - clear vector */

float *v_zap(float *v,int16_t n)
{
    int16_t i;

    for(i=0; i < n; i++)
        v[i] = 0.0;
    return(v);
} /* V_ZAP */

int16_t *v_zap_int(int16_t *v,int16_t n)
{
    int16_t i;

    for(i=0; i < n; i++)
        v[i] = 0;
    return(v);
} /* V_ZAP */
