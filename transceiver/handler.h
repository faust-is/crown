#ifndef _HANDLER_H
#define _HANDLER_H

#include "vocoderAPI.h"
//#include <ad.h>
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

static const int num_of_vocoders = sizeof(supported_vocoders) / sizeof(vocoder_info);

typedef int (*send_to_handler)(const char* data, size_t size);

int send_command(short unsigned int time, int tty_handle);
int send_voice_from_channel_to_socket(short vocoder_identification, int tty_handle, send_to_handler handler, int n_frames_for_out);

#endif /* HANDLER_H */