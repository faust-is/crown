
/*  compiler include files  */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "vocoderAPI.h"
#include "source.h"

#define serial_size 138
typedef struct _Tetra_Encoder_Internal {
	uint32_t vocoder_identification;
	uint32_t direction;
	Pre_Process_Context pre_process;
	Coder_Tetra_Context coder;
} Tetra_Encoder_Internal;

typedef struct _Tetra_Decoder_Internal {
	uint32_t vocoder_identification;
	uint32_t direction;
	Decoder_Tetra_Context decoder;
} Tetra_Decoder_Internal;


int tetra_library_setup(void) {
    /* init library global variables, not needed*/
    return 0;
}

/* library destroy, not needed */
int tetra_library_destroy(void) {
    return 0;
}

void* tetra_create(short vocoder_identification, short direction) {
    Tetra_Encoder_Internal* enc;
    Tetra_Decoder_Internal* dec;

    if (direction == VOCODER_DIRECTION_ENCODER) {
        enc = (Tetra_Encoder_Internal*) malloc(sizeof(Tetra_Encoder_Internal));
        if (!enc) return NULL;
        memset(enc, 0, sizeof(Tetra_Encoder_Internal));
        enc->vocoder_identification = vocoder_identification;
        enc->direction = direction;
        Init_Pre_Process(&enc->pre_process);
        Init_Coder_Tetra(&enc->coder);
        return (void*) enc;
    } else if (direction == VOCODER_DIRECTION_DECODER) {
        dec = (Tetra_Decoder_Internal*) malloc(sizeof(Tetra_Decoder_Internal));
        if (!dec) return NULL;
        memset(dec, 0, sizeof(Tetra_Decoder_Internal));
        dec->vocoder_identification = vocoder_identification;
        dec->direction = direction;
        Init_Decod_Tetra(&dec->decoder);
        return (void*) dec;
    } else
    return NULL;
}

int tetra_free(void* tetra_handle) {
    if (tetra_handle) free (tetra_handle);
    return 0;
}

/* convert nbit 16 bit words (serial) to packed bytes(buf)) */
/* WARNING: bit order: big endian, first word in serial is msb bit of buf[0] */
static void serial_to_packed( Word16* serial, unsigned char* buf, int nbit) {
    int i;
    unsigned char bit_index;
    unsigned char value = 0;

    for (i=0; i<nbit; i++)
    {
        bit_index = i & 0x7;
        if (serial[i] == 1) value |= (0x80 >> bit_index);
        if (bit_index == 7) {
            *buf = value; buf++;
            value = 0;
            if (i == (nbit -1)) return;
        }
    }

    *buf = value;
}

/* convert packed bytes (buf), nbit = number of bits, to nbit 16-bit words (serial)) */
/* WARNING: bit order: big endian, first word in serial is msb bit of buf[0] */
static void packed_to_serial(unsigned char* buf, Word16* serial, int nbit) {
    int i;
    unsigned char value;
    unsigned char bit_index;

    for (i=0; i<nbit; i++) {
        value = buf[i >> 3];
        bit_index = (i & 0x7);
        if (value & (0x80 >> bit_index)) serial[i]=1; else serial[i]=0;
    }
}

int tetra_process(void* tetra_handle,  unsigned char *buf, short *sp) {
    int32_t *info = (int32_t*)  tetra_handle;
    int32_t vocoder_identification = info[0];
    Word16 params[PARAMETERS_SIZE + 1];
    Word16 serial[serial_size * 2];
    int32_t direction = info[1];
    Tetra_Encoder_Internal* enc;
    Tetra_Decoder_Internal* dec;
    Word16 temp, bfi;

    if (direction == VOCODER_DIRECTION_ENCODER)
    {
        enc = (Tetra_Encoder_Internal*) tetra_handle;
        if (vocoder_identification == TETRA_RATE_4800) {
            memcpy(enc->coder.new_speech, sp, L_framesize * sizeof(short));
            Pre_Process(&enc->pre_process, enc->coder.new_speech, (Word16)L_framesize);
            Coder_Tetra(&enc->coder, params, 0);
            Prm2bits_Tetra(params, serial);
            serial_to_packed(serial, buf, serial_size);
        } else if (vocoder_identification == TETRA_RATE_4666) {
            /* 1-st frame of 30 ms */
            memcpy(enc->coder.new_speech, sp, L_framesize * sizeof(short));
            Pre_Process(&enc->pre_process, enc->coder.new_speech, (Word16)L_framesize);
            Coder_Tetra(&enc->coder, params, 0);
            Prm2bits_Tetra(params, serial);
            /* 2-nd frame of 30 ms */
            memcpy(enc->coder.new_speech, sp+L_framesize, L_framesize * sizeof(short));
            Pre_Process(&enc->pre_process, enc->coder.new_speech, (Word16)L_framesize);
            Coder_Tetra(&enc->coder, params, 0);
            temp = serial[137]; /* cut off first serial word (BFI) */
            Prm2bits_Tetra(params, &serial[137]);
            serial[137] = temp;
            /* now we have serial[1..274] */
            serial_to_packed(&serial[1], buf, 274);
        } else return -1;
   } else {
        dec = (Tetra_Decoder_Internal*) tetra_handle;
        if (vocoder_identification == TETRA_RATE_4800) {
            packed_to_serial(buf, serial, serial_size);
            Bits2prm_Tetra(serial, params);
            Decod_Tetra(&dec->decoder, params, sp);
            Post_Process(sp, (Word16)L_framesize);
        } else if (vocoder_identification == TETRA_RATE_4666) {
            bfi = !!(buf[35 - 1] & 0x30);
            packed_to_serial(buf, &serial[1], 274);
            serial[0] = bfi; /* BFI=0 corresponds to valid 1-st frame */
            /* decode 1-st frame of 30 ms */
            Bits2prm_Tetra(serial, params);
            Decod_Tetra(&dec->decoder, params, sp);
            Post_Process(sp, (Word16)L_framesize);
            serial[137] = bfi; /* BFI=0 corresponds to valid 2-nd frame */
            /* decode 2-nd frame of 30 ms */
            Bits2prm_Tetra(&serial[137], params);
            Decod_Tetra(&dec->decoder, params, sp+L_framesize);
            Post_Process(sp+L_framesize, (Word16)L_framesize);
        } else return -1;
    }
    return 0;
}
