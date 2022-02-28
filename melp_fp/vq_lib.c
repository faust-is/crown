/*

2.4 kbps MELP Proposed Federal Standard speech coder

version 1.2

Copyright (c) 1996, Texas Instruments, Inc.  

Texas Instruments has intellectual property rights on the MELP
algorithm.  The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).


*/

/*

  vq_lib.c: vector quantization subroutines 

*/


#include <stdio.h>
#include <math.h>
#include "spbstd.h"
#include "vq.h"
#include "mat.h"
#include "lpc.h"
#include "melp.h"

#define BIGVAL 1E20

/* VQ_LSPW- compute LSP weighting vector-

    Atal's method:
        From Atal and Paliwal (ICASSP 1991)
        (Note: Paliwal and Atal used w(k)^2(u(k)-u_hat(k))^2,
         and we use w(k)(u(k)-u_hat(k))^2 so our weights are different
         but the method (i.e. the weighted MSE) is the same.

                */

float *vq_lspw(float *w,float *lsp,float *a,int16_t p)
{
    int16_t j;

        for(j=0; j < p; j++)
        {
            w[j] = (float)pow((double)lpc_aejw(a,lsp[j]*M_PI,p),(double)-0.3);
            if (j == 8)
                w[j] *= 0.64;
            else if (j == 9)
                w[j] *= 0.16;
        }

    return(w);
} /* VQ_LSPW */

/*
    VQ_MS4-
        Tree search multi-stage VQ encoder 

    Synopsis: vq_ms4(cb,u,u_est,levels,ma,stages,p,w,u_hat,a_indices)
        Input:
            cb- one dimensional linear codebook array (codebook is structured 
                as [stages][levels for each stage][p])
            u- dimension p, the parameters to be encoded (u[0..p-1])
            u_est- dimension p, the estimated parameters or mean (if NULL, assume
               estimate is the all zero vector) (u_est[0..p-1])
            levels- the number of levels at each stage (levels[0..stages-1])
            MSVQ_NUM_BEST- the tree search size (keep ma candidates from one stage to the
               next)
            MSVQ_NUM_STAGES - the number of stages of msvq
            VQ_DIMENSION - the predictor order
            w- the weighting vector (w[0..p-1])
            max_inner- the maximum number of times the swapping procedure
                       in the inner loop can be executed
        Output:
            u_hat-   the reconstruction vector (if non null)
            a_indices- the codebook indices (for each stage) a_indices[0..stages-1]
        Parameters:

*/

#define P_SWAP(x,y,type) do{type u__p;u__p = x;x = y;y = u__p;}while(0)

