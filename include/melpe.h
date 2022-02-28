
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MELPELIB__
#define  __MELPELIB__

/* MELPE rate 1200/2400 vocoder indentification string (used in votest utility) */
#define MELPE_RATE_2400_STR		"melpe_2400"
#define MELPE_RATE_1200_STR		"melpe_1200"

/* MELPE rate 1200/2400 vocoder indentification number (not necessary to be exact rate) */
#define MELPE_RATE_2400			2400
#define MELPE_RATE_1200			1200

#define MELPE_PLANE_SAMPLES_2400	180
#define MELPE_PLANE_SAMPLES_1200	540

#define MELPE_CODE_BYTES_2400		7
#define MELPE_CODE_BYTES_1200		11

/*! \brief library setup for global structures. Call it once for global initialization.
 *         Implementers note: if global initialization is not used, set this function empty.
 *
 *  Call this function once for global initialization before vocoder creations.
 */
int melpe_library_setup(void);

/*! \brief library destroy for global structures. Call it once for global object destruction.
 *         Implementers note: if global initialization is not used, set this function empty.
 *
 *  Call this function once after all vocoders are freed.
 */
int melpe_library_destroy(void);

/* create MELPe object:
    rate - coding rate, bps ( MELPE_RATE_2400 or MELPE_RATE_1200)
    direction - encoder (VOCODER_DIRECTION_ENCODER) or decoder (VOCODER_DIRECTION_DECODER)
    return value is MELPe instance handle
*/
void* melpe_create(short rate, short direction);

/* if direction==VOCODER_DIRECTION_ENCODER encode 180 samples -> 7 bytes (rate=2400) or 540 samples ->11 bytes  (rate=1200)
 * if direction==VOCODER_DIRECTION_DECODER decode 7 bytes -> 180 samples  (rate=2400) or 11 bytes -> 540 samples  (rate=1200) 
 * return value is 0 on success or error code on fail;
 */
int melpe_process(void* melpe_handle,  unsigned char *buf, short *sp);


/* destroy MELPe object 
 * return value is 0 on success or error code on fail;
 */
int melpe_free(void* melpe_handle);

#endif
#ifdef __cplusplus
}
#endif
