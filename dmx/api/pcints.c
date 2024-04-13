/*===========================================================================
* PCINTS.C - PC Interrupt Interface
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
* $Log:$
*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>

#include "dmx.h"

/*---------------------------------------------------------------------------
* PIC ports and commands
*---------------------------------------------------------------------------*/
#define PORT_PIC1_wOCW2         0x20
#define PORT_PIC2_wOCW2         0xA0
#define PORT_PIC1_rwIMR         0x21
#define PORT_PIC2_rwIMR         0xA1

#define CMD_PIC_EOI             0x20
#define CMD_PIC_ID				0x0b

#define D32RealSeg(p)	((((DWORD)(p))>>4) & 0xFFFF)
#define D32RealOfs(p)	(((DWORD)(p)) & 0x000F)


extern void rmInt( void );
extern void rmEnd( void );
extern void rmFixup( void );

typedef struct
{
	void		( __interrupt __far *this_isr )();
	void		( __interrupt __far *prev_isr )();
	int			(*callback)( int irq );
	int			flags;
    unsigned	realmode_sel;
	unsigned	realmode_ofs;
	unsigned	PIC1_IMR;
	unsigned	PIC2_IMR;
} IRQ_SERVICE;

static IRQ_SERVICE	irqHandler[ 16 ];

#define STACKS		4
#define STACK_PAGE	4096
#define STACK_SIZE	(STACK_PAGE*STACKS)

static BYTE		*stack_base;
static BYTE		*stack_top;
static int		stacks;

extern int CallFuncWithNewStack( int (*func)( int ), int irq, void *new_stack_ptr );
#pragma aux CallFuncWithNewStack = \
        "sub    edx,16"         /* allocate space from new stack */\
        "mov    [edx],esp"      /* save current SS:ESP on new stack */\
        "mov    4[edx],ss"      \
        "mov    8[edx],edx"     /* set up new values for SS:ESP */\
        "mov    12[edx],ds"     \
        "lss    esp,8[edx]"     /* switch to new stack */\
        "call   ebx"            /* call user's function */\
        "lss    esp,[esp]"      /* switch back to original stack */\
        parm [ebx] [eax] [edx] modify [ECX ESI EDI ]

extern void pcIRQ( int irq );

static void __interrupt __far irq0() { pcIRQ( 0 ); }
static void __interrupt __far irq1() { pcIRQ( 1 ); }
static void __interrupt __far irq2() { pcIRQ( 2 ); }
static void __interrupt __far irq3() { pcIRQ( 3 ); }
static void __interrupt __far irq4() { pcIRQ( 4 ); }
static void __interrupt __far irq5() { pcIRQ( 5 ); }
static void __interrupt __far irq6() { pcIRQ( 6 ); }
static void __interrupt __far irq7() { pcIRQ( 7 ); }
static void __interrupt __far irq8() { pcIRQ( 8 ); }
static void __interrupt __far irq9() { pcIRQ( 9 ); }
static void __interrupt __far irq10() { pcIRQ( 10 ); }
static void __interrupt __far irq11() { pcIRQ( 11 ); }
static void __interrupt __far irq12() { pcIRQ( 12 ); }
static void __interrupt __far irq13() { pcIRQ( 13 ); }
static void __interrupt __far irq14() { pcIRQ( 14 ); }
static void __interrupt __far irq15() { pcIRQ( 15 ); }


static BYTE			pic1_mask;
static BYTE			pic2_mask;
static unsigned		rm_selector;
static BYTE 		*rm_handler;
static BYTE			*rmTrap = NULL;
static int			WatchId = -1;
static int			pcLibReady = 0;

void
pcIRQ(
	int		irq
	)
{
	BYTE	*stack_frame;
	int		rc;
	BYTE	bPrevColor;

	_disable();

#ifdef __386__
	set_es();
#endif
#ifndef PROD
	if ( DmxDebug & DMXBUG_PCINT )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 3 );
	}
