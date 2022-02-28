
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "mat.h"
#include "npp_fl.h"
#include "DSPF_sp_fftSPxSP.h"
#include "dsp_sub.h"
#include "special.h"

extern const float fft_twiddle[512];


/* Tukey window function, square root */
static const float sqrt_tukey_256_180[ENH_WINLEN] = {
0.02066690122755,0.04132497424881,0.06196539462859,0.08257934547233,0.10315802119236,0.12369263126935,0.14417440400735,0.16459459028073,
0.18494446727156,0.20521534219563,0.22539855601581,0.24548548714080,0.26546755510807,0.28533622424911,0.30508300733555,0.32469946920468,
0.34417723036264,0.36350797056383,0.38268343236509,0.40169542465297,0.42053582614271,0.43919658884737,0.45766974151568,0.47594739303707,
0.49402173581250,0.51188504908960,0.52952970226071,0.54694815812243,0.56413297609525,0.58107681540194,0.59777243820324,0.61421271268967,
0.63039061612796,0.64629923786094,0.66193178225957,0.67728157162574,0.69234204904483,0.70710678118655,0.72156946105306,0.73572391067313,
0.74956408374113,0.76308406819981,0.77627808876576,0.78914050939639,0.80166583569749,0.81384871727019,0.82568394999656,0.83716647826253,
0.84829139711757,0.85905395436989,0.86944955261637,0.87947375120649,0.88912226813919,0.89839098189198,0.90727593318156,0.91577332665506,
0.92387953251129,0.93159108805128,0.93890469915743,0.94581724170063,0.95232576287481,0.95842748245825,0.96411979400121,0.96940026593933,
0.97426664263229,0.97871684532735,0.98274897304736,0.98636130340272,0.98955229332720,0.99232057973705,0.99466498011324,0.99658449300667,
0.99807829846587,0.99914575838730,0.99978641678793,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,
1.00000000000000,1.00000000000000,1.00000000000000,1.00000000000000,0.99978641678793,0.99914575838730,0.99807829846587,0.99658449300667,
0.99466498011324,0.99232057973705,0.98955229332720,0.98636130340272,0.98274897304736,0.97871684532735,0.97426664263229,0.96940026593933,
0.96411979400121,0.95842748245825,0.95232576287481,0.94581724170063,0.93890469915743,0.93159108805128,0.92387953251129,0.91577332665506,
0.90727593318156,0.89839098189198,0.88912226813919,0.87947375120649,0.86944955261637,0.85905395436989,0.84829139711757,0.83716647826253,
0.82568394999656,0.81384871727019,0.80166583569749,0.78914050939639,0.77627808876576,0.76308406819981,0.74956408374113,0.73572391067313,
0.72156946105306,0.70710678118655,0.69234204904483,0.67728157162574,0.66193178225957,0.64629923786094,0.63039061612796,0.61421271268967,
0.59777243820324,0.58107681540194,0.56413297609525,0.54694815812243,0.52952970226071,0.51188504908960,0.49402173581250,0.47594739303707,
0.45766974151568,0.43919658884737,0.42053582614271,0.40169542465297,0.38268343236509,0.36350797056383,0.34417723036264,0.32469946920468,
0.30508300733555,0.28533622424911,0.26546755510807,0.24548548714080,0.22539855601581,0.20521534219563,0.18494446727156,0.16459459028073,
0.14417440400735,0.12369263126935,0.10315802119236,0.08257934547233,0.06196539462859,0.04132497424881,0.02066690122755,0.0
};


/* ====== Prototypes ====== */

static void smoothing_win(float* initial_noise);
static void compute_qk(npp_context* n, float gamaK[], float GammaQ_TH);
static void gain_log_mmse(npp_context* n, float gamaK[], int32_t m);
static float ksi_min_adapt(BOOLEAN n_flag, float ksi_min, float sn_lt);
static void smoothed_periodogram(npp_context* n, float YY_av);
static float noise_slope(npp_context* n);
static void min_search(npp_context* n, float biased_spect[],float biased_spect_sub[]);
static void enh_init(npp_context* n);
static void minstat_init(npp_context* n);
static void process_frame(npp_context* n, float inspeech[], float outspeech[]);
static void gain_mod(npp_context* n, float GainD[], int32_t m);
void fft_npp(npp_context* n, int16_t dir);

