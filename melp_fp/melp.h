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

/*                                                                  */
/*  melp.h: include file for MELP coder                             */
/*                                                                  */
#ifndef __MELP_H_
#define __MELP_H_

#include <stdint.h>

/*  compiler constants */
#define FRAME 180           /* speech frame size */
#define LPC_ORD 10          /* LPC order */
#define LPC_ORD_HALF 5      /* LPC order divided by 2 */
#define NUM_HARM 10         /* number of Fourier magnitudes */
#define NUM_GAINFR 2        /* number of gains per frame */
#define LPC_FRAME 200       /* LPC window size */
#define PITCHMIN 20         /* minimum pitch lag */
#define PITCHMAX 160        /* maximum pitch lag */
#define NUM_BANDS 5         /* number of frequency bands */
#define LPF_ORD 6           /* lowpass filter order */
#define DC_ORD 4            /* DC removal filter order */
#define BPF_ORD 6           /* bandpass analysis filter order */
#define ENV_ORD 2           /* bandpass envelope filter order */
#define MIX_ORD 32          /* mixed excitation filtering order */
#define DISP_ORD 64         /* pulse dispersion filter order */
#define DEFAULT_PITCH 50.0  /* default pitch value */
#define UV_PITCH 50         /* unvoiced pitch value */
#define VMIN 0.8            /* minimum strongly voiced correlation */
#define VJIT 0.5            /* jitter threshold for correlations */
#define BPTHRESH 0.6        /* bandpass voicing threshold */
#define MAX_JITTER 0.25     /* maximum jitter percentage (as a fraction) */
#define ASE_NUM_BW 0.5      /* adaptive spectral enhancement - numerator */
#define ASE_DEN_BW 0.8      /* adaptive spectral enhancement - denominator */
#define GAINFR (FRAME/NUM_GAINFR)  /* size of gain frame */
#define MIN_GAINFR 120      /* minimum gain analysis window */
#define MINLENGTH 160       /* minimum correlation length */
#define FSAMP 8000.0        /* sampling frequency */
#define MSVQ_M 8            /* M-best for MSVQ search */
#define MSVQ_MAXCNT 256     /* maximum inner loop counter for MSVQ search */
#define BWMIN (50.0*2/FSAMP) /* minimum LSF separation */
#define MSVQ_NUM_STAGES 4
#define FSVQ_NUM_STAGES 1
#define VQ_DIMENSION NUM_HARM
#define MSVQ_NUM_BEST MSVQ_M

/* Noise suppression/estimation parameters */
/* Up by 3 dB/sec (0.5*22.5 ms frame), down by 12 dB/sec */
#define UPCONST 0.0337435    /* noise estimation up time constant */
#define DOWNCONST -0.135418  /* noise estimation down time constant */
#define NFACT 3.0            /* noise floor boost in dB */
#define MAX_NS_ATT 6.0       /* maximum noise suppression */
#define MAX_NS_SUP 20.0      /* maximum noise level to use in suppression */  
#define MIN_NOISE 10.0       /* minimum value allowed in noise estimation */
#define MAX_NOISE 80.0       /* maximum value allowed in noise estimation */

/* Channel I/O constants */
#define	CHWORDSIZE 8         /* number of bits per channel word */
#define ERASE_MASK 0x4000    /* erasure flag mask for channel word */

#define GN_QLO 10.0          /* minimum gain in dB */
#define GN_QUP 77.0          /* maximum gain in dB */
#define GN_QLEV 32           /* number of second gain quantization levels */
#define PIT_BITS 7           /* number of bits for pitch coding */
#define PIT_QLEV 99          /* number of pitch levels */
#define PIT_QLO 1.30103      /* minimum log pitch for quantization */
#define PIT_QUP 2.20412      /* maximum log pitch for quantization */
#define FS_BITS 8            /* number of bits for Fourier magnitudes */
#define FS_LEVELS (1<<FS_BITS) /* number of levels for Fourier magnitudes */

/* Define number of channel bits per frame */
#define NUM_CH_BITS 54

/* Filter orders */
#define ENV_ORD 2
#define BPF_ORD 6
#define NUM_PITCHES 2
#define LMIN MINLENGTH
#define PIT_BEG BPF_ORD
#define PIT_P_FR ((2*PITCHMAX)+1)
#define PIT_FR_BEG (-PITCHMAX)
#define FIRST_CNTR (PIT_BEG+PITCHMAX)

/* Synthesis defines */
#define TILT_ORD 1
#define SYN_GAIN 1000.0
#define SCALEOVER 10

#ifndef NPP
#define NPP 1
#endif

/* FFT defines */
#define FFTLENGTH 512
#define FFTLENGTH_HALF 256

/* Structure definitions */
struct melp_param {         /* MELP parameters */
    int32_t rate;
    float pitch;
    float lsf[LPC_ORD+1];
    float gain[NUM_GAINFR];
    float jitter;
    float bpvc[NUM_BANDS];
    float prev_gain;
    int16_t pitch_index;
    int16_t lsf_index[LPC_ORD];
    int16_t jit_index;
    int16_t bpvc_index;
    int16_t gain_index[NUM_GAINFR];
    unsigned char *chptr;
    int16_t chbit;
    int16_t uv_flag;
    float fs_mag[NUM_HARM];
    int16_t *fsvq_index;
    int16_t *msvq_index;
    int16_t msvq_stages;
    int16_t *msvq_bits;
    int16_t *msvq_levels;
    int16_t codewd74[8];
    int16_t codewd84[8];
    unsigned char bit_buffer[NUM_CH_BITS];
    int16_t direction;
    int16_t sync_bit;
    int16_t prev_gain_err;
    void* proc_ctx;
    void* npp_handle;
};

/* External function definitions */

#include "dsp_sub.h"
#include "melp_sub.h"

void melp_ana(float sp_in[],struct melp_param *par);
void melp_syn(struct melp_param *par, float sp_out[]);
void* melp_ana_init();
void melp_ana_free(void* ana_handle);
void* melp_syn_init();
void melp_syn_free(void* syn_handle);
void fsvq_cb_init();
void msvq_cb_init();
void melp_chn_write(struct melp_param *par);
int16_t melp_chn_read(struct melp_param *par, struct melp_param *prev_par, const float* msvq_codebook, const float* fsvq_codebook);

void fec_code(struct melp_param *par);
int16_t fec_decode(struct melp_param *par, int16_t erase);

#endif
