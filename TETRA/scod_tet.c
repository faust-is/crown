/************************************************************************
*
*	FILENAME		:	scod_tet.c
*
*	DESCRIPTION		:	Main routines for speech source encoding
*
************************************************************************
*
*	SUB-ROUTINES	:	- Init_Coder_Tetra()
*					- Coder_Tetra()
*
************************************************************************
*
*	INCLUDED FILES	:	source.h
*
************************************************************************/

#include "source.h"


/**************************************************************************
*
*	ROUTINE				:	Init_Coder_Tetra
*
*	DESCRIPTION			:	Initialization of variables for the speech encoder
*
**************************************************************************
*
*	USAGE				:	Init_Coder_Tetra()
*
*	INPUT ARGUMENT(S)		:	Coder_Tetra_Context pointer
*
*	OUTPUT ARGUMENT(S)		:	None
*
*	RETURNED VALUE		:	None
*
**************************************************************************/

void Init_Coder_Tetra(Coder_Tetra_Context* ctx)
{
  Word16 i,j;


/*-----------------------------------------------------------------------*
 *      Initialize pointers to speech vector.                            *
 *                                                                       *
 *                                                                       *
 *   |--------------------|--------|--------|......|-------|-------|     *
 *     previous speech       sf1      sf2             sf4    L_next      *
 *                                                                       *
 *   <----------------  Total speech vector (L_total)   ----------->     *
 *   |           <----  LPC analysis window (L_window)  ----------->     *
 *   |           |        <---- present frame (L_framesize) --->             *
 *  ctx->old_speech   |        |       <----- new speech (L_framesize) ----->     *
 *            p_window    |       |                                      *
 *                     speech     |                                      *
 *                             new_speech                                *
 *-----------------------------------------------------------------------*/

  ctx->new_speech = ctx->old_speech + L_total - L_framesize;	/* New speech     */
  ctx->speech     = ctx->new_speech - L_next;			/* Present frame  */
  ctx->p_window   = ctx->old_speech + L_total - L_window;	/* For LPC window */

  /* Initialize global variables */

  ctx->last_ener_cod = 0;
  ctx->last_ener_pit = 0;

  /* Initialize static pointers */

  ctx->wsp    = ctx->old_wsp + pit_max;
  ctx->exc    = ctx->old_exc + pit_max + L_inter;
  ctx->zero   = ctx->ai_zero + LPCORDP1;


  /* Static vectors to zero */

  for(i=0; i<L_total; i++)
    ctx->old_speech[i] = 0;

  for(i=0; i<pit_max + L_inter; i++)
    ctx->old_exc[i] = ctx->old_wsp[i] = 0;

  for(i=0; i<LPCORD; i++)
    ctx->mem_syn[i] = ctx->mem_w[i] = ctx->mem_w0[i] = 0;

  for(i=0; i<L_subframesize; i++)
    ctx->zero[i] = 0;

  for(i=0; i< dim_rr; i++)
    for(j=0; j< dim_rr; j++)
    ctx->rr[i][j] = 0;

  /* Initialisation of lsp values for first */
  /* frame lsp interpolation */

  for(i=0; i<LPCORD; i++) {
    ctx->lspold_q[i] = ctx->lspold[i] = lspold_init[i];
    ctx->lsp_old[i] = lspold_init[i];
  }

  /* Compute LPC spectral expansion factors */

  Fac_Pond(gamma1, ctx->F_gamma1);
  Fac_Pond(gamma2, ctx->F_gamma2);
  Fac_Pond(gamma3, ctx->F_gamma3);
  Fac_Pond(gamma4, ctx->F_gamma4);

  /* Last A(z) for case of unstable filter */
  /* old_A = {4096,0,0,0,0,0,0,0,0,0,0} moved from static (Levin32) */;
  for(i=1; i<LPCORD+1; i++) ctx->old_A[i] = 0;
  ctx->old_A[0]=4096;

 return;
}


