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
    Name: melp_ana.c
    Description: MELP analysis
    Inputs:
      speech[] - input speech signal
    Outputs: 
      *par - MELP parameter structure
    Returns: void
*/

/* compiler include files */

#include <stdio.h>
#include <math.h>
#include "melp.h"
#include "spbstd.h"
#include "lpc.h"
#include "mat.h"
#include "vq.h"
#include "fs.h"
#include "pit.h"

/* compiler constants */

#define BEGIN 0
#define END 1
#define BWFACT 0.994
#define PEAK_THRESH 1.34
#define PEAK_THR2 1.6
#define SILENCE_DB 30.0
#define MAX_ORD LPF_ORD
#define FRAME_BEG (PITCHMAX-(FRAME/2))
#define FRAME_END (FRAME_BEG+FRAME)
#define PITCH_BEG (FRAME_END-PITCHMAX)
#define PITCH_FR ((2*PITCHMAX)+1)
#define IN_BEG (PITCH_BEG+PITCH_FR-FRAME)
#define SIG_LENGTH (LPF_ORD+PITCH_FR)

/* external memory references */
 
extern const float win_cof[LPC_FRAME];
extern const float lpf_num[LPF_ORD+1];
extern const float lpf_den[LPF_ORD+1];
extern float msvq_cb[];
extern float fsvq_cb[];
extern float global_w_fs[];



/* analysis context */
typedef struct _ana_context {
    float sigbuf[FFTLENGTH];
    float fftbuf[FFTLENGTH];
    float speech[IN_BEG+FRAME];
    float dcdel[DC_ORD];
    float lpfsp_del[LPF_ORD];
    float pitch_avg;
    float fpitch[2];
    struct msvq_param vq_par;  /* MSVQ parameters */
    struct msvq_param fs_vq_par;  /* Fourier series VQ parameters */
    float **bpfdel;
    float **envdel;
    float *envdel2;
    float *bpvc_sigbuf;
    float *dc_rmv_buf;
    float *lpc_y1;
    float *lpc_y2;
    float **lpc_c;
    float **lpc_f;
    int16_t *vq_indices;
    float *vq_errors;
    float *vq_uhatw;
    float *vq_d;
    int16_t *vq_parents;
    void* pitch_ctx;
} ana_context;


