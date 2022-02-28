/***********************************************************************
*
*      TYPE DEFINITIONS
*
************************************************************************/

#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <stdint.h>
typedef int16_t  Word16;
typedef int32_t  Word32;
typedef int16_t  Flag;

#ifdef DEBUG_PRINT
#define PRINTF(str) fprintf(stderr, str)
#else
#define PRINTF(str)
#endif

typedef struct _Pre_Process_Context {
    Word16 y_hi;
    Word16 y_lo;
    Word16 x0;
    Word16 pad;
} Pre_Process_Context;

#endif
