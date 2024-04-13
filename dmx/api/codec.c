/*===========================================================================
* CODEC.C - Logic for CODEC based Sound Boards
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <conio.h>
#include <dos.h>

#include "dmx.h"
#include "dspapi.h"

#define SILENCE         0x80
#define DMA_BUFSIZE     2048
#define DMA_BLOCKSIZE   128
#define DMA_TESTSIZE    2

// hardware specific globals
LOCAL WORD gwPorts[] =
   { 0x530, 0x604, 0xe80, 0xf40, 0xffff } ;
LOCAL BYTE gbInterrupts[] =
   { 7, 9, 10, 11, 0xff } ;

LOCAL BYTE gabIntConfig[] =
   { 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0x08,
     0xff, 0x10, 0x18, 0x20 } ;

LOCAL BYTE gabInitRegValues[16] =
   { 0x00, 0x00, 0x8f, 0x8f,
     0x8f, 0x8f, 0x3f, 0x3f,
     0x4b, 0x00, 0x40, 0x00,
     0x00, 0xfc, 0xff, 0xff } ;

LOCAL BOOL      gfDmaActive;
LOCAL VOID      ( *SetupDMA )( VOID ) = NULL;
LOCAL BOOL		gfManualOverride = FALSE;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
PRIVATE BOOL
clRead(
    BYTE	*pByte
    )
{
	register int 	n;

	for ( n = DSP_POLL_MAX_TRIES; n; --n )
   	{
    	if ( inp( gDspPorts.wRd_OkToRead ) & 0x80 )
       	{
        	*pByte = ( BYTE ) inp( gDspPorts.wRd_Data );
			return TRUE;
       	}
   	}
	return FALSE;
}

PRIVATE BOOL
clWrite (
    BYTE    bVal
    )
{
	register int 	n;

	for ( n = DSP_POLL_MAX_TRIES; n; --n )
   	{
    	if ( ! ( inp( gDspPorts.wWr_Data ) & 0x80 ) )
       	{
			outp( gDspPorts.wWr_Data, bVal );
			return TRUE;
		}
	}
	return FALSE;
}

/*--------------------------------------------------------------------------*
* clGetVersion - Get Version of DSP chip
*---------------------------------------------------------------------------*
* Returns:
*---------------------------------------------------------------------------*/
PRIVATE WORD	// Returns: 0 = Not Detected, HIBYTE=Major LOWBYTE=Minor
clGetVersion(
	void
	)
{
	BYTE		major = 1;
	BYTE		minor = 0;
/*
* Get Card Version
*/
	if ( ! clWrite( CMD_DSP_GetVersion ) )
		return 0;

	if ( ! clRead( &major ) || ! clRead( &minor ) )
		return 0;

	if ( major > 4 )
		major = 1, minor = 0;

//	major = 2, minor = 0;

	return ( WORD )( ( WORD ) major << 8 ) | ( WORD ) minor;
}


PRIVATE BOOL
clSetupEchoCard(
	void
	)
 {
 	int 		ASIC_ID;
	int			soft_version;
	DWORD		clk;
	WORD		status;
  
	ASIC_ID = inpw( gDspPorts.wBaseAddr + 4 );	/* Read ASIC ID register */
/*
* ID should be 0x4501, 0x4502, or 0x4503
*/
  	if ( ( ASIC_ID & 0xff00 ) != 0x4500 )  
		return FALSE;
/*
* Send command 1, get software version - wait upto 10ms for result
*/
	outpw( gDspPorts.wBaseAddr, 1 );  
	clk = TSM_Clock() + 10;
	do
	{
		status = inpw( gDspPorts.wBaseAddr + 2 ) & 0x4000;
	}
	while ( status == 0 && TSM_Clock() < clk );
	if ( status == 0 )
		return FALSE;
/*
* Must be software version 1.5 or later
*/
	soft_version = inpw( gDspPorts.wBaseAddr );
	if ( soft_version < 0x132 )
		return FALSE;           
/*
* Send command 0x15, switch to SB Pro mode
*/
	outpw( gDspPorts.wBaseAddr, 0x15 );  
	return TRUE;
} 

PRIVATE BOOL
clReset (
    void
    )
{
	BYTE    byte;

	outp( gDspPorts.wRdWr_Reset, 1 );
	TSM_Delay( 30 );
	outp( gDspPorts.wRdWr_Reset, 0 );
	TSM_Delay( 20 );
	if ( clRead( &byte ) == TRUE && byte == 0xAA )
	{
		if ( clSetupEchoCard() == TRUE && DmxDebug & DMXBUG_MESSAGES )
		{
			printf( "ECHO Personal Sound System Enabled.\n" );
		}
    	return TRUE;
	}
   	return FALSE;
}

