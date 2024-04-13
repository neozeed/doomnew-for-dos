/*===========================================================================
* PAS.C - Pro Audio Spectrum (Media Vision) Support.
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
*  
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <limits.h>
#include <dos.h>

#include "dmx.h"
#include "dspapi.h"
#include "mvpas.h"

// #define MV100

#define VERBOSE

typedef enum 
{
	mvINTRCTLRST,		//  B89 Interrupt Status 		
	mvAUDIOFILT,		//  B8A Audio Filter Control		
	mvINTRCTLR,			//  B8B Interrupt Control		
	mvPCMDATA,			//  F88 PCM Data I/O Register	
	mvPCMDATAH,			//  F89 PCM Data I/O Register	
	mvCROSSCHANNEL,		//  F8A Cross Channel			
	mvSAMPLERATE,		// 1388 Sample Rate Timer		
	mvSAMPLECNT,		// 1389 Sample Count Register
	mvTMRCTLR,			// 138B	Local Timer Control Register 	*/
	mvPORTS				// Number of ports we care about
} mvPORT;
LOCAL WORD		mvPort[ mvPORTS ];

#ifdef MV100
typedef union
{
	VOID		*v;
	WORD		*w;
	BYTE		*b;
} MEMPORT;

LOCAL MEMPORT	mvIOState[ mvPORTS ];
#endif

LOCAL int		mvIrq;
LOCAL int		mvDma;
LOCAL WORD		mvXlat;
LOCAL DWORD		mvID;
LOCAL BOOL		mvActive;
LOCAL MVState	*mvState;

PRIVATE void
int2f(
    union REGS 	*r
	)
{
	int386( 0x2f, r, r );
}

PRIVATE VOID
mvDelay(
	void
	)
{
	inp( INTRCTLR ^ mvXlat );
}

PRIVATE VOID
mvOutpw(
	mvPORT		port,	// INPUT : Port to write to
	WORD		data	// INPUT : Data to write to port
	)
{
	outp( mvPort[ port ], LOBYTE( data ) );
	outp( mvPort[ port ], HIBYTE( data ) );
#ifdef MV100
	*mvIOState[ port ].w = data;
#endif
}

PRIVATE VOID
mvOutp(
	mvPORT		port,	// INPUT : Port to write to
	BYTE		data	// INPUT : Data to write to port
	)
{
	outp( mvPort[ port ], data );
#ifdef MV100
	*mvIOState[ port ].b = data;
#endif
}

PRIVATE BYTE
mvInp(
	mvPORT		port 	// INPUT : Port to read from
	)
{
#ifdef MV100
	if ( mvID & bMV101 )
		return inp( mvPort[ port ] );
	else
		return *mvIOState[ port ].b;
#else
	return inp( mvPort[ port ] );
#endif
}

PRIVATE int
mvServiceIRQ(
	)
{
	BYTE		bits;
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
		DmxTrace( "> mvServiceIRQ" );
	}
#endif
	bits = inp( INTRCTLRST ^ mvXlat );
	if ( ( bits & bISsampbuff ) != 0 && mvActive == TRUE )
    {
		dspNextPage();
    }
	mvOutp( mvINTRCTLRST, bits );	// Flush Interrupt
	POP_DISABLE( flags );
#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
		DmxTrace( "< mvServiceIRQ" );
	}
#endif
	return PCI_OKAY;
}

PUBLIC WORD
mvGetPlayCounter(
    WORD    wBufferSize     // INPUT : Size of DMA Ring
    )
{
    WORD        cnt;

	if ( mvActive == TRUE )
	{
    	cnt = PC_GetDmaCount( mvDma );
		if ( mvDma >= 5 )
			cnt <<= 1;
    	if ( cnt > wBufferSize )
        	cnt = wBufferSize;
    	return wBufferSize - cnt;
	}
	return 0;
}

