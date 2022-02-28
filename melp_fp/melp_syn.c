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
    Name: melp_syn.c
    Description: MELP synthesis
      This program takes the new parameters for a speech
      frame and synthesizes the output speech.  It keeps
      an internal record of the previous frame parameters
      to use for interpolation.			
    Inputs:
      *par - MELP parameter structure
    Outputs: 
      speech[] - output speech signal
    Returns: void
*/

/* compiler include files */
 
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "melp.h"
#include "spbstd.h"
#include "lpc.h"
#include "mat.h"
#include "vq.h"
#include "fs.h"

/* compiler constants */
 
#if (MIX_ORD > DISP_ORD)
#define BEGIN MIX_ORD
#else
#define BEGIN DISP_ORD
#endif

#define PDEL SCALEOVER

/* external memory references (read-only) */
 
extern const float bp_cof[NUM_BANDS][MIX_ORD+1];
extern const float disp_cof[DISP_ORD+1];
extern float msvq_cb[];
extern float fsvq_cb[];
extern float global_w_fs[];
extern float w_fs_inv[NUM_HARM];

/* synthesis context  memory */
typedef struct _syn_context {
    float sigbuf[BEGIN+PITCHMAX];
    float sig2[BEGIN+PITCHMAX];
    float fs_real[PITCHMAX];
    float sigsave[FRAME+PITCHMAX];
   
    float prev_scale;
    float noise_gain;
    float pulse_del[MIX_ORD];
    float noise_del[MIX_ORD];
    
    float lpc_del[LPC_ORD];
    float ase_del[LPC_ORD];
    float tilt_del[TILT_ORD];
    float disp_del[DISP_ORD];

    /* these can be saved or recomputed */
    float prev_pcof[MIX_ORD+1];
    float prev_ncof[MIX_ORD+1];
    
    float prev_tilt;
    struct melp_param prev_par;
    struct msvq_param vq_par;  /* MSVQ parameters */
    struct msvq_param fs_vq_par;  /* Fourier series VQ parameters */
    int16_t firstcall; /* Just used for noise gain init */
    int16_t syn_begin;
    float ** lpc_f;
    rand_context random_seed;
} syn_context;


void melp_syn(struct melp_param *par,float sp_out[])

