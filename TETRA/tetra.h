/************************************************************************
*
*	DESCRIPTION		:	FUNCTION PROTOTYPES for the TETRA
*					speech source coder and decoder
*
************************************************************************/

#ifndef TETRA_H
#define TETRA_H


/* Basic functions */
/************************************************************************
*
*	Function Name : add_sh
*
*	Purpose :
*
*		Add var1 with a left shift(0-15) to L_var2.
*		Control saturation and set overflow flag 
*
*	Complexity Weight : 1
*
*	Inputs :
*
*		L_var2
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var2 <= 0x7fff ffff.
*
*		var1
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0xffff 8000 <= var1 <= 0x0000 7fff.
*
*		shift
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0 <= shift <= 15.
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		L_var_out
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.
*
************************************************************************/
static inline Word32 add_sh(Word32 L_var, Word16 var1, Word16 shift) {
  return L_add(L_var, (int32_t) var1 << shift);
}

/************************************************************************
*
*	Function Name : add_sh16
*
*	Purpose :
*
*		Add var1 with a left shift of 16 to L_var2.
*		Control saturation and set overflow flag 
*
*	Complexity Weight : 1
*
*	Inputs :
*
*		L_var2
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var2 <= 0x7fff ffff.
*
*		var1
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0xffff 8000 <= var1 <= 0x0000 7fff.
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		L_var_out
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.
*
************************************************************************/
static inline Word32 add_sh16(Word32 L_var, Word16 var1) {
	return L_add(L_var, (int32_t) var1 << 16);
}

/************************************************************************
*
*	Function Name : Load_sh
*
*	Purpose :
*
*		Load the 16 bit var1 left shift(0-15) into the 32 bit output.
*		MS bits of the output are sign extended.
*
*	Complexity Weight : 1
*
*	Inputs :
*
*		var1
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0xffff 8000 <= var1 <= 0x0000 7fff.
*
*		shift
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0 <= var1 <= 15.
*
*	Outputs :
*
*		none.
*
*	Returned Value :
*
*		L_var_out
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.
*
************************************************************************/
static inline Word32 Load_sh(Word16 var1, Word16 shift) {
    return (int32_t) var1 << shift;
}

/************************************************************************
*
*	Function Name : Load_sh16
*
*	Purpose :
*
*		Load the 16 bit var1 with a left shift of 16 into the 32 bit output.
*
*	Complexity Weight : 1
*
*	Inputs :
*
*		var1
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0xffff 8000 <= var1 <= 0x0000 7fff.
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		L_var_out
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.
*
************************************************************************/
static inline Word32 Load_sh16(Word16 var1) {
    return (int32_t) var1 << 16;
}

/************************************************************************
*
*	Function Name : store_hi
*
*	Purpose :
*
*		Store high part of a L_var1 with a left shift of var2.
*
*	Complexity Weight : 3
*
*	Inputs :
*
*		L_var1
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var1 <= 0x7fff ffff.
*
*		var2
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0 <= var2 <= 7.
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		var_out
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0xffff 8000 <= var_out <= 0x0000 7fff.
*
************************************************************************/
static inline Word16 store_hi(Word32 L_var1, Word16 var2) {
    return extract_l(L_var1 >> (16 - var2));
}


/************************************************************************
*
*	Function Name : sub_sh
*
*	Purpose :
*
*		Subtract var1 with a left shift(0-15) to L_var2.
*		Control saturation and set overflow_flag.
*
*	Complexity Weight : 1
*
*	Inputs :
*
*		L_var2
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var2 <= 0x7fff ffff.
*
*		var1
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0xffff 8000 <= var1 <= 0x0000 7fff.
*
*		shift
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0 <= shift <= 15.
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		L_var_out
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.
*
************************************************************************/
static inline Word32 sub_sh(Word32 L_var, Word16 var1, Word16 shift) {
    return L_sub(L_var, (int32_t) var1 << shift);
}

