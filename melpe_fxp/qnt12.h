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

/*------------------------------------------------------------------*/
/*																	*/
/* File:		qnt12.c 											*/
/*																	*/
/* Description: quantization for 1200bps							*/
/*																	*/
/*------------------------------------------------------------------*/

#ifndef _QNT12_H_
#define _QNT12_H_

void pitch_vq(void* melpe_handle, struct melp_param *par);

void gain_vq(void* melpe_handle, struct melp_param *par);

void quant_bp(void* melpe_handle, struct melp_param *par, int16_t num_frames);

BOOLEAN lspStable(int16_t lsp[], int16_t order);

void lspSort(int16_t lsp[], int16_t order);

void lsf_vq(void* melpe_handle, struct melp_param *par);

void deqnt_msvq(int16_t qout[], const int16_t codebook[], int16_t tos,
		const int16_t cb_size[], int16_t * index, int16_t dim);

void quant_jitter(void* melpe_handle, struct melp_param *par);

void quant_fsmag(void* melpe_handle, struct melp_param *par);

void* qnt12_init(void* context);

#endif