void
clWriteMixer (
    BYTE    bMixerRegister,
    BYTE    bVal
    )
{
	outp( gDspPorts.wWr_MixerAddx, bMixerRegister );
	outp( gDspPorts.wRdWr_MixerData, bVal );
}

BYTE
clReadMixer (
    BYTE 	bMixerRegister
    )
{
	outp( gDspPorts.wWr_MixerAddx, bMixerRegister );
	return (BYTE) inp( gDspPorts.wRdWr_MixerData );
}

PRIVATE BOOL
clAckInterrupt(
    VOID
    )
{
	BYTE 	bIntrStatus;

	if ( gVersion >= 0x0400 )
	{
		bIntrStatus = clReadMixer( REG_MIXER_IntrStatus );
    	if ( bIntrStatus & BIT_INTRSTATUS_Voice08 )
		{
    		inp( gDspPorts.wRd_AckVoice08Intr );
    		return TRUE;
		}
		if ( bIntrStatus & BIT_INTRSTATUS_Voice16 )
		{
			inp( gDspPorts.wRd_AckVoice16Intr );
			return TRUE;
		}
		return FALSE;
	}
   	inp( gDspPorts.wRd_AckVoice08Intr );
   	return TRUE;
}



PUBLIC int
clGetBlasterEnv(
    char    IDchar
    )
{
	char	*pEnv,
        	*pTarget;
	int 	i,
        	j,
        	BaseAddx;
    char	k[5+1];


	if ( ( pEnv = getenv( "BLASTER" ) ) == NULL )
		return -1;

	strupr( pEnv );
    if ( ( pTarget = strchr( pEnv, IDchar ) ) == NULL )
	    return -1;

	i = pTarget - pEnv + 1;
	j = 0;
	while ( pEnv[ i ] != ' ' && pEnv[ i ] && j < sizeof( k ) - 1 )
        k[ j++ ] = pEnv[ i++ ];
    k[ j ] = 0;

    if ( IDchar == 'A' || IDchar == 'V' )
    {
        sscanf( k, "%x", &BaseAddx );
        return BaseAddx;
    }
    return atoi( k );
}

PRIVATE void
clSetHWBaseAddr(
	WORD	wBaseAddx
	)
{
	gDspPorts.wBaseAddr			 = wBaseAddx;
    gDspPorts.wWr_MixerAddx      = wBaseAddx + 0x04;
    gDspPorts.wRdWr_MixerData    = wBaseAddx + 0x05;
    gDspPorts.wRdWr_Reset        = wBaseAddx + 0x06;
    gDspPorts.wRd_Data           = wBaseAddx + 0x0A;
    gDspPorts.wWr_Data           = wBaseAddx + 0x0C;
    gDspPorts.wRd_OkToRead       = wBaseAddx + 0x0E;
    gDspPorts.wRd_AckVoice08Intr = wBaseAddx + 0x0E;
    gDspPorts.wRd_AckVoice16Intr = wBaseAddx + 0x0F;
}

PUBLIC void
clSetHWParam(
    void
    )
{
	WORD    wBaseAddx;

	wBaseAddx = (WORD) clGetBlasterEnv('A');
	if ( wBaseAddx < 0x210 || wBaseAddx > 0x280 ||
		 ( wBaseAddx & 0x00F ) != 0
	   )
   	{
        wBaseAddx = 0x220;
    }
	clSetHWBaseAddr( wBaseAddx );

	if ( ( gHwSetup.bIRQ = (BYTE)clGetBlasterEnv('I') ) == 255 )
        gHwSetup.bIRQ = 7;

	if ( ( gHwSetup.bLowDMA = (BYTE)clGetBlasterEnv('D') ) == 255 )
        gHwSetup.bLowDMA = 1;

}

/****************************************************************************
* clCallBack - DSP Callback Function - Interrupt Handler
*****************************************************************************/
PUBLIC int
clCallBack(
	)
{
	BYTE		bPrevColor;
	UINT		flags;

	flags = PUSH_DISABLE();
#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3f );
		DmxTrace( "> clCallBack" );
	}
#endif
	if ( gfDmaActive == TRUE && clAckInterrupt() )
	{
    	if ( SetupDMA )
        	SetupDMA();

		dspNextPage();
	}
	POP_DISABLE( flags );