/************************************************************************
*
*	Function Name : sub_sh16
*
*	Purpose :
*
*		Subtract var1 with a left shift of 16 to L_var2.
*		Control saturation and set overflow flag. 
*
*	Complexity Weight : 1
*
*	Inputs :
*
*		L_var2
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var2 <= 0x7fff ffff.
*
*		var1
*			16 bit short signed integer (Word16) whose value falls in the
*			range : 0xffff 8000 <= var1 <= 0x0000 7fff.
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		L_var_out
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.

************************************************************************/

static inline Word32 sub_sh16(Word32 L_var, Word16 var1) {
	return L_sub(L_var, (int32_t) var1 << 16);
}


/* Extended precision functions */

/************************************************************************
*
*	Function Name : L_comp
*
*	Purpose :
*
*		Compose from two 16 bit DPF a normal 32 bit integer.
*		L_32 = hi<<15 + lo
*
*	Complexity Weight : 2
*
*	Inputs :
*
*		hi
*			msb
*
*		lo
*			lsb (with sign)
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		L_32
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0xc000 0000 <= L_32 <= 0x3fff ffff.
*
************************************************************************/
static inline Word32 L_comp(Word16 hi, Word16 lo)
{
	return ((int32_t) hi << 15) + lo;
}

/************************************************************************
*
*	Function Name : L_extract
*
*	Purpose :
*
*		Extract from a 31 bit integer two 16 bit DPF.
*
*	Complexity Weight : 5
*
*	Inputs :
*
*		L_32
*			32 bit long signed integer (Word32) with b30 == b31
*			whose value falls in the range : 0xc000 0000 <= L_32 <= 0x3fff ffff.
*
*	Outputs :
*
*		hi
*			b15 to b30 of L_32
*
*		lo
*			L_32 - hi<<15
*
*	Returned Value :
*
*		none
*
************************************************************************/
static inline void L_extract(Word32 L_32, Word16 *hi, Word16 *lo)
{
  *hi  = extract_h( L_shl( L_32,(Word16)1 ) );
  *lo  = extract_l( L_sub( L_32, ((int32_t) (*hi) << 15 )));
}


/************************************************************************
*
*	Function Name : mpy_32
*
*	Purpose :
*
*		Multiply two 32 bit integers (DPF). The result is divided by 2**32
*		L_32 = hi1*hi2 + (hi1*lo2)>>15 + (lo1*hi2)>>15)
*
*	Complexity Weight : 7
*
*	Inputs :
*
*		hi1
*			hi part of first number
*
*		lo1
*			lo part of first number
*
*		hi2
*			hi part of second number
*
*		lo2
*			lo part of second number
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		L_var_out
*			32 bit long signed integer (Word32) whose value falls in the
*			range : 0x0000 0000 <= L_var_out <= 0x7fff ffff.
*
*************************************************************************/
static inline Word32 mpy_32(Word16 hi1, Word16 lo1, Word16 hi2, Word16 lo2) 
{
	Word16 p1, p2;
	Word32 L_32;
	
	p1   = extract_h(L_mult0(hi1, lo2));
	p2   = extract_h(L_mult0(lo1, hi2));
	L_32 = L_mult0(hi1, hi2);
	L_32 = add_sh( L_32, p1, (Word16)1 );
	return (add_sh( L_32, p2, (Word16)1 ));
}
/* 32 bit division (not inlined) */
Word32 div_32(Word32 L_num, Word16 hi, Word16 lo);

/* bitstream */
Word16 bin2int(Word16 no_of_bits, Word16 *bitstream);
void   int2bin(Word16 value, Word16 no_of_bits, Word16 *bitstream);

/* Mathematic functions  */

Word32 inv_sqrt(Word32 L_x);
void   Log2(Word32 L_x, Word16 *exponant, Word16 *fraction);
Word32 pow2(Word16 exponant, Word16 fraction);

/* General signal processing */