/****************************************************************************
**
** Function:        npp
**
** Description:     The noise pre-processor
**
** Arguments:
**
**    float    sp_in[]  ---- input speech data buffer (SP)
**    float    sp_out[] ---- output speech data buffer (SP)
**
** Return value:    None
**
*****************************************************************************/
void npp(npp_context* n, float* sp_in, float* sp_out)
{
    float speech_out_npp[ENH_WINLEN];   /* output of one frame */

    if (n->first_time) 
    {
        v_zap(n->initial_noise, ENH_OVERLAP_LEN);
        v_equ(&n->initial_noise[ENH_OVERLAP_LEN], sp_in, ENH_WIN_SHIFT);
        enh_init(n);    /* Initialize enhancement routine */
        v_zap(n->speech_in_npp, ENH_WINLEN);
        n->first_time = FALSE;
    }

    /* Shift input buffer from the previous frame */
    v_equ(n->speech_in_npp, &(n->speech_in_npp[ENH_WIN_SHIFT]), ENH_OVERLAP_LEN);
    v_equ(&(n->speech_in_npp[ENH_OVERLAP_LEN]), sp_in, ENH_WIN_SHIFT);

    /* ======== Process one frame ======== */
    process_frame(n, n->speech_in_npp, speech_out_npp);

    /* Overlap-add the output buffer. */
    v_add(speech_out_npp, n->speech_overlap, ENH_OVERLAP_LEN);
    v_equ(n->speech_overlap, &(speech_out_npp[ENH_WIN_SHIFT]), ENH_OVERLAP_LEN);

    /* ======== Output enhanced speech ======== */
    v_equ(sp_out, speech_out_npp, ENH_WIN_SHIFT);
}

/*****************************************************************************/
/*      Subroutine gain_mod: compute gain modification factor based on       */
/*      signal absence probabilities qk                                      */
/*****************************************************************************/
static void gain_mod(npp_context* n, float GainD[], int32_t m) {
    int32_t i;
    float pk, temp, gm;

    for (i = 0; i < m; i++) {
        pk = 1.0 - n->qk[i];
        temp = pk * pk;
        gm = temp/(temp + (pk + n->ksi[i]) * n->qk[i] * exp(-n->vk[i]));
        if (gm < GM_MIN) gm = GM_MIN;
            GainD[i] *= gm;
        }
}


/*****************************************************************************/
/*  Subroutine compute_qk: compute the probability of speech absence         */
/*  This uses an harddecision approach due to Malah (ICASSP 1999).           */
/*****************************************************************************/
static void compute_qk(npp_context* n, float gamaK[], float GammaQ_TH)
{
    int32_t i;

    /*      qla[] = alphaq * qla[]; */
    v_scale(n->qla, ENH_ALPHAQ, ENH_VEC_LENF);
    for (i = 0; i < ENH_VEC_LENF; i++) {
        if (gamaK[i] < GammaQ_TH)
            n->qla[i] += ENH_BETAQ;
    }
    v_equ(n->qk, n->qla, ENH_VEC_LENF);

    /*      vec_limit_top(qk, qk, qk_max, vec_lenf); */
    /*      vec_limit_bottom(qk, qk, qk_min, vec_lenf); */
    for (i = 0; i < ENH_VEC_LENF; i++) {
        if (n->qk[i] > ENH_QK_MAX) n->qk[i] = ENH_QK_MAX;
        else if (n->qk[i] < ENH_QK_MIN) n->qk[i] = ENH_QK_MIN;
    }
}

/*****************************************************************************/
/*    Subroutine gain_log_mmse: compute the gain factor and the auxiliary    */
/*    variable vk for the Ephraim&Malah 1985 log spectral estimator.         */
/*    Approximation of the exponential integral due to Malah, 1996           */
/*****************************************************************************/
#define ENH_VK_LOW  2.220446049250313e-16 /* some floor of expint evaluation */
#define ENH_VK_LOW_VAL 5.02850745348e+7
#define ENH_VK_BIG1  200.0
#define ENH_VK_BIG2  30.0
static void gain_log_mmse(npp_context* n, float gamaK[],  int32_t m)
{
    int32_t i;
    float ksi_vq, pk;

    for (i = 0; i < m; i++) {
        pk = 1.0 - n->qk[i];
        ksi_vq = n->ksi[i] / (n->ksi[i] + pk);
        n->vk[i] = ksi_vq * gamaK[i];
        if (n->vk[i] < ENH_VK_LOW) {
            n->vk[i]= ENH_VK_LOW;
            n->Gain[i] = ksi_vq * ENH_VK_LOW_VAL;
        } else if (n->vk[i] > ENH_VK_BIG1) {
            n->vk[i] = ENH_VK_BIG1;
            n->Gain[i] = ksi_vq;
        } else if (n->vk[i] > ENH_VK_BIG2) {
           n->Gain[i] = ksi_vq;
        } else n->Gain[i] = ksi_vq * exp (0.5 * expint(n->vk[i]));

        if (n->Gain[i] > 1.0) n->Gain[i]=1.0;
    }
}