void melp_ana(float sp_in[],struct melp_param *par)
{
    ana_context *ana = (ana_context*) par->proc_ctx;
    int16_t i;
    int16_t begin;
    float sub_pitch;
    float temp,pcorr,bpthresh;
    float r[LPC_ORD+1],refc[LPC_ORD+1],lpc[LPC_ORD+1];
    float weights[LPC_ORD];
        
    /* Remove DC from input speech */
    dc_rmv(sp_in,&ana->speech[IN_BEG],ana->dcdel, ana->dc_rmv_buf);
    
    /* Copy input ana->speech to pitch window and lowpass filter */
    v_equ(&ana->sigbuf[LPF_ORD],&ana->speech[PITCH_BEG],PITCH_FR);
    v_equ(ana->sigbuf,ana->lpfsp_del,LPF_ORD);
    polflt(&ana->sigbuf[LPF_ORD],lpf_den,&ana->sigbuf[LPF_ORD],LPF_ORD,PITCH_FR);
    v_equ(ana->lpfsp_del,&ana->sigbuf[FRAME],LPF_ORD);
    zerflt(&ana->sigbuf[LPF_ORD],lpf_num,&ana->sigbuf[LPF_ORD],LPF_ORD,PITCH_FR);
    
    /* Perform global pitch search at frame end on lowpass speech signal */
    /* Note: avoid short pitches due to formant tracking */
    ana->fpitch[END] = find_pitch(&ana->sigbuf[LPF_ORD+(PITCH_FR/2)],&temp,
			     (2*PITCHMIN),PITCHMAX,PITCHMAX);
    
    /* Perform bandpass voicing analysis for end of frame */
    bpvc_ana(&ana->speech[FRAME_END], ana->fpitch, &par->bpvc[0], &sub_pitch, ana->bpfdel, ana->envdel, ana->envdel2, ana->bpvc_sigbuf);
    
    /* Force jitter if lowest band voicing strength is weak */    
    if (par->bpvc[0] < VJIT)
	par->jitter = MAX_JITTER;
    else
	par->jitter = 0.0;
    
    /* Calculate LPC for end of frame */
    window(&ana->speech[(FRAME_END-(LPC_FRAME/2))],win_cof,ana->sigbuf,LPC_FRAME);
    autocorr(ana->sigbuf,r,LPC_ORD,LPC_FRAME);
    lpc[0] = 1.0;
    lpc_schr(r, lpc, refc, ana->lpc_y1, ana->lpc_y2);

    lpc_bw_expand(lpc,lpc,BWFACT,LPC_ORD);
    
    /* Calculate LPC residual */
    zerflt(&ana->speech[PITCH_BEG],lpc,&ana->sigbuf[LPF_ORD],LPC_ORD,PITCH_FR);
        
    /* Check peakiness of residual signal */
    begin = (LPF_ORD+(PITCHMAX/2));
    temp = peakiness(&ana->sigbuf[begin],PITCHMAX);
    
    /* Peakiness: force lowest band to be voiced  */
    if (temp > PEAK_THRESH) {
	par->bpvc[0] = 1.0;
    }
    
    /* Extreme peakiness: force second and third bands to be voiced */
    if (temp > PEAK_THR2) {
	par->bpvc[1] = 1.0;
	par->bpvc[2] = 1.0;
    }
		
    /* Calculate overall frame pitch using lowpass filtered residual */
    par->pitch = pitch_ana(ana->pitch_ctx, &ana->speech[FRAME_END], &ana->sigbuf[LPF_ORD+PITCHMAX], 
			   sub_pitch,ana->pitch_avg,&pcorr);
    bpthresh = BPTHRESH;
    
    /* Calculate gain of input speech for each gain subframe */
    for (i = 0; i < NUM_GAINFR; i++) {
	if (par->bpvc[0] > bpthresh) {

	    /* voiced mode: pitch synchronous window length */
	    temp = sub_pitch;
	    par->gain[i] = gain_ana(&ana->speech[FRAME_BEG+(i+1)*GAINFR],
				    temp,MIN_GAINFR,2*PITCHMAX);
	}
	else {
	    temp = 1.33*GAINFR - 0.5;
	    par->gain[i] = gain_ana(&ana->speech[FRAME_BEG+(i+1)*GAINFR],
				    temp,0,2*PITCHMAX);
	}
    }
    
    /* Update average pitch value */
    if (par->gain[NUM_GAINFR-1] > SILENCE_DB)
      temp = pcorr;
    else
      temp = 0.0;
    ana->pitch_avg = p_avg_update(ana->pitch_ctx, par->pitch, temp, VMIN);
    
    /* Calculate Line Spectral Frequencies */
    lpc_pred2lsp(lpc, par->lsf, ana->lpc_c);
    
    /* Force minimum LSF bandwidth (separation) */
    lpc_clamp(par->lsf,BWMIN,LPC_ORD);
    
    /* Quantize MELP parameters to 2400 bps and generate bitstream */
    
    /* Quantize LSF's with MSVQ */
    vq_lspw(weights, &par->lsf[1], lpc, LPC_ORD);
    vq_ms4(ana->vq_par.cb,&par->lsf[1],ana->vq_par.num_levels,weights,&par->lsf[1],ana->vq_par.indices,
      ana->vq_indices, ana->vq_errors, ana->vq_uhatw, ana->vq_d, ana->vq_parents);
    par->msvq_index = ana->vq_par.indices;
    
    /* Force minimum LSF bandwidth (separation) */
    lpc_clamp(par->lsf,BWMIN,LPC_ORD);
    
    /* Quantize logarithmic pitch period */
    /* Reserve all zero code for completely unvoiced */
    par->pitch = log10(par->pitch);
    quant_u(&par->pitch,&par->pitch_index,PIT_QLO,PIT_QUP,PIT_QLEV);
    par->pitch = pow(10.0,par->pitch);
    
    /* Quantize gain terms with uniform log quantizer	*/
    q_gain(par->gain, par->gain_index, &par->prev_gain);
    
    /* Quantize jitter and bandpass voicing */
    quant_u(&par->jitter,&par->jit_index,0.0,MAX_JITTER,2);
    par->uv_flag = q_bpvc(&par->bpvc[0],&par->bpvc_index,bpthresh);
    
    /*	Calculate Fourier coefficients of residual signal from quantized LPC */
    fill(par->fs_mag,1.0,NUM_HARM);
    if (par->bpvc[0] > bpthresh) {
	lpc_lsp2pred(par->lsf, lpc, ana->lpc_f);
	zerflt(&ana->speech[(FRAME_END-(LPC_FRAME/2))],lpc,ana->sigbuf,
	       LPC_ORD,LPC_FRAME);
	window(ana->sigbuf,win_cof,ana->sigbuf,LPC_FRAME);
	find_harm(ana->sigbuf, par->fs_mag, ana->fftbuf, par->pitch, NUM_HARM, LPC_FRAME);
    }
    
    /* quantize Fourier coefficients */
    /* pre-weight vector, then use Euclidean distance */
    window(&par->fs_mag[0],global_w_fs,&par->fs_mag[0],NUM_HARM);
    vq_enc(ana->fs_vq_par.cb,&par->fs_mag[0],*(ana->fs_vq_par.num_levels), VQ_DIMENSION, &par->fs_mag[0], ana->fs_vq_par.indices);
    
    /* Set MELP indeces to point to same array */
    par->fsvq_index = ana->fs_vq_par.indices;

    /* Update MSVQ information */
    par->msvq_stages = MSVQ_NUM_STAGES;
    par->msvq_bits = ana->vq_par.num_bits;

    /* Write channel bitstream */
    melp_chn_write(par);

    /* Update delay buffers for next frame */
    v_equ(&ana->speech[0],&ana->speech[FRAME],IN_BEG);
    ana->fpitch[BEGIN] = ana->fpitch[END];
}



