;* ======================================================================== *;
;*  TEXAS INSTRUMENTS, INC.                                                 *;
;*                                                                          *;
;*  DSPLIB  DSP Signal Processing Library                                   *;
;*                                                                          *;
;*      Release:        Revision 1.04b                                      *;
;*      CVS Revision:   1.2     Sun Sep 29 03:32:18 2002 (UTC)              *;
;*      Snapshot date:  23-Oct-2003                                         *;
;*                                                                          *;
;*  This library contains proprietary intellectual property of Texas        *;
;*  Instruments, Inc.  The library and its source code are protected by     *;
;*  various copyrights, and portions may also be protected by patents or    *;
;*  other legal protections.                                                *;
;*                                                                          *;
;*  This software is licensed for use with Texas Instruments TMS320         *;
;*  family DSPs.  This license was provided to you prior to installing      *;
;*  the software.  You may review this license by consulting the file       *;
;*  TI_license.PDF which accompanies the files in this library.             *;
;* ------------------------------------------------------------------------ *;
;*          Copyright (C) 2003 Texas Instruments, Incorporated.             *;
;*                          All Rights Reserved.                            *;
;* ======================================================================== *;


;* ======================================================================== *;
;*  Assembler compatibility shim for assembling 4.30 and later code on      *;
;*  tools prior to 4.30.                                                    *;
;* ======================================================================== *;

        .if $isdefed(".ASSEMBLER_VERSION")
        .asg    .ASSEMBLER_VERSION, $asmver
        .else
        .asg    0,    $asmver
        .endif

        .if ($asmver < 430)

        .asg    B,    CALL     ; Function Call
        .asg    B,    RET      ; Return from a Function
        .asg    B,    CALLRET  ; Function call with Call / Ret chaining.

        .if .TMS320C6400
        .asg    BNOP, CALLNOP  ; C64x BNOP as a Fn. Call
        .asg    BNOP, RETNOP   ; C64x BNOP as a Fn. Return
        .asg    BNOP, CRNOP    ; C64x Fn call w/, Call/Ret chaining via BNOP.
        .endif

        .asg    , .asmfunc     ; .func equivalent for hand-assembly code
        .asg    , .endasmfunc  ; .endfunc equivalent for hand-assembly code

        .endif

;* ======================================================================== *;
;*  End of assembler compatibility shim.                                    *;
;* ======================================================================== *;


* ========================================================================= *
*   NAME                                                                    *
*      DSP_blk_move -- Move block of memory (overlapping). Endian Neutral   *
*                                                                           *
*   REVISION DATE                                                           *
*       19-Jul-2001                                                         *
*                                                                           *
*   USAGE                                                                   *
*       This routine is C-callable and can be called as:                    *
*                                                                           *
*       void DSP_blk_move(short *x, short *r, int nx);                      *
*                                                                           *
*           x  --- block of data to be moved                                *
*           r  --- destination of block of data                             *
*           nx --- number of elements in block                              *
*                                                                           *
*   DESCRIPTION                                                             *
*       Move nx 16-bit elements from one memory location to another.        *
*       Source and destination may overlap.                                 *
*                                                                           *
*       void DSP_blk_move(short *x, short *r, int nx);                      *
*       {                                                                   *
*           int i;                                                          *
*                                                                           *
*           if (r < x)                                                      *
*           {                                                               *
*               for (i = 0 ; i < nx; i++)                                   *
*                   r[i] = x[i];                                            *
*           } else                                                          *
*           {                                                               *
*               for (i = nx-1 ; i >= 0; i--)                                *
*                   r[i] = x[i];                                            *
*           }                                                               *
*       }                                                                   *
*                                                                           *
*   ASSUMPTIONS                                                             *
*       nx greater than or equal to 32                                      *
*       nx a multiple of 8                                                  *
*                                                                           *
*   TECHNIQUES                                                              *
*       Twin input and output pointers are used.                            *
*                                                                           *
*   INTERRUPT NOTE                                                          *
*       This code is interrupt tolerant but not interruptible.              *
*                                                                           *
*   CYCLES                                                                  *
*       nx/4 + 18                                                           *
*                                                                           *
*       nx = 1024, cycles = 274.                                            *
*                                                                           *
*   CODESIZE                                                                *
*       112 bytes                                                           *
* ------------------------------------------------------------------------- *
*             Copyright (c) 2003 Texas Instruments, Incorporated.           *
*                            All Rights Reserved.                           *
* ========================================================================= *


        .sect ".text:_blk_move_ov"
        .global _DSP_blk_move
_DSP_blk_move:
* ===================== SYMBOLIC REGISTER ASSIGNMENTS ===================== *
        .asg            A1,         A_i
        .asg            A4,         A_x
        .asg            A5,         A_r
        .asg            A6,         A_count
        .asg            A6,         A_in10
        .asg            A7,         A_in32
        .asg            A8,         A_fix
        .asg            A8,         A_dir
        .asg            B1,         B_rev
        .asg            B4,         B_r
        .asg            B5,         B_x
        .asg            B6,         B_in54
        .asg            B7,         B_in76
        .asg            B8,         B_dir
* ========================================================================= *
* =========================== PIPE LOOP PROLOG ============================ *
            SUB             A_count,    8,          A_fix
||          CMPGTU          B_r,        A_x,        B_rev

   [ B_rev] ADD             A_fix,      A_fix,      A_fix
|| [!B_rev] MVK             2,          A_dir
|| [!B_rev] MVK             2,          B_dir

   [ B_rev] ADD             A_fix,      A_x,        A_x
||          SHR             A_count,    3,          A_i

            ADD             A_x,        8,          B_x
   [ B_rev] MVK             -2,         A_dir
|| [ B_rev] MVK             -2,         B_dir
|| [ B_rev] ADD             A_fix,      B_r,        B_r

            BDEC    .S1     loop,       A_i                         ;[ 1,1]
||          LDDW    .D2T2   *B_x++[B_dir],          B_in76:B_in54   ;[ 1,1]
||          LDDW    .D1T1   *A_x++[A_dir],          A_in32:A_in10   ;[ 1,1]

            ADD             B_r,        8,          A_r

            BDEC    .S1     loop,       A_i                         ;[ 1,2]
||          LDDW    .D2T2   *B_x++[B_dir],          B_in76:B_in54   ;[ 1,2]
||          LDDW    .D1T1   *A_x++[A_dir],          A_in32:A_in10   ;[ 1,2]

            SUB             A_i,        3,          A_i
* =========================== PIPE LOOP KERNEL ============================ *
loop:
            BDEC    .S1     loop,       A_i                         ;[ 1,3]
||          LDDW    .D2T2   *B_x++[B_dir],          B_in76:B_in54   ;[ 1,3]
||          LDDW    .D1T1   *A_x++[A_dir],          A_in32:A_in10   ;[ 1,3]

            STDW    .D1T2   B_in76:B_in54,          *A_r++[A_dir]   ;[ 6,1]
||          STDW    .D2T1   A_in32:A_in10,          *B_r++[B_dir]   ;[ 6,1]

* =========================== PIPE LOOP EPILOG ============================ *
; 1 epilog stage collapsed
            RET             B3

            STDW    .D1T2   B_in76:B_in54,          *A_r            ;[ 6,3]
||          STDW    .D2T1   A_in32:A_in10,          *B_r            ;[ 6,3]

            NOP             4

* ========================================================================= *
*   End of file:  blk_move_h.asm                                            *
* ------------------------------------------------------------------------- *
*             Copyright (c) 2003 Texas Instruments, Incorporated.           *
*                            All Rights Reserved.                           *
* ========================================================================= *