void   Autocorr(Word16 x[], Word16 p, Word16 r_h[], Word16 r_l[]);
void   Az_Lsp(Word16 a[], Word16 lsp[], Word16 old_lsp[]);
void   Back_Fil(Word16 x[], Word16 h[], Word16 y[], Word16 L);
Word16 Chebps(Word16 x, Word16 f[], Word16 n);
void   Convolve(Word16 x[], Word16 h[], Word16 y[], Word16 L);
void   Fac_Pond(Word16 gamma, Word16 fac[]);
void   Get_Lsp_Pol(Word16 *lsp, Word32 *f);
void   Int_Lpc4(Word16 lsp_old[], Word16 lsp_new[], Word16 a_4[]);
void   Lag_Window(Word16 p, Word16 r_h[], Word16 r_l[]);
void   Levin_32(Word16 Rh[], Word16 Rl[], Word16 A[], Word16 old_A[]);
Word32 Lpc_Gain(Word16 a[]);
void   Lsp_Az(Word16 lsp[], Word16 a[]);
void   Pond_Ai(Word16 a[], Word16 fac[], Word16 a_exp[]);
void   Residu(Word16 a[], Word16 x[], Word16 y[], Word16 lg);
void   Syn_Filt(Word16 a[], Word16 x[], Word16 y[], Word16 lg, Word16 mem[],
                Word16 update);

/* Specific coder functions */

void Init_Coder_Tetra(Coder_Tetra_Context* ctx);
void   Coder_Tetra(Coder_Tetra_Context* ctx, Word16 ana[], Word16 synth[]);
void   Cal_Rr2(Word16 h[], Word16 *rr);
void   Clsp_334(Word16 *lsp, Word16 *lsp_q, Word16 *indice, Word16* lsp_old);
Word16 D4i60_16(Word16 dn[], Word16 f[], Word16 h[], Word16 rr[][32],
                Word16 cod[], Word16 y[], Word16 *sign, Word16 *shift_code);
Word16 Ener_Qua(Coder_Tetra_Context* ctx, Word16 A[], Word16 prd_lt[], Word16 code[], Word16 L_subfr,
                Word16 *gain_pit, Word16 *gain_cod);
Word16 G_Code(Word16 xn2[], Word16 y2[], Word16 L_subfr);
Word16 G_Pitch(Word16 xn[], Word16 y1[], Word16 L_subfr);
void   Init_Pre_Process(Pre_Process_Context* pre_process);
Word16 Lag_Max(Word16 signal[], Word16 sig_dec[], Word16 L_frame,
               Word16 lag_max, Word16 lag_min, Word16 *cor_max);
Word16 Pitch_Fr(Word16 exc[], Word16 xn[], Word16 h[], Word16 L_subfr,
                Word16 t0_min, Word16 t0_max, Word16 i_subfr,
	    Word16 *pit_frac);
Word16 Pitch_Ol_Dec(Word16 signal[], Word16 L_frame);
void   Pred_Lt(Word16 exc[], Word16 T0, Word16 frac, Word16 L_subfr);
void   Pre_Process(Pre_Process_Context* pre_process, Word16 signal[], Word16 lg);
void   Prm2bits_Tetra(Word16 prm[], Word16 bits[]);

/* Specific decoder functions */

void   Init_Decod_Tetra(Decoder_Tetra_Context * dc);
void   Decod_Tetra(Decoder_Tetra_Context * dc, Word16 parm[], Word16 synth[]);
void   Bits2prm_Tetra(Word16 bits[], Word16 prm[]);
Word16 Dec_Ener(Decoder_Tetra_Context * dc, Word16 index, Word16 bfi, Word16 A[], Word16 prd_lt[],
        Word16 code[], Word16 L_subfr, Word16 *gain_pit, Word16 *gain_cod);
void   D_D4i60(Word16 index,Word16 sign,Word16 shift, Word16 F[], 
        Word16 cod[]);
void   D_Lsp334(Word16 indice[], Word16 lsp[], Word16 old_lsp[]);
void   Post_Process(Word16 signal[], Word16 lg);

#endif