PRIVATE void		// SetupPCMDMAIO  --  Setup to output to the DAC
mvStartPCM(
	int		iChannels,		// INPUT : # of Channels 1..8
	WORD	wSampleRate
	)
{
	WORD		sr = ( WORD )( ( 1193180L >> 1 ) / wSampleRate );
	BYTE		mask;
	WORD		PageSize;

    if ( dspInit( DSP_STEREO_LR8, wSampleRate, iChannels, mvGetPlayCounter, NULL, TRUE ) )
        return ;

	_disable();
/*
* Stop any DMA or PCM playback and MV Interrupts
*/
	mvOutp( mvAUDIOFILT, 0 );
	mvDelay();
	mvOutp( mvCROSSCHANNEL, 0 );
	mvDelay();
	mvOutp( mvINTRCTLR, 0 );
	mvDelay();
/*
* Program Sample Rate
*/
	mvOutp( mvTMRCTLR, 0x36 );	// Timer 0 & square wave
	mvDelay();
	mvOutpw( mvSAMPLERATE, sr );

	mvOutp( mvTMRCTLR, 0x74 );	// Timer 1 & rate generator
	mvDelay();
	PageSize = dspGetPageSize();
	mvOutpw( mvSAMPLECNT,
		( mvDma < 4 ? PageSize : PageSize >> 1 ) );
/*
* Setup the Interrupt Control Register
*/
	// Flush any pending interrupts
	mask = inp( INTRCTLRST ^ mvXlat );
	mvDelay();
	mvOutp( mvINTRCTLRST, mask );

	// interrupt on sample buffer count
	mvOutp( mvINTRCTLR, bICsampbuff );
/*
* enable the 12/16 bit stuff
*/
	if ( mvID & bMV101 )
	{
		mask = inp( SYSCONFIG2 ^ mvXlat );
		mask &= ~( bSC216bit | bSC212bit );
		outp( SYSCONFIG2 ^ mvXlat, mask );
	}
/*
* setup the direction, stereo/mono and DMA enable bits
*/
	mask = ( bCCdac | bCCdrq | bCCr2r | bCCl2l );
	mvOutp( mvCROSSCHANNEL, mask );
	mvDelay();
	mvOutp( mvCROSSCHANNEL, mask | bCCenapcm );
/*
* setup PC-DMA Controller
*/
    PC_PgmDMA( mvDma, dspGetDmaBuffer(), dspGetDmaSize(), FALSE, TRUE );
/*
* Setup the audio filter sample bits
*/
	// enable the sample count/buff counters
	mvOutp( mvAUDIOFILT, bFIsrate | bFIsbuff | bFImute );

	mvActive = TRUE;
	_enable();
}

PRIVATE void
mvStopPCM(
	void
	)
{
	BYTE	bits;

	_disable();
/*
* clear the audio filter sample bits
*/
	mvActive = FALSE;

	bits = mvInp( mvAUDIOFILT );
	bits &= ~( bFIsrate | bFIsbuff );
	mvOutp( mvAUDIOFILT, bits );
/*
* clear the PCM enable bit
*/
	bits = mvInp( mvCROSSCHANNEL );
	bits &= ~bCCenapcm;
	bits |= bCCdac;
	mvOutp( mvCROSSCHANNEL, bits );
/*
* disable the 16 bit stuff
*/
	if ( mvID & bMV101 )
	{
		bits = inp( SYSCONFIG2 ^ mvXlat );
		bits &= ~( bSC216bit | bSC212bit );
		outp( SYSCONFIG2 ^ mvXlat, bits );
	}
	bits = inp( INTRCTLR ^ mvXlat );
	bits &= ~( bICsamprate | bICsampbuff );
	mvOutp( mvINTRCTLR, bits );
	_enable();
/*
* Kill DMA
*/
	PC_StopDMA( mvDma );
}

