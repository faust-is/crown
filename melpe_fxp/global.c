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
/* global.c: global variables for the sc1200 coder */
/* =============================================== */

#include "sc1200.h"
#include "global.h"
#include "vq_lib.h"
#include "mat_lib.h"
#include "mathhalf.h"
#include "constant.h"
#include <string.h>
#include <stdlib.h>

/* ====== Quantization ====== */
const int16_t msvq_bits[MSVQ_STAGES] = { 7, 6, 6, 6 };
const int16_t msvq_levels[MSVQ_STAGES] = { 128, 64, 64, 64 };

/* ======================================= */
/* global_init(): this is to workaround the TI
 * compiler feature to omit static and global variables zeroing */
/* ======================================= */
void* global_init(void* context, short rate, short direction)
{
	int16_t i;
	global_context* g = (global_context*) context;
	if (!g) g = (global_context*) malloc(sizeof(global_context));
	if (!g) return 0;
	memset(g, 0x00, sizeof(global_context));
	g->rate = rate;
	g->direction = direction;

	if (rate == RATE2400) {
		/* initialize codec at 2400 bps */
		g->frameSize = (int16_t) FRAME;
		g->chwordsize = 8;
		g->bitNum = 54; 
	} else {
		/* initialize codec at 1200 bps */
		g->frameSize = (int16_t) BLOCK;
		g->chwordsize = 8;
		g->bitNum = 81;
	}
	if (direction == 0x1 /* encoder */) {
		g->samplebuf = (int16_t*) malloc(sizeof(short) * g->frameSize);
		if (!g->samplebuf) {
			free(g);
			return 0;
		}
	}
	g->rand_minstdgen_next = 1; /* seed; must not be zero!!! */
	/* Initialize fixed MSE weighting and inverse of weighting */

	vq_fsw(g->w_fs, NUM_HARM, X60_Q9);
	for (i = 0; i < NUM_HARM; i++)
		g->w_fs_inv[i] = melpe_divide_s(ONE_Q13, g->w_fs[i]);	/* w_fs_inv in Q14 */
	
	return (void*) g;
}
