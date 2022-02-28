
/*  compiler include files  */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "melp.h"
#include "vocoderAPI.h"
#include "spbstd.h"
#include "mat.h"
#if NPP
#include "npp_fl.h"
#endif

/* library setup. Must be called once! */
int melpe_library_setup(void) {
    /* MELP code invokes code-books pre-calculation */
    /*  once per library, so this function is not reenterable */
    msvq_cb_init();
    fsvq_cb_init();
    return 0;
}

/* library destroy. Must be called once! */
int melpe_library_destroy(void) {
    return 0;
}

void* melpe_create(short vocoder_identification, short direction) {
    struct melp_param* ptr;
#if NPP
    npp_context* npp;
#endif
    if (vocoder_identification != MELPE_RATE_2400) return NULL;
    if (direction == VOCODER_DIRECTION_ENCODER) {
        ptr = malloc(sizeof(struct  melp_param));
        if (!ptr) return NULL;
        memset(ptr, 0, sizeof(struct  melp_param));
        ptr->rate = vocoder_identification;
        ptr->direction = direction;
        ptr->prev_gain = 0.f;
        ptr->proc_ctx = melp_ana_init();
#if NPP
        npp = (void*) malloc(sizeof(npp_context));
        memset(npp, 0, sizeof(npp_context));
        ptr->npp_handle = (void*) npp;
        npp_init(npp);
#else
        ptr->npp_handle = NULL;
#endif
    } else {
        ptr = malloc(sizeof(struct  melp_param));
        if (!ptr) return NULL;
        memset(ptr, 0, sizeof(struct  melp_param));
        ptr->rate = vocoder_identification;
        ptr->direction = direction;
        ptr->prev_gain = 0.f;
        ptr->proc_ctx = melp_syn_init();
    }
    return (void*) ptr;
}

int melpe_free(void* melpe_handle) {
    struct melp_param* par = (struct melp_param*) melpe_handle;
    if (!par) return 0;
    if (par->direction == VOCODER_DIRECTION_ENCODER) {
        melp_ana_free(par->proc_ctx);
#if NPP
        free(par->npp_handle);
#endif
    } else {
        melp_syn_free(par->proc_ctx);
    }
    if (melpe_handle) free (melpe_handle);
    return 0;
}

static inline float math_round (float x) {
    if (x > 0.f ) {
        return floorf (x + 0.5f);
    } else {
        return ceilf (x - 0.5f);
    }
}

int melpe_process(void* melpe_handle,  unsigned char *buf, short *sp) {
    float speech[FRAME];
    float temp;
    int i;
    struct melp_param* par = (struct melp_param*) melpe_handle;
    if (par->direction == VOCODER_DIRECTION_ENCODER)
    {
        for(i=0; i<FRAME; i++) speech[i] = (float) sp[i];
#if NPP
        npp((npp_context*) par->npp_handle, speech, speech);
#endif

        par->chptr = buf;
        par->chbit = 0;
        melp_ana(speech, par);

        if (par->chptr >= &buf[7]) {
            return -1; /*out of buffer */
        }
  } else {
        par->chptr = buf;
        par->chbit = 0;
        melp_syn(par, speech);

        for(i=0; i<180; i++) {
            temp = math_round(speech[i]); /* rounding */
            if (temp > 32767) temp = 32767; else
            if (temp < -32767) temp = -32767;
            sp[i] = (short) temp;
        }
    }
    return 0;
}
