
#ifndef _LIBNPP_H_
#define _LIBNPP_H_

#include <stdint.h>
#include "spbstd.h"

/* ====== Parameters from Noise_Params_Minstat ====== */

#define WTR_FRONT_LEN                   32
#define GM_MIN                          0.12 /* max value for Min. Gain Modif. Factor */

#define NOISE_B_BY_NSMTH_B              1.41421356237309     /* bias factor for initial noise estimate, sqrt(2) */
#define GAMMAX_THR                      35.35533905932737    /* gamma_max threshold = 50/NOISE_BIAS*/

#define GAMMAV_THR                      1.41421356237309     /* gamma_av threshold, sqrt(2) */
#define LEN_MINWIN                      9    /* length of subwindow */
#define NUM_MINWIN                      8    /* number of subwindows */
#define ALPHA_N_MAX                     0.94 /* maximum of time varying smoothing parameter */
#define PDECAY_NUM                      -0.3125
#define MINV                            1.19283835953  /* parameter used to compute the minimum bias */

#define MINV_SUB                        1.7051077144138
#define MINV2                           1.422863351963
#define MINV_SUB2                       2.907392317753447
#define FVAR                            8.066905819615
#define FVAR_SUB                        1.13786305163652

/* ====== Parameters from Enhance_Params ====== */

#define USE_SYN_WINDOW                  TRUE    /* switch on synthesis window */

#define ENH_WINLEN                      256     /* frame length */
#define ENH_WINLEN_HALF                 128
#define ENH_WIN_SHIFT                   180     /* frame advance suitable for MELP coder */
#define ENH_OVERLAP_LEN                 (ENH_WINLEN - ENH_WIN_SHIFT)
                                        /* length of overlapping of adjacent frames, 76 */
#define ENH_VEC_LENF                    (ENH_WINLEN_HALF + 1)    /* length of spectrum vectors */
#define ENH_NOISE_FRAMES                1    /* number of initial look ahead frames */
#define ENH_NOISE_BIAS                  1.41421356237309    /* noise overestimation factor, 1.5dB = sqrt(2) */
#define ENH_INV_NOISE_BIAS              0.707106781187      /* 1.0/ENH_NOISE_BIAS, Q15 */
#define ENH_ALPHA_LT                    0.977273            /* smoothing constant for long term SNR */
#define ENH_BETA_LT                     0.022727            /* 0.022727, Q15 */
#define ENH_QK_MAX                      0.999 /* Max value for prob. of signal absence */
#define ENH_QK_MIN                      0.001 /* 33 ??? Min value for prob. of signal absence */
#define ENH_ALPHAK                      0.93  /* decision directed ksi weight */
#define ENH_BETAK                       0.07  /* 0.07, Q18 */
#define ENH_GAMMAQ_THR                  0.588154886   /* 0.588154886, Q15 */
#define ENH_ALPHAQ                      0.93375 /* Smoothing factor of hard-decision qk-value, 0.93375 */
#define ENH_BETAQ                       0.06625 /* 0.06625, Q15 */
#define ENH_ALPHA_ALPHACORR             0.7
#define ENH_SNLT_RATIO                  1875000.0  /* 180.0 x 1000000 / 96.0, where 180.0=sum of Tukey window */
#define ENH_WINLEN_INV                  0.00390625  /* 1.0 / ENH_WINLEN */

/* Must be at least 4-word aligned (for DSP), word size 32 bit */
typedef struct _npp_context {
    /* ====== Entities from Enhanced_Data ====== */
    float ybuf[2 * ENH_WINLEN + 2];
    float fftbuf[2 * ENH_WINLEN + 2];
     /* buffer for FFT, this can be eliminated if
      * we can write a better real-FFT program for DSP */

    float lambdaD[ENH_VEC_LENF + 1];   /* overestimated noise  psd(noise_bias * noisespect) */

    float YY[ENH_VEC_LENF + 1]; /* signal periodogram of current frame */
    float act_min[ENH_VEC_LENF + 1];   /* current minimum of long window */
    float act_min_sub[ENH_VEC_LENF + 1]; /* current minimum of sub-window */
    float vk[ENH_VEC_LENF + 1];
    float ksi[ENH_VEC_LENF + 1];    /* a priori SNR */

    float var_rel[ENH_VEC_LENF + 1]; /* est. relative var. of smoothedspect */
    /* only for minimum statistics */

    float smoothedspect[ENH_VEC_LENF + 1]; /* smoothed signal spectrum */
    float var_sp_av[ENH_VEC_LENF + 1];     /* estimated mean of smoothedspect */
    float var_sp_2[ENH_VEC_LENF + 1];      /* esitmated 2nd moment of smoothedspect */
    float circb[NUM_MINWIN][ENH_VEC_LENF + 1];  /* ring buffer */

    float initial_noise[ENH_WINLEN];       /* look ahead noise samples (Q0) */
    float speech_in_npp[ENH_WINLEN];       /* input of one frame */

    float speech_overlap[ENH_OVERLAP_LEN];
    float qla[ENH_VEC_LENF + 1];
    BOOLEAN localflag[ENH_VEC_LENF + 1];   /* local minimum indicator */
    float circb_min[ENH_VEC_LENF + 1];     /* minimum of circular buffer */
    float agal[ENH_VEC_LENF + 1];          /* GainD * Ymag */

    float qk[ENH_VEC_LENF + 1];            /* probability of noise only, Q15 */
    float Gain[ENH_VEC_LENF + 1];          /* MMSE LOG STA estimator gain */

    float YY_LT;
    float SN_LT0;
    int32_t npp_counter;                   /* formerly D->I */
    float SN_LT;                           /* long term SNR, longterm_snr */
    float n_pwr;
    float var_rel_av;                      /* average relative var. of smoothedspect */
    int32_t minspec_counter;               /* count sub-windows */
    int32_t circb_index;                   /* ring buffer counter */
    float alphacorr;                       /* correction factor for alpha_var, Q15 */
    float Ksi_min_var;
    BOOLEAN first_time;
} npp_context;

void npp(npp_context* n, float* sp_in, float* sp_out);
void npp_init(npp_context* n);

#endif
