/************************************************************************
*
*	FILENAME		:	sdec_tet.c
*
*	DESCRIPTION		:	Main routines for speech source decoding
*
************************************************************************
*
*	SUB-ROUTINES	:	- Init_Decod_Tetra()
*					- Decod_Tetra()
*
************************************************************************
*
*	INCLUDED FILES	:	source.h
*
************************************************************************/

#include "source.h"


/**************************************************************************
*
*	ROUTINE				:	Init_Decod_Tetra
*
*	DESCRIPTION			:	Initialization of variables for the speech decoder
*
**************************************************************************
*
*	USAGE				:	Init_Decod_Tetra()
*
*	INPUT ARGUMENT(S)		:	None
*
*	OUTPUT ARGUMENT(S)		:	None
*
*	RETURNED VALUE		:	None
*
**************************************************************************/

void Init_Decod_Tetra(Decoder_Tetra_Context * dc)
{
  Word16 i;

  dc->old_T0 = 60;
  for(i=0; i<23; i++)
     dc->old_parm[i] = 0;

  /* Initialize static pointer */

  dc->exc    = dc->old_exc + pit_max + L_inter;

  /* Initialize global variables */

  dc->last_ener_cod = 0;
  dc->last_ener_pit = 0;

  /* Static vectors to zero */

  for(i=0; i<pit_max + L_inter; i++)
    dc->old_exc[i] = 0;

  for(i=0; i<LPCORD; i++)
    dc->mem_syn[i] = 0;


  /* Initialisation of lsp values for first */
  /* frame lsp interpolation */

  for(i=0; i<LPCORD; i++)
    dc->lspold[i] = lspold_init[i];


  /* Compute LPC spectral expansion factors */

  Fac_Pond(gamma3, dc->F_gamma3);
  Fac_Pond(gamma4, dc->F_gamma4);

 return;
}


/**************************************************************************
*
*	ROUTINE				:	Decod_Tetra
*
*	DESCRIPTION			:	Main speech decoder function
*
**************************************************************************
*
*	USAGE				:	Decod_Tetra(parm,synth)
*							(Routine_Name(input1,output1))
*
*	INPUT ARGUMENT(S)		:	
*
*		INPUT1			:	- Description : Synthesis parameters
*							- Format : 24 * 16 bit-samples
*
*	OUTPUT ARGUMENT(S)		:	
*
*		OUTPUT1			:	- Description : Synthesis
*							- Format : 240 * 16 bit-samples
*
*	RETURNED VALUE		:	None
*
**************************************************************************/