/* 
 * melp_ana_init: perform initialization 
 */


void* melp_ana_init()
{

    ana_context* ana = NULL;
    MEM_ALLOC(MALLOC,ana,1,ana_context);
    if (!ana) return 0;
    memset(ana, 0, sizeof(ana_context));

    /* Allocate memory for bpvc_ana*/
    MEM_2ALLOC(MALLOC,ana->bpfdel,NUM_BANDS,BPF_ORD,float);
    if (ana->bpfdel == 0) return 0;
    v_zap(&ana->bpfdel[0][0],NUM_BANDS*BPF_ORD);

    MEM_2ALLOC(MALLOC,ana->envdel,NUM_BANDS,ENV_ORD,float);
    if (ana->envdel == 0) return 0;
    v_zap(&ana->envdel[0][0],NUM_BANDS*ENV_ORD);
    MEM_ALLOC(MALLOC,ana->envdel2,NUM_BANDS,float);
    if (ana->envdel2 == 0) return 0;
    v_zap(ana->envdel2,NUM_BANDS);

    /* Allocate scratch buffer */
    MEM_ALLOC(malloc,ana->bpvc_sigbuf,PIT_BEG+PIT_P_FR,float);
    if (ana->bpvc_sigbuf == 0) return 0;

    ana->pitch_ctx = pitch_ana_init();

    v_zap(ana->speech,IN_BEG+FRAME);
    ana->pitch_avg=DEFAULT_PITCH;
    fill(ana->fpitch,DEFAULT_PITCH,2);
    v_zap(ana->lpfsp_del,LPF_ORD);

    /* Initialize multi-stage vector quantization (read codebook) */

    /* 
     * Allocate memory for number of levels per stage and indices
     * and for number of bits per stage 
     */

    MEM_ALLOC(MALLOC,ana->vq_par.num_levels,MSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,ana->vq_par.indices,MSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,ana->vq_par.num_bits,MSVQ_NUM_STAGES,int16_t);
    if (ana->vq_par.num_levels == 0 || ana->vq_par.indices == 0 || ana->vq_par.num_bits == 0) return 0;

    ana->vq_par.num_levels[0] = 128;
    ana->vq_par.num_levels[1] = 64;
    ana->vq_par.num_levels[2] = 64;
    ana->vq_par.num_levels[3] = 64;

    ana->vq_par.num_bits[0] = 7;
    ana->vq_par.num_bits[1] = 6;
    ana->vq_par.num_bits[2] = 6;
    ana->vq_par.num_bits[3] = 6;

    ana->vq_par.cb = msvq_cb;


    /* Initialize Fourier magnitude vector quantization (read codebook) */

    /*
     * Allocate memory for number of levels per stage and indices
     * and for number of bits per stage 
     */

    MEM_ALLOC(MALLOC,ana->fs_vq_par.num_levels,FSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,ana->fs_vq_par.indices,FSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,ana->fs_vq_par.num_bits,FSVQ_NUM_STAGES,int16_t);
    if (ana->fs_vq_par.num_levels == 0 || ana->fs_vq_par.indices == 0 || ana->fs_vq_par.num_bits == 0) return 0;

    ana->fs_vq_par.cb = fsvq_cb;

    ana->fs_vq_par.num_levels[0] = FS_LEVELS;
    ana->fs_vq_par.num_bits[0] = FS_BITS;

    MEM_ALLOC(MALLOC, ana->dc_rmv_buf, FRAME+DC_ORD, float);
    MEM_ALLOC(MALLOC, ana->lpc_y1, LPC_ORD+2, float);
    MEM_ALLOC(MALLOC, ana->lpc_y2, LPC_ORD+2, float);
    MEM_2ALLOC(MALLOC, ana->lpc_c, 2, LPC_ORD_HALF+1, float);
    MEM_2ALLOC(MALLOC, ana->lpc_f, 2, LPC_ORD_HALF+1, float);
    if (ana->dc_rmv_buf ==0 || ana->lpc_y1 == 0 || ana->lpc_y2 == 0 || ana->lpc_c == 0 ||  ana->lpc_f == 0) return 0;

    MEM_ALLOC(MALLOC,ana->vq_indices,2*MSVQ_NUM_BEST*MSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,ana->vq_errors,2*MSVQ_NUM_BEST*VQ_DIMENSION,float);
    MEM_ALLOC(MALLOC,ana->vq_uhatw,VQ_DIMENSION,float);
    MEM_ALLOC(MALLOC,ana->vq_d,2*MSVQ_NUM_BEST,float);
    MEM_ALLOC(MALLOC,ana->vq_parents,2*MSVQ_NUM_BEST,int16_t);
    if (ana->vq_indices == 0 || ana->vq_errors == 0 || ana->vq_uhatw == 0 || ana->vq_d == 0 || ana->vq_parents == 0) return 0;

    return ana;
}