#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
		DmxTrace( "< clCallBack" );
	}
#endif
	return PCI_OKAY;
}


PUBLIC VOID
clStop(
    VOID
    )
{
    gfDmaActive = FALSE;
    PC_StopDMA( gHwSetup.bLowDMA );
    clReset();
}

/*--------------------------------------------------------------------------*
* clDeInit - Reset SB and Shutdown
*---------------------------------------------------------------------------*/
PUBLIC VOID
clDeInit(
	void
	)
{
	clStop();
	pcRemoveISR( gHwSetup.bIRQ );
    dspDeInit();
	if ( gVersion >= 0x300 )
	{
        gFilterStatus = clReadMixer( 0x0e );
		gFilterStatus |= 0x20;
		gFilterStatus &= ~0x02;
        clWriteMixer( 0x0E, ( BYTE )( gFilterStatus ) );
	}
}

PRIVATE VOID
clContinueDMA(
    void
    )
{
    WORD        tc = dspGetPageSize() - 1;

    clWrite( CMD_DSP_StdPlaySample );
    clWrite( LOBYTE( tc ) );
    clWrite( HIBYTE( tc ) );
}

PRIVATE VOID
clStartDMA(
    void
    )
{
    WORD        tc;

    SetupDMA = NULL;
    tc = dspGetPageSize() - 1;
/*
* Program PC DMAC continuous auto-initialize transfer
*/
    PC_StopDMA( gHwSetup.bLowDMA );

	if ( gVersion >= 0x0300 )
	{
	/*
	* Program Mixer for Filterd Mode
	*/
        gFilterStatus = clReadMixer( 0x0e );
        clWriteMixer( 0x0E, ( BYTE )( gFilterStatus & ~0x20 ) );
	}
    if ( gVersion >= 0x0200 )
    {
    	PC_PgmDMA( gHwSetup.bLowDMA,
			dspGetDmaBuffer(), dspGetDmaSize(),
			FALSE, TRUE );
    /*
    * Set DMA Block Size, this sets the sample interval in which
    * we will have a chance to call the mixer.
    */
        clWrite( CMD_DSP_SetBlockSize );
        clWrite( LOBYTE( tc ) );
        clWrite( HIBYTE( tc ) );

/*
* Do to the fact that some so-called 100% SoundBlaster CLONES don't return
* the correct version to features supported, The following code has been
* ifdef'd out.  I realy wish that the shit heads that design this stuff
* would actually read the documentation that Creative Labs wrote explaining
* what capabilities are available at each revision level and stick to the
* defined standard.
*
* When support for >22K samples is desired, this code will have to be
* uncommented, and all clones must be blown off the face of the earth.
*/
#if 1
		if ( gVersion >= 0x300 )
		/*
		* Send Hi-Speed Play Command, the only way to stop it is to reset the DSP.
    	* NOTE: This is auto-initialize mode
		*/
        	clWrite( CMD_DSP_HsPlaySample );
		else
#endif
			clWrite( CMD_DSP_AIPlaySample );
    }
	else
	{
    	PC_PgmDMA( gHwSetup.bLowDMA,
					dspGetDmaBuffer(), dspGetDmaSize(),
					FALSE, TRUE );
		SetupDMA = clContinueDMA;
        clWrite( CMD_DSP_StdPlaySample );
        clWrite( LOBYTE( tc ) );
        clWrite( HIBYTE( tc ) );
	}
}

PUBLIC WORD
clGetPlayCounter(
    WORD    wBufferSize     // INPUT : Size of DMA Ring
    )
{
    WORD        cnt;

    cnt = PC_GetDmaCount( gHwSetup.bLowDMA );
    if ( cnt > wBufferSize )
        cnt = wBufferSize;
    return wBufferSize - cnt;
}


