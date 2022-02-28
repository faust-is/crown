/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

/* ================================================================== */
/*                                                                    */
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */
/* ================================================================== */

/* =============================================== */
/* global.h: global variables for the sc1200 coder */
/* =============================================== */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "sc1200.h"

/* ====== Quantization constants ====== */
extern const int16_t msvq_bits[];
extern const int16_t msvq_levels[];

typedef struct _global_context {
	/* ====== Buffers ====== */
	int16_t hpspeech[IN_BEG + BLOCK];	/* input speech buffer dc removed, melp_ana.c */
	int16_t dcdelin[DC_ORD];     /* melp_ana.c */
	int16_t dcdelout_hi[DC_ORD]; /* melp_ana.c */
	int16_t dcdelout_lo[DC_ORD]; /* melp_ana.c */
	/* ====== Fourier Harmonics Weights ====== */
	int16_t w_fs[NUM_HARM];	/* Q14 */ /* melp_chn.c  melp_ana.c, melp_syn.c, qnt12.c */
	int16_t w_fs_inv[NUM_HARM]; /*  melp_ana.c, melp_syn.c */
	/* ====== Classifier ====== */
	int16_t voicedEn, silenceEn;	/* classify.c melp_ana.c *//* Q11 */

	int32_t voicedCnt; /* classify.c melp_ana.c */
	uint32_t rand_minstdgen_next;   /* seed; must not be zero!!! */

	int16_t chwordsize; /* melp_chn.c */
	/* ====== Data I/O for high level language implementation ====== */
	short rate; /* melp_ana.c, melp_syn.c, npp.c */
	short direction; /* encoder or decoder */
	/* ====== General parameters ====== */
	int16_t frameSize;  /* melp_syn.c*/ /* frame size 2.4 = 180 1.2 = 540 */
	int16_t bitNum; /* melp_chn.c */ /* number of bits */

	struct quant_param quant_par; /* melp_ana.c melp_syn.c qnt12.c */

	unsigned char chbuf[CHSIZE];	/* channel bit data buffer, melpe.c analysis, synthesis, CHSIZE=27 */ 
	int16_t * samplebuf;

} global_context;

typedef struct _melpe_context {
	int32_t rate;
	void* global_handle;
	void* classify_handle;
	void* chn_handle;
	void* npp_handle;
	void* fec_code_handle;
	void* lpc_handle;
	void* melp_sub_handle;
	void* postfilt_handle;
	void* qnt12_handle;
	void* fft_handle;
	void* pit_handle;
	void* ana_handle;
	void* syn_handle;
} melpe_context;

void* global_init(void* context, short rate, short direction);
#endif