#endif
	if ( irqHandler[ irq ].callback )
	{
        if ( irq == 7 )
        {
        /*
        * check of phantom interrupt
        */
			outp( PORT_PIC1_wOCW2, CMD_PIC_ID );
            if ( ( inp( PORT_PIC1_wOCW2 ) & 0x80 ) == 0 )
            {
	        	outp( PORT_PIC1_wOCW2, CMD_PIC_EOI );
                return;
            }
        }
#ifndef PROD
		{
		char 	*str = "CallNewFunc[-] IRQ-0";
		str[ 12 ] = '0' + STACKS - stacks;
		str[ 19 ] = '0' + irq;
		DmxTrace( str );
		}
#endif
		--stacks;
		stack_frame = stack_top;
		stack_top -= STACK_PAGE;

		if ( stacks > 0
		     && ( irqHandler[ irq ].flags & PCI_PRE_EOI ) != 0 )
		{
			if ( irq > 7 )
	        	outp( PORT_PIC2_wOCW2, CMD_PIC_EOI );
        	outp( PORT_PIC1_wOCW2, CMD_PIC_EOI );
		}
		_enable();
		if ( stacks >= 0 )
			rc = CallFuncWithNewStack( irqHandler[ irq ].callback, irq, stack_frame );
		else
			rc = irqHandler[ irq ].callback( irq );
#ifndef PROD
		DmxTrace( "BackFrom irq" );
#endif
		_disable();
		
		stack_top += STACK_PAGE;
		++stacks;

		if ( rc == PCI_CHAIN_ISR )
		{
			irqHandler[ irq ].prev_isr();
		}
        else if ( stacks == 0
				|| ( irqHandler[ irq ].flags & PCI_POST_EOI ) != 0 )
		{
			if ( irq > 7 )
	        	outp( PORT_PIC2_wOCW2, CMD_PIC_EOI );
        	outp( PORT_PIC1_wOCW2, CMD_PIC_EOI );
		}
	}
	else
	{
		if ( irq > 7 )
	        outp( PORT_PIC2_wOCW2, CMD_PIC_EOI );
        outp( PORT_PIC1_wOCW2, CMD_PIC_EOI );
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_PCINT )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
}

/****************************************************************************
* PC_WatchDog - Watches for missed interrupts and will trigger software
*				interrupt.
*****************************************************************************/
static int
PC_WatchDog(
	void
	)
{
	WORD	mask;
	BYTE	bPrevColor;
	int		irq;

#ifndef PROD
	if ( DmxDebug & DMXBUG_PCINT )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 3 );
	}
#endif
	mask = 0;
	if ( rmTrap && *rmTrap != 0 )
	{
		mask = *rmTrap;
		*rmTrap = 0;
	}

	outp( 0xA0, 0x0B );
	outp( 0x20, 0x0B );
	mask |= inp( 0xA0 );
	mask <<= 8;
	mask |= inp( 0x20 );

	if ( mask != 0 )
	{
		for ( irq = 1; irq < 16; irq++ )
		{
			if ( ( mask & ( 1 << irq ) ) == 0 )
				continue;

			if ( irqHandler[ irq ].prev_isr )
			{
            	irqHandler[ irq ].callback( irq );
#ifndef PROD
				if ( DmxDebug & DMXBUG_PCINT )
				{
					inp( 0x3da );
					outp( 0x3c0, 0x31 );
					outp( 0x3c0, 3 );
				}
#endif
			}
		}
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_PCINT )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
	return TS_OKAY;
}


