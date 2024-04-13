/*===========================================================================
* TIMERS.C - PC Timer Logic
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
* $Log:   F:/DMX/API/VCS/TSMAPI.C_V  $
*  
*     Rev 1.3   02 Oct 1993 14:43:12   pjr
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <conio.h>
#include <dos.h>

#include "dmx.h"

#define PT_PROGTIMER    0x36
#define PT_CMD_REG      0x43
#define PT_DATA_REG     0x40

#define TIMER_INT       0x08

#define MAX_SERVICES    12

typedef struct
{
	int 		( *ServiceProc )( void );
	UINT 		Frequency;
	INT	 		FreqQuotient;
	INT	 		FreqRemainder;
	INT	 		FreqAccumulator;
	LONG 		NextInvocationTick;
	UINT 		Priority;
	WORD		Active;
	WORD 		Flags;
} SERVICE;

/*
*   Service Flags:
*/
#define SF_ALLOCATED    0x01
#define SF_PAUSED       0x02
#define SF_EXECUTING    0x04

LOCAL SERVICE			Service[ MAX_SERVICES ];
LOCAL INT				ServiceOrder[ MAX_SERVICES ];
LOCAL UINT				ServicesActive;
LOCAL BOOL          	TsmInstalled = FALSE;
LOCAL volatile UINT		AccessLock;
LOCAL UINT				MasterFreq;
LOCAL volatile DWORD	MasterTicks;
LOCAL WORD				MasterDivisor;
LOCAL WORD				ChainRate;

PRIVATE int
bcd2bin(
    BYTE		bcd
    )
{
    int         val;

    val = ( ( bcd >> 4 ) & 0x0F ) * 10;
    val += ( bcd & 0x0F );
    return val;
}

PRIVATE void
TSM_CorrectTime(
	void
    )
{
    int         hour;
    int         minute;
    int         second;
    long        ltime;
    long        *sysclock;

#ifdef __386__
    union REGS 	r;

	r.h.ah = 0x02;
	int386( 0x1A, &r, &r );
	
    hour = bcd2bin( r.h.ch );
    minute = bcd2bin( r.h.cl );
    second = bcd2bin( r.h.dh );
#else
    BYTE	    h,m,s;

    _asm {
        mov ah,2
        int 1ah
        mov h,ch
        mov m,cl
        mov s,dh
    }
    hour = bcd2bin( h );
    minute = bcd2bin( m );
    second = bcd2bin( s );
#endif

    sysclock = ( long * )( 0x0000046CL );
    ltime = ( long ) hour * 65543335l;
    ltime += ( long ) minute * 1092389l;
    ltime += ( long ) second * 18206l;
    *sysclock = ( ltime / 1000 ) + 1;
}

/****************************************************************************
* TSM_ResumeService - Resume execution of service procedure.
*****************************************************************************/
void
TSM_ResumeService(
	INT			ServiceId			// INPUT: Service ID to Resume
	)
{
	SERVICE	*sp;
	UINT   	so;
	UINT   	sa;
	UINT   	priority;
	UINT   	freq;
	DWORD	pflags;

	if ( ServiceId < 0 || ServiceId >= MAX_SERVICES )
		return;

	sp = &Service[ ServiceId ];

	priority = sp->Priority;
	freq     = sp->Frequency;

	if ( Service[ ServiceId ].Flags & SF_PAUSED )
	{
	/*
	* Insert service into execution order by priority/freq
	*/
		pflags = PUSH_DISABLE();
		for ( so = 0; so < ServicesActive; so++ )
		{
			sp = &Service[ ServiceOrder[ so ] ];

			if ( sp->Priority < priority ||
				( sp->Priority == priority && sp->Frequency < freq )
				)
			{
				break;
			}
		}
		sp = &Service[ ServiceId ];
		sp->FreqAccumulator = sp->FreqRemainder;

		for ( sa = ServicesActive; sa > so; sa-- )
		{
			ServiceOrder[ sa ] = ServiceOrder[ sa - 1 ];
		}
		ServiceOrder[ so ] = ServiceId;
		sp->Flags &= ~SF_PAUSED;
		sp->NextInvocationTick = MasterTicks + sp->FreqQuotient;
		++ServicesActive;
		POP_DISABLE( pflags );
	}
}