PRIVATE int
mvInitHardware(
	void
	)
{
	LOCAL WORD	base_addr[] = { DEFAULT_BASE, ALT_BASE_1, ALT_BASE_2, ALT_BASE_3, 0 };
	union REGS	r;
	WORD		bits;
	WORD		bits2;
	int			i;

/*
* Check for MVSOUND.SYS driver
*/
	memset( &r, 0, sizeof( r ) );
	r.x.eax = 0x0BC00;
	r.x.ebx = 0x03F3F;
	int2f( &r );
	if ( ( r.x.eax != 0x0BC00 && r.x.eax != 0x0000 ) ||
		 ( r.x.ebx != 0x06D00 && r.x.ebx != 0x4D00 ) ||
		 ( r.x.ecx != 0x00076 && r.x.ecx != 0x0056 ) ||
		 r.x.edx != 0x02020
	   )
	{
		if ( DmxDebug & DMXBUG_MESSAGES )
		{
			printf( "NO MVSOUND.SYS\n" );
		}
		return RS_BOARD_NOT_FOUND;
	}
/*
* Get DMA,IRQ
*/
	memset( &r, 0, sizeof( r ) );
	r.x.eax = 0x0BC04;
	int2f( &r );
	mvDma = r.x.ebx;
	mvIrq = r.x.ecx;

	if ( DmxDebug & DMXBUG_MESSAGES )
		printf( "PAS IRQ: %d  DMA:%d\n", mvIrq, mvDma );

/*
* Get State Table Address
*/
	memset( &r, 0, sizeof( r ) );
	r.x.eax = 0x0BC02;
	int2f( &r );
	mvState = (MVState * )( r.x.edx << 4 | r.x.ebx );

#if 0
	printf( "State Table %08lxh:\n", mvState );
	printf( "  %02x _sysspkrtmr\n", mvState->_sysspkrtmr );
	printf( "  %02x _systmrctlr\n", mvState->_systmrctlr );
	printf( "  %02x _sysspkrreg\n", mvState->_sysspkrreg );
	printf( "  %02x _joystick\n", mvState->_joystick );
	printf( "  %02x _lfmaddr\n", mvState->_lfmaddr );
	printf( "  %02x _lfmdata\n", mvState->_lfmdata );
	printf( "  %02x _rfmaddr\n", mvState->_rfmaddr );
	printf( "  %02x _rfmdata\n", mvState->_rfmdata );
	printf( "  %02x _dfmaddr\n", mvState->_dfmaddr );
	printf( "  %02x _dfmdata\n", mvState->_dfmdata );
	printf( "  %02x _paudiomixr\n", mvState->_paudiomixr );
	printf( "  %02x _audiomixr\n", mvState->_audiomixr );
	printf( "  %02x _intrctlrst\n", mvState->_intrctlrst );
	printf( "  %02x _audiofilt\n", mvState->_audiofilt );
	printf( "  %02x _intrctlr\n", mvState->_intrctlr );
	printf( "  %02x _pcmdata\n", mvState->_pcmdata );
	printf( "  %02x _crosschannel\n", mvState->_crosschannel );
	printf( "%04x _samplerate\n", mvState->_samplerate );
	printf( "%04x _samplecnt\n", mvState->_samplecnt );
	printf( "%04x _spkrtmr\n", mvState->_spkrtmr );
	printf( "  %02x _tmrctlr\n", mvState->_tmrctlr );
	printf( "  %02x _mdirqvect\n", mvState->_mdirqvect );
	printf( "  %02x _mdsysctlr\n", mvState->_mdsysctlr );
	printf( "  %02x _mdsysstat\n", mvState->_mdsysstat );
	printf( "  %02x _mdirqclr\n", mvState->_mdirqclr );
	printf( "  %02x _mdgroup1\n", mvState->_mdgroup1 );
	printf( "  %02x _mdgroup2\n", mvState->_mdgroup2 );
	printf( "  %02x _mdgroup3\n", mvState->_mdgroup3 );
	printf( "  %02x _mdgroup4\n", mvState->_mdgroup4 );
#endif
/*
* Initialize to known state
*/
	for ( i = 0; base_addr[ i ]; i++ )
	{
		mvXlat = base_addr[ i ] ^ DEFAULT_BASE;

		if ( ( bits = inp( INTRCTLR ^ mvXlat ) ) == 0xFF )
			continue;

		outp( INTRCTLR ^ mvXlat, bits ^ fICrevbits );
		memset( &r, 0, sizeof( r ) );	/* Do this for a delay */
		bits2 = inp( INTRCTLR ^ mvXlat );
		outp( INTRCTLR ^ mvXlat, bits );
		if ( bits2 != bits )
			continue;
	/*
	* We have hardware! Load the product bit definitions
	*/	
		bits >>= fICrevshr;
		mvID = bMVSCSI;

		if ( bits )		// Not rev 1.0
		{
		/*
		* All second generation Pro Audio cards use the MV101
		* and have SB emulation.
		*/
			mvID |= ( bMVSBEMUL+bMV101 );
		/*
		* determine AT/PS2, CDPC slave mode
		*/
			bits2 = inp( MASTERMODRD ^ mvXlat );
			if ( ! ( bits2 & bMMRDatps2 ) )
				mvID |= bMVPS2;
			if ( bits2 & bMMRDmsmd )
  				mvID |= bMVSLAVE;
		/*
		* Get Revision Bits
		*/
			bits2 = inp( MASTERCHIPR ^ mvXlat );
			bits2 &= 0x000F;
			bits2 <<= 11;
			mvID |= bits2;
		/*
		* determine the FM chip, 8/16 bit DAC, and mixer
		*/
			bits2 = inp( SLAVEMODRD ^ mvXlat );
			if ( bits2 & bSMRDdactyp )		// 16 bit DAC?
				mvID |= bMVDAC16;
			if ( bits2 & bSMRDfmtyp )		// OPL3 chip?
				mvID |= bMVOPL3;
			if (  mvID & ( bMVSLAVE+bMVDAC16 ) != bMVDAC16 )
				mvID |= bMVA508;
		/*
		* determine if MPU-401 emulation is active
		*/
			if ( inp( COMPATREGE ^ mvXlat ) & cpMPUEmulation )
				mvID |= bMVMPUEMUL;
		}
	/*
	* Initialize static tables
	*/
		mvPort[ mvINTRCTLRST ]  = INTRCTLRST ^ mvXlat;
		mvPort[ mvAUDIOFILT ]	= AUDIOFILT ^ mvXlat;
		mvPort[ mvINTRCTLR ] 	= INTRCTLR ^ mvXlat;
		mvPort[ mvPCMDATA ]		= PCMDATA ^ mvXlat;
		mvPort[ mvPCMDATAH ]	= PCMDATAH ^ mvXlat;
		mvPort[ mvCROSSCHANNEL ]= CROSSCHANNEL ^ mvXlat;
		mvPort[ mvSAMPLERATE ]	= SAMPLERATE ^ mvXlat;
		mvPort[ mvSAMPLECNT ]	= SAMPLECNT ^ mvXlat;
		mvPort[ mvTMRCTLR ]		= TMRCTLR ^ mvXlat;

#ifdef MV100
		mvIOState[ mvINTRCTLRST ].v		= ( VOID * ) &mvState->_intrctlrst;
		mvIOState[ mvAUDIOFILT ].v   	= ( VOID * ) &mvState->_audiofilt;
		mvIOState[ mvINTRCTLR ].v    	= ( VOID * ) &mvState->_intrctlr;
		mvIOState[ mvPCMDATA ].v	   	= ( VOID * ) &mvState->_pcmdata;
		mvIOState[ mvPCMDATAH ].v	   	= ( VOID * ) &mvState->_RESRVD2;
		mvIOState[ mvCROSSCHANNEL ].v	= ( VOID * ) &mvState->_crosschannel;
		mvIOState[ mvSAMPLERATE ].v  	= ( VOID * ) &mvState->_samplerate;
		mvIOState[ mvSAMPLECNT ].v   	= ( VOID * ) &mvState->_samplecnt;
		mvIOState[ mvTMRCTLR ].v		= ( VOID * ) &mvState->_tmrctlr;
		return RS_OKAY;
#else
		if ( mvID & bMV101 )
			return RS_OKAY;
		else if ( DmxDebug & DMXBUG_MESSAGES )
			printf( "Pro Audio Spectrum is NOT version 1.01.\n" );
#endif
	}
	return RS_BOARD_NOT_FOUND;
}

