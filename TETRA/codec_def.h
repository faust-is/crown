#ifndef _CODEC_DEF_H
#define _CODEC_DEF_H

/*----------------------------------------------------------------------*
 *         Coder constant parameters.                                   *
 *                                                                      *
 *   L_window    : LPC analysis window size.                            *
 *   L_next      : Samples of next frame needed for autocor.            *
 *   L_framesize : Frame size.                                          *
 *   L_subframesize     : Sub-frame size.                                      *
 *   LPCORD           : LPC order.                                           *
 *   LPCORDP1         : LPC order+1                                          *
 *   L_total     : Total speech size.                                   *
 *   dim_rr      : Dimension of matrix rr[][].                          *
 *   pit_min     : Minimum pitch lag.                                   *
 *   pit_max     : Maximum pitch lag.                                   *
 *   L_inter     : Length of filter for interpolation                   *
 *----------------------------------------------------------------------*/

#define  L_window (Word16)256
#define  L_next   (Word16)40
#define  L_framesize  (Word16)240
#define  L_subframesize  (Word16)60
#define  LPCORD        (Word16)10
#define  LPCORDP1      (Word16)11
#define  L_total  (Word16)(L_framesize+L_next+LPCORD)
#define  dim_rr   (Word16)32
#define  pit_min  (Word16)20
#define  pit_max  (Word16)143
#define  L_inter  (Word16)15

/*--------------------------------------------------------*
 *   LPC bandwidth expansion factors.                     *
 *      In Q15 = 0.95, 0.60, 0.75, 0.85                   *
 *--------------------------------------------------------*/

#define gamma1  (Word16)31130
#define gamma2  (Word16)19661
#define gamma3  (Word16)24576
#define gamma4  (Word16)27853


/*--------------------------------------------------------*
 *       Decoder constants parameters.                    *
 *                                                        *
 *   L_framesize     : Frame size.                            *
 *   L_subframesize     : Sub-frame size.                        *
 *   LPCORD           : LPC order.                             *
 *   LPCORDP1         : LPC order+1                            *
 *   pit_min     : Minimum pitch lag.                     *
 *   pit_max     : Maximum pitch lag.                     *
 *   L_inter     : Length of filter for interpolation     *
 *   PARAMETERS_SIZE   : Lenght of vector parm[]                *
 *--------------------------------------------------------*/

#define  PARAMETERS_SIZE (Word16)23

/* Initial lsp values used after each time */
    /* a reset is executed */
static const Word16 lspold_init[LPCORD]={
              30000, 26000, 21000, 15000, 8000, 0,
		  -8000,-15000,-21000,-26000};

typedef struct _Coder_Tetra_Context {
          /* Speech vector */

    Word16 old_speech[L_total];
    Word16 *speech, *p_window;
    Word16 *new_speech;
     /* Weighted speech vector */

    Word16 old_wsp[L_framesize+pit_max];
    Word16 *wsp;

        /* Excitation vector */

    Word16 old_exc[L_framesize+pit_max+L_inter];
    Word16 *exc;

        /* All-zero vector */

    Word16 ai_zero[L_subframesize+LPCORDP1];
    Word16 *zero;

        /* Spectral expansion factors */

    Word16 F_gamma1[LPCORD];
    Word16 F_gamma2[LPCORD];
    Word16 F_gamma3[LPCORD];
    Word16 F_gamma4[LPCORD];

        /* Lsp (Line Spectral Pairs in the cosine domain) */

    Word16 lspold[LPCORD];
    Word16 lspnew[LPCORD];
    Word16 lspnew_q[LPCORD], lspold_q[LPCORD];

   /* Initial lsp values used after each time */
    /* a reset is executed */

        /* Filters memories */

    Word16 mem_syn[LPCORD], mem_w0[LPCORD], mem_w[LPCORD];

        /* Matrix rr[dim_rr][dim_rr] */

    Word16 rr[dim_rr][dim_rr];
    Word16 lsp_old[LPCORD];

    Word16 old_A[LPCORD+1];

       /* Energy */

    Word16 last_ener_cod;
    Word16 last_ener_pit;
} Coder_Tetra_Context;

typedef struct _Decoder_Tetra_Context {
      /* Excitation vector */

    Word16 old_exc[L_framesize+pit_max+L_inter];
    Word16 *exc;

        /* Spectral expansion factors */

    Word16 F_gamma3[LPCORD];
    Word16 F_gamma4[LPCORD];

        /* Lsp (Line spectral pairs in the cosine domain) */

    Word16 lspold[LPCORD];
    Word16 lspnew[LPCORD];


    Word16 mem_syn[LPCORD];

        /* Default parameters */

    Word16 old_parm[PARAMETERS_SIZE], old_T0;

       /* Energy */

    Word16 last_ener_cod;
    Word16 last_ener_pit;

} Decoder_Tetra_Context;


#endif