/*****************************************************************************/
/*         Subroutine ksi_min_adapt: computes the adaptive ksi_min           */
/*****************************************************************************/
static float ksi_min_adapt(BOOLEAN n_flag, float ksi_min, float sn_lt)
{
    float ksi_min_new, sum;

    if (n_flag) /* RM: adaptive modification of ksi limit (10/98) */
        ksi_min_new = ksi_min;
    else {
        sum =  sn_lt + 0.5;
        /* assert sum > 0 */
        if (sum <= 0.0) {
            ksi_min_new = GM_MIN;
        }
        ksi_min_new = GM_MIN * exp(0.65*log(sum)-5.0);
        if (ksi_min_new > 0.25) ksi_min_new = 0.25;
    }

    return (ksi_min_new);
}

/*****************************************************************************/
/* Subroutine smoothing_win: applies the Parzen window.  The window applies  */
/* an inverse trapezoid window and wtr_front[] supplies the coefficients for */
/* the two edges.                                                            */
/*****************************************************************************/
static const float wtr_front[WTR_FRONT_LEN] = {
    1.00000000000000,0.99432373046875,0.97802734375000,0.95220947265625,0.91796875000000,0.87640380859375,0.82861328125000,0.77569580078125,
    0.71875000000000,0.65887451171875,0.59716796875000,0.53472900390625,0.47265625000000,0.41204833984375,0.35400390625000,0.29962158203125,
    0.25000000000000,0.20599365234375,0.16748046875000,0.13409423828125,0.10546875000000,0.08123779296875,0.06103515625000,0.04449462890625,
    0.03125000000000,0.02093505859375,0.01318359375000,0.00762939453125,0.00390625000000,0.00164794921875,0.00048828125000,0.00006103515625
};

static void smoothing_win(float* initial_noise)
{
    int32_t i;

    for (i = 1; i < WTR_FRONT_LEN; i++)
    initial_noise[i] = initial_noise[i]* wtr_front[i];

    for (i = ENH_WINLEN - WTR_FRONT_LEN + 1; i < ENH_WINLEN; i++)
        initial_noise[i] = initial_noise[i] * wtr_front[ENH_WINLEN - i];

    /* Clearing the central part of initial_noise[]. */
    v_zap(&(initial_noise[WTR_FRONT_LEN]), ENH_WINLEN - 2 * WTR_FRONT_LEN + 1);
}

