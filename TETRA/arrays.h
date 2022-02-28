/**************************************************************************
*
*	FILENAME				:	arrays.h
*
*	DESCRIPTION			:	Arrays for speech channel coding and decoding
*
**************************************************************************/
#ifndef __ARRAYS_H
#define __ARRAYS_H

/* ARRAYS FOR INITIALIZATION OF THE CHANNEL CODING */
/* Definition of the sensitivity classes : 3 classes */
const Word16	TAB0[N0] =	{35,36,37,38,39,40,41,42,43,  47,48,
						56,  61,62,63,64,65,66,67,68,69,70,
						74,75,  83,  88,89,90,91,92,93,94,95,
						96,97,  101,102,  110,115,116,117,118,
						119,120,121,122,123,124,  128,129,
						137};
/* Class 0
   51 terms : Non protected class
	      Least sensitive bits */

const Word16	TAB1[N1] =	{58,85,112,  54,81,108,135,  50,77,
						104,131,  45,72,99,126,  55,82,109,
						136,  5,13,34,  8,16,17,  22,23,24,
						25,26,  6,14,7,15,  60,87,114,  46,
						73,100,127,  44,71,98,125,  33,49,
						76,103,130,  59,86,113,  57,84,111};
/* Class 1
   56 terms : Protected by Rate 8/12 
	      Ranked with the most important bits at the boundaries,
	      and least important at the middle */

const Word16	TAB2[N2] =		{18,19,20,21,	/* LSF3 */
						31,32,		/* Pitch0 */
						53,80,107,134,	/* Energie 3 */
						1,2,3,4,		/* LSF1 */
						9,10,11,12,		/* LSF2 */
						27,28,29,30,	/* Pitch0 */
						52,79,106,133,	/* Energie 4 */
						51,78,105,132};/* Energie 5 */
/* Class 2
   30 terms : Protected by Rate 8/18
	      Ranked from least to most important bits */


/* PUNCTURING CHARACTERISTICS: */
/* Puncturing Tables for the two protected classes (Class 1 and 2) */

const Word16	A1[Period_pct*3]		=
						{1, 1, 1, 1, 1, 1, 1, 1,
						 1, 0, 1, 0, 1, 0, 1, 0,
						 0, 0, 0, 0, 0, 0, 0, 0};
/* Puncturing Table for Class 1 : Rate 8/12 (2/3) */

const Word16	A2[Period_pct*3]		=
						{1, 1, 1, 1, 1, 1, 1, 1,
						 1, 1, 1, 1, 1, 1, 1, 1,
						 1, 0, 0, 0, 1, 0, 0, 0};
/* Puncturing Table for Class 2 : Rate 8/18 */

const Word16	Fs_A2[Period_pct*3]	=
						{1, 1, 1, 1, 1, 1, 1, 1,
						 1, 1, 1, 1, 1, 1, 1, 1,
						 1, 0, 0, 0, 0, 0, 0, 0};
/* In case of Frame Stealing */
/* Puncturing Table for Class 2 : Rate 8/17 */


/* BAD FRAME INDICATOR : CRC (Cyclic Redundant Check) */
/* Definition of the 8 CRC bits */

const Word16	TAB_CRC1[SIZE_TAB_CRC1] =
						{1,  5,  8,9,  13,  15,16,
						17,  19,  21,22,  24,25,  31,
						32,  35,36,  38,  40,  43,44,
						45,  48,49,50,51,  53,54,  56};
/* Ranks of the bits in Class2 protected by the First bit of the CRC */

const Word16	TAB_CRC2[SIZE_TAB_CRC2] =
						{2,  6,  9,10,  14,
						16,17,18,  20,  22,23,  25,26,
						32,33,  36,37,  39,  41,  44,45,
						46,  49,50,51,52,  54,55,  57};
/* Ranks of the bits in Class 2 protected by the Second bit of the CRC */

const Word16	TAB_CRC3[SIZE_TAB_CRC3] =
						{3,  7,  10,11,  15,
						17,18,19,  21,  23,24,  26,27,
						33,34,  37,38,  40,  42,  45,
						46,47,  50,51,52,53,  55,56,  58};
/* Ranks of the bits in Class 2 protected by the Third bit of the CRC */

