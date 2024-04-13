/*===========================================================================
* AWE32.C - Creative Labs AWE32 General MIDI Support
*----------------------------------------------------------------------------
*                CONFIDENTIAL & PROPRIETARY INFORMATION 			     
*----------------------------------------------------------------------------
*            Copyright (C) 1993,94 Digital Expressions, Inc. 		     
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
#include <dos.h>
#include <string.h>
#include <io.h>
#include <conio.h>

#include "dmx.h"

#ifdef  __WATCOMC__
#pragma  data_seg ("_CODE", "");
#ifdef __386__
#pragma aux awe32SPad1	"*";
#pragma aux awe32SPad2	"*";
#pragma aux awe32SPad3	"*";
#pragma aux awe32SPad4	"*";
#pragma aux awe32SPad5	"*";
#pragma aux awe32SPad6	"*";
#pragma aux awe32SPad7	"*";

#pragma aux awe32SPad1Obj	"*";
#pragma aux awe32SPad2Obj	"*";
#pragma aux awe32SPad3Obj	"*";
#pragma aux awe32SPad4Obj	"*";
#pragma aux awe32SPad5Obj	"*";
#pragma aux awe32SPad6Obj	"*";
#pragma aux awe32SPad7Obj	"*";
#endif
#endif

#define PASCAL      __pascal
typedef BYTE FAR*               LPBYTE;
typedef WORD FAR*               LPWORD;
typedef DWORD FAR*              LPDWORD;

extern LPBYTE       awe32SPad1;
extern LPBYTE       awe32SPad2;
extern LPBYTE       awe32SPad3;
extern LPBYTE       awe32SPad4;
extern LPBYTE       awe32SPad5;
extern LPBYTE       awe32SPad6;
extern LPBYTE       awe32SPad7;

/* SoundFont objects */
extern BYTE awe32SPad1Obj[];
extern BYTE awe32SPad2Obj[];
extern BYTE awe32SPad3Obj[];
extern BYTE awe32SPad4Obj[];
extern BYTE awe32SPad5Obj[];
extern BYTE awe32SPad6Obj[];
extern BYTE awe32SPad7Obj[];

/* MIDI support functions */
extern WORD PASCAL awe32InitMIDI(void);
extern WORD PASCAL awe32NoteOn(WORD, WORD, WORD);
extern WORD PASCAL awe32NoteOff(WORD, WORD, WORD);
extern WORD PASCAL awe32ProgramChange(WORD, WORD);
extern WORD PASCAL awe32Controller(WORD, WORD, WORD);
extern WORD PASCAL awe32PolyKeyPressure(WORD, WORD, WORD);
extern WORD PASCAL awe32ChannelPressure (WORD, WORD);
extern WORD PASCAL awe32PitchBend (WORD, WORD, WORD);
extern WORD PASCAL awe32Sysex(WORD, LPBYTE, WORD);

/* Hardware support functions */
extern WORD PASCAL awe32Detect(WORD);
extern WORD PASCAL awe32InitHardware(void);

LOCAL WORD	wSBAddx   	= 0x220;
LOCAL BOOL 	fOverride	= FALSE;
LOCAL INT 	ready 		= 0;
LOCAL BYTE 	Map[ 16 ] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 9 };
LOCAL INT	awe_semaphore = 0;

/*--------------------------------------------------------------------------*
* AweNoteOff - Turn Note OFF
*---------------------------------------------------------------------------*/
PRIVATE VOID 
AweNoteOff(
	WORD		Channel,	// INPUT : Channel 					0..15
	WORD		Note		// INPUT : Note To Kill     		0..107
	)
{
	if ( ready )
	{
		awe32NoteOff( Map[ Channel ], Note, 127 );
	}
}


/*--------------------------------------------------------------------------*
* AweNoteOn - Turn Note ON
*---------------------------------------------------------------------------*/
PRIVATE VOID 
AweNoteOn(
 	WORD		Channel,	// INPUT : Channel to Note On		0..15
	WORD		Note,		// INPUT : Note To Play      		0..107
	WORD		Velocity	// INPUT : Velocity of Note			1..127
	)
{
	if ( ready )
	{
		awe32NoteOn( Map[ Channel ], Note, Velocity );
	}
}