/****************************************************************************
* pcInitIRQLib - Initialize PC Interrupt Library
*****************************************************************************/
int		// Returns: 0=Ready, -1=Failed
pcInitIRQLib(
	void
    )
{
    union REGS 	r;
	DWORD		size;
	WORD		*fixup;
    
	if ( pcLibReady )
		return 0;

	stack_base = malloc( STACK_SIZE + 1024 );
	if ( stack_base == NULL )
		return -1;
	stack_top = (BYTE*)(((DWORD)( stack_base + 255 ) & ~255 ) + ( STACK_SIZE - 256 ));
	stacks = STACKS;
/*
* Relocate real-mode interrupt handler for use with irq's 7-15
*/
	size = (DWORD)rmEnd - (DWORD)rmInt;
    r.x.eax = 0x100; 				/* DOS allocate DOS memory */
    r.x.ebx = ( size + 15 ) >> 4;	/* Number of paragraphs requested */
    int386 (0x31, &r, &r );

    if ( ! r.x.cflag )
    {
    	rm_selector = (unsigned)( r.x.edx );
		rm_handler = ( BYTE * )((r.x.eax & 0x0000FFFF ) << 4 );
		memcpy( rm_handler, ( void * ) rmInt, size );
		rmTrap = rm_handler + 2;
		fixup = (WORD*)(rm_handler + ((DWORD)rmFixup-(DWORD)rmInt)+3);
		*fixup = 2;
    }
/*
* Initialize Structures and save PIC Masks
*/
	memset( irqHandler, 0, sizeof( irqHandler ) );
    irqHandler[ 0 ].this_isr = irq0;
    irqHandler[ 1 ].this_isr = irq1;
    irqHandler[ 2 ].this_isr = irq2;
    irqHandler[ 3 ].this_isr = irq3;
    irqHandler[ 4 ].this_isr = irq4;
    irqHandler[ 5 ].this_isr = irq5;
    irqHandler[ 6 ].this_isr = irq6;
    irqHandler[ 7 ].this_isr = irq7;
    irqHandler[ 8 ].this_isr = irq8;
    irqHandler[ 9 ].this_isr = irq9;
    irqHandler[ 10 ].this_isr = irq10;
    irqHandler[ 11 ].this_isr = irq11;
    irqHandler[ 12 ].this_isr = irq12;
    irqHandler[ 13 ].this_isr = irq13;
    irqHandler[ 14 ].this_isr = irq14;
    irqHandler[ 15 ].this_isr = irq15;
    
	pic1_mask = inp( PORT_PIC1_rwIMR );
	pic2_mask = inp( PORT_PIC2_rwIMR );

	pcLibReady = 1;
	return 0;
}

/****************************************************************************
* pcRemoveISR - Remove installed ISR
*****************************************************************************/
PUBLIC VOID
pcRemoveISR(
	int		irq				// INPUT : IRQ int Remove 
	)
{
	IRQ_SERVICE	*ih;
	BYTE		imr, mask;
	UINT		pflags;

	if ( ! pcLibReady )
		return ;

	pflags = PUSH_DISABLE();
    ih = &irqHandler[ irq ];
    if ( ih->prev_isr )
    {
    	if ( irq > 7 )
    	{
        /*
        * Unhook ISR from protect-mode interrupt 
        */
		    _dos_setvect( 0x68 + irq, ih->prev_isr );
		/*
		* Unhoook from real-mode interrupt
		*/		
			if ( rm_handler != NULL )
			{
				union REGS		r;

				r.x.eax = 0x201;	/* DPMI set real mode vector */
				r.h.bl  = 0x68 + irq;
				r.x.ecx = ih->realmode_sel;
				r.x.edx = ih->realmode_ofs;
				int386( 0x31, &r, &r );
			}
		/*
        * Restore PIC mask
        */
			mask = 0x01 << ( irq - 8 );
			if ( ( ih->PIC2_IMR & mask ) != 0 )
			{
				imr = inp( PORT_PIC2_rwIMR );
				imr |= mask;
	        	outp( PORT_PIC2_rwIMR, imr );
			}
			if ( ( ih->PIC1_IMR & 0x04 ) != 0 )
			{
				imr = inp( PORT_PIC1_rwIMR );
				imr |= 0x04;
				outp( PORT_PIC1_rwIMR, imr );
			}
		}
		else
		{
        /*
        * Unhook ISR from protect-mode interrupt 
        */
		    _dos_setvect( 0x08 + irq, ih->prev_isr );

		/*
        * Restore PIC mask
        */
			mask = 0x01 << irq;
			if ( ( ih->PIC1_IMR & mask ) != 0 )
			{
				imr = inp( PORT_PIC1_rwIMR );
				imr |= mask;
	        	outp( PORT_PIC1_rwIMR, imr );
			}
		}
		ih->prev_isr = NULL;
	}
	POP_DISABLE( pflags );
}

