
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __TETRALIB__
#define  __TETRALIB__

/* TETRA single frame mode with rounding 138 bits to 18 bytes : 30ms <-> 18 bytes */
#define TETRA_RATE_4800			4800

/* TETRA double frame mode with rounding 137*2 bits to 35 bytes: 60ms <-> 35 bytes */
#define TETRA_RATE_4666			4666

/* Frame size in 16 bit samples */
#define TETRA_PLANE_SAMPLES_4800		240 /* 30 ms */

/* Encoded frame in 8 bit chars */
#define TETRA_CODE_BYTES_4800		18

/* Frame size in 16 bit samples */
#define TETRA_PLANE_SAMPLES_4666		480 /* 60 ms */

/* Encoded frame in 8 bit chars */
#define TETRA_CODE_BYTES_4666		35

/* TETRA rate 4800 vocoder indentification string (used in votest utility) */
#define TETRA_RATE_4800_STR		"tetra_4800"

#define TETRA_RATE_4666_STR		"tetra_4666"

/* library setup. Must be called once! */ 
int tetra_library_setup(void);

/* library destroy. Must be called once! */
int tetra_library_destroy(void);

/* create TETRA object:
    vocoder_identification:  TETRA_RATE_4800 or TETRA_RATE_4666)
    direction - encoder (VOCODER_DIRECTION_ENCODER) or decoder (VOCODER_DIRECTION_DECODER)
    return value is TETRA instance handle
*/
void* tetra_create(short vocoder_identification, short direction);

/* if direction==VOCODER_DIRECTION_ENCODER encode TETRA_PLANE_SAMPLES_xxx samples -> TETRA_CODE_BYTES_xxx bytes
 * if direction==VOCODER_DIRECTION_DECODER decode TETRA_CODE_BYTES_xxx bytes -> TETRA_PLANE_SAMPLES_xxx samples
 * return value is 0 on success or error code on fail;
 */
int tetra_process(void* tetra_handle,  unsigned char *buf, short *sp);


/* destroy TETRA object
 * return value is 0 on success or error code on fail;
 */
int tetra_free(void* tetra_handle);

#endif
#ifdef __cplusplus
}
#endif