/*****************************************************************************/
/*    Subroutine smoothed_periodogram: compute short time psd with optimal   */
/*    smoothing                                                              */
/*****************************************************************************/
static void smoothed_periodogram(npp_context* n, float YY_av)
{
    int32_t i;
    float smoothed_av, alphacorr_new, alpha_N_min_1, alpha_num;
    float tmpalpha, sum;
    float temp;
    float beta_var;
    float inv_noisespect;

    /* ---- compute smoothed_av ---- */
    sum = n->smoothedspect[0] + n->smoothedspect[ENH_VEC_LENF - 1];
    for (i = 1; i < ENH_VEC_LENF - 1; i++) sum+=(2.0 * n->smoothedspect[i]);

    /* Here we can not multiply L_sum with                                   */
    /* ENH_WINLEN_INV because we do not do this on YY_av either.             */

    smoothed_av = sum;
    temp = smoothed_av / YY_av - 1.0;
    alphacorr_new = 1.0 / (1.0+temp*temp);


    if (alphacorr_new < ENH_ALPHA_ALPHACORR)
        alphacorr_new = ENH_ALPHA_ALPHACORR;

    n->alphacorr = ENH_ALPHA_ALPHACORR * n->alphacorr + (1.0 - ENH_ALPHA_ALPHACORR) * alphacorr_new;
    /* -- alphacorr = 0.7*alphacorr + 0.3*alphacorr_new -- */
    alpha_N_min_1 = pow(n->SN_LT, PDECAY_NUM);

    alpha_N_min_1 = (0.3 < alpha_N_min_1 ? 0.3 : alpha_N_min_1);
    alpha_N_min_1 = (alpha_N_min_1 < 0.05 ? 0.05 : alpha_N_min_1);

    alpha_num = ALPHA_N_MAX * n->alphacorr;

    /* -- compute smoothed spectrum -- */
    for (i = 0; i < ENH_VEC_LENF; i++) {
        inv_noisespect = ENH_NOISE_BIAS/n->lambdaD[i];
        temp = n->smoothedspect[i] * inv_noisespect -1.0;
        tmpalpha = alpha_num/(1.0 + temp * temp);

        if (tmpalpha < alpha_N_min_1)
            tmpalpha = alpha_N_min_1;


        n->smoothedspect[i] = tmpalpha * n->smoothedspect[i] + (1.0 - tmpalpha) * n->YY[i];
        beta_var = tmpalpha * tmpalpha;
        if (beta_var > 0.8)
            beta_var = 0.8;
        n->var_sp_av[i] = beta_var * n->var_sp_av[i] + (1.0 - beta_var) * n->smoothedspect[i];
        n->var_sp_2[i] = beta_var * n->var_sp_2[i] + (1.0 - beta_var) * n->smoothedspect[i] * n->smoothedspect[i];
        temp = n->var_sp_2[i] - n->var_sp_av[i] * n->var_sp_av[i];
        temp *= inv_noisespect * inv_noisespect;
        if (temp > 0.5) temp = 0.5;
        if (temp < 0.0) temp = 0.0;
        n->var_rel[i] = temp;
    }
}


/*****************************************************************************/
/*    Subroutine noise_slope: compute maximum of the permitted increase of   */
/*    the noise estimate as a function of the mean signal variance           */
/*****************************************************************************/
static float noise_slope(npp_context* n)
{
    float noise_slope_max;

    if (n->var_rel_av > 0.18)
        noise_slope_max = 1.32;
    else if ((n->var_rel_av < 0.03) || (n->npp_counter < 50)) /* TODO: 49 by STANAG */
        noise_slope_max = 8.8;
    else if (n->var_rel_av < 0.05)
        noise_slope_max = 4.4;
    else if (n->var_rel_av < 0.06)
        noise_slope_max = 2.2;
    else
        noise_slope_max = 2.2; /* TODO: 2.2 by STANAG or 1.32 by reference implementation ? */

    return (noise_slope_max);
}