/**************************************************************************
*
*	ROUTINE				:	Coder_Tetra
*
*	DESCRIPTION			:	Main speech coder function
*
**************************************************************************
*
*	USAGE				:	Coder_Tetra(ana,synth)
*							(Routine_Name(output1,output2))
*
*	INPUT ARGUMENT(S)		:	None
*
*	OUTPUT ARGUMENT(S)		:	
*
*		OUTPUT1			:	- Description : Analysis parameters
*							- Format : 23 * 16 bit-samples
*
*		OUTPUT2			:	- Description : Local synthesis
*							- Format : 240 * 16 bit-samples
*
*	RETURNED VALUE		:	None
*
*	COMMENTS			:	- 240 speech data should have been copied to vector
*						ctx->new_speech[].  This vector is global and is declared in
*						this function.
*						- Output2 is for debugging only
*
**************************************************************************/

void Coder_Tetra(Coder_Tetra_Context* ctx, Word16* ana, Word16* synth)
{

  /* LPC coefficients */

  Word16 r_l[LPCORDP1], r_h[LPCORDP1];	/* Autocorrelations low and high        */
  Word16 A_t[(LPCORDP1)*4];		/* A(z) unquantized for the 4 subframes */
  Word16 Aq_t[(LPCORDP1)*4];		/* A(z)   quantized for the 4 subframes */
  Word16 Ap1[LPCORDP1];		/* A(z) with spectral expansion         */
  Word16 Ap2[LPCORDP1];		/* A(z) with spectral expansion         */
  Word16 Ap3[LPCORDP1];		/* A(z) with spectral expansion         */
  Word16 Ap4[LPCORDP1];		/* A(z) with spectral expansion         */
  Word16 *A, *Aq;			/* Pointer on A_t and Aq_t              */

  /* Other vectors */

  Word16 h1[L_subframesize];
  Word16 zero_h2[L_subframesize+64], *h2;
  Word16 zero_F[L_subframesize+64],  *F;
  Word16 res[L_subframesize];
  Word16 xn[L_subframesize];
  Word16 xn2[L_subframesize];
  Word16 dn[L_subframesize+4];
  Word16 code[L_subframesize+4];
  Word16 y1[L_subframesize];
  Word16 y2[L_subframesize];



  /* Scalars */

  Word16 i, i_subfr;
  Word16 T0, T0_min, T0_max, T0_frac;
  Word16 gain_pit, gain_code, index;
  Word16 sign_code, shift_code;
  Word16 temp;
  Word32 L_temp;

  /* Initialization of F and h2 */

  F  = &zero_F[64];
  h2 = &zero_h2[64];
  for(i=0; i<64; i++)
   zero_F[i] = zero_h2[i] = 0;

/*------------------------------------------------------------------------*
 *  - Perform LPC analysis:                                               *
 *       * autocorrelation + lag windowing                                *
 *       * Levinson-Durbin algorithm to find a[]                          *
 *       * convert a[] to lsp[]                                           *
 *       * quantize and code the LSPs                                     *
 *       * find the interpolated LSPs and convert to a[] for all          *
 *         subframes (both quantized and unquantized)                     *
 *------------------------------------------------------------------------*/

  Autocorr(ctx->p_window, LPCORD, r_h, r_l);		/* Autocorrelations */

  Lag_Window(LPCORD, r_h, r_l);			/* Lag windowing    */

  Levin_32(r_h, r_l, A_t, ctx->old_A);			/* Levinson-Durbin  */

  Az_Lsp(A_t, ctx->lspnew, ctx->lspold);		/* From A(z) to lsp */

  Clsp_334(ctx->lspnew, ctx->lspnew_q, ana, ctx->lsp_old);		/* Lsp quantization */

  ana += 3;				/* Increment analysis parameters pointer */

  /* Interpolation of LPC for the 4 subframes */

  Int_Lpc4(ctx->lspold,   ctx->lspnew,   A_t);
  Int_Lpc4(ctx->lspold_q, ctx->lspnew_q, Aq_t);

  /* update the LSPs for the next frame */

  for(i=0; i<LPCORD; i++)
  {
    ctx->lspold[i]   = ctx->lspnew[i];
    ctx->lspold_q[i] = ctx->lspnew_q[i];
  }


 /*----------------------------------------------------------------------*
  * - Find the weighted input speech ctx->wsp[] for the whole speech frame *
  * - Find open-loop pitch delay                                         *
  * - Set the range for searching closed-loop pitch                      *
  *----------------------------------------------------------------------*/

  A = A_t;
  for (i = 0; i < L_framesize; i += L_subframesize)
  {
    Pond_Ai(A, ctx->F_gamma1, Ap1);
    Pond_Ai(A, ctx->F_gamma2, Ap2);
    Residu(Ap1, &ctx->speech[i], &ctx->wsp[i], L_subframesize);
    Syn_Filt(Ap2, &ctx->wsp[i], &ctx->wsp[i], L_subframesize, ctx->mem_w, (Word16)1);
    A += LPCORDP1;
  }

  /* Find open loop pitch delay */

  T0 = Pitch_Ol_Dec(ctx->wsp, L_framesize);

  /* range for closed loop pitch search */

  T0_min = sub(T0, (Word16)2);
  if (T0_min < pit_min) T0_min = pit_min;
  T0_max = add(T0_min, (Word16)4);
  if (T0_max > pit_max)
  {
     T0_max = pit_max;
     T0_min = sub(T0_max, (Word16)4);
  }


 /*------------------------------------------------------------------------*
  *          Loop for every subframe in the analysis frame                 *
  *------------------------------------------------------------------------*
  *  To find the pitch and innovation parameters. The subframe size is     *
  *  L_subframesize and the loop is repeated L_framesize/L_subframesize times. *
  *     - find the weighted LPC coefficients                               *
  *     - find the LPC residual signal res[]                               *
  *     - compute the target signal for pitch search                       *
  *     - compute impulse response of weighted synthesis filter (h1[])     *
  *     - find the closed-loop pitch parameters                            *
  *     - encode the pitch delay                                           *
  *     - update the impulse response h1[] by including fixed-gain pitch   *
  *     - find the autocorrelations of h1[] (ctx->rr[][])                  *
  *     - find target vector for codebook search                           *
  *     - backward filtering of target vector                              *
  *     - codebook search                                                  *
  *     - encode codebook address                                          *
  *     - VQ of pitch and codebook gains                                   *
  *     - find synthesis speech                                            *
  *     - update states of weighting filter                                *
  *------------------------------------------------------------------------*/

  Aq = Aq_t;	/* pointer to interpolated quantized LPC parameters */

  for (i_subfr = 0;  i_subfr < L_framesize; i_subfr += L_subframesize)
  {


   /*---------------------------------------------------------------*
     * Find the weighted LPC coefficients for the weighting filter.  *
     *---------------------------------------------------------------*/

    Pond_Ai(Aq, ctx->F_gamma3, Ap3);
    Pond_Ai(Aq, ctx->F_gamma4, Ap4);


    /*---------------------------------------------------------------*
     * Compute impulse response, h1[], of weighted synthesis filter  *
     *---------------------------------------------------------------*/

    ctx->ai_zero[0] = 4096;				/* 1 in Q12 */
    for (i = 1; i <= LPCORD; i++) ctx->ai_zero[i] = 0;

    Syn_Filt(Ap4, ctx->ai_zero, h1, L_subframesize, ctx->zero, (Word16)0);

    /*---------------------------------------------------------------*
     * Compute LPC residual and copy it to ctx->exc[i_subfr]              *
     *---------------------------------------------------------------*/

    Residu(Aq, &ctx->speech[i_subfr], res, L_subframesize);

    for(i=0; i<L_subframesize; i++) ctx->exc[i_subfr+i] = res[i];

    /*---------------------------------------------------------------*
     * Find the target vector for pitch search:  ->xn[]              *
     *---------------------------------------------------------------*/

    Syn_Filt(Ap4, res, xn, L_subframesize, ctx->mem_w0, (Word16)0);

    /*----------------------------------------------------------------------*
     *                 Closed-loop fractional pitch search                  *
     *----------------------------------------------------------------------*
     * The pitch range for the first subframe is divided as follows:        *
     *   19 1/3  to   84 2/3   resolution 1/3                               *
     *   85      to   143      resolution 1                                 *
     *                                                                      *
     * The period in the first subframe is encoded with 8 bits.             *
     * For the range with fractions:                                        *
     *   code = (T0-19)*3 + frac - 1;   where T0=[19..85] and frac=[-1,0,1] *
     * and for the integer only range                                       *
     *   code = (T0 - 85) + 197;        where T0=[86..143]                  *
     *----------------------------------------------------------------------*
     * For other subframes: if t0 is the period in the first subframe then  *
     * T0_min=t0-5   and  T0_max=T0_min+9   and  the range is given by      *
     *      T0_min-1 + 1/3   to  T0_max + 2/3                               *
     *                                                                      *
     * The period in the 2nd,3rd,4th subframe is encoded with 5 bits:       *
     *  code = (T0-(T0_min-1))*3 + frac - 1;  where T0[T0_min-1 .. T0_max+1]*
     *---------------------------------------------------------------------*/

    T0 = Pitch_Fr(&ctx->exc[i_subfr], xn, h1, L_subframesize, T0_min, T0_max,
                  i_subfr, &T0_frac);

    if (i_subfr == 0)
    {
      /* encode pitch delay (with fraction) */

      if (T0 <= 85)
      {
        /* index = T0*3 - 58 + T0_frac; */
        index = add(T0, add(T0, T0));
        index = sub(index, (Word16)58);
        index = add(index, T0_frac);
      }
      else
        index = add(T0, (Word16)112);


      /* find T0_min and T0_max for other subframes */

      T0_min = sub(T0, (Word16)5);
      if (T0_min < pit_min) T0_min = pit_min;
      T0_max = add(T0_min, (Word16)9);
      if (T0_max > pit_max)
      {
        T0_max = pit_max;
        T0_min = sub(T0_max, (Word16)9);
      }
    }

    else						/* other subframes */
    {
      i = sub(T0, T0_min);
							/* index = i*3 + 2 + T0_frac;  */
      index = add(i, add(i, i));
      index = add(index, (Word16)2);
      index = add(index, T0_frac);
    }

    *ana++ = index;


   /*-----------------------------------------------------------------*
    *   - find unity gain pitch excitation (adaptive codebook entry)  *
    *     with fractional interpolation.                              *
    *   - find filtered pitch ctx->exc. y1[]=ctx->exc[] filtered by 1/Ap4(z)    *
    *   - compute pitch gain and limit between 0 and 1.2              *
    *   - update target vector for codebook search                    *
    *-----------------------------------------------------------------*/


    Pred_Lt(&ctx->exc[i_subfr], T0, T0_frac, L_subframesize);

    Syn_Filt(Ap4, &ctx->exc[i_subfr], y1, L_subframesize, ctx->zero, (Word16)0);

    gain_pit = G_Pitch(xn, y1, L_subframesize);

	/* xn2[i] = xn[i] - y1[i]*gain_pit */

    for (i = 0; i < L_subframesize; i++)
    {
      L_temp = L_mult(y1[i], gain_pit);
      L_temp = L_shl(L_temp, (Word16)3);	/* gain_pit in Q12 */
      L_temp = L_sub( Load_sh16(xn[i]), L_temp);
      xn2[i] = extract_h(L_temp);
    }


   /*----------------------------------------------------------------*
    * -Compute impulse response F[] and h2[] for innovation codebook *
    * -Find correlations of h2[];  ctx->rr[i][j] = sum h2[n-i]*h2[n-j]    *
    *----------------------------------------------------------------*/

    for (i = 0; i <= LPCORD; i++) ctx->ai_zero[i] = Ap3[i];
    Syn_Filt(Ap4, ctx->ai_zero, F, L_subframesize, ctx->zero, (Word16)0);

    /* Introduce pitch contribution with fixe gain of 0.8 to F[] */

    for (i = T0; i < L_subframesize; i++)
    {
      temp = mult(F[i-T0], (Word16)26216);
      F[i] = add(F[i], temp);
    }

    /* Compute h2[]; -> F[] filtered by 1/Ap4(z) */

    Syn_Filt(Ap4, F, h2, L_subframesize, ctx->zero, (Word16)0);

    Cal_Rr2(h2, (Word16*)ctx->rr);

   /*-----------------------------------------------------------------*
    * - Backward filtering of target vector (find dn[] from xn2[])    *
    * - Innovative codebook search (find index and gain)              *
    *-----------------------------------------------------------------*/

    Back_Fil(xn2, h2, dn, L_subframesize);	/* backward filtered target vector dn */

    *ana++ =D4i60_16(dn,F,h2, ctx->rr, code, y2, &sign_code, &shift_code);
    *ana++ = sign_code;
    *ana++ = shift_code;
    gain_code = G_Code(xn2, y2, L_subframesize);

   /*-----------------------------------------------------------------*
    * - Quantization of gains.                                        *
    *-----------------------------------------------------------------*/

    *ana++ = Ener_Qua(ctx, Aq,&ctx->exc[i_subfr],code, L_subframesize, &gain_pit, &gain_code);

   /*-------------------------------------------------------*
    * - Find the total excitation                           *
    * - Update filter memory ctx->mem_w0 for finding the target  *
    *   vector in the next subframe.                        *
    *   The filter state ctx->mem_w0[] is found by filtering by  *
    *   1/Ap4(z) the error between res[i] and ctx->exc[i]        *
    *-------------------------------------------------------*/

    for (i = 0; i < L_subframesize;  i++)
    {
      /* ctx->exc[i] = gain_pit*ctx->exc[i] + gain_code*code[i]; */
      /* ctx->exc[i]  in Q0   gain_pit in Q12               */
      /* code[i] in Q12  gain_cod in Q0                */
      L_temp = L_mult0(ctx->exc[i+i_subfr], gain_pit);
      L_temp = L_mac0(L_temp, code[i], gain_code);
      ctx->exc[i+i_subfr] = L_shr_r(L_temp, (Word16)12);
    }

    for(i=0; i<L_subframesize; i++)
      res[i] = sub(res[i], ctx->exc[i_subfr+i]);

    Syn_Filt(Ap4, res, code, L_subframesize, ctx->mem_w0, (Word16)1);

   /* Note: we use vector code[] as output only as temporary vector */

   /*-------------------------------------------------------*
    * - find synthesis speech corresponding to ctx->exc[]   *
    *   This filter is to help debug only.                  *
    *-------------------------------------------------------*/

    if (synth) Syn_Filt(Aq, &ctx->exc[i_subfr], &synth[i_subfr], L_subframesize, ctx->mem_syn,
			(Word16)1);

    Aq += LPCORDP1;
  }

 /*--------------------------------------------------*
  * Update signal for next frame.                    *
  * -> shift to the left by L_framesize:                 *
  *     speech[], ctx->wsp[] and  ctx->exc[]                   *
  *--------------------------------------------------*/

  for(i=0; i< L_total-L_framesize; i++)
    ctx->old_speech[i] = ctx->old_speech[i+L_framesize];

  for(i=0; i<pit_max; i++)
    ctx->old_wsp[i] = ctx->old_wsp[i+L_framesize];

  for(i=0; i<pit_max+L_inter; i++)
    ctx->old_exc[i] = ctx->old_exc[i+L_framesize];

  return;
}

