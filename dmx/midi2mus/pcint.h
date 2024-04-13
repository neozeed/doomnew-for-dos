/*===========================================================================
* PCINT.H
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
* $Log:   F:/DMX/INC/VCS/PCINT.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:19:40   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#ifndef _PC_INT_H
#define _PC_INT_H

#define PC_ChainToPrevISR(a)	_chain_intr( (a).oldvect )

#define PC_DISABLE		0x80

typedef struct
{
	VOID 	( INTERRUPT * oldvect)();
	VOID    ( INTERRUPT * Isr )();
	WORD	wRmSel;
	WORD	wRmOfs;
	BYTE	bIRQ;
	BYTE	bPIC1_IMR;
	BYTE	bPIC2_IMR;
	BYTE	bForced;
} PC_INT;


/****************************************************************************
* PC_NewISR - Install new ISR
*****************************************************************************/
PUBLIC VOID
PC_NewISR(
	PC_INT	*pcint,				// INPUT : PC_INT Handle
    int		irq, 				// INPUT : IRQ for Interrupt
    void 	(INTERRUPT *Isr)()  // INPUT : ISR
    );

/****************************************************************************
* PC_DelISR - Delete installed ISR
*****************************************************************************/
PUBLIC VOID
PC_DelISR(
	PC_INT	*pcint			// INPUT : PC_INT Handle
	);

/****************************************************************************
* PC_SendEOI - Send End Of Interrupt to Controller
*****************************************************************************/
PUBLIC void
PC_SendEOI(
	PC_INT	*pcint			// INPUT : PC_INT Handle
	);


#endif
