#ifndef _HANDLER_H
#define _HANDLER_H

#include "vocoderAPI.h"
#include <ad.h>
#include <stdio.h>

typedef struct _vocoder_info {
	const char* vocoder_string;
	short vocoder_identification;
} vocoder_info;

// TODO: static?
static vocoder_info supported_vocoders[] = {
	{MELPE_RATE_2400_STR, MELPE_RATE_2400},
	{MELPE_RATE_1200_STR, MELPE_RATE_1200},
	{TETRA_RATE_4800_STR, TETRA_RATE_4800},
	{TETRA_RATE_4666_STR, TETRA_RATE_4666},
	/* Add more vocoders here! */
};

void recognize_from_file(FILE * rawfd, int samplerate, int vocoder_identification, int tty_handle);
void recognize_from_microphone(ad_rec_t *ad, int vocoder_identification, int tty_handle);

#endif /* HANDLER_H */