float vq_ms4(const float *cb, float *u, int16_t *levels, float *w, float *u_hat, int16_t *a_indices,
       int16_t *indices, float *errors, float *uhatw, float *d, int16_t *parents)
{
    float tmp,uhatw_sq;
    float d_cj,d_opt;
    float *p_d,*n_d,*p_distortion;
    const float *cb_currentstage,*cbp;
    float *p_errors,*n_errors,*p_e;
    double acc = 0;
    int16_t i,j,m,s,c,p_max,inner_counter;
    int16_t *p_indices,*n_indices;
    int16_t *p_parents,*n_parents;

    /* initialize memory */
    v_zap_int(indices,2*MSVQ_NUM_STAGES*MSVQ_NUM_BEST);
    v_zap_int(parents,2*MSVQ_NUM_BEST);

    /* initialize inner loop counter */
    inner_counter = 0;

    /* set up memory */
    p_indices = &indices[0];
    n_indices = &indices[MSVQ_NUM_BEST*MSVQ_NUM_STAGES];
    p_errors = &errors[0];
    n_errors = &errors[MSVQ_NUM_BEST*VQ_DIMENSION];
    p_d = &d[0];
    n_d = &d[MSVQ_NUM_BEST];
    p_parents = &parents[0];
    n_parents = &parents[MSVQ_NUM_BEST];

    for(j=0; j < VQ_DIMENSION; j++)
    {
        /* make double precision to prevent underflow */
        acc += u[j]*u[j]*w[j];
    }

    tmp = (float) acc;

    /* set up inital error vectors (i.e. error vectors = u) */
    for(c=0; c < MSVQ_NUM_BEST; c++)
    {
        (void)v_equ(&n_errors[c*VQ_DIMENSION],u,VQ_DIMENSION);
        n_d[c] = tmp;
    }

    /* codebook pointer is set to point to first stage */
    cbp = cb;

    /* set m to 1 for the first stage
       and loop over all MSVQ_NUM_STAGES */

    for(m=1,s=0; s < MSVQ_NUM_STAGES; s++)
    {
        /* Save the pointer to the beginning of the
           current stage.  Note: cbp is only incremented in
           one spot, and it is incremented all the way through
           all the stages. */
        cb_currentstage = cbp;

        /* set up pointers to the parent and current nodes */
        P_SWAP(p_indices,n_indices,int16_t*);
        P_SWAP(p_parents,n_parents,int16_t*);
        P_SWAP(p_errors,n_errors,float*);
        P_SWAP(p_d,n_d,float*);

        /* p_max is the pointer to the maximum distortion
           node over all candidates.  The so-called worst
           of the best. */
        p_max = 0;

        /* set the distortions to a large value */
        for(c=0; c < MSVQ_NUM_BEST; c++)
            n_d[c] = BIGVAL;

        for(j=0; j < levels[s]; j++)
        {
            /* compute weighted codebook element, increment codebook pointer */
            for(i=0,uhatw_sq=0.0; i < VQ_DIMENSION; i++,cbp++)
            {
                uhatw_sq += *cbp * (tmp = *cbp * w[i]);
                uhatw[i] = -2.0*tmp;
            }

            /* p_e points to the error vectors and p_distortion
               points to the node distortions.  Note: the error
               vectors are contiguous in memory, as are the distortions.
               Thus, the error vector for the 2nd node comes immediately
               after the error for the first node.  (This saves on
               repeated initializations) */
            p_e = p_errors;
            p_distortion = p_d;

            /* iterate over all parent nodes */
            for(c=0; c < m; c++)
            {
                d_cj = *p_distortion++ + uhatw_sq;
                for(i=0; i < VQ_DIMENSION; i++)
                    d_cj += *p_e++ * uhatw[i];

                /* determine if d is less than the maximum candidate distortion                   i.e., is the distortion found better than the so-called
                   worst of the best */
                if (d_cj <= n_d[p_max])
                {
                    /* replace the worst with the values just found */
                    n_d[p_max] = d_cj;
                    n_indices[p_max*MSVQ_NUM_STAGES+s] = j;
                    n_parents[p_max] = c;

                    /* want to limit the number of times the inner loop
                       is entered (to reduce the *maximum* complexity) */
                    if (inner_counter < MSVQ_MAXCNT)
                    {
                        inner_counter++;
		        if (inner_counter < MSVQ_MAXCNT)
			{
                            p_max = 0;
                            /* find the new maximum */
		            for(i=1; i < MSVQ_NUM_BEST; i++)
                            {
                                if (n_d[i] > n_d[p_max])
                                    p_max = i;
		            }
		        }
			else /* inner_counter == MSVQ_MAXCNT */
                        {
                               /* The inner loop counter now exceeds the
                               maximum, and the inner loop will now not
                               be entered.  Instead of quitting the search
                               or doing something drastic, we simply keep
                               track of the best candidate (rather than the
                               M best) by setting p_max to equal the index
                               of the minimum distortion
                               i.e. only keep one candidate around
                                the MINIMUM distortion */
		            for(i=1; i < MSVQ_NUM_BEST; i++)
                            {
                                if (n_d[i] < n_d[p_max])
                                    p_max = i;
		            }
		        }
		    }
                }
            } /* for c */
        } /* for j */

        /* compute the error vectors for each node */
        for(c=0; c < MSVQ_NUM_BEST; c++)
        {
            /* get the error from the parent node and subtract off
               the codebook value */
            (void)v_equ(&n_errors[c*VQ_DIMENSION],&p_errors[n_parents[c]*VQ_DIMENSION],VQ_DIMENSION);
            (void)v_sub(&n_errors[c*VQ_DIMENSION],&cb_currentstage[n_indices[c*MSVQ_NUM_STAGES+s]*VQ_DIMENSION],VQ_DIMENSION);

            /* get the indices that were used for the parent node */
            (void)v_equ_int(&n_indices[c*MSVQ_NUM_STAGES],&p_indices[n_parents[c]*MSVQ_NUM_STAGES],s);
        }

        m = (m*levels[s] > MSVQ_NUM_BEST) ? MSVQ_NUM_BEST : m*levels[s];
    } /* for s */

    /* find the optimum candidate c */
    for(i=1,c=0; i < MSVQ_NUM_BEST; i++)
    {
        if (n_d[i] < n_d[c])
	    c = i;
    }

    d_opt = n_d[c];
    
    if (a_indices)
    {
        (void)v_equ_int(a_indices,&n_indices[c*MSVQ_NUM_STAGES],MSVQ_NUM_STAGES);
    }
    if (u_hat)
    {
        (void)v_zap(u_hat,VQ_DIMENSION);
        cb_currentstage = cb;
        for(s=0; s < MSVQ_NUM_STAGES; s++)
        {
            (void)v_add(u_hat,&cb_currentstage[n_indices[c*MSVQ_NUM_STAGES+s]*VQ_DIMENSION],VQ_DIMENSION);
            cb_currentstage += levels[s]*VQ_DIMENSION;
        }
    }

    return(d_opt);
}

