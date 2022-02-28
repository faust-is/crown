
/* ========================================= */
/* melp.c: Mixed Excitation LPC speech coder */
/* ========================================= */


#include <string.h>
#include <stdlib.h>

#include "sc1200.h"
#include "mat_lib.h"
#include "global.h"
#include "macro.h"
#include "mathhalf.h"
#include "dsp_sub.h"
#include "melp_sub.h"
#include "constant.h"
#include "math_lib.h"
#include "fec_code.h"
#include "cprv.h"
#include "fs_lib.h"
#include "lpc_lib.h"
#include "pitch.h"
#include "postfilt.h"
#include "qnt12.h"
#include "fft_lib.h"
#include "pit_lib.h"
#include "vocoderAPI.h"
#if NPP
#include "npp.h"
#endif

/* ========== Version definitions ========== */

#define PROGRAM_NAME			"SC1200 1200/2400 bps speech coder"
#define PROGRAM_VERSION			"Version 7 / 42 Bits"
#define PROGRAM_DATE			"09/27/2016"


/* MELPE object constructor */
void*  melpe_create(short rate, short direction)
{
	short encoder = (direction == VOCODER_DIRECTION_ENCODER);
	short decoder = (direction == VOCODER_DIRECTION_DECODER);
	melpe_context* m;
	
	/* check valid args */
	if (!encoder && !decoder) return NULL;
	if ( (rate != MELPE_RATE_2400) && (rate != MELPE_RATE_1200)) return NULL;
	
	m = (melpe_context*) malloc(sizeof(melpe_context));
	memset(m, 0, sizeof(melpe_context));
	m->rate = rate;
        /* create subcomponents, creation sequence must be preserved */
	m->global_handle = global_init(m->global_handle, rate, direction); /* common */
	m->chn_handle = melp_chn_init(m->chn_handle); /* common */
	m->fec_code_handle = fec_code_init(m->fec_code_handle); /* common */
	if (m->global_handle == NULL || m->chn_handle == NULL || 
	  m->fec_code_handle == NULL) return NULL;
#if NPP
	if (encoder) {
		m->npp_handle = npp_init(m->npp_handle); 
		if (m->npp_handle == NULL) return NULL;
	}
#endif
	if (encoder) {
		if (rate == MELPE_RATE_1200) {
			m->classify_handle = classify_init(m->classify_handle);
			if (m->classify_handle == NULL) return NULL;
		}
		m->lpc_handle = lpc_init(m->lpc_handle);
		m->melp_sub_handle = melp_sub_init(m->melp_sub_handle);
		if (m->lpc_handle == NULL || m->melp_sub_handle == NULL) return NULL;
	}
#if POSTFILTER
	if (decoder) {
		m->postfilt_handle = postfilt_init(m->postfilt_handle);
		if (m->postfilt_handle == NULL) return NULL;
	}
#endif
	if (encoder) {
		m->qnt12_handle = qnt12_init(m->qnt12_handle);
		m->pit_handle = pit_lib_init(m->pit_handle); 
		m->fft_handle = fft_init(m->fft_handle); 
		m->ana_handle = melp_ana_init(m->ana_handle); 
		if (m->qnt12_handle == NULL || m->pit_handle == NULL || 
		  m->fft_handle == NULL || m->ana_handle == NULL) return NULL;
	}
	if (decoder) {
		m->syn_handle = melp_syn_init(m->syn_handle);
		if (m->syn_handle == NULL) return NULL;
	}
	return (void*) m;
}

int melpe_free(void* melpe_handle) 
{
	melpe_context* m = (melpe_context*) melpe_handle;
	if (m) {
		if (m->classify_handle) free(m->classify_handle);
		if (m->global_handle) free(m->global_handle);
		if (m->chn_handle) free(m->chn_handle);
		if (m->fec_code_handle) free(m->fec_code_handle);
		if (m->npp_handle) free(m->npp_handle); 
		if (m->lpc_handle) free(m->lpc_handle); 
		if (m->melp_sub_handle) free(m->melp_sub_handle); 
		if (m->postfilt_handle) free(m->postfilt_handle); 
		if (m->qnt12_handle) free(m->qnt12_handle);
		if (m->pit_handle) free(m->pit_handle);
		if (m->fft_handle) free(m->fft_handle);
		if (m->ana_handle) free(m->ana_handle);
		if (m->syn_handle) free(m->syn_handle);
		
		free(m);
	}
	return 0;
}


int melpe_process(void* melpe_handle,  unsigned char *buf, short *sp) {
	struct melp_param melp_par[NF];
	melpe_context* m = (melpe_context*) melpe_handle;
	global_context* g = m->global_handle;
	memset(melp_par, 0x00, sizeof(melp_par));
	
	if (g->direction == VOCODER_DIRECTION_ENCODER) {
		if (m->ana_handle == NULL) return -1;
#if NPP
		/* noise pre-pocessor */
		npp(melpe_handle, sp, g->samplebuf);
#else
		memcpy(g->samplebuf, sp, g->frameSize << 1); /* bypass NPP */
#endif
		
		if (g->rate == MELPE_RATE_2400) {
			/* compress 180 samples  (22.5 mS) -> 54 bits buf (7 bytes) */
			analysis(melpe_handle, g->samplebuf, melp_par);
			memcpy(buf, g->chbuf, 7);
		} else {
			/* compress 540 samples (67.5 mS) -> 81 bits buf (11 bytes) */
#if NPP
			npp(melpe_handle, &sp[FRAME], &g->samplebuf[FRAME]);
			npp(melpe_handle, &sp[2 * FRAME], &g->samplebuf[2 * FRAME]);
#endif
			analysis(melpe_handle, g->samplebuf, melp_par);
			memcpy(buf, g->chbuf, 11);
		}
	} else {
		if (m->syn_handle == NULL) return -1;

		memset(melp_par, 0x00, sizeof(melp_par));
		if (g->rate == MELPE_RATE_2400) {
			/* decompress 54 bits buf (7 bytes) -> 180 samples sp (22.5 mS) */
			memcpy(g->chbuf, buf, 7);
			synthesis(melpe_handle, melp_par, sp);
		} else {
			/* decompress 81 bits buf (11 bytes) -> 540 samples sp (67.5 mS) */
			memcpy(g->chbuf, buf, 11);
			synthesis(melpe_handle, melp_par, sp);
		}
	}
	return 0;
}

  
/* library setup stub */ 
int melpe_library_setup(void) 
{
	return 0;
}

/* library destroy stub */
int melpe_library_destroy(void) 
{
	return 0;
}

