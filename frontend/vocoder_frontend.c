
/* host header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

/* local header files */
#include "vocoderAPI.h"

/* Generic defines */

/* private data */
static int vocoder_frontend_initialized = 0;

size_t vocoder_get_input_size(short vocoder_identification, short direction)
{
	switch(vocoder_identification) {
	case  MELPE_RATE_2400:
		if (direction == VOCODER_DIRECTION_ENCODER) return MELPE_PLANE_SAMPLES_2400 * sizeof(short);
		else return (MELPE_CODE_BYTES_2400) * sizeof(char);
	case MELPE_RATE_1200:
		if (direction == VOCODER_DIRECTION_ENCODER) return MELPE_PLANE_SAMPLES_1200 * sizeof(short);
		else return (MELPE_CODE_BYTES_1200) * sizeof(char);
	case TETRA_RATE_4800:
		if (direction == VOCODER_DIRECTION_ENCODER) return TETRA_PLANE_SAMPLES_4800 * sizeof(short);
		else return (TETRA_CODE_BYTES_4800) * sizeof(char);
	case TETRA_RATE_4666:
		if (direction == VOCODER_DIRECTION_ENCODER) return TETRA_PLANE_SAMPLES_4666 * sizeof(short);
		else return (TETRA_CODE_BYTES_4666) * sizeof(char);
	default: return 0;
	}
}

size_t vocoder_get_output_size(short vocoder_identification, short direction) 
{
	short reverted_type = (direction == VOCODER_DIRECTION_DECODER) ? VOCODER_DIRECTION_ENCODER : VOCODER_DIRECTION_DECODER;
	return vocoder_get_input_size(vocoder_identification, reverted_type);
}

/* SW emulation layer for all vocoders (if run-time detection of DSP failed) */
static int vocoder_library_setup_sw(void)
{
	int res = 0;
#ifdef VOCODERLIB_USE_MELPE
	if (res == 0) res =  melpe_library_setup();
#endif
#ifdef VOCODERLIB_USE_TETRA
	if (res == 0) res =  tetra_library_setup();
#endif
	return res;
}

static int vocoder_library_destroy_sw(void) 
{
	int res = 0;
#ifdef VOCODERLIB_USE_MELPE
	if (res == 0) res =  melpe_library_destroy();
#endif
#ifdef VOCODERLIB_USE_TETRA
	if (res == 0) res =  tetra_library_destroy();
#endif
	return res;
}

static void* vocoder_create_sw(short vocoder_identification, short direction) {
	switch(vocoder_identification) 
	{
#ifdef VOCODERLIB_USE_MELPE
	case MELPE_RATE_1200:
	case MELPE_RATE_2400:
		return melpe_create(vocoder_identification, direction);
#endif
#ifdef VOCODERLIB_USE_TETRA
	case TETRA_RATE_4800:
	case TETRA_RATE_4666:
		return tetra_create(vocoder_identification, direction);
#endif
	default: return 0;
	}
}

static int vocoder_free_sw(void* handle)
{
	int32_t vocoder_identification;
	if (handle == 0) return 0;
	vocoder_identification = * (int32_t*) handle;
	switch(vocoder_identification)
	{
#ifdef VOCODERLIB_USE_MELPE
	case MELPE_RATE_1200:
	case MELPE_RATE_2400:
		return melpe_free(handle);
#endif
#ifdef VOCODERLIB_USE_TETRA
	case TETRA_RATE_4800:
	case TETRA_RATE_4666:
		return tetra_free(handle);
#endif
	default: return -1;
	}
}

int vocoder_process_sw(void* handle,  unsigned char *buf, short *sp)
{
	int32_t vocoder_identification;
	if (handle == 0)
	{
		return 0;
	}
	vocoder_identification = * (int32_t*) handle;
	switch(vocoder_identification)
	{
#ifdef VOCODERLIB_USE_MELPE
	case MELPE_RATE_1200:
	case MELPE_RATE_2400:
		return melpe_process(handle, buf, sp);
#endif
#ifdef VOCODERLIB_USE_TETRA
	case TETRA_RATE_4800:
	case TETRA_RATE_4666:
		return tetra_process(handle, buf, sp);
#endif
	default:
		return -1;
	}
}


/*
 *  ======== Vocoder frontend setup (must be called once!) ========
 */
int vocoder_library_setup(void)
{
	if (vocoder_frontend_initialized) return 1;
	/* already initialized is not an error, so return positive code */
	
	vocoder_frontend_initialized = 1;
	
	return vocoder_library_setup_sw();
}


/*
 *  ======== Vocoder frontend destroy (must be called once!) ========
 */
int vocoder_library_destroy(void)
{
	return vocoder_library_destroy_sw();
}


void* vocoder_create(short vocoder_identification, short direction)
{
	return vocoder_create_sw(vocoder_identification, direction);
}

int vocoder_free(void* vocoder_handle)
{
	return vocoder_free_sw(vocoder_handle);
}

int vocoder_process(void* vocoder_handle, unsigned char *buf, short *sp) 
{
	return vocoder_process_sw(vocoder_handle, buf, sp);
}