{

    syn_context* syn = (syn_context*) par->proc_ctx;
    int16_t i, gaincnt;
    int16_t erase;
    int16_t length;
    float intfact, ifact, ifact_gain;
    float gain,pulse_gain,pitch,jitter;
    float curr_tilt,tilt_cof[TILT_ORD+1];
    float temp,sig_prob;
    float lsf[LPC_ORD+1];
    float lpc[LPC_ORD+1];
    float ase_num[LPC_ORD+1],ase_den[LPC_ORD+1];
    float curr_pcof[MIX_ORD+1],curr_ncof[MIX_ORD+1];
    float pulse_cof[MIX_ORD+1],noise_cof[MIX_ORD+1];
    
    /* Copy previous period of processed speech to output array */
    if (syn->syn_begin > 0) {
	if (syn->syn_begin > FRAME) {
	    /* assert(syn->syn_begin <= PITCHMAX + FRAME) */
	    v_equ(&sp_out[0],&syn->sigsave[0],FRAME);
	    /* past end: save remainder in syn->sigsave[0] */
	    v_equ(&syn->sigsave[0],&syn->sigsave[FRAME],syn->syn_begin-FRAME);
	}
	else {
	  v_equ(&sp_out[0],&syn->sigsave[0],syn->syn_begin);
	}
    }
    
    erase = 0; /* no erasures yet */
    
    /* Update MSVQ information */
    par->msvq_stages = MSVQ_NUM_STAGES;
    par->msvq_bits = syn->vq_par.num_bits;
    par->msvq_levels = syn->vq_par.num_levels;
    par->msvq_index = syn->vq_par.indices;
    par->fsvq_index = syn->fs_vq_par.indices;
    
    /*	Read and decode channel input buffer	*/
    erase = melp_chn_read(par,&syn->prev_par, msvq_cb, fsvq_cb);

    if (par->uv_flag != 1 && !erase) { 
	/* Un-weight Fourier magnitudes */
	window(&par->fs_mag[0],w_fs_inv,&par->fs_mag[0],NUM_HARM);
    }

    /* Update adaptive noise level estimate based on last gain	*/
    if (syn->firstcall) {
	syn->firstcall = 0;
	syn->noise_gain = par->gain[NUM_GAINFR-1];
    }
    
    else if (!erase) {
	for (i = 0; i < NUM_GAINFR; i++) {
	    noise_est(par->gain[i],&syn->noise_gain,UPCONST,DOWNCONST,
		      MIN_NOISE,MAX_NOISE);

	    /* Adjust gain based on noise level (noise suppression) */
	    noise_sup(&par->gain[i],syn->noise_gain,MAX_NS_SUP,MAX_NS_ATT,NFACT);

	}
	    
    }
    
    /* Clamp LSP bandwidths to avoid sharp LPC filters */
    lpc_clamp(par->lsf,BWMIN,LPC_ORD);
    
    /* Calculate spectral tilt for current frame for spectral enhancement */
    tilt_cof[0] = 1.0;
    lpc_lsp2pred(par->lsf, lpc, syn->lpc_f);
    lpc_pred2refl(lpc,syn->sig2);
    if (syn->sig2[1] < 0.0)
      curr_tilt = 0.5*syn->sig2[1];
    else
      curr_tilt = 0.0;
    
    /* Disable pitch interpolation for high-pitched onsets */
    if (par->pitch < 0.5*syn->prev_par.pitch && 
	par->gain[0] > 6.0 + syn->prev_par.gain[NUM_GAINFR-1]) {
	
	/* copy current pitch into previous */
	syn->prev_par.pitch = par->pitch;
    }
    
    /* Set pulse and noise coefficients based on voicing strengths */
    v_zap(curr_pcof,MIX_ORD+1);
    v_zap(curr_ncof,MIX_ORD+1);
    for (i = 0; i < NUM_BANDS; i++) {
	if (par->bpvc[i] > 0.5)
	    v_add(curr_pcof,&bp_cof[i][0],MIX_ORD+1);
	else
	    v_add(curr_ncof,&bp_cof[i][0],MIX_ORD+1);
    }
	
    /* Process each pitch period */
    
    while (syn->syn_begin < FRAME) {

	/* interpolate previous and current parameters */

	ifact = ((float) syn->syn_begin ) / FRAME;
			
	if (syn->syn_begin >= GAINFR) {
	    gaincnt = 2;
	    ifact_gain = ((float) syn->syn_begin-GAINFR) / GAINFR;
	}
	else {
	    gaincnt = 1;
	    ifact_gain = ((float) syn->syn_begin) / GAINFR;
	}
	
	/* interpolate gain */
	if (gaincnt > 1) {
	    gain = ifact_gain*par->gain[gaincnt-1] + 
		(1.0-ifact_gain)*par->gain[gaincnt-2];
	}
	else {
	    gain = ifact_gain*par->gain[gaincnt-1] + 
		(1.0-ifact_gain)*syn->prev_par.gain[NUM_GAINFR-1];
	}
	
	/* Set overall interpolation path based on gain change */
	
	temp = par->gain[NUM_GAINFR-1] - syn->prev_par.gain[NUM_GAINFR-1];
	if (fabs(temp) > 6) {

	    /* Power surge: use gain adjusted interpolation */
	    intfact = (gain - syn->prev_par.gain[NUM_GAINFR-1]) / temp;
	    if (intfact > 1.0)
		intfact = 1.0;
	    if (intfact < 0.0)
		intfact = 0.0;
	}
	else

	    /* Otherwise, linear interpolation */
	    intfact = ((float) syn->syn_begin) / FRAME;
	
	/* interpolate LSF's and convert to LPC filter */
	interp_array(syn->prev_par.lsf,par->lsf,lsf,intfact,LPC_ORD+1);
	lpc_lsp2pred(lsf, lpc, syn->lpc_f);
	
	/* Check signal probability for adaptive spectral enhancement filter */
	sig_prob = lin_int_bnd(gain,syn->noise_gain+12.0,syn->noise_gain+30.0,
			       0.0,1.0);

	/* Calculate adaptive spectral enhancement filter coefficients */
	ase_num[0] = 1.0;
	lpc_bw_expand(lpc,ase_num,sig_prob*ASE_NUM_BW,LPC_ORD);
	lpc_bw_expand(lpc,ase_den,sig_prob*ASE_DEN_BW,LPC_ORD);
	tilt_cof[1] = sig_prob*(intfact*curr_tilt + 
				(1.0-intfact)*syn->prev_tilt);
	
	/* interpolate pitch and pulse gain */
	pitch = intfact*par->pitch + (1.0-intfact)*syn->prev_par.pitch;
	pulse_gain = SYN_GAIN*sqrt(pitch);
	
	/* interpolate pulse and noise coefficients */
	temp = sqrt(ifact);
	interp_array(syn->prev_pcof,curr_pcof,pulse_cof,temp,MIX_ORD+1);
	interp_array(syn->prev_ncof,curr_ncof,noise_cof,temp,MIX_ORD+1);
	
	/* interpolate jitter */
	jitter = ifact*par->jitter + 
	    (1.0-ifact)*syn->prev_par.jitter;
	
	/* convert gain to linear */
	gain = pow(10.0,0.05*gain);
	
	/* Set period length based on pitch and jitter */
	temp = rand_num(1.0, &syn->random_seed);
	length = pitch * (1.0-jitter*temp) + 0.5;
	if (length < PITCHMIN)
	    length = PITCHMIN;
	if (length > PITCHMAX)
	    length = PITCHMAX;
	
	/* Use inverse DFT for pulse excitation */
	fill(syn->fs_real,1.0,length);
	syn->fs_real[0] = 0.0;
	interp_array(syn->prev_par.fs_mag,par->fs_mag,&syn->fs_real[1],intfact,
		     NUM_HARM);
	idft_real(syn->fs_real,&syn->sigbuf[BEGIN],length);
	
	/* Delay overall signal by PDEL samples (circular shift) */
	/* use syn->fs_real as a scratch buffer */
	v_equ(syn->fs_real,&syn->sigbuf[BEGIN],length);
	v_equ(&syn->sigbuf[BEGIN+PDEL],syn->fs_real,length-PDEL);
	v_equ(&syn->sigbuf[BEGIN],&syn->fs_real[length-PDEL],PDEL);
	
	/* Scale by pulse gain */
	v_scale(&syn->sigbuf[BEGIN],pulse_gain,length);	
	
	/* Filter and scale pulse excitation */
	v_equ(&syn->sigbuf[BEGIN-MIX_ORD],syn->pulse_del,MIX_ORD);
	v_equ(syn->pulse_del,&syn->sigbuf[length+BEGIN-MIX_ORD],MIX_ORD);
	zerflt(&syn->sigbuf[BEGIN],pulse_cof,&syn->sigbuf[BEGIN],MIX_ORD,length);
	
	/* Get scaled noise excitation */
	rand_vec(&syn->sig2[BEGIN],(1.732*SYN_GAIN),length, &syn->random_seed);
	
	/* Filter noise excitation */
	v_equ(&syn->sig2[BEGIN-MIX_ORD],syn->noise_del,MIX_ORD);
	v_equ(syn->noise_del,&syn->sig2[length+BEGIN-MIX_ORD],MIX_ORD);
	zerflt(&syn->sig2[BEGIN],noise_cof,&syn->sig2[BEGIN],MIX_ORD,length);
	
	/* Add two excitation signals (mixed excitation) */
	v_add(&syn->sigbuf[BEGIN],&syn->sig2[BEGIN],length);
	
	/* Adaptive spectral enhancement */
	v_equ(&syn->sigbuf[BEGIN-LPC_ORD],syn->ase_del,LPC_ORD);
	lpc_synthesis(&syn->sigbuf[BEGIN],&syn->sigbuf[BEGIN],ase_den,LPC_ORD,length);
	v_equ(syn->ase_del,&syn->sigbuf[BEGIN+length-LPC_ORD],LPC_ORD);
	zerflt(&syn->sigbuf[BEGIN],ase_num,&syn->sigbuf[BEGIN],LPC_ORD,length);
	v_equ(&syn->sigbuf[BEGIN-TILT_ORD],syn->tilt_del,TILT_ORD);
	v_equ(syn->tilt_del,&syn->sigbuf[length+BEGIN-TILT_ORD],TILT_ORD);
	zerflt(&syn->sigbuf[BEGIN],tilt_cof,&syn->sigbuf[BEGIN],TILT_ORD,length);
	
	/* Perform LPC synthesis filtering */
	v_equ(&syn->sigbuf[BEGIN-LPC_ORD],syn->lpc_del,LPC_ORD);
	lpc_synthesis(&syn->sigbuf[BEGIN],&syn->sigbuf[BEGIN],lpc,LPC_ORD,length);
	v_equ(syn->lpc_del,&syn->sigbuf[length+BEGIN-LPC_ORD],LPC_ORD);
		
	/* Adjust scaling of synthetic speech */
	scale_adj(&syn->sigbuf[BEGIN],gain,&syn->prev_scale,length);

	/* Implement pulse dispersion filter */
	v_equ(&syn->sigbuf[BEGIN-DISP_ORD],syn->disp_del,DISP_ORD);
	v_equ(syn->disp_del,&syn->sigbuf[length+BEGIN-DISP_ORD],DISP_ORD);
	zerflt(&syn->sigbuf[BEGIN],disp_cof,&syn->sigbuf[BEGIN],DISP_ORD,length);
		
	/* Copy processed speech to output array (not past frame end) */
	if (syn->syn_begin+length > FRAME) {
	    v_equ(&sp_out[syn->syn_begin],&syn->sigbuf[BEGIN],FRAME-syn->syn_begin);
	    /* past end: save remainder in syn->sigsave[0] */
	    v_equ(&syn->sigsave[0],&syn->sigbuf[BEGIN+FRAME-syn->syn_begin], length-(FRAME-syn->syn_begin));
	}
			
	else
	    v_equ(&sp_out[syn->syn_begin],&syn->sigbuf[BEGIN],length);
		
	/* Update syn->syn_begin for next period */
	syn->syn_begin += length;

    }
		
    /* Save previous pulse and noise filters for next frame */
    v_equ(syn->prev_pcof,curr_pcof,MIX_ORD+1);
    v_equ(syn->prev_ncof,curr_ncof,MIX_ORD+1);
    
    /* Copy current parameters to previous parameters for next time */
    syn->prev_par = *par;
    syn->prev_tilt = curr_tilt;
    
    /* Update syn->syn_begin for next frame */
    syn->syn_begin -= FRAME;
    
}