/*****************************************************************************/
/*    Subroutine min_search: find minimum of psd's in circular buffer        */
/*****************************************************************************/
static void min_search(npp_context* n, float* biased_spect, float* biased_spect_sub)
{
    int32_t i,k;
    float temp, biased_smoothedspect, biased_smoothedspect_sub;
    float var_rel_av_sqrt;
    float sum;
    float noise_slope_max, tmp, temp1;
    BOOLEAN update;


    /* ---- compute variance of smoothed psd ---- */
    sum = n->var_rel[0] + n->var_rel[ENH_VEC_LENF - 1];
    for (i = 1; i < ENH_VEC_LENF-1; i++) {
        /*
         * TODO: check this condition:
         */
        /* if (n->var_rel[i] < 0) n->var_rel[i] = 0; */
        sum += 2.0 * n->var_rel[i];
    }
    n->var_rel_av = sum * ENH_WINLEN_INV;
    if(n->var_rel_av < 0.0) n->var_rel_av = 0.0;
    var_rel_av_sqrt = 1.0 + 1.5 * sqrt(n->var_rel_av);

    for (i = 0; i < ENH_VEC_LENF; i++) {
        temp = n->var_rel[i];
        biased_smoothedspect = 1.0 + FVAR * temp * (MINV2 + temp * (MINV + temp));
        biased_smoothedspect_sub = 1.0 + FVAR_SUB * temp * (MINV_SUB2 + temp * (MINV_SUB + temp));
        temp = n->smoothedspect[i] * var_rel_av_sqrt;
        biased_smoothedspect = biased_smoothedspect * temp;
        biased_smoothedspect_sub = biased_smoothedspect_sub * temp;
        biased_spect[i] = biased_smoothedspect;
        biased_spect_sub[i] = biased_smoothedspect_sub;
    }

    /* localflag[] must be initialized to FALSE. */

    if (n->minspec_counter == 0) {
        noise_slope_max = noise_slope(n);

        for (i = 0; i < ENH_VEC_LENF; i++) {
            if (n->act_min[i] > biased_spect[i]) {
                n->act_min[i] = biased_spect[i];   /* update minimum */
                n->act_min_sub[i] = biased_spect_sub[i];
                n->localflag[i] = FALSE;
            }
        }

        /* write new minimum into ring buffer */
        v_equ(n->circb[n->circb_index], n->act_min, ENH_VEC_LENF);

        for (i = 0; i < ENH_VEC_LENF; i++) {
            /* Find minimum of ring buffer.  Using temp1 as cache   */
            /* for circb_min[i]. */

            temp1 = n->circb[0][i];
            for (k = 1; k < NUM_MINWIN; k++) {
                if (n->circb[k][i] < temp1) {
                    temp1 = n->circb[k][i];
                }
            }
            n->circb_min[i] = temp1;
        }

        for (i = 0; i < ENH_VEC_LENF; i++) {
            /* rapid update in case of local minima which do not deviate
               more than noise_slope_max from the current minima 
             */
            tmp = noise_slope_max * n->circb_min[i];
            update = n->localflag[i] && (n->act_min_sub[i] > n->circb_min[i]) && (n->act_min_sub[i] < tmp);
            if (update) {
                n->circb_min[i] = n->act_min_sub[i];
                /* propagate new rapid update minimum into ring buffer */
                for (k = 0; k < NUM_MINWIN; k++) {
                    n->circb[k][i] = n->circb_min[i];
                }
            }
        }

        /* reset local minimum indicator */
        for (i = 0; i < ENH_VEC_LENF; i++) n->localflag[i] = FALSE;

            /* increment ring buffer pointer */
            n->circb_index++;
            if (n->circb_index == NUM_MINWIN)
                n->circb_index = 0;

    } else if (n->minspec_counter == 1) {
        v_equ(n->act_min, biased_spect, ENH_VEC_LENF);
        v_equ(n->act_min_sub, biased_spect_sub, ENH_VEC_LENF);
    } else {  /* minspec_counter > 1 */

        /* At this point localflag[] is all FALSE.  As we loop through        */
        /* minspec_counter, if any localflag[] is turned TRUE, it will be     */
        /* preserved until we go through the (minspec_counter == 0) branch.   */

        for (i = 0; i < ENH_VEC_LENF; i++) {
            if (biased_spect[i] <  n->act_min[i]) {
                /* update minimum */
                n->act_min[i] = biased_spect[i];
                n->act_min_sub[i] = biased_spect_sub[i];
                n->localflag[i] = TRUE;
            }
        }
        for (i = 0; i < ENH_VEC_LENF; i++) {
            if (n->act_min_sub[i] < n->circb_min[i]) {
                n->circb_min[i] = n->act_min_sub[i];
            }
        }

        for (i = 0; i < ENH_VEC_LENF; i++) {
            n->lambdaD[i] = ENH_NOISE_BIAS * n->circb_min[i];
        }
    }
    n->minspec_counter++;
    if (n->minspec_counter == LEN_MINWIN)
        n->minspec_counter = 0;
}

/*****************************************************************************/
/*    Subroutine enh_init: initialization of variables for the enhancement   */
/*****************************************************************************/

#define LAMBDAD_FLOOR 0.001