PUBLIC void
clPlayMode(
	int		iChannels,		// INPUT : # of Channels 1..8
	WORD	wSampleRate
	)
{
    DWORD       crystal_osc = 1000000L;
    BYTE        data;
	WORD	    tc;
    CHNL_TYPE   eType;

	if ( gVersion == 0xFFFF )
		return;

	clAckInterrupt();

    if ( gVersion >= 0x0400 )
        eType = DSP_STEREO_LR8;
    else if ( gVersion >= 0x0300 )
	{
        eType = DSP_STEREO_RL8;
	/*
	* Because of the stupid engineers at MediaVision got the channels reversed,
	* we have to do this bullshit test for the Pro Audio Spectrum 3D
	*/
		if ( clWrite( CMD_DSP_DetectJazz ) )
		{
			int 	j;
			BYTE	b;

			for ( j = 0; j < 10; j++ )
			{
				if ( ! clRead( &b ) )
					break;
			}
			if ( j == 1 && b != 0 )
			{
			/* YES, it is all Jazz'd up */
				if ( DmxDebug & DMXBUG_MESSAGES )
				{
					printf( "Pro Audio Spectrum 3D JAZZ\n" );
				}
				eType = DSP_STEREO_LR8;
			}
			else
			{
				clReset();
			}
		}
	}
    else
        eType = DSP_MONO_8;

    if ( dspInit( eType, wSampleRate, iChannels, clGetPlayCounter, NULL,
				  ( gVersion >= 0x0200 ? TRUE : FALSE ) ) )
        return ;

	gfDmaActive = TRUE;
	if ( gVersion < 0x0400 )
	{
    /*
    * DSP Versions < 4.0 must have the Speaker Turned ON
    * NOTE: ample time must be allowed before attempting to start transfer.
    */
		clWrite( CMD_DSP_DACSpeakerOn );
		TSM_Delay( 200 );
	/*
	* Program PC DMAC for 1 byte single-cycle transfer to sync DMAC with
    * SB DSP.
	*/
		SetupDMA = clStartDMA;
	    PC_PgmDMA( gHwSetup.bLowDMA, dspGetDmaBuffer(), 12,	FALSE, FALSE );

	    if ( gVersion >= 0x0300 )
	    {
	    /*
	    * Turn on Stereo Mode
	    */
            data = clReadMixer( 0x0E );
            clWriteMixer( 0x0E, ( BYTE )( data | 0x02 ) );
/*
* UNCOMMENT THIS FOR HARDCODED VOLUMES
*/
			clWriteMixer( 0x04, 0xEE );		// DSP Volume
			clWriteMixer( 0x26, 0xEE );		// FM Volume
//			clWriteMixer( 0x2E, 0x00 );		// Line Input
//			clWriteMixer( 0x22, 0xAA );		// Master Volume
	    }
	/*
	* Set Time Constant (sample rate)
	*/
		if ( gVersion >= 0x0300 )
            crystal_osc /= 2;       // Stereo mode has twice the data

		tc = 256 - ( WORD ) ( crystal_osc / wSampleRate );
		clWrite( CMD_DSP_SetTimeConstant );
		clWrite( ( BYTE ) tc );
	/*
	* Send I/O Command followed by data transfer count.
	*/
        clWrite( CMD_DSP_StdPlaySample );
        clWrite( 11 );
        clWrite( 0 );
	}
    else    // DSP >= 4.0 ( SB 16(asp) or better )
    {
		clAckInterrupt();
        SetupDMA = NULL;
	/*
	* Program PC DMAC continuous auto-initialize transfer
	*/
        PC_PgmDMA( gHwSetup.bLowDMA,
			dspGetDmaBuffer(), dspGetDmaSize(),
			FALSE, TRUE );
	/*
	* Set Samples per Second
	*/
        clWrite( CMD_DSP_DACSamplingRate );
        clWrite( HIBYTE( wSampleRate ) );
        clWrite( LOBYTE( wSampleRate ) );
    /*
    * Begin DMA Block Transfer
    */
        clWrite( CMD_DSP_DAC8bit );
        clWrite( MODE_DSP_StereoUnsigned );
        tc = dspGetPageSize() - 1;
        clWrite( LOBYTE( tc ) );
        clWrite( HIBYTE( tc ) );
    }
}

/****************************************************************************
* clInit - Initialize Digital Sound System.
*****************************************************************************/
PUBLIC INT
clInit(
	VOID
	)
{
	if ( gfManualOverride == FALSE )
	{
	/*
	* Reset Card at Location specified by BLASTER env.
	*/
		clSetHWParam();
	}
	if ( ! clReset() )
	{
		return RS_BOARD_NOT_FOUND;
	}
    gfDmaActive = FALSE;
	pcInstallISR( gHwSetup.bIRQ, clCallBack, PCI_POST_EOI );
	gVersion = clGetVersion();

/*
* 	issue 0xFA to get JAZZ Chip ID for Audio Spectrum 3D
* 	issue 6 reads
*/
	if ( DmxDebug & DMXBUG_MESSAGES )
	{
    	printf( "\nDSP Version: %x.%02x\n", gVersion >> 8, gVersion & 0x00FF );
    	printf(   "        IRQ: %d\n", gHwSetup.bIRQ );
    	printf(   "        DMA: %d\n", gHwSetup.bLowDMA );
	}
	clAckInterrupt();
  	return RS_OKAY;
}


