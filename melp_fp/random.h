#ifndef __RANDOM_H
#define __RANDOM_H

#include <stdint.h>

#if defined RANDOM_GENERATOR_STDFAST
typedef struct _rand_context {
    uint32_t seed;
} rand_context;

#define MINSTD_RAND_MAX 32767
static const double minstd_scale = (1.0 / MINSTD_RAND_MAX);

/* initialize random generator */
static inline void rand_init(rand_context *p_rnd) {
   p_rnd->seed = 1;
}

/* generate random float from (0,1), bounds are not included */
static inline float rand_num(float amp, rand_context *p_rnd) {
   int32_t r;
   p_rnd->seed = p_rnd->seed * 1103515245 + 12345;
   r = (p_rnd->seed >> 16) & MINSTD_RAND_MAX;
   return (amp*2.0) * ((float) r * minstd_scale - 0.5); 
}
#elif defined RANDOM_GENERATOR_SHUFFLE

#define IA 16807
#define IM 2147483647
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)
static const double AM = 1.0/IM;

typedef struct _rand_context {
    int32_t idum;
    int32_t iy;
    int32_t iv[NTAB];
} rand_context;

static inline void rand_init(rand_context *p_rnd) 
{
   int16_t j;
   int32_t k;

   p_rnd->idum = 12345;
   p_rnd->iy = 0;
   for (j=NTAB+7; j>=0; j--) { /* Load the shuffle table (after 8 warm-ups).*/
       k=(p_rnd->idum) / IQ;
       p_rnd->idum=IA * (p_rnd->idum - k * IQ) - IR * k;
       if (p_rnd->idum < 0) p_rnd->idum += IM;
       if (j < NTAB) p_rnd->iv[j] = p_rnd->idum;
   }

   p_rnd->iy=p_rnd->iv[0];

}

static inline float rand_num(float amp, rand_context *p_rnd) {
    int16_t j;
    int32_t k;
    float temp;


    if (p_rnd->idum <= 0 || !p_rnd->iy) {  /* Initialize. */
        if (-(p_rnd->idum) < 1) p_rnd->idum=1; /* Be sure to prevent idum = 0.*/
        else p_rnd->idum = -(p_rnd->idum);
        for (j=NTAB+7;j>=0;j--) { /* Load the shuffle table (after 8 warm-ups).*/
            k=(p_rnd->idum)/IQ;
            p_rnd->idum=IA*(p_rnd->idum-k*IQ)-IR*k;
            if (p_rnd->idum < 0) p_rnd->idum += IM;
            if (j < NTAB) p_rnd->iv[j] = p_rnd->idum;
        }
        p_rnd->iy=p_rnd->iv[0];
    }

    k=(p_rnd->idum)/IQ; /* Start here when not initializing.*/
    p_rnd->idum=IA*(p_rnd->idum-k*IQ)-IR*k; /* Compute idum=(IA*idum) % IM without */
    if  (p_rnd->idum < 0) p_rnd->idum += IM;  /* overflows by Schrage’s method. */
    j=p_rnd->iy/NDIV; /* Will be in the range 0..NTAB-1. */
    p_rnd->iy=p_rnd->iv[j]; /* Output previously stored value and refill the */
    p_rnd->iv[j] = p_rnd->idum; /* shuffle table. */
    temp = AM*p_rnd->iy;
    if (temp > RNMX) temp = RNMX; /* Because users don’t expect endpoint values.*/
    return (amp*2.0) * (temp - 0.5);
}
#elif defined RANDOM_GENERATOR_XOR64

typedef struct _rand_context 
{
    int64_t u;
    int64_t v;
    int64_t w;
} rand_context;

static inline int64_t rand_get64(rand_context *p_rnd) 
{
    int64_t x;
    p_rnd->u = p_rnd->u * 2862933555777941757LL + 7046029254386353087LL;
    p_rnd->v ^= p_rnd->v >> 17; 
    p_rnd->v ^= p_rnd->v << 31; 
    p_rnd->v ^= p_rnd->v >> 8;
    p_rnd->w = 4294957665U*(p_rnd->w & 0xffffffff) + (p_rnd->w >> 32);
    x = p_rnd->u ^ (p_rnd->u << 21); 
    x ^= x >> 35; 
    x ^= x << 4;
    return (x + p_rnd->v) ^ p_rnd->w;
}

static inline void rand_init(rand_context *p_rnd) 
{
    int64_t seed = 0xebdfacca19276365ULL;
    p_rnd->v = 4101842887655102017LL;
    p_rnd->w = 1;
    p_rnd->u = seed ^ p_rnd->v; 
    (void) rand_get64(p_rnd);
    p_rnd->v = p_rnd->u; 
    (void) rand_get64(p_rnd);
    p_rnd->w = p_rnd->v; 
    (void) rand_get64(p_rnd);
}

static inline float rand_num(float amp, rand_context *p_rnd) 
{
    double d = 5.42101086242752217E-20 * rand_get64(p_rnd) - 0.5;
    return 2.0 * amp * d;
}

#else
#error "Define specific RANDOM_GENERATOR_* macro"
#endif

#endif