void melp_ana_free(void* ana_handle) 
{
    ana_context* ana = (ana_context*) ana_handle;

    if (ana == NULL) return;

    MEM_FREE(FREE, ana->dc_rmv_buf);
    MEM_FREE(FREE, ana->lpc_y1);
    MEM_FREE(FREE, ana->lpc_y2);
    MEM_2FREE(FREE, ana->lpc_c);
    MEM_2FREE(FREE, ana->lpc_f);
    MEM_FREE(FREE, ana->vq_parents);
    MEM_FREE(FREE, ana->vq_d);
    MEM_FREE(FREE, ana->vq_uhatw);
    MEM_FREE(FREE, ana->vq_errors);
    MEM_FREE(FREE, ana->vq_indices);


    MEM_FREE(FREE, ana->fs_vq_par.num_bits);
    MEM_FREE(FREE, ana->fs_vq_par.indices);
    MEM_FREE(FREE, ana->fs_vq_par.num_levels);

    MEM_FREE(FREE, ana->vq_par.num_bits);
    MEM_FREE(FREE, ana->vq_par.indices);
    MEM_FREE(FREE, ana->vq_par.num_levels);

    pitch_ana_free(ana->pitch_ctx);

    MEM_FREE(FREE, ana->bpvc_sigbuf);

    MEM_FREE(FREE, ana->envdel2);

    MEM_2FREE(FREE, ana->envdel);
    MEM_2FREE(FREE, ana->bpfdel);

    /* free context memory */
    MEM_FREE(FREE, ana);
}