/****************************************************************************
* TSM_PauseService - Pause execution of service procedure.
*****************************************************************************/
void
TSM_PauseService(
	INT			ServiceId			// INPUT: Service ID to Pause
	)
{
	UINT			so;
	DWORD			pflags;

	if ( ServiceId < 0 || ServiceId >= MAX_SERVICES )
		return;

	if ( Service[ ServiceId ].Flags & SF_PAUSED )
		return;

	for ( so = 0; so < ServicesActive; so++ )
	{
	/*
	* Remove service from execution list.
	*/
		if ( ServiceOrder[ so ] == ServiceId )
		{
			pflags = PUSH_DISABLE();
			--ServicesActive;
			for ( ; so < ServicesActive; so++ )
			{
				ServiceOrder[ so ] = ServiceOrder[ so + 1 ];
			}
			Service[ ServiceId ].Flags |= SF_PAUSED;
			POP_DISABLE( pflags ); 
			return;
		}
	}
}

/****************************************************************************
* TSM_NewService - Assign procedure to be service at given frequency.
*****************************************************************************/
INT		// Returns: Service ID >= 0, -1 = Failed    
TSM_NewService(
	VOID		( *SProc )( void ), 	// INPUT: Ptr to Service Procedure
	UINT		Frequency,          	// INPUT: Frequency in Hz
	INT			Priority,           	// INPUT: Scheduler Pri. HIGH=255..0=LOW
	INT			Pause               	// INPUT: Wait for Resume to Start Service
	)
{
	SERVICE     *sp;
	INT			sb;

	if ( Frequency > MasterFreq || TsmInstalled == FALSE )
		return -1;

	if ( Frequency == 0 )
		Frequency = MasterFreq;
/*
*   Scan for unused service block.
*/
	for ( sb = 0; sb < MAX_SERVICES; sb++ )
	{
		if ( Service[ sb ].Flags & SF_ALLOCATED )
			continue;

	/*
	* Found unused block, now init...
	*/
		sp = &Service[ sb ];
		sp->ServiceProc       	= SProc;
		sp->Frequency         	= Frequency;
		sp->FreqQuotient      	= ( MasterFreq / Frequency );
		sp->FreqRemainder     	= ( MasterFreq % Frequency );
		sp->FreqAccumulator   	= sp->FreqRemainder;
		sp->NextInvocationTick	= MasterTicks + sp->FreqQuotient;
		sp->Priority          	= Priority;
		sp->Active				= FALSE;
		sp->Flags             	= ( SF_PAUSED | SF_ALLOCATED );

		if ( ! Pause )
		{
			TSM_ResumeService( sb );
		}
		return sb;
	}
	return -1;
}

/****************************************************************************
* TSM_DelService - Delete the service procedure from the timer manager.
*****************************************************************************/
void
TSM_DelService(
	INT			ServiceId			// INPUT: Service ID to Delete
	)
{
	if ( ServiceId >= 0 && ServiceId < MAX_SERVICES )
	{
		TSM_PauseService( ServiceId );
		Service[ ServiceId ].Flags = 0;
	}
}

/****************************************************************************
* TSM_ResetCount - Reset service invocation counter.
*****************************************************************************/
void
TSM_ResetCount(
	INT			ServiceId			// INPUT: Service ID to Reset Counters
	)
{
	SERVICE     	*sp;
	DWORD			pflags;

	if ( ServiceId < 0 || ServiceId >= MAX_SERVICES )
		return;

	sp = &Service[ ServiceId ];
	if ( sp->Flags & SF_PAUSED )
		return;

	pflags = PUSH_DISABLE();
	sp->FreqAccumulator = sp->FreqRemainder;
	sp->NextInvocationTick = MasterTicks + sp->FreqQuotient;
	POP_DISABLE( pflags );
}