/* 
 *
 * Subroutine melp_syn_init(): perform initialization for melp 
 *	synthesis
 *
 */

void* melp_syn_init()
{
    int16_t i;
    syn_context* syn = NULL;
    
    MEM_ALLOC(MALLOC,syn,1,syn_context);
    if (!syn) return 0;
    
    memset(syn, 0, sizeof(syn_context));
    
    rand_init(&syn->random_seed);
    v_zap(syn->prev_par.gain,NUM_GAINFR);
    syn->prev_par.pitch = UV_PITCH;
    syn->prev_par.lsf[0] = 0.0;
    
    for (i = 1; i < LPC_ORD+1; i++)
      syn->prev_par.lsf[i] = syn->prev_par.lsf[i-1] + (1.0/(LPC_ORD+1));
    
    syn->prev_par.jitter = 0.0;
    v_zap(&syn->prev_par.bpvc[0],NUM_BANDS);
    syn->prev_tilt=0.0;
    syn->prev_scale = 0.0;
    syn->syn_begin = 0;
    syn->noise_gain = MIN_NOISE;
    syn->firstcall = 1;
   
    v_zap(syn->pulse_del,MIX_ORD);
    v_zap(syn->noise_del,MIX_ORD);
    v_zap(syn->lpc_del,LPC_ORD);
    v_zap(syn->ase_del,LPC_ORD);
    v_zap(syn->tilt_del,TILT_ORD);
    v_zap(syn->disp_del,DISP_ORD);
    v_zap(syn->sig2,BEGIN+PITCHMAX);
    v_zap(syn->sigbuf,BEGIN+PITCHMAX);
    v_zap(syn->sigsave,FRAME+PITCHMAX);
    v_zap(syn->prev_pcof,MIX_ORD+1);
    v_zap(syn->prev_ncof,MIX_ORD+1);
    syn->prev_ncof[MIX_ORD/2] = 1.0;

    fill(syn->prev_par.fs_mag,1.0,NUM_HARM);

    /* 
     * Initialize multi-stage vector quantization (read codebook) 
     */

    /* 
     * Allocate memory for number of levels per stage and indices
     * and for number of bits per stage 
     */

    MEM_ALLOC(MALLOC,syn->vq_par.num_levels,MSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,syn->vq_par.indices,MSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,syn->vq_par.num_bits,MSVQ_NUM_STAGES,int16_t);
    if (syn->vq_par.num_levels == 0 || syn->vq_par.indices == 0 || syn->vq_par.num_bits == 0) return 0;


    syn->vq_par.num_levels[0] = 128;
    syn->vq_par.num_levels[1] = 64;
    syn->vq_par.num_levels[2] = 64;
    syn->vq_par.num_levels[3] = 64;

    syn->vq_par.num_bits[0] = 7;
    syn->vq_par.num_bits[1] = 6;
    syn->vq_par.num_bits[2] = 6;
    syn->vq_par.num_bits[3] = 6;

    syn->vq_par.cb = msvq_cb;

    /* 
     * Initialize Fourier magnitude vector quantization (read codebook) 
     */

    /* 
     * Allocate memory for number of levels per stage and indices
     * and for number of bits per stage 
     */

    MEM_ALLOC(MALLOC,syn->fs_vq_par.num_levels,FSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,syn->fs_vq_par.indices,FSVQ_NUM_STAGES,int16_t);
    MEM_ALLOC(MALLOC,syn->fs_vq_par.num_bits,FSVQ_NUM_STAGES,int16_t);
    if (syn->fs_vq_par.num_levels == 0 || syn->fs_vq_par.indices == 0 || syn->fs_vq_par.num_bits == 0) return 0;


    syn->fs_vq_par.num_levels[0] = FS_LEVELS;

    syn->fs_vq_par.num_bits[0] = FS_BITS;

    syn->fs_vq_par.cb = fsvq_cb;

    MEM_2ALLOC(MALLOC, syn->lpc_f, 2, LPC_ORD_HALF+1, float);

    return syn;
}

void melp_syn_free(void* syn_handle) 
{
    syn_context* syn = (syn_context*) syn_handle;
    if (!syn) return;

    MEM_FREE(FREE,syn->fs_vq_par.num_bits);
    MEM_FREE(FREE,syn->fs_vq_par.indices);
    MEM_FREE(FREE,syn->fs_vq_par.num_levels);

    MEM_FREE(FREE,syn->vq_par.num_bits);
    MEM_FREE(FREE,syn->vq_par.indices);
    MEM_FREE(FREE,syn->vq_par.num_levels);

    MEM_2FREE(FREE, syn->lpc_f);
    MEM_FREE(FREE,syn);

}