const Word16	TAB_CRC4[SIZE_TAB_CRC4] =
						{1,  4,5,  9,  11,12,13,  15,
						17,18,  20,21,  27,28,  31,
						32,  34,  36,  39,40,41,  44,45,
						46,47,  49,50,  52,  57,  59};
/* Ranks of the bits in Class 2 protected by the Fourth bit of the CRC */

const Word16	TAB_CRC5[SIZE_TAB_CRC5] =
						{2,  5,6,  10,  12,13,14,
						16,  18,19,  21,22,  28,29,
						32,33,  35,  37,  40,41,42,  45,
						46,47,48,  50,51,  53,  58,  60};
/* Ranks of the bits in Class 2 protected by the Fifth bit of the CRC */

const Word16	TAB_CRC6[SIZE_TAB_CRC6] =
						{3,  6,7,  11,  13,14,15,
						17,  19,20,  22,23,  29,30,
						33,34,  36,  38,  41,42,43,
						46,47,48,49,  51,52,  54,  59};
/* Ranks of the bits in Class 2 protected by the Sixth bit of the CRC */

const Word16	TAB_CRC7[SIZE_TAB_CRC7] =
						{4,  7,8,  12,  14,15,
						16,  18,  20,21,  23,24,  30,
						31,  34,35,  37,  39,  42,43,44,
						47,48,49,50,  52,53,  55,  60};
/* Ranks of the bits in Class 2 protected by the Seventh bit of the CRC */

const Word16	TAB_CRC8[SIZE_TAB_CRC8] =
						{1,2,3,4,  8,  13,14,  16,  19,
						20,  22,23,  25,26,27,28,29,30,
						32,33,34,  36,37,  40,41,42,  44,
						48,  50,  53,  56,57,58,59,60};
/* Ranks of the bits in Class 2 protected by the Eighth bit of the CRC */



/* IN CASE OF FRAME STEALING :
   BAD FRAME INDICATOR : CRC (Cyclic Redundant Check)
   Definition of the 4 CRC bits */

#define	Fs_SIZE_TAB_CRC1	16
#define	Fs_SIZE_TAB_CRC2	16
#define	Fs_SIZE_TAB_CRC3	16
#define	Fs_SIZE_TAB_CRC4	16

const Word16	Fs_TAB_CRC1[Fs_SIZE_TAB_CRC1] =
						{1,  4,5,  7,  9,10,11,12,
						16,  19,20,  22,  24,25,26,27  };
/* Ranks of the bits in Class2 protected by the First bit of the CRC */

const Word16	Fs_TAB_CRC2[Fs_SIZE_TAB_CRC2] =
						{1,2,  4,  6,7,8,9,  13,
						16,17,  19,  21,22,23,24,  28  };
/* Ranks of the bits in Class 2 protected by the Second bit of the CRC */

const Word16	Fs_TAB_CRC3[Fs_SIZE_TAB_CRC3] =
						{  2,3,  5,  7,8,9,10,  14,
						  17,18,  20,  22,23,24,25,  29  };
/* Ranks of the bits in Class 2 protected by the Third bit of the CRC */

const Word16	Fs_TAB_CRC4[Fs_SIZE_TAB_CRC4] =
						{  3,4,  6,  8,9,10,11,  15,
						  18,19,  21,  23,24,25,26,  30};
/* Ranks of the bits in Class 2 protected by the Fourth bit of the CRC */


#define	N1_coded	84	/* Size of Class1 (one speech frame) after coding 8/12 */
#define	N2_coded	81	/* Size of Class2 (one speech frame) after coding 8/17 */

/* not used yet, reserved for channel coder  */
typedef struct _Chancoder_Arrays_Context {
	Word16	Previous[(1 << (K - 1))][2];
	Word16	Best_previous[(1 << (K - 1))][Decoding_delay];
	Word16	T1[(1 << (K - 1))][2], T2[(1 << (K - 1))][2], T3[(1 << (K - 1))][2];
	Word16	Score[(1 << (K - 1))];
	Word16	Ex_score[(1 << (K - 1))];
	Word16	Received[3];

	Word16	Initialization;
	Word16	Nber_Info_Bits;
	Word16	Msb_bit;
	Word16	M_1;
	Word16	Min_value_allowed;
	Word16	Max_value_allowed;
} Chancoder_Arrays_Context;

#endif

/* END GLOBAL VARIABLES ***********************************/

