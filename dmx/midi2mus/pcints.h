/*===========================================================================
* PCINTS.H
*----------------------------------------------------------------------------
*                CONFIDENTIAL & PROPRIETARY INFORMATION 			     
*----------------------------------------------------------------------------
*             Copyright (C) 1993,94 Digital Expressions, Inc. 		     
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
* $Log:$
*---------------------------------------------------------------------------*/

#ifndef _PC_INTS_H
#define _PC_INTS_H

#define IRQ_0		0
#define IRQ_1		1
#define IRQ_2		2
#define IRQ_3		3
#define IRQ_4		4
#define IRQ_5		5
#define IRQ_6		6
#define IRQ_7		7
#define IRQ_8		8
#define IRQ_9		9
#define IRQ_10		10
#define IRQ_11		11
#define IRQ_12		12
#define IRQ_13		13
#define IRQ_14		14
#define IRQ_15		15

/*
* FLAGS
*/
#define	PCI_PRE_EOI			(1<<0)
#define PCI_POST_EOI		(1<<1)

/*
* RETURN CODES
*/
#define PCI_OKAY			0
#define PCI_CHAIN_ISR		1

/****************************************************************************
* pcInitIRQLib - Initialize PC Interrupt Library
*****************************************************************************/
int		// Returns: 0=Ready, -1=Failed
pcInitIRQLib(
	void
    );

/****************************************************************************
* pcDropIRQLib - Drop IRQ Library and remove any isr's 
*****************************************************************************/
void
pcDropIRQLib(
	void
    );

/****************************************************************************
* pcInstallISR - Install new ISR
*****************************************************************************/
PUBLIC VOID
pcInstallISR(
    int		irq, 				// INPUT : IRQ for Interrupt
    int 	(*Isr)(int irq),  	// INPUT : ISR
    int		flags				// INPUT : Flags
    );

/****************************************************************************
* pcRemoveISR - Remove installed ISR
*****************************************************************************/
PUBLIC VOID
pcRemoveISR(
	int		irq				// INPUT : IRQ int Remove 
	);


#endif