/****************************************************************************
* TSM_SyncService - Synchronize Service (call on next timer tick) with
*                   current event.
*****************************************************************************/
void
TSM_SyncService(
	INT				ServiceId			// INPUT: Service ID to Reset Counters
	)
{
	SERVICE     	*sp;
	UINT			flags;

	if ( ServiceId < 0 || ServiceId >= MAX_SERVICES )
		return;

	flags = PUSH_DISABLE();

	sp = &Service[ ServiceId ];
	sp->NextInvocationTick = MasterTicks;
	sp->FreqAccumulator = sp->FreqRemainder;

	POP_DISABLE( flags );
}

/*==========================================================================*
* SetTimer0 - Change Frequency Divsor for Timer 0
*===========================================================================*/
PRIVATE VOID
SetTimer0(
	WORD			Divisor			// INPUT: Rate = 1193180 / Divisor 
	)
{
	outp( PT_CMD_REG, PT_PROGTIMER );
    TSM_Delay( 0 );
	outp( PT_DATA_REG, LOBYTE( Divisor ) );
    TSM_Delay( 0 );
	outp( PT_DATA_REG, HIBYTE( Divisor ) );
    TSM_Delay( 0 );
}

/*==========================================================================*
* TimerISR - Main Timer Interrupt Service Routine
*===========================================================================*/
PRIVATE int
TimerISR()
{
	LONG		s;
	SERVICE		*sp;
	BYTE		bPrevColor;
    int         rc;

#ifndef PROD
	if ( DmxDebug & DMXBUG_TSMAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 1 );
//		DmxTrace( "> TimerISR" );
	}
#endif
	++MasterTicks;
	_enable();

	for ( s = 0; s < ServicesActive; s++ )
	{
		sp = &Service[ ServiceOrder[ s ] ];
		if ( sp->Active )
			break;

		if ( sp->Flags == SF_ALLOCATED
		  && sp->NextInvocationTick <= MasterTicks )
		{
			sp->Active = TRUE;
			if ( ( rc = sp->ServiceProc() ) == TS_OKAY )
			{
				sp->NextInvocationTick = MasterTicks + sp->FreqQuotient;
				sp->FreqAccumulator += sp->FreqRemainder;
				if ( sp->FreqAccumulator >= MasterFreq )
				{
					sp->FreqAccumulator -= MasterFreq;
					sp->NextInvocationTick++;
				}
			}
            else if ( rc == TS_SYNC )
            {
                SetTimer0( MasterDivisor );
				sp->NextInvocationTick = MasterTicks + sp->FreqQuotient;
	            sp->FreqAccumulator = sp->FreqRemainder;
            }
			sp->Active = FALSE;
		}
	}
	_disable();

#ifndef PROD
	if ( DmxDebug & DMXBUG_TSMAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
//		DmxTrace( "< TimerISR" );
	}
#endif
	if ( ( MasterTicks % ChainRate ) == 0 )
		return PCI_CHAIN_ISR;
	else
		return PCI_OKAY;
}

/****************************************************************************
* TSM_Install - Install Timer Service Manager.
*****************************************************************************/
BOOL			// Returns : True - Installed OK, FALSE - Failed to Install
TSM_Install(
	UINT			MasterFrequency		// INPUT: Frequency in Hz
	)
{
	if ( MasterFrequency < 18 || TsmInstalled )
		return FALSE;

	ChainRate = MasterFrequency / 18;
	AccessLock = 0;
	ServicesActive = 0;
	MasterTicks = 0;
	MasterFreq = MasterFrequency;
	memset( Service, 0, sizeof( Service ) );

	if ( pcInitIRQLib() != 0 )
		return FALSE;

	pcInstallISR( IRQ_0, TimerISR, PCI_PRE_EOI );
	MasterDivisor = ( UINT )( 1193180L / ( LONG ) MasterFrequency );
	SetTimer0( MasterDivisor );
    TsmInstalled = TRUE;

	return TRUE;
}