/****************************************************************************
* pcDropIRQLib - Drop IRQ Library and remove any isr's 
*****************************************************************************/
void
pcDropIRQLib(
	void
    )
{
    union REGS 	r;
	int			irq;
	UINT		pflags;

	if ( ! pcLibReady )
		return ;

	pflags = PUSH_DISABLE();
	for ( irq = 0; irq < 16; irq++ )
	{
    	pcRemoveISR( irq );
	}
/*
* Put PIC Masks back to the way they were
*/
	outp( PORT_PIC2_rwIMR, pic2_mask );
	outp( PORT_PIC1_rwIMR, pic1_mask );
/*
* Release memory back to DOS
*/
    r.x.eax = 0x101; 				/* release DOS memory */
    r.x.edx = rm_selector;
    int386 (0x31, &r, &r );

	pcLibReady = 0;
	POP_DISABLE( pflags );
	free( stack_base );
}

/****************************************************************************
* pcInstallISR - Install new ISR
*****************************************************************************/
PUBLIC VOID
pcInstallISR(
    int		irq, 				// INPUT : IRQ for Interrupt
    int 	(*Isr)(int irq),  	// INPUT : ISR
    int		flags				// INPUT : Flags
    )
{
	BYTE			imr;
	union REGS		r;
    IRQ_SERVICE		*ih;
	UINT			pflags;

	if ( irq < 0 || irq > 15  || ! pcLibReady )
    	return;

	if ( irqHandler[ irq ].prev_isr )
		return ;

	if ( flags & PCI_PRE_EOI )
		flags = PCI_PRE_EOI;
	else 
		flags = PCI_POST_EOI;

	pflags = PUSH_DISABLE();
    
    ih = &irqHandler[ irq ];
    ih->callback = Isr;
	ih->flags = flags;
	    
    if ( irq > 7 )
    {
		if ( WatchId < 0 )
		{
			WatchId = TSM_NewService( PC_WatchDog, 0, 255, 0 );
		}
		if ( rm_handler != NULL )
		{
			r.x.eax = 0x200;	/* DPMI get real mode vector */
			r.h.bl  = ( irq > 7 ? irq + 0x68 : irq + 0x08 );
			int386( 0x31, &r, &r );
			ih->realmode_sel = r.x.ecx;
			ih->realmode_ofs = r.x.edx;
		}
        ih->prev_isr = _dos_getvect( irq + 0x68 );  // save old vector
        _dos_setvect( irq + 0x68, ih->this_isr );

		if ( rm_handler != NULL )
		{
			r.x.eax = 0x201;	/* DPMI set real mode vector */
			r.h.bl  = irq + 0x68;
			r.x.ecx = D32RealSeg( rm_handler );
			r.x.edx = D32RealOfs( rm_handler );
			int386( 0x31, &r, &r );
		}

		imr = ih->PIC2_IMR = inp( PORT_PIC2_rwIMR );
        outp( PORT_PIC2_rwIMR, imr & ( ~( 0x01 << ( irq - 8 ) ) ) );

		imr = ih->PIC1_IMR = inp( PORT_PIC1_rwIMR );
		outp( PORT_PIC1_rwIMR, imr & 0xFB );
    }
    else
    {
       	ih->prev_isr = _dos_getvect( irq + 8 );
		imr = ih->PIC1_IMR = inp( PORT_PIC1_rwIMR );
        _dos_setvect( irq+8, ih->this_isr );
        outp( PORT_PIC1_rwIMR, imr & ( ~ ( 0x01 << irq ) ) );
    }
	POP_DISABLE( pflags );
}