void enh_init(npp_context* n)
{
    int32_t i;
    float sum, data;

    /* initialize noise spectrum */

    /* Because initial_noise[] is read once and then never used, we can use   */
    /* it for tempV2[] too.                                                   */

    window(n->initial_noise, sqrt_tukey_256_180, n->initial_noise, ENH_WINLEN);

    /* transfer to frequency domain */
    v_zap(n->ybuf, 2 * ENH_WINLEN + 2);
    for (i = 0; i < ENH_WINLEN; i++)
        n->ybuf[2 * i] = n->initial_noise[i];

    fft_npp(n, 1);

    n->ybuf[0] = (n->fftbuf[0] * n->fftbuf[0]);
    n->ybuf[1] = 0;
    n->ybuf[ENH_WINLEN] = (n->fftbuf[ENH_WINLEN] * n->fftbuf[ENH_WINLEN]);
    n->ybuf[ENH_WINLEN + 1] = 0;

    for (i = 2; i < ENH_WINLEN - 1; i += 2) {
        n->ybuf[i + 1] = 0;
        n->ybuf[i] = (n->fftbuf[i]* n->fftbuf[i] + n->fftbuf[i + 1] * n->fftbuf[i + 1]);
    }

    /* convert to correlation domain */
    for (i = 0; i < ENH_WINLEN - 2; i+=2) {
        n->ybuf[ENH_WINLEN + i + 2] = n->ybuf[ENH_WINLEN - i - 2];
        n->ybuf[ENH_WINLEN + i + 3] = -n->ybuf[ENH_WINLEN - i - 2 + 1];
    }
    fft_npp(n, -1); /* inverse fft */

    for (i = 0; i < ENH_WINLEN; i++)
        n->initial_noise[i] = n->fftbuf[2 * i];

    smoothing_win(n->initial_noise);

    /* convert back to frequency domain */
    v_zap(n->ybuf, 2 * ENH_WINLEN + 2);
    for (i = 0; i < ENH_WINLEN; i++)
        n->ybuf[2 * i] = n->initial_noise[i];

    fft_npp(n, 1);

    for (i = 0; i <= (ENH_WINLEN * 2); i += 2) { 
        /* TODO: need or not this thresholding (not affected -?) */
        if (n->fftbuf[i] < 0) n->fftbuf[i] = 0;
    }


    /* rough estimate of lambdaD */
    sum = n->fftbuf[0] * NOISE_B_BY_NSMTH_B + LAMBDAD_FLOOR;
    n->lambdaD[0] = sum;

    data = n->fftbuf[ENH_WINLEN] * NOISE_B_BY_NSMTH_B + LAMBDAD_FLOOR;
    n->lambdaD[ENH_WINLEN_HALF] = data;
    sum+=data;

    for (i = 1; i < ENH_WINLEN_HALF; i++) {
        data = n->fftbuf[i << 1] * NOISE_B_BY_NSMTH_B + LAMBDAD_FLOOR;
        n->lambdaD[i] = data;
        sum+=(2.0 * data);
    }

    n->n_pwr = sum * ENH_WINLEN_INV;

    /* compute initial long term SNR; speech signal power depends on
       the window; the Hanning window is used as a reference here with
       a squared norm of 96
    */

    n->SN_LT = ENH_SNLT_RATIO / n->n_pwr;
    n->SN_LT0 = n->SN_LT;

    minstat_init(n);
}

/*****************************************************************************/
/*    Subroutine minstat_init: initialization of variables for minimum       */
/*    statistics noise estimation                                            */
/*****************************************************************************/
static void minstat_init(npp_context* n)
{
    /* Initialize Minimum Statistics Noise Estimator */
    int32_t i;
    float temp;

    v_equ(n->smoothedspect, n->lambdaD, ENH_VEC_LENF);
    v_scale(n->smoothedspect, ENH_INV_NOISE_BIAS, ENH_VEC_LENF);

    for (i = 0; i < NUM_MINWIN; i++) {
        v_equ(n->circb[i], n->smoothedspect, ENH_VEC_LENF);
    }

    v_equ(n->act_min, n->smoothedspect, ENH_VEC_LENF);
    v_equ(n->act_min_sub, n->smoothedspect, ENH_VEC_LENF);


    for (i = 0; i < ENH_VEC_LENF; i++) {
        temp = n->smoothedspect[i]; /* lambdaD[i] / NOISE_BIAS */
        n->var_sp_av[i] = temp * 1.22474487139159; /* sqrt(3/2) */
        n->var_sp_2[i] = 2.0 * temp * temp;
    }

    n->alphacorr = 0.9;
}

/***************************************************************************/
/* Subroutine:   process_frame
**
** Description:     Enhance a given frame of speech
**
** Arguments:
**
**  float[]   inspeech[]  ---- input speech data buffer (SP)
**  float[]   outspeech[] ---- output speech data buffer (SP)
**
** Return value:    None
**
 ***************************************************************************/
