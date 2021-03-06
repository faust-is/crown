/* ======================================================================== */
/*  TEXAS INSTRUMENTS, INC.                                                 */
/*                                                                          */
/*  DSPLIB  DSP Signal Processing Library                                   */
/*                                                                          */
/*      Release:        Revision 1.04b                                      */
/*      CVS Revision:   1.2     Sun Sep 29 03:32:18 2002 (UTC)              */
/*      Snapshot date:  23-Oct-2003                                         */
/*                                                                          */
/*  This library contains proprietary intellectual property of Texas        */
/*  Instruments, Inc.  The library and its source code are protected by     */
/*  various copyrights, and portions may also be protected by patents or    */
/*  other legal protections.                                                */
/*                                                                          */
/*  This software is licensed for use with Texas Instruments TMS320         */
/*  family DSPs.  This license was provided to you prior to installing      */
/*  the software.  You may review this license by consulting the file       */
/*  TI_license.PDF which accompanies the files in this library.             */
/* ------------------------------------------------------------------------ */
/*          Copyright (C) 2003 Texas Instruments, Incorporated.             */
/*                          All Rights Reserved.                            */
/* ======================================================================== */
/* ======================================================================== */
/*  Assembler compatibility shim for assembling 4.30 and later code on      */
/*  tools prior to 4.30.                                                    */
/* ======================================================================== */
/* ======================================================================== */
/*  End of assembler compatibility shim.                                    */
/* ======================================================================== */
/* ======================================================================== */
/*  NAME                                                                    */
/*     DSP_blk_move -- Move block of memory (overlapping). Endian Neutral   */
/*                                                                          */
/*  REVISION DATE                                                           */
/*      19-Jul-2001                                                         */
/*                                                                          */
/*  USAGE                                                                   */
/*      This routine is C-callable and can be called as:                    */
/*                                                                          */
/*      void DSP_blk_move(short *x, short *r, int nx);                      */
/*                                                                          */
/*          x  --- block of data to be moved                                */
/*          r  --- destination of block of data                             */
/*          nx --- number of elements in block                              */
/*                                                                          */
/*  DESCRIPTION                                                             */
/*      Move nx 16-bit elements from one memory location to another.        */
/*      Source and destination may overlap.                                 */
/*                                                                          */
/*      void DSP_blk_move(short *x, short *r, int nx);                      */
/*      {                                                                   */
/*          int i;                                                          */
/*                                                                          */
/*          if (r < x)                                                      */
/*          {                                                               */
/*              for (i = 0 ; i < nx; i++)                                   */
/*                  r[i] = x[i];                                            */
/*          } else                                                          */
/*          {                                                               */
/*              for (i = nx-1 ; i >= 0; i--)                                */
/*                  r[i] = x[i];                                            */
/*          }                                                               */
/*      }                                                                   */
/*                                                                          */
/*  ASSUMPTIONS                                                             */
/*      nx greater than or equal to 32                                      */
/*      nx a multiple of 8                                                  */
/*                                                                          */
/*  TECHNIQUES                                                              */
/*      Twin input and output pointers are used.                            */
/*                                                                          */
/*  INTERRUPT NOTE                                                          */
/*      This code is interrupt tolerant but not interruptible.              */
/*                                                                          */
/*  CYCLES                                                                  */
/*      nx/4 + 18                                                           */
/*                                                                          */
/*      nx = 1024, cycles = 274.                                            */
/*                                                                          */
/*  CODESIZE                                                                */
/*      112 bytes                                                           */
/* ------------------------------------------------------------------------ */
/*            Copyright (c) 2003 Texas Instruments, Incorporated.           */
/*                           All Rights Reserved.                           */
/* ======================================================================== */
#ifndef DSP_BLK_MOVE_H_
#define DSP_BLK_MOVE_H_ 1

void _DSP_blk_move(short *x, short *r, int nx);

#endif
/* ======================================================================== */
/*  End of file:  dsp_blk_move.h                                            */
/* ------------------------------------------------------------------------ */
/*            Copyright (c) 2003 Texas Instruments, Incorporated.           */
/*                           All Rights Reserved.                           */
/* ======================================================================== */