/****************************************************************************
* TSM_Delay - Wait until the specified amount of time elapses.
*****************************************************************************/
void
TSM_Delay(
	DWORD			ms		// INPUT : Amount of time in milliseconds
	)
{
	DWORD			mt;
	BYTE			bPrevColor;
    DWORD           ticks;

#ifndef PROD
	if ( DmxDebug & DMXBUG_TSMAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 1 );
//		DmxTrace( "> TSM_Delay" );
	}
#endif

    ticks = ( ( ms * MasterFreq ) / 1000 );
    if ( ms > 0 && ticks == 0 )
        ticks++;
	mt = MasterTicks;
	while ( ticks > 0 )
	{
		while ( mt == MasterTicks )
			;
		mt = MasterTicks;
		ticks--;
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_TSMAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
//		DmxTrace( "< TSM_Delay" );
	}
#endif
}

/****************************************************************************
* TSM_Clock - Returns the amount of time elapsed in milliseconds since
*             TSM_Install was called.
*****************************************************************************/
DWORD
TSM_Clock(
	VOID
	)
{
    return ( MasterTicks * 1000 ) / MasterFreq;
}

/****************************************************************************
* TSM_Remove - Remove Timer Service Manager.
*****************************************************************************/
void
TSM_Remove(
	void
	)
{
	if ( TsmInstalled == FALSE )
		return ;

    TsmInstalled = FALSE;
	SetTimer0( (WORD) 0 );
	pcRemoveISR( IRQ_0 );
	memset( Service, 0, sizeof( Service ) );
	pcDropIRQLib();
  	TSM_CorrectTime();
}


/****************************************************************************
* TSM_TimeVR - Time Vertical Retrace
*****************************************************************************/
int     // returns frequency in hz
TSM_TimeVR(
    int     port        // INPUT : Video Port Address
    )
{
    UINT    pflags;
    int     j;
    BYTE    lsb;
    BYTE    msb;
    WORD    cnt;


    pflags = PUSH_DISABLE();
    outp( PT_CMD_REG, 0x34 );   // Switch to Mode 2
    TSM_Delay( 0 );
/*
* Set the timer count to 0, so we know we won't get another
* timer interrupt right away.
*/
    outp( PT_DATA_REG, 0 );
    TSM_Delay( 0 );
    outp( PT_DATA_REG, 0 );
    POP_DISABLE( pflags );
/*
* Wait before clearing interrupts to allow the interrupt generated
* when switching from mode 3 to mode 2 to be recognized.
*/
    for ( j = 0; j < 10; j++)
    {
        TSM_Delay( 0 );
    }
/*
* Disable interrupts to get an accurate count.
*/
    pflags = PUSH_DISABLE();
/*
* Wait for Vertical Blanking
*/
    while ( ( inp( port ) & 0x08 ) == 0 )
        ;
/*
* Set the timer count to 0 again to start the timing interval.
*/
    outp( PT_CMD_REG, 0x34 );
    TSM_Delay( 0 );
    outp( PT_DATA_REG, 0 );
    TSM_Delay( 0 );
    outp( PT_DATA_REG, 0 );
/*
* Wait for Vertical Retrace to End
*/
    while ( inp( port ) & 0x08 )
        ;
/*
* Wait for Vertical Blanking
*/
    while ( ( inp( port ) & 0x08 ) == 0 )
        ;
/*
* Latch Timer (stop counting) and Read Elapsed Time
*/
    outp( PT_CMD_REG, 0 );
    lsb = inp( PT_DATA_REG );
    TSM_Delay( 0 );
    msb = inp( PT_DATA_REG );

    POP_DISABLE( pflags );
/*
* Wait before clearing interrupts to allow the interrupt generated
* when switching from mode 3 to mode 2 to be recognized.
*/
	SetTimer0( MasterDivisor );
    for ( j = 0; j < 10; j++)
    {
        TSM_Delay( 0 );
    }
    cnt = ( WORD )( 65536L - ( ((WORD) msb << 8 ) | lsb ));
    return ( int )( 1193180L / cnt );
}