PRIVATE int
clDetectIrq(
	int 		irq
	)
{
	gIrqDetected = irq;
	return PCI_OKAY;
}


/****************************************************************************
* SB_SetCard - Set card to specific configuration.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*****************************************************************************/
PUBLIC VOID
SB_SetCard(
	int		iBaseAddr,		// INPUT : Base Address of Board
	int		iIrq,			// INPUT : Irq of Board
	int		iDma			// INPUT : DMA Channel
	)
{
	gfManualOverride = TRUE;
	
	clSetHWBaseAddr( iBaseAddr );
	gHwSetup.bIRQ 	 = iIrq;
	gHwSetup.bLowDMA = iDma;
}

PRIVATE BOOL
clCheckIRQ(
	int		*channel,
	int		*iIrq
	)
{
	int		j;
	int		d;
	int		irq;
	int		def_irq;
	DWORD	clk;

	clReset();

	pcInstallISR( IRQ_2, clDetectIrq, PCI_POST_EOI );
	pcInstallISR( IRQ_3, clDetectIrq, PCI_POST_EOI );
	pcInstallISR( IRQ_4, clDetectIrq, PCI_POST_EOI );
	pcInstallISR( IRQ_5, clDetectIrq, PCI_POST_EOI );
	pcInstallISR( IRQ_7, clDetectIrq, PCI_POST_EOI );
	pcInstallISR( IRQ_10, clDetectIrq, PCI_POST_EOI );

	def_irq = gHwSetup.bIRQ;
	_disable();
/*
* 	Make sure all IRQ's are EOI'd
*/
	outp( 0x20, 0x0b );
	while( inp( 0x20 ) != 0 )
	{
		outp( 0x20, 0x20 );
	}
/*
* First try for a Creative Labs Card vie CMD_DSP_GenreateInt
*/
	gIrqDetected = 0;
	_enable();
	clWrite( CMD_DSP_GenerateInt );
	clk = TSM_Clock() + 200;	// 200 ms
	while ( ! gIrqDetected && TSM_Clock() < clk )
		;
	irq = gIrqDetected;
	if ( gIrqDetected )
	{
	/*
	* Verify IRQ
	*/
		clAckInterrupt();
		gIrqDetected = 0;
		clWrite( CMD_DSP_GenerateInt );
		clk = TSM_Clock() + 200;	// 200 ms
		while ( gIrqDetected != irq && TSM_Clock() < clk )
			;
		if ( irq != gIrqDetected )
			gIrqDetected = 0;
		clAckInterrupt();
	}
	_disable();
/*
* 	Make sure all IRQ's are EOI'd
*/
	outp( 0x20, 0x0b );
	while( inp( 0x20 ) != 0 )
	{
		outp( 0x20, 0x20 );
	}
	if ( gIrqDetected == 0 )
	{
		PC_PgmDMA( *channel, gDmaHandle, 4, FALSE, FALSE );

		clWrite( CMD_DSP_DACSpeakerOn );
		_enable();
		TSM_Delay( 400 );

		clWrite( CMD_DSP_SetTimeConstant );
		clWrite( 211 );
   		clWrite( CMD_DSP_StdPlaySample );
		gIrqDetected = 0;
		clWrite( 3 );
   		clWrite( 0 );


		clk = TSM_Clock() + 200;	// 200 ms
		while ( ! gIrqDetected && TSM_Clock() < clk )
			;
		PC_StopDMA( *channel );
		if ( gIrqDetected )
			*iIrq = gIrqDetected;
		else
			*iIrq = def_irq;
		gHwSetup.bIRQ = *iIrq;
		clAckInterrupt();
	/*
	* if failed, try another way...
	*/
		if ( ! gIrqDetected )
		{
			clReset();
			gVersion = clGetVersion();

			for ( d = 0; d < 3; d++ )
			{
				if ( cDma[ d ] == *channel )
					break;
			}
			PC_StopDMA( 0 );
			PC_StopDMA( 1 );
			PC_StopDMA( 3 );

			clWrite( CMD_DSP_SetTimeConstant );
			clWrite( 211 );
	   		clWrite( CMD_DSP_StdPlaySample );
			gIrqDetected = 0;
	   		clWrite( 10 );
	   		clWrite( 0 );
			for ( j = 0; j < 3 && ! gIrqDetected; j++ )
			{
				PC_PgmDMA( cDma[ d ], gDmaHandle, 11, FALSE, FALSE );
				clk = TSM_Clock() + 200;	// 200 ms
				while ( ! gIrqDetected && TSM_Clock() < clk )
					;
				if ( gIrqDetected )
				{
					*channel = cDma[ d ];
					gHwSetup.bIRQ = *iIrq = gIrqDetected;
					clAckInterrupt();
				}
				PC_StopDMA( cDma[ d ] );
				if ( ! gIrqDetected )
				{
					d = ( d + 1 ) % 3;
				}
			}
		}
	}
	else
	{
		*iIrq = gIrqDetected;
	}
	_enable();
	pcRemoveISR( IRQ_2 );
	pcRemoveISR( IRQ_3 );
	pcRemoveISR( IRQ_4 );
	pcRemoveISR( IRQ_5 );
	pcRemoveISR( IRQ_7 );
	pcRemoveISR( IRQ_10 );

	clAckInterrupt();
	return (BOOL) ( gIrqDetected ? TRUE : FALSE );
}