/*
    VQ_MSD2-
        Tree search multi-stage VQ decoder

    Synopsis: vq_msd(cb,u,u_est,a,indices,levels,stages,p,conversion)
        Input:
            cb- one dimensional linear codebook array (codebook is structured 
                as [stages][levels for each stage][p])
            indices- the codebook indices (for each stage) indices[0..stages-1]
            levels- the number of levels (for each stage) levels[0..stages-1]
            u_est- dimension p, the estimated parameters or mean (if NULL, assume
               estimate is the all zero vector) (u_est[0..p-1])
            stages- the number of stages of msvq
            p- the predictor order
            conversion- the conversion constant (see lpc.h, lpc_conv.c)
        Output:
            u- dimension p, the decoded parameters (if NULL, use alternate
               temporary storage) (u[0..p-1])
            a- predictor parameters (a[0..p]), if NULL, don't compute
        Returns:
            pointer to reconstruction vector (u)
        Parameters:

*/

float *vq_msd2(const float *cb, float *u, int16_t *indices, int16_t *levels, int16_t stages, int16_t p)
{
    const float *cb_currentstage;
    int16_t i;
    /* WARNING: u_est is not used now (all zero) */

    /* add estimate on (if non-null), or clear vector */
    v_zap(u,p);

    /* add the contribution of each stage */
    cb_currentstage = cb;
    for(i=0; i < stages; i++)
    {
        (void)v_add(u,&cb_currentstage[indices[i]*p],p);
        cb_currentstage += levels[i]*p;
    }

    return(u);
}

/* VQ_ENC -
   encode vector with full VQ using unweighted Euclidean distance
    Synopsis: vq_enc(cb, u, levels, p, u_hat, indices)
        Input:
            cb- one dimensional linear codebook array
            u- dimension p, the parameters to be encoded (u[0..p-1])
            levels- the number of levels
            p- the predictor order
        Output:
            u_hat-   the reconstruction vector (if non null)
            a_indices- the codebook indices (for each stage) a_indices[0..stages-1]
        Parameters:
*/

float vq_enc(const float *cb, float *u, int16_t levels, int16_t p, float *u_hat, int16_t *indices)
{
    int16_t i,j,index;
    float d,dmin;
    const float *p_cb;

    /* Search codebook for minimum distance */
    index = 0;
    dmin = BIGVAL;
    p_cb = cb;
    for (i = 0; i < levels; i++) {
	d = 0.0;
	for (j = 0; j < p; j++) {
	    d += SQR(u[j] - *p_cb);
	    p_cb++;
	}
	if (d < dmin) {
	    dmin = d;
	    index = i;
	}
    }

    /* Update index and quantized value, and return minimum distance */
    *indices = index;
    v_equ(u_hat,&cb[p*index],p);
    return(dmin);
}

/* VQ_FSW - 
   compute the weights for Euclidean distance of Fourier harmonics  */

void vq_fsw(float *w_fs, int16_t num_harm, float pitch)
{

    int16_t j;
    float w0;

    /* Calculate fundamental frequency */
    w0 = TWOPI/pitch;    
    for(j=0; j < num_harm; j++) {

	/* Bark-scale weighting */
	w_fs[j] = 117.0 / (25.0 + 75.0*
			   pow(1.0 + 1.4*SQR(w0*(j+1)/(0.25*PI)),0.69));
    }

}