void Decod_Tetra(Decoder_Tetra_Context * dc, Word16 parm[], Word16 synth[])
{
  /* LPC coefficients */

  Word16 A_t[(LPCORDP1)*4];		/* A(z) unquantized for the 4 subframes */
  Word16 Ap3[LPCORDP1];		/* A(z) with spectral expansion         */
  Word16 Ap4[LPCORDP1];		/* A(z) with spectral expansion         */
  Word16 *A;			/* Pointer on A_t                       */

  /* Other vectors */

  Word16 zero_F[L_subframesize+64],  *F;
  Word16 code[L_subframesize+4];

  /* Scalars */

  Word16 i, i_subfr;
  Word16 T0, T0_min, T0_max, T0_frac;
  Word16 gain_pit, gain_code, index;
  Word16 sign_code, shift_code;
  Word16 bfi, temp;
  Word32 L_temp;

  /* Initialization of F */

  F  = &zero_F[64];
  for(i=0; i<64; i++)
   zero_F[i] = 0;

  /* Test bfi */

  bfi = *parm++;

  if(bfi == 0)
  {
    D_Lsp334(&parm[0], dc->lspnew, dc->lspold);	/* lsp decoding   */

    for(i=0; i< PARAMETERS_SIZE; i++)		/* keep parm[] as dc->old_parm */
      dc->old_parm[i] = parm[i];
  }
  else
  {
    for(i=1; i<LPCORD; i++)
      dc->lspnew[i] = dc->lspold[i];

    for(i=0; i< PARAMETERS_SIZE; i++)		/* use old parm[] */
      parm[i] = dc->old_parm[i];
  }

  parm += 3;			/* Advance synthesis parameters pointer */

  /* Interpolation of LPC for the 4 subframes */

  Int_Lpc4(dc->lspold,   dc->lspnew,   A_t);

  /* update the LSPs for the next frame */

  for(i=0; i<LPCORD; i++)
    dc->lspold[i]   = dc->lspnew[i];

/*------------------------------------------------------------------------*
 *          Loop for every subframe in the analysis frame                 *
 *------------------------------------------------------------------------*
 * The subframe size is L_subframesize and the loop is repeated L_framesize/L_subframesize  *
 *  times                                                                 *
 *     - decode the pitch delay                                           *
 *     - decode algebraic code                                            *
 *     - decode pitch and codebook gains                                  *
 *     - find the excitation and compute synthesis speech                 *
 *------------------------------------------------------------------------*/

  A = A_t;				/* pointer to interpolated LPC parameters */

  for (i_subfr = 0; i_subfr < L_framesize; i_subfr += L_subframesize)
  {

    index = *parm++;				/* pitch index */

    if (i_subfr == 0)				/* if first subframe */
    {
      if (bfi == 0)
      {						/* if bfi == 0 decode pitch */
         if (index < 197)
         {
           /* T0 = (index+2)/3 + 19; T0_frac = index - T0*3 + 58; */

           i = add(index, (Word16)2);
           i = mult(i, (Word16)10923);	/* 10923 = 1/3 in Q15 */
           T0 = add(i, (Word16)19);

           i = add(T0, add(T0, T0) );	/* T0*3 */
           i = sub((Word16)58, (Word16)i);
           T0_frac = add(index, (Word16)i);
         }
         else
         {
           T0 = sub(index, (Word16)112);
           T0_frac = 0;
         }
      }
      else   /* bfi == 1 */
      {
        T0 = dc->old_T0;
        T0_frac = 0;
      }


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

    else  /* other subframes */

    {
      if (bfi == 0)				/* if bfi == 0 decode pitch */
      {
         /* T0 = (index+2)/3 - 1 + T0_min; */

         i = add(index, (Word16)2);
         i = mult(i, (Word16)10923);	/* 10923 = 1/3 in Q15 */
         i = sub(i, (Word16)1);
         T0 = add(T0_min, i);

         /* T0_frac = index - 2 - i*3; */

         i = add(i, add(i,i) );		/* i*3 */
         T0_frac = sub( index , add(i, (Word16)2) );
      }
    }

   /*-------------------------------------------------*
    * - Find the adaptive codebook vector.            *
    *-------------------------------------------------*/

    Pred_Lt(&dc->exc[i_subfr], T0, T0_frac, L_subframesize);

   /*-----------------------------------------------------*
    * - Compute noise filter F[].                         *
    * - Decode codebook sign and index.                   *
    * - Find the algebraic codeword.                      *
    *-----------------------------------------------------*/

    Pond_Ai(A, dc->F_gamma3, Ap3);
    Pond_Ai(A, dc->F_gamma4, Ap4);

    for (i = 0;   i <= LPCORD;      i++) F[i] = Ap3[i];
    for (i = LPCORDP1; i < L_subframesize; i++) F[i] = 0;

    Syn_Filt(Ap4, F, F, L_subframesize, &F[LPCORDP1], (Word16)0);

    /* Introduce pitch contribution with fixed gain of 0.8 to F[] */

    for (i = T0; i < L_subframesize; i++)
    {
      temp = mult(F[i-T0], (Word16)26216);
      F[i] = add(F[i], temp);
    }

    index = *parm++;
    sign_code  = *parm++;
    shift_code = *parm++;

    D_D4i60(index, sign_code, shift_code, F, code);


   /*-------------------------------------------------*
    * - Decode pitch and codebook gains.              *
    *-------------------------------------------------*/

    index = *parm++;        /* index of energy VQ */

    Dec_Ener(dc,index,bfi,A,&dc->exc[i_subfr],code, L_subframesize, &gain_pit, &gain_code);

   /*-------------------------------------------------------*
    * - Find the total excitation.                          *
    * - Find synthesis speech corresponding to dc->exc[].       *
    *-------------------------------------------------------*/

    for (i = 0; i < L_subframesize;  i++)
    {
      /* dc->exc[i] = gain_pit*dc->exc[i] + gain_code*code[i]; */
      /* dc->exc[i]  in Q0   gain_pit in Q12               */
      /* code[i] in Q12  gain_cod in Q0                */

      L_temp = L_mult0(dc->exc[i+i_subfr], gain_pit);
      L_temp = L_mac0(L_temp, code[i], gain_code);
      dc->exc[i+i_subfr] = L_shr_r(L_temp, (Word16)12);
    }

    Syn_Filt(A, &dc->exc[i_subfr], &synth[i_subfr], L_subframesize, dc->mem_syn, (Word16)1);

    A  += LPCORDP1;    /* interpolated LPC parameters for next subframe */
  }

 /*--------------------------------------------------*
  * Update signal for next frame.                    *
  * -> shift to the left by L_framesize  dc->exc[]           *
  *--------------------------------------------------*/

  for(i=0; i<pit_max+L_inter; i++)
    dc->old_exc[i] = dc->old_exc[i+L_framesize];

  dc->old_T0 = T0;

  return;
}