/****************************************************************************
* mvInit - Initialize Digital Sound System.
*****************************************************************************/
PRIVATE INT
mvInit(
	VOID
	)
{
	int		rc;
/*
* Reset Card at Location specified by BLASTER env.
*/
	if ( ( rc = mvInitHardware() ) != RS_OKAY )
		return rc;
/*
* Chain Interrupt
*/
	pcInstallISR( mvIrq, mvServiceIRQ, PCI_POST_EOI );
  	return RS_OKAY;
}


/*--------------------------------------------------------------------------*
* mvDeInit - Shutdown Media Vision Card
*---------------------------------------------------------------------------*/
PUBLIC VOID
mvDeInit(
	void
	)
{
	mvStopPCM();
	if ( mvIrq )
		pcRemoveISR( mvIrq );
    dspDeInit();
}

PRIVATE int
mvSetCallback(
	VOID	(* func )( void )
	)
{
	return 0;
}

PUBLIC DMX_WAV	mvWav =
{
	mvInit,
	mvDeInit,
	mvStartPCM,
	mvStopPCM,
	mvSetCallback
};

/****************************************************************************
* MV_Detect - Detects Media Vision Chip based Cards
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
MV_Detect(
	void
	)
{
	if ( mvInitHardware() == RS_OKAY )
		return 0;
	else
		return 1;
}

/*	
main()
{
	union REGS	r;
	MVState		*st;
	int			rc;

	rc = mvInitHardware();
	printf( "rc=%d, bits=%08lX\n", rc, mvID );
	memset( &r, 0, sizeof( r ) );
	r.x.eax = 0x0BC00;
	r.x.ebx = 0x03F3F;
	int2f( &r );
	printf( "Check For Driver\nAX=%04x\nBX=%04x\nCX=%04x\nDX=%04x\n",
		r.x.eax, r.x.ebx, r.x.ecx, r.x.edx  );

	memset( &r, 0, sizeof( r ) );
	r.x.eax = 0x0BC04;
	int2f( &r );
	printf( "Get DMA/IRQ/INT\nAX=%04x\nBX=%04x\nCX=%04x\nDX=%04x\n",
		r.x.eax, r.x.ebx, r.x.ecx, r.x.edx  );

	memset( &r, 0, sizeof( r ) );
	r.x.eax = 0x0BC02;
	int2f( &r );
	printf( "State Table:\nAX=%04x\nBX(offset)=%04x\nCX(len)=%04x\nDX(segment)=%04x\n",
		r.x.eax, r.x.ebx, r.x.ecx, r.x.edx  );

	st = (MVState * )( r.x.edx << 4 | r.x.ebx );
	printf( "State Table %08lxh:\n", mvState );
	printf( "  %02x _sysspkrtmr\n", st->_sysspkrtmr );
	printf( "  %02x _systmrctlr\n", st->_systmrctlr );
	printf( "  %02x _sysspkrreg\n", st->_sysspkrreg );
	printf( "  %02x _joystick\n", st->_joystick );
	printf( "  %02x _lfmaddr\n", st->_lfmaddr );
	printf( "  %02x _lfmdata\n", st->_lfmdata );
	printf( "  %02x _rfmaddr\n", st->_rfmaddr );
	printf( "  %02x _rfmdata\n", st->_rfmdata );
	printf( "  %02x _dfmaddr\n", st->_dfmaddr );
	printf( "  %02x _dfmdata\n", st->_dfmdata );
	printf( "  %02x _paudiomixr\n", st->_paudiomixr );
	printf( "  %02x _audiomixr\n", st->_audiomixr );
	printf( "  %02x _intrctlrst\n", st->_intrctlrst );
	printf( "  %02x _audiofilt\n", st->_audiofilt );
	printf( "  %02x _intrctlr\n", st->_intrctlr );
	printf( "  %02x _pcmdata\n", st->_pcmdata );
	printf( "  %02x _crosschannel\n", st->_crosschannel );
	printf( "%04x _samplerate\n", st->_samplerate );
	printf( "%04x _samplecnt\n", st->_samplecnt );
	printf( "%04x _spkrtmr\n", st->_spkrtmr );
	printf( "  %02x _tmrctlr\n", st->_tmrctlr );
	printf( "  %02x _mdirqvect\n", st->_mdirqvect );
	printf( "  %02x _mdsysctlr\n", st->_mdsysctlr );
	printf( "  %02x _mdsysstat\n", st->_mdsysstat );
	printf( "  %02x _mdirqclr\n", st->_mdirqclr );
	printf( "  %02x _mdgroup1\n", st->_mdgroup1 );
	printf( "  %02x _mdgroup2\n", st->_mdgroup2 );
	printf( "  %02x _mdgroup3\n", st->_mdgroup3 );
	printf( "  %02x _mdgroup4\n", st->_mdgroup4 );
}

*/
