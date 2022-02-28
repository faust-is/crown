/************************************************************************
*
*	FILENAME		:	fbas_tet.c
*
*	DESCRIPTION		:	Library of basic functions used in the TETRA speech
*					codec, other than accepted operators
*
************************************************************************
*
*	FUNCTIONS		:	- bin2int()
*					- int2bin()
*
************************************************************************
*
*	INCLUDED FILES	:	source.h
*
************************************************************************/

#include "source.h"



/************************************************************************
*
*	Function Name : bin2int
*
*	Purpose :
*
*		Read "no_of_bits" bits from the array bitstream[] and convert to integer 
*
*	Inputs :
*
*		no_of_bits
*			16 bit 
*
*		*bitstream
*			16 bit 
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		value
*			32 bit 
*
************************************************************************/

#define BIT_1  1

Word16 bin2int(Word16 no_of_bits, Word16 *bitstream)
{
   Word16 value, i, bit;

   value = 0;
   for (i = 0; i < no_of_bits; i++)
   {
     value = shl( value,(Word16)1 );
     bit = *bitstream++;
     if (bit == BIT_1)  value += 1;
   }
   return(value);
}

/************************************************************************
*
*	Function Name : int2bin
*
*	Purpose :
*
*		Convert integer to binary and write the bits to the array bitstream [] 
*
*	Inputs :
*
*		no_of_bits
*			16 bit 
*
*		*bitstream
*			16 bit 
*
*	Outputs :
*
*		none
*
*	Returned Value :
*
*		value
*			32 bit 
*
************************************************************************/

#define BIT_0     0
#define BIT_1     1
#define MASK      1

void int2bin(Word16 value, Word16 no_of_bits, Word16 *bitstream)
{
   Word16 *pt_bitstream, i, bit;

   pt_bitstream = bitstream + no_of_bits;

   for (i = 0; i < no_of_bits; i++)
   {
     bit = value & MASK;
     if (bit == 0)
         *--pt_bitstream = BIT_0;
     else
         *--pt_bitstream = BIT_1;
     value = shr( value,(Word16)1 );
   }
}