/*--------------------------------------------------------------------------*
* AwePitchBend - Perform 
*---------------------------------------------------------------------------*/
PRIVATE VOID 
AwePitchBend(
	WORD		Channel,	// INPUT : Channel to Note On	  0..14
	WORD		Bend		// INPUT : Pitch Bend             0..255 127=Normal
	)
{
	if ( ready )
	{
		Bend <<= 6;
		awe32PitchBend( Map[ Channel ], ( Bend & 0x007F ), ( Bend >> 7 ) );
	}
}

/*--------------------------------------------------------------------------*
* AweCommand - Perform Command Logic
*---------------------------------------------------------------------------*/
PRIVATE VOID 
AweCommand(
	WORD		Channel,// INPUT : Channel           	  0..15
	WORD		CmdCode,// INPUT : Command Code
	WORD		CmdData	// INPUT : Command Related Data
	)
{
	if ( ! ready )
		return;

	switch ( CmdCode )
	{
		case CMD_CHANGE_PROGRAM:
			awe32ProgramChange( Map[ Channel ], CmdData );
			break;

		case CMD_BANK_SELECT:
			break;

		case CMD_MODULATION:
			awe32Controller( Map[ Channel ], 0x01, CmdData );
			break;

		case CMD_VOLUME:
			awe32Controller( Map[ Channel ], 0x07, CmdData );
			break;

		case CMD_PANPOT:
			awe32Controller( Map[ Channel ], 0x0A, CmdData );
			break;

		case CMD_EXPRESSION:
			awe32Controller( Map[ Channel ], 0x0B, CmdData );
			break;

		case CMD_REVERB_DEPTH:
			awe32Controller( Map[ Channel ], 0x5B, CmdData );
			break;

		case CMD_CHORUS_DEPTH:
			awe32Controller( Map[ Channel ], 0x5D, CmdData );
			break;

		case CMD_HOLD1:
			awe32Controller( Map[ Channel ], 0x40, CmdData );
			break;

		case CMD_SOFT:
			awe32Controller( Map[ Channel ], 0x43, CmdData );
			break;

		case CMD_ALL_SOUNDS_OFF:
			awe32Controller( Map[ Channel ], 0x78, CmdData );
			break;

		case CMD_RESET_CHANNEL:
			awe32Controller( Map[ Channel ], 0x79, CmdData );
			break;

		case CMD_ALL_NOTES_OFF:
			awe32Controller( Map[ Channel ], 0x7B, CmdData );
			break;

		case CMD_MONO:
		case CMD_POLY:
		default:
			break;
	}
}

/*--------------------------------------------------------------------------*
* AweReset
*---------------------------------------------------------------------------*/
PRIVATE VOID
AweReset(
	void
	)
{
	int 	i;

	for ( i = 0; i < 16; i++ )
	{
		AweCommand( i, CMD_ALL_SOUNDS_OFF, 0 );
		AweCommand( i, CMD_ALL_NOTES_OFF, 0 );
		AweCommand( i, CMD_RESET_CHANNEL, 0 );
	}
}

/*--------------------------------------------------------------------------*
* Init - Initialize Driver
*---------------------------------------------------------------------------*/
PRIVATE DMX_STATUS
AweInit(
	VOID
	)
{
	int			port;

	if ( fOverride == FALSE )
	{
		port = 0;
		if ( AWE_Detect( &port ) != 0 )
		{
			ready = DS_NOT_READY;
			return RS_BOARD_NOT_FOUND;
		}
	}
	else
	{
    	if ( awe32Detect( wSBAddx ) )
    	{
			ready = DS_NOT_READY;
			return RS_BOARD_NOT_FOUND;
    	}
	}
    if( awe32InitHardware() )
    {
		ready = DS_NOT_READY;
		return RS_BOARD_NOT_FOUND;
    }
    awe32SPad1 = awe32SPad1Obj;
    awe32SPad2 = awe32SPad2Obj;
    awe32SPad3 = awe32SPad3Obj;
    awe32SPad4 = awe32SPad4Obj;
    awe32SPad5 = awe32SPad5Obj;
    awe32SPad6 = awe32SPad6Obj;
    awe32SPad7 = awe32SPad7Obj;

    awe32InitMIDI();
	ready = DS_READY;
	return RS_OKAY;
}



