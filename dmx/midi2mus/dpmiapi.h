/*===========================================================================
* DPMIAPI.H
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
* $Log:   F:/DMX/INC/VCS/DPMIAPI.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:21:04   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _TYPES_H
#include <types.h>
#endif

#ifndef _DPMI_API
#define _DPMI_API

/***************************************************************************
_dpmi_getmemsize() - Returns size of largest block of memory
 ***************************************************************************/
INT
_dpmi_getmemsize (
VOID
);

/***************************************************************************
_dpmi_lockregion () - locks area of memory
 ***************************************************************************/
INT                       // RETURN: TRUE = Failed, FALSE = OK
_dpmi_lockregion (
VOID * inmem,              // INPUT : pointer to start of lock
INT length                 // INPUT : length of lock
);

/***************************************************************************
_dpmi_unlockregion () - unlocks area of memory
 ***************************************************************************/
INT                       // RETURN: TRUE = Failed, FALSE = OK
_dpmi_unlockregion (
VOID * inmem,              // INPUT : pointer to start of lock
INT length                 // INPUT : length of lock
);

/***************************************************************************
_dpmi_dosalloc () - Allocs DOS memory
 ***************************************************************************/
INT                       // RETURN: TRUE = Failed, FALSE = OK
_dpmi_dosalloc (
INT length,                // INPUT : length in 16byte blocks
INT * rseg,                // OUTPUT: Real Mode segment
INT * psel                 // OUTPUT: base selector
);

/***************************************************************************
_dpmi_dosfree () - de-Allocs DOS memory
 ***************************************************************************/
INT                       // RETURN: TRUE = Failed, FALSE = OK
_dpmi_dosfree (
INT * psel                 // INPUT: base selector
);

/***************************************************************************
* _dpmi_getvect - gets the cs:eip of the current protected-mode interrupt
*                 handler for a specifed interrupt number.
****************************************************************************/
void __interrupt __far *
_dpmi_getvect(
	int		intnum			// INPUT : Interrupt #
	);

/***************************************************************************
* _dpmi_setvect - sets the address of the specifed protected-mode interrupt
*                 vector.
****************************************************************************/
void 
_dpmi_setvect(
	int		intnum,			// INPUT : Interrupt #
	void	(__interrupt __far *__handler)()
	);

#endif
