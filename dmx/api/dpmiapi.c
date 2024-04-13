/*===========================================================================
* DPMI.C Dos Protected Mode Interface
*----------------------------------------------------------------------------
*                CONFIDENTIAL & PROPRIETARY INFORMATION 			     
*----------------------------------------------------------------------------
*             Copyright (C) 1993 Digital Expressions, Inc. 		     
*                        All Rights Reserved.		   		     
*----------------------------------------------------------------------------
* "This software is furnished under a license and may be used only 	     
* in accordance with the terms of such license and with the inclusion 	     
* of the above copyright notice. This software or any other copies 	     
* thereof may not be provided or otherwise made available to any other 	     
* person. No title to and ownership of the software is hereby transfered."   
*----------------------------------------------------------------------------
*                     Written by Paul J. Radek
*----------------------------------------------------------------------------
* $Log:   F:/DMX/API/VCS/DPMIAPI.C_V  $
*  
*     Rev 1.1   02 Oct 1993 15:10:34   pjr
*  Updated header
*  
*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>

#include "dpmiapi.h"  

/***************************************************************************
_dpmi_getmemsize() - Returns size of largest block of memory
 ***************************************************************************/
INT
_dpmi_getmemsize (
VOID
)
{
	struct SREGS    segregs;
	union REGS      regs;
	INT             meminfo[32];
	INT             heap;
	INT             i;

	memset ( meminfo, 0, sizeof ( meminfo ) );
	segread(&segregs);
	segregs.es = segregs.ds;
	regs.w.ax = 0x500;      // get memory info
	regs.x.edi = (int)&meminfo;
	int386x( 0x31, &regs, &regs, &segregs );

	heap = meminfo[0];

	return ( heap );
}
  
/***************************************************************************
_dpmi_lockregion () - locks area of memory
 ***************************************************************************/
INT                       // RETURN: TRUE = Failed, FALSE = OK
_dpmi_lockregion (
VOID * inmem,              // INPUT : pointer to start of lock
INT length                 // INPUT : length of lock
)
{
   union REGS      r;
  
   r.w.ax = 0x600;
   r.w.bx = ( INT ) inmem >> 16;
   r.w.cx = ( INT ) inmem & 0xffff;
  
   r.w.si = length >> 16;
   r.w.di = length & 0xffff;
  
   int386 ( 0x31, &r, &r );
  
   return ( ( INT )( r.w.cflag & INTR_CF ) );
}
  
/***************************************************************************
_dpmi_unlockregion () - unlocks area of memory
 ***************************************************************************/
INT                       // RETURN: TRUE = Failed, FALSE = OK
_dpmi_unlockregion (
VOID * inmem,              // INPUT : pointer to start of lock
INT length                 // INPUT : length of lock
)
{
   union REGS      r;
  
   r.w.ax = 0x601;
   r.w.bx = ( INT ) inmem >> 16;
   r.w.cx = ( INT ) inmem & 0xffff;
  
   r.w.si = length >> 16;
   r.w.di = length & 0xffff;
  
   int386 ( 0x31, &r, &r );
  
   return ( ( INT )( r.w.cflag & INTR_CF ) );
}
  
/***************************************************************************
_dpmi_dosalloc () - Allocs DOS memory
 ***************************************************************************/
INT                       // RETURN: TRUE = Failed, FALSE = OK
_dpmi_dosalloc (
INT length,                // INPUT : length in 16byte blocks
INT * rseg,                // OUTPUT: Real Mode segment
INT * psel                 // OUTPUT: base selector
)
{
   union REGS      r;
  
   r.w.ax = 0x100;
   r.w.bx = length;
  
   int386 ( 0x31, &r, &r );
  
   *rseg = r.w.ax;
   *psel = r.w.dx;
  
   return ( ( INT )( r.w.cflag & INTR_CF ) );
}
  
/***************************************************************************
_dpmi_dosfree () - de-Allocs DOS memory
 ***************************************************************************/
INT                       // RETURN: TRUE = Failed, FALSE = OK
_dpmi_dosfree (
INT * psel                 // INPUT: base selector
)
{
   union REGS      r;
  
   r.w.ax = 0x101;
   r.w.dx = (short)psel;
  
   int386 ( 0x31, &r, &r );
  
   return ( ( INT )( r.w.cflag & INTR_CF ) );
}


/***************************************************************************
* _dpmi_getvect - gets the cs:eip of the current protected-mode interrupt
*                 handler for a specifed interrupt number.
****************************************************************************/
void __interrupt __far *
_dpmi_getvect(
	int		intnum			// INPUT : Interrupt #
	)
{
   union REGS      r;
  
   r.w.ax = 0x204;
   r.h.bl = ( BYTE ) intnum;
  
   int386( 0x31, &r, &r );
  
   return ( MK_FP( r.w.cx, r.x.edx ) );
}

/***************************************************************************
* _dpmi_setvect - sets the address of the specifed protected-mode interrupt
*                 vector.
****************************************************************************/
void 
_dpmi_setvect(
	int		intnum,			// INPUT : Interrupt #
	void	(__interrupt __far *__handler)()
	)
{
   union REGS      r;
  
   r.w.ax = 0x205;
   r.h.bl = ( BYTE ) intnum;
   r.w.cx = FP_SEG( __handler );
   r.x.edx = FP_OFF( __handler );
  
   int386( 0x31, &r, &r );
}