/*--------------------------------------------------------------------------*
* AweDeInit - Reset Wave Blaster
*---------------------------------------------------------------------------*/
PRIVATE VOID 
AweDeInit(
	void
	)
{
	if ( ready == DS_READY )
	{
		AweReset();
		// aweTerminate();
	}
	ready = DS_NOT_READY;
}


/****************************************************************************
* AweEnterBusy - Entry into critical section of code, only if not busy.
*****************************************************************************/
PUBLIC int	// returns: 0 = OK, 1 = Busy
AweEnterBusy(
	void
	)
{
	++awe_semaphore;
	if ( awe_semaphore > 1 )
	{
		--awe_semaphore;
		return 1;
	}
	return 0;
}

/****************************************************************************
* AweLeave - Leave critical section of code.
*****************************************************************************/
PUBLIC void
AweLeave(
	void
	)
{
	if ( awe_semaphore )
		--awe_semaphore;
}

PUBLIC DMX_MUS AweMus = 
{
	AweInit,
	AweDeInit,
	AweNoteOff,
	AweNoteOn,
	AwePitchBend,
	AweCommand,
	AweEnterBusy,
	AweLeave
};


/****************************************************************************
* AWE_SetCard - Changes card properties.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*****************************************************************************/
PUBLIC VOID
AWE_SetCard(
    int         iPort           // INPUT : Port Address of Card
    )
{
	wSBAddx = iPort;
	fOverride = TRUE;
}

#define DSP_POLL_MAX_TRIES      200
#define CMD_DSP_GetVersion		0xE1

PRIVATE BOOL
AWE_Read(
	int		port,
    BYTE	*pByte
    )
{
	register int 	n;

	for ( n = DSP_POLL_MAX_TRIES; n; --n )
   	{
    	if ( inp( port + 0xE ) & 0x80 )
       	{
        	*pByte = ( BYTE ) inp( port + 0xA );
			return TRUE;
       	}
   	}
	return FALSE;
}

PRIVATE BOOL
AWE_Write (
	int		port,
    BYTE    bVal
    )
{
	register int 	n;

	for ( n = DSP_POLL_MAX_TRIES; n; --n )
   	{
    	if ( ! ( inp( port + 0x0C ) & 0x80 ) )
       	{
			outp( port + 0x0C, bVal );
			return TRUE;
		}
	}
	return FALSE;
}


PRIVATE int
AWE_DetectCard(
	int		port
	)
{
	BYTE    byte;

	outp( port + 6, 1 );
	TSM_Delay( 30 );
	outp( port + 6, 0 );
	TSM_Delay( 20 );
	if ( AWE_Read( port, &byte ) == TRUE && byte == 0xAA )
	{
		BYTE		major = 1;
		BYTE		minor = 0;
	/*
	* Get Card Version
	*/
		if ( ! AWE_Write( port, CMD_DSP_GetVersion ) )
			return 1;

		if ( ! AWE_Read( port, &major ) || ! AWE_Read( port, &minor ) )
			return 1;

		if ( major < 4 )
			return 1;
	/*
	* It's safe to do a aweDetect
	*/
		return awe32Detect( port );
   	}
   	return 1;
}

/****************************************************************************
* AWE_Detect - Detects AWE32 Card and returns type of MIDI
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = No AWE32 Port Found.
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
AWE_Detect(
    int         *iPort    	// IN/OUT: Port Address of Card 0=AutoDetect
    )
{
#define CMD_DSP_GetVersion			0xE1
	extern int	clGetBlasterEnv(char);
	LOCAL WORD 	Port[] = { 0x220, 0x240, 0x260 };
	int			j;

	if ( iPort != NULL && *iPort != 0 )
	{
		wSBAddx = *iPort;
    	if ( AWE_DetectCard( wSBAddx ) == 0 )
			return 0;
	}
	else
	{
		for ( j = 0; j < ASIZE( Port ); j++ )
		{
    		if ( AWE_DetectCard( Port[ j ] ) == 0 )
			{
				wSBAddx = Port[ j ];

				if ( *iPort )
					*iPort = wSBAddx;
				return 0;
			}
		}
	}
	return 1;
}