/****************************************************************************
* SB_Detect - Detects SB Series Card and returns best guess of how it is
*             configured.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*	2 = Could not allocate DMA buffer
*	3 = Could not detect interrupt or DMA
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
SB_Detect(
	int		*iBaseAddr,		// IN/OUT: -2=Scan, -1=Default, >0=User Specified
	int		*iIrq,			// IN/OUT: -2=Scan, -1=Default, >0=User Specified
	int		*iDma,			// IN/OUT: -2=Scan, -1=Default, >=0 User Specified
	WORD	*wVersion		// OUTPUT: Card Version
	)
{
	BOOL		benv;
	int			rc = 0;
	int			j;

	gfManualOverride = FALSE;
/*
* If BLASTER environment is set, then get "DEFAULT" values.
*/
	benv = ( getenv( "BLASTER" ) != NULL ? TRUE : FALSE );
	clSetHWParam();

	if ( *iBaseAddr == -1 )
		*iBaseAddr = gDspPorts.wBaseAddr;
	if ( *iBaseAddr > 0 )
	{
		clSetHWBaseAddr( *iBaseAddr );
		if ( clReset() == FALSE )
			return 1;
	}
	if ( *iBaseAddr < 0 )					// Scan for Card
	{
		*iBaseAddr = gDspPorts.wBaseAddr;	// Check Default
		clSetHWBaseAddr( *iBaseAddr );
		if ( clReset() == FALSE )
		{
			for ( j = 0; j < 6; j++ )
			{
				if ( cAddr[ j ] == ( WORD )( *iBaseAddr ) )
					continue;				// Already Checked

				clSetHWBaseAddr( cAddr[ j ] );
				if ( clReset() == TRUE )
					break;					// Card Found
			}
			if ( j == 6 )
				return 1;
		}
		*iBaseAddr = gDspPorts.wBaseAddr;
	}
	*wVersion = clGetVersion();
/*
* Next Verify IRQ & DMA Settings
*/
	if ( *iIrq == 0 )		// 0 = Don't Verify Irq & DMA
	{
		*iIrq = gHwSetup.bIRQ;
		if ( *iDma < 0 )
			*iDma = gHwSetup.bLowDMA;
		return 0;
	}
	if ( ( gDmaHandle = PC_AllocDMABuffer( DMA_BUFSIZE ) ) == NULL )
		return 2;
#ifdef __386__
    gDmaBuffer = gDmaHandle;
#else
    gDmaBuffer = Addr32Bit( gDmaHandle );
#endif
	if ( *iDma < 0 )
		*iDma = gHwSetup.bLowDMA;

	gHwSetup.bLowDMA = *iDma;

	if ( clCheckIRQ( iDma, iIrq ) == TRUE )
		rc = 0;
	else
		rc = 3;

	PC_FreeDMABuffer( gDmaHandle );
	gDmaHandle = NULL;
	gDmaBuffer = NULL;
	return rc;
}

PRIVATE int
clSetCallback(
	VOID	(* func )( void )
	)
{
	return 0;
}

PUBLIC DMX_WAV	clWav =
{
	clInit,
	clDeInit,
	clPlayMode,
	clStop,
	clSetCallback
};