static void process_frame(npp_context* n, float inspeech[], float outspeech[])
{
    int32_t i;
    BOOLEAN n_flag;
    float YY_av, gamma_av, gamma_max;
    float Ymag[ENH_VEC_LENF]; /* magnitude of current frame */
    float GainD[ENH_VEC_LENF]; /* Gain[].*GM[] */
    float gamaK[ENH_VEC_LENF]; /* a posteriori SNR */
    float biased_smoothedspect[ENH_VEC_LENF]; /* weighted smoothed spect. for long window */
    float biased_smoothedspect_sub[ENH_VEC_LENF]; /* for subwindow */
    float sum;

    if (n->npp_counter < 50)    /* This ensures npp_counter does not overflow. */
        n->npp_counter++;

    /* analysis window */
    window(inspeech, sqrt_tukey_256_180, n->fftbuf, ENH_WINLEN);

    v_zap(n->ybuf, 2 * ENH_WINLEN + 2);
    for (i = 0; i < ENH_WINLEN; i++)
        n->ybuf[i << 1] = n->fftbuf[i];
    fft_npp(n, 1);

    /* Previously here we copy ybuf[] to Y[] and process Y[].  In fact this   */
    /* is not necessary because Y[] is not changed before we use ybuf[] and   */
    /* update ybuf[] the next time.                                           */

    n->ybuf[0] = n->fftbuf[0] * n->fftbuf[0];
    n->ybuf[ENH_VEC_LENF - 1] = n->fftbuf[ENH_WINLEN] * n->fftbuf[ENH_WINLEN];

    /* compute spectrum magnitude */
    for (i = 1; i < ENH_VEC_LENF - 1; i++)
        n->ybuf[i] = (n->fftbuf[2 * i] * n->fftbuf[2 * i])+ (n->fftbuf[2 * i + 1] * n->fftbuf[2 * i + 1]);

        /* normalize magnitude spectrum */
    for(i = 0; i < ENH_VEC_LENF; i++) {
        n->YY[i] = n->ybuf[i];
        if (n->YY[i] < 0.001) n->YY[i] = 0.001;
        Ymag[i] = sqrt(n->YY[i]);
    }

    /* ---- Compute YY_av ---- */
    sum = n->YY[0] + n->YY[ENH_VEC_LENF - 1];
    for (i = 1; i < ENH_VEC_LENF - 1; i++) sum+= (2.0 * n->YY[i]);
    YY_av = sum;


    /* compute smoothed short time periodogram */
    smoothed_periodogram(n, YY_av);

    /* determine unbiased noise psd estimate by minimum search */
    min_search(n, biased_smoothedspect,  biased_smoothedspect_sub);

    /* compute 'gammas' */
    for (i = 0; i < ENH_VEC_LENF; i++) {
        gamaK[i] = n->YY[i] / n->lambdaD[i];
    }
    sum = gamaK[0] + gamaK[ENH_VEC_LENF - 1];

    /* compute average of gammas */

    for (i = 1; i < ENH_VEC_LENF - 1; i++) {
        sum += 2.0 * gamaK[i];
    }
    gamma_av = sum * ENH_WINLEN_INV;

    gamma_max = gamaK[0];
    for (i = 1; i < ENH_VEC_LENF; i++) {
        if (gamma_max < gamaK[i])  gamma_max = gamaK[i];
    }

    /* determine signal presence */
    n_flag = FALSE;   /* default flag - signal present */

    if ((gamma_max < GAMMAX_THR) && (gamma_av < GAMMAV_THR)) {
        n_flag = TRUE; /* noise-only */
        if (YY_av > n->n_pwr * GAMMAV_THR * 2.0) n_flag = FALSE; /* overiding if frame SNR > 3dB (9/98) */
    }

    if (n->npp_counter == 1) { /* Initial estimation of apriori SNR and Gain */
        fill(GainD, GM_MIN, ENH_VEC_LENF);
        for (i = 0; i < ENH_VEC_LENF; i++) {
            n->ybuf[i] = Ymag[i] * GM_MIN;
            n->agal[i] = n->ybuf[i];
        }
    } else { /* npp_counter > 1 */

        /* estimation of apriori SNR */
        for (i = 0; i < ENH_VEC_LENF; i++) {
            n->ksi[i] = (n->agal[i] * n->agal[i] * ENH_ALPHAK) / (n->lambdaD[i] * ENH_WINLEN);
            if (gamaK[i] > ENH_INV_NOISE_BIAS)  {
                n->ksi[i] += ENH_BETAK * (gamaK[i] -  ENH_INV_NOISE_BIAS);
            }
        }
        n->Ksi_min_var = 0.9*n->Ksi_min_var + 0.1*ksi_min_adapt(n_flag, GM_MIN, n->SN_LT);

        /* ---- limit the bottom of the ksi ---- */
        for (i = 0; i < ENH_VEC_LENF; i++) {
            if (n->ksi[i] < n->Ksi_min_var) {
                n->ksi[i] = n->Ksi_min_var;
            }
        }

        /* estimation of k-th component 'signal absence' prob and gain */
        /* default value for qk's % (9/98)      */
        fill(n->qk, ENH_QK_MAX, ENH_VEC_LENF);

        if (!n_flag) { /* SIGNAL PRESENT */
            /* computation of the long term SNR */
            if (gamma_av > GAMMAV_THR) {
                n->YY_LT = n->YY_LT * ENH_ALPHA_LT + ENH_BETA_LT * YY_av;
                /*      n->SN_LT = (YY_LT/n->n_pwr) - 1;  Long-term S/N */
                n->SN_LT = n->YY_LT / n->n_pwr;

                /*      if (n->SN_LT < 0); we didn't subtract 1 from n->SN_LT yet.      */
                if (n->SN_LT < 1.0) {
                    n->SN_LT = n->SN_LT0;
                } else {
                    n->SN_LT = n->SN_LT - 1.0;
                }
                /*      n->SN_LT0 = n->SN_LT; */
                n->SN_LT0 = n->SN_LT;
            }

            /* Estimating qk's using hard-decision approach (7/98) */
            compute_qk(n, gamaK, ENH_GAMMAQ_THR);

        }

        gain_log_mmse(n, gamaK, ENH_VEC_LENF);

        /* Previously the gain modification is done outside of gain_mod() and */
        /* now it is moved inside. */

        v_equ(GainD, n->Gain, ENH_VEC_LENF);
        gain_mod(n, GainD, ENH_VEC_LENF);

        /*      vec_mult(Agal, GainD, Ymag, vec_lenf); */
        for (i = 0; i < ENH_VEC_LENF; i++) {
            n->agal[i] = GainD[i] * Ymag[i];
        }
    }


    /* enhanced signal in frequency domain */
    /*       (implement ygal = GainD .* Y)   */

    for (i = 0; i < ENH_WINLEN + 2; i++)
            n->ybuf[i] = n->fftbuf[i] * GainD[i >> 1];

    /* transformation to time domain */
    for (i = 0; i < ENH_WINLEN - 2; i+=2) {
        n->ybuf[ENH_WINLEN + i + 2] = n->ybuf[ENH_WINLEN - i - 2];
        n->ybuf[ENH_WINLEN + i + 3] = -n->ybuf[ENH_WINLEN - i - 2 + 1];
    }

    fft_npp(n, -1);


#if USE_SYN_WINDOW
    for (i = 0; i < ENH_WINLEN; i++) {
        outspeech[i] = n->fftbuf[2 * i] * sqrt_tukey_256_180[i];
    }
#endif

    /* Misc. updates */
    sum = n->lambdaD[0] + n->lambdaD[ENH_VEC_LENF - 1];

    for (i = 1; i < ENH_VEC_LENF - 1; i++)
        sum += n->lambdaD[i] * 2.0;

    n->n_pwr = sum * ENH_WINLEN_INV;

}

/* ======================================= */
/* npp_init(): this is to workaround the TI
 * compiler feature to omit static and global variables zeroing */
/* ======================================= */
void npp_init(npp_context* n)
{
    n->first_time = TRUE;

    /* Initializing qla[] */
    fill(n->qla, 0.5f, ENH_VEC_LENF);

    /* Initializing process_frame variables */
    v_zap(n->agal, ENH_VEC_LENF);

    fill(n->ksi, GM_MIN, ENH_VEC_LENF);

    fill(n->qk, ENH_QK_MAX, ENH_VEC_LENF);
    fill(n->Gain, GM_MIN, ENH_VEC_LENF);
    n->YY_LT = 0;
    n->Ksi_min_var = GM_MIN;
}


void fft_npp(npp_context* n, int16_t dir)
{
    if (dir > 0) {
        DSPF_sp_fftSPxSP(256, n->ybuf, &fft_twiddle[0], n->fftbuf, NULL, 4, 0, 256);
    } else {
        DSPF_sp_ifftSPxSP(256, n->ybuf, &fft_twiddle[0], n->fftbuf, NULL, 4, 0, 256);
    }
}

