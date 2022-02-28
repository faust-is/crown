
#ifdef __cplusplus
extern "C" {
#endif

#ifndef  __VOCODER_API__
#define  __VOCODER_API__

#include <stdlib.h> /* NULL, size_t */

/* MELPe 1200/2400 bps support */
#include "melpe.h"

/* TETRA 4600 bps support (with byte-rounding: 4800 or 4666 modes) */
#include "tetra.h"

/* Vocoder directions (encoder or decoder) */
#define VOCODER_DIRECTION_ENCODER	1
#define VOCODER_DIRECTION_DECODER	2
/* aliases (old style) */
#define VOCODER_TYPE_ENCODER		VOCODER_DIRECTION_ENCODER
#define VOCODER_TYPE_DECODER		VOCODER_DIRECTION_DECODER

typedef struct _vocoder_interface {
	void* (*create_fxn)(short, short);
	int (*delete_fxn) (void*);
	int (*process_fxn) (void*, unsigned char *, short *);
} vocoder_interface;



/*! \brief library setup for global structures. Call it once for global initialization.
 *         Implementers note: if global initialization is not used, set this function empty.
 *
 *  Call this function once for global initialization before vocoder creations.
 */
int vocoder_library_setup(void);

/*! \brief library destroy for global structures. Call it once for global object destruction.
 *         Implementers note: if global initialization is not used, set this function empty.
 *
 *  Call this function once after all vocoders are freed.
 */
int vocoder_library_destroy(void);

/*! \brief create vocoder object.

    this function allocates internal memory for vocoder instance, use vocoder_free to free this memory.
    \param codec_identification_number codec identification.
    \param direction - VOCODER_DIRECTION_ENCODER for encoder or VOCODER_DIRECTION_DECODER for decoder.
    \return vocoder handle (this function allocates memory internally) if success, or NULL if resource allocation failed.
*/
void* vocoder_create(short codec_identification_number, short direction);


/*! \brief encode or decode for common vocoder API.

    if direction==VOCODER_DIRECTION_ENCODER encode,
    if direction==VOCODER_DIRECTION_DECODER decode.
    \param vocoder_handle - vocoder handle returned by vocoder_create.
    \param buf - coded data (chars). input for decoder, output for encoder.
    \param sp - speech PCM data, format S16_LE (16 bit little endian).
    \return 0 on success or error code on fail.
*/
int vocoder_process(void* vocoder_handle,  unsigned char *buf, short *sp);


/*! \brief free vocoder object for common API.

    this frees internal memory for vocoder instance
    \param vocoder_handle - vocoder handle returned by vocoder_create.
    \return 0 on success or error code on fail.
*/
int vocoder_free(void* vocoder_handle);

/*! \brief input size for specific vocoder in 8-bit chars.

    \param vocoder_identification - vocoder identification number.
    \param direction - VOCODER_DIRECTION_ENCODER or VOCODER_DIRECTION_DECODER
    \return input size in 8 bit chars or 0 if vocoder identification number is not supported.
*/
size_t vocoder_get_input_size(short codec_identification_number, short direction);

/*! \brief output size for specific vocoder in 8-bit chars.

    \param vocoder_identification - vocoder identification number.
    \param direction - VOCODER_DIRECTION_ENCODER or VOCODER_DIRECTION_DECODER
    \return output size in 8 bit chars or 0 if vocoder identification number is not supported.
*/
size_t vocoder_get_output_size(short codec_identification_number, short direction);

#endif
#ifdef __cplusplus
}
#endif
