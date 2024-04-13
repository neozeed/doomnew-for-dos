/*===========================================================================
* PCINT.C - PC Interrupt Interface
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
* $Log:   F:/DMX/API/VCS/PCINT.C_V  $
*  
*     Rev 1.0   02 Oct 1993 14:44:44   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <conio.h>
#include <string.h>
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

#define D32RealSeg(p)	((((DWORD)(p))>>4) & 0xFFFF)
#define D32RealOfs(p)	(((DWORD)(p)) & 0x000F)

PRIVATE BYTE *rm_handler 	= NULL;
PRIVATE PC_INT *IrqIsr[ 16 ];
PRIVATE int	WatchId = -1;

extern void rmInt( void );
extern void rmEnd( void );
extern void rmFixup( void );
PRIVATE BYTE	*rmTrap = NULL;

/****************************************************************************
* PC_WatchDog - Watches for missed interrupts and will trigger software
*				interrupt.
*****************************************************************************/
int
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

			if ( IrqIsr[ irq ] != NULL )
			{
				IrqIsr[ irq ]->bForced = 1;
				IrqIsr[ irq ]->Isr();
				IrqIsr[ irq ]->bForced = 0;
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


PRIVATE void
PC_RmHandler()
{
    union REGS 	r;
	DWORD		size;
	WORD		*fixup;

	if ( rm_handler == NULL )
	{
		size = (DWORD)rmEnd - (DWORD)rmInt;
    	r.x.eax = 0x100; 				/* DOS allocate DOS memory */
    	r.x.ebx = ( size + 15 ) >> 4;	/* Number of paragraphs requested */
    	int386 (0x31, &r, &r );

    	if ( r.x.cflag )  /* Failed */
	    	return;

		rm_handler = ( BYTE * )((r.x.eax & 0x0000FFFF ) << 4 );
		memcpy( rm_handler, ( void * ) rmInt, size );
		rmTrap = rm_handler + 2;
		fixup = (WORD*)(rm_handler + ((DWORD)rmFixup-(DWORD)rmInt)+3);
		*fixup = 2;
	}
}

/****************************************************************************
* PC_NewISR - Install new ISR
*****************************************************************************/
PUBLIC VOID
PC_NewISR(
	PC_INT	*pcint,				// INPUT : PC_INT Handle
    int		irq, 				// INPUT : IRQ for Interrupt
    void 	(INTERRUPT *Isr)()  // INPUT : ISR
    )
{
	BYTE			imr, mask;
	union REGS		r;

	if ( WatchId < 0 )
	{
		memset( IrqIsr, 0, sizeof( IrqIsr ) );
		WatchId = TSM_NewService( PC_WatchDog, 0, 255, 0 );
	}
	IrqIsr[ irq ] = pcint;

	pcint->bIRQ = ( BYTE ) ( irq & ~PC_DISABLE );
    _disable();

	pcint->Isr = Isr;
	pcint->bForced = 0;

	if ( irq & PC_DISABLE )
	{
		irq &= ~PC_DISABLE;

		pcint->oldvect = NULL;

		if ( irq > 7 )
		{
			mask = 0x01 << ( irq - 8 );
			imr = inp( PORT_PIC2_rwIMR );
			imr |= mask;
	        outp( PORT_PIC2_rwIMR, imr );
		}
		else
		{
			mask = 0x01 << irq;
			imr = inp( PORT_PIC1_rwIMR );
			imr |= mask;
        	outp( PORT_PIC1_rwIMR, imr );
		}
		_enable();
		return ;
	}
    if ( irq > 7 )
    {
		PC_RmHandler();
		if ( rm_handler != NULL )
		{
			r.x.eax = 0x200;	/* DPMI get real mode vector */
			r.h.bl  = ( irq > 7 ? irq + 0x68 : irq + 0x08 );
			int386( 0x31, &r, &r );
			pcint->wRmSel = ( WORD ) r.x.ecx;
			pcint->wRmOfs = ( WORD ) r.x.edx;
		}
        pcint->oldvect = _dos_getvect( irq + 0x68 );  // save old vector
        _dos_setvect( irq + 0x68, Isr );

		if ( rm_handler != NULL )
		{
			r.x.eax = 0x201;	/* DPMI set real mode vector */
			r.h.bl  = irq + 0x68;
			r.x.ecx = D32RealSeg( rm_handler );
			r.x.edx = D32RealOfs( rm_handler );
			int386( 0x31, &r, &r );
		}

		imr = pcint->bPIC2_IMR = inp( PORT_PIC2_rwIMR );
        outp( PORT_PIC2_rwIMR, imr & ( ~( 0x01 << ( irq - 8 ) ) ) );

		imr = pcint->bPIC1_IMR = inp( PORT_PIC1_rwIMR );
       outp( PORT_PIC1_rwIMR, imr & 0xFB );
    }
    else
    {
        pcint->oldvect = _dos_getvect( irq+8 );
        _dos_setvect( irq+8, Isr );

		imr = pcint->bPIC1_IMR = inp( PORT_PIC1_rwIMR );
        outp( PORT_PIC1_rwIMR, imr & ( ~ ( 0x01 << irq ) ) );
    }
    _enable();
}


/****************************************************************************
* PC_DelISR - Delete installed ISR
*****************************************************************************/
PUBLIC VOID
PC_DelISR(
	PC_INT	*pcint			// INPUT : PC_INT Handle
	)
{
	BYTE	imr, mask;
	BYTE	irq = pcint->bIRQ;

    _disable();

	IrqIsr[ irq ] = NULL;
	if ( pcint->oldvect != NULL )
	{
    	if ( irq > 7 )
    	{
		    _dos_setvect( 0x68 + irq, pcint->oldvect );

			if ( rm_handler != NULL )
			{
				union REGS		r;

				r.x.eax = 0x201;	/* DPMI set real mode vector */
				r.h.bl  = 0x68 + irq;
				r.x.ecx = pcint->wRmSel;
				r.x.edx = pcint->wRmOfs;
				int386( 0x31, &r, &r );
				IrqIsr[ irq ] = NULL;
			}

			mask = 0x01 << ( irq - 8 );
			if ( ( pcint->bPIC2_IMR & mask ) != 0 )
			{
				imr = inp( PORT_PIC2_rwIMR );
				imr |= mask;
	        	outp( PORT_PIC2_rwIMR, imr );
			}
			if ( ( pcint->bPIC1_IMR & 0x04 ) != 0 )
			{
				imr = inp( PORT_PIC1_rwIMR );
				imr |= 0x04;
				outp( PORT_PIC1_rwIMR, imr );
			}
		}
		else
		{
		    _dos_setvect( 0x08 + irq, pcint->oldvect );

			mask = 0x01 << irq;
			if ( ( pcint->bPIC1_IMR & mask ) != 0 )
			{
				imr = inp( PORT_PIC1_rwIMR );
				imr |= mask;
	        	outp( PORT_PIC1_rwIMR, imr );
			}
		}
		pcint->oldvect = NULL;
	}
    _enable();
}


/****************************************************************************
* PC_SendEOI - Send End Of Interrupt to Controller
*****************************************************************************/
PUBLIC void
PC_SendEOI(
	PC_INT	*pcint			// INPUT : PC_INT Handle
	)
{
	BYTE	bPrevColor;

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
	if ( pcint->bForced )
		pcint->bForced = 0;
	else
	{
		_disable();
    	if ( pcint->bIRQ > 7 )
		{
        	outp( PORT_PIC2_wOCW2, CMD_PIC_EOI );
        	outp( PORT_PIC1_wOCW2, CMD_PIC_EOI );
		}
		else
		{
        	outp( PORT_PIC1_wOCW2, CMD_PIC_EOI );
		}
	}
	_enable();
#ifndef PROD
	if ( DmxDebug & DMXBUG_PCINT )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
}
