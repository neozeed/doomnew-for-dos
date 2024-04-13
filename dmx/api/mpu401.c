/*===========================================================================
* MPU401.C - Creative Labs WAVE BLASTER 
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
* $Log:   F:/DMX/API/VCS/MPU401.C_V  $
*  
*     Rev 1.0   02 Oct 1993 15:03:26   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <io.h>
#include <conio.h>

#include "dmx.h"

/* Stack and pointer checking off */

#define MPU_NOT_READY		  		0x40
#define MPU_DATA_NOT_AVAIL	  		0x80
#define MPU_TIMEOUT			  		32000

#define MPU_ACK				  		0xFE
#define MPU_REQEST_PLAY_DATA 		0xF0

#define MPU_CMD_RESET		  		0xFF
#define MPU_CMD_PLAY_START	  		0x0A
#define MPU_CMD_UNKNOWN		  		0xC8
#define MPU_CMD_TRACKS		  		0xEC
#define MPU_CMD_CONDUCTOR_ON 		0x8F
#define MPU_CMD_CLR_PLAY_CTRS		0xB8
#define MPU_UART_MODE				0x3F

LOCAL int MpuDataPort 		= 0x300;
LOCAL int MpuStatusCmdPort	= 0x301;

LOCAL INT		MpuPlaying 			= 0;
LOCAL INT 		ready 				= 0;

LOCAL BYTE 		ChannelMap[ 16 ] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 9 };
LOCAL BYTE		cmd[ 30 ];
LOCAL BOOL 		fOverride = FALSE;
LOCAL INT		mpu_semaphore = 0;

LOCAL BYTE	reset_cmd[] =
{
	0xF0,	// Exclusive Communications
	0x41, 	// Manufactures ID (Roland)
	0x10,	// Device ID (Unit #17)
	0x42,	// Model ID (SCC-1)
	0x12,	// Command ID (DT1)
	0x40, 0x00, 0x7F, 0x00, // Reset All Parameters
	0x41, 	// Checksum
	0xF7	// End of Exclusive Communications
};

/*==========================================================================*
* MpuSendData - Send Data to MPU-401
*===========================================================================*/
PRIVATE VOID
MpuSendData(
	BYTE		data
	)
{
	int			j;

	for ( j = 0; j < MPU_TIMEOUT; j++ )
	{
		if ( ( inp( MpuStatusCmdPort ) & MPU_NOT_READY ) == 0 )
		{
			outp( MpuDataPort, data );
			break;
		}
	}
}

/*==========================================================================*
* MpuWriteUART - Write Data into MPU-401 Data Que.
*===========================================================================*/
PRIVATE VOID
MpuWriteUART(
	INT		Length		/* INPUT : Length to Send		*/
	)
{
	int		k;

	for ( k = 0; k < Length; k++ )
	{
		MpuSendData( cmd[ k ] );
	}
}

/*==========================================================================*
* MpuSendCmd - Send Command to MPU-401
*===========================================================================*/
PRIVATE VOID
MpuSendCmd(
	BYTE		cmd
	)
{
	int			j;
	BYTE		data;

	for ( j = 0; j < MPU_TIMEOUT; j++ )
	{
		if ( (  ( BYTE ) inp( MpuStatusCmdPort ) & MPU_NOT_READY ) == 0 )
			break;
	}
	if ( j == MPU_TIMEOUT )
		return;

	outp( MpuStatusCmdPort, cmd );
	for ( ;; )
	{
		for ( j = 0; j < MPU_TIMEOUT; j++ )
		{
			if ( (  ( BYTE ) inp( MpuStatusCmdPort ) & MPU_DATA_NOT_AVAIL ) == 0 )
				break;
		}
		if ( j == MPU_TIMEOUT )
			return;

		data = ( BYTE ) inp( MpuDataPort );
		if ( data == MPU_ACK )
			break;

		if ( data == MPU_REQEST_PLAY_DATA )
			MpuSendData( ( BYTE )( 0x2f ) );
	}
}

/*--------------------------------------------------------------------------*
* MpuNoteOff - Turn Note OFF
*---------------------------------------------------------------------------*/
PRIVATE VOID 
MpuNoteOff(
	WORD		Channel,	// INPUT : Channel 					0..15
	WORD		Note		// INPUT : Note To Kill     		0..107
	)
{
	BYTE		bPrevColor;

	if ( ! MpuPlaying )
		return;

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3f );
	}
#endif
	Channel = ( BYTE ) ChannelMap[ Channel ];
	cmd[ 0 ] = ( BYTE )( 0x80 | (BYTE) Channel );
	cmd[ 1 ] = ( BYTE ) Note;
	cmd[ 2 ] = 64;

	MpuWriteUART( 3 );

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
}


/*--------------------------------------------------------------------------*
* MpuNoteOn - Turn Note ON
*---------------------------------------------------------------------------*/
PRIVATE VOID 
MpuNoteOn(
 	WORD		Channel,	// INPUT : Channel to Note On		0..15
	WORD		Note,		// INPUT : Note To Play      		0..107
	WORD		Velocity	// INPUT : Velocity of Note		1..127
	)
{
	BYTE		bPrevColor;

	if ( ! MpuPlaying )
		return;

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3f );
	}
#endif
	Channel = ChannelMap[ Channel ];
	cmd[ 0 ] = ( BYTE )( 0x90 | (BYTE)Channel );
	cmd[ 1 ] = ( BYTE ) Note;
	cmd[ 2 ] = ( BYTE ) Velocity;
	MpuWriteUART( 3 );

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
}

/*--------------------------------------------------------------------------*
* MpuPitchBend - Perform 
*---------------------------------------------------------------------------*/
PRIVATE VOID 
MpuPitchBend(
	WORD		Channel,	// INPUT : Channel to Note On	  0..14
	WORD		Bend		// INPUT : Pitch Bend           0..255 127=Normal
	)
{
	WORD		wBend = ( WORD ) Bend << 6;
	BYTE		bPrevColor;

	if ( ! MpuPlaying )
		return;

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3f );
	}
#endif

	Channel = ChannelMap[ Channel ];
	cmd[ 0 ] = ( BYTE )( 0xE0 | (BYTE)Channel );
	cmd[ 1 ] = ( BYTE )( wBend & 0x007F );
	cmd[ 2 ] = ( BYTE )( ( wBend >> 7 ) & 0x7F );
	MpuWriteUART( 3 );

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
}


/*--------------------------------------------------------------------------*
* MpuCommand - Perform Command Logic
*---------------------------------------------------------------------------*/
PRIVATE VOID 
MpuCommand(
	WORD		Channel,// INPUT : Channel           	  0..15
	WORD		CmdCode,// INPUT : Command Code
	WORD		CmdData	// INPUT : Command Related Data
	)
{
	BYTE		bPrevColor;

	if ( ! MpuPlaying )
		return;

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3f );
	}
#endif

	Channel = ChannelMap[ Channel ];
	cmd[ 0 ] = ( BYTE )( 0xB0 | (BYTE) Channel );
	switch ( CmdCode )
	{
		case CMD_CHANGE_PROGRAM:
			cmd[ 0 ] = ( BYTE )( 0xC0 | (BYTE)Channel );
			cmd[ 1 ] = ( BYTE ) CmdData;
			MpuWriteUART( 2 );
			break;

		case CMD_BANK_SELECT:
			cmd[ 1 ] = 0x00;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_MODULATION:
			cmd[ 1 ] = 0x01;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_VOLUME:
			cmd[ 1 ] = 0x07;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_PANPOT:
			cmd[ 1 ] = 0x0A;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_EXPRESSION:
			cmd[ 1 ] = 0x0B;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_REVERB_DEPTH:
			cmd[ 1 ] = 0x5B;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_CHORUS_DEPTH:
			cmd[ 1 ] = 0x5D;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_HOLD1:
			cmd[ 1 ] = 0x40;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_SOFT:
			cmd[ 1 ] = 0x43;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_ALL_SOUNDS_OFF:
			cmd[ 1 ] = 0x78;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_RESET_CHANNEL:
			cmd[ 1 ] = 0x79;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
#if 0
			cmd[ 1 ] = 0x00;
			cmd[ 2 ] = 0;
			MpuWriteUART( 3 );

			cmd[ 1 ] = 0x0A;
			cmd[ 2 ] = 64;
			MpuWriteUART( 3 );

			cmd[ 1 ] = 0x5B;
			cmd[ 2 ] = 40;
			MpuWriteUART( 3 );

			cmd[ 1 ] = 0x5D;
			cmd[ 2 ] = 0;
			MpuWriteUART( 3 );
#endif
			break;

		case CMD_ALL_NOTES_OFF:
			cmd[ 1 ] = 0x7B;
			cmd[ 2 ] = ( BYTE ) CmdData;
			MpuWriteUART( 3 );
			break;

		case CMD_MONO:
		case CMD_POLY:
		default:
			break;
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
}

/*--------------------------------------------------------------------------*
* MpuReset
*---------------------------------------------------------------------------*/
PRIVATE VOID
MpuReset(
	void
	)
{
	LOCAL BYTE	go_genmidi[] =
	{
		0xF0,	// Exclusive Communications
		0x7E, 	// "non real time"           
		0x7F,	// Device ID (ALL)
		0x09,	// sub ID #1 = General MIDI message
		0x01,	// sub ID #2 = General MIDI ON
		0xF7	// End of Exclusive Communications
	};

	if ( MpuPlaying )
	{
//		memcpy( cmd, reset_cmd, sizeof( reset_cmd ) );
//		MpuWriteUART( sizeof( reset_cmd ) );

		memcpy( cmd, go_genmidi, sizeof( go_genmidi ) );
		MpuWriteUART( sizeof( go_genmidi ) );
	}
}

#if 0
/*--------------------------------------------------------------------------*
* IsSCC1 - Can't get this working....(yet)
*---------------------------------------------------------------------------*/
PRIVATE int	// Returns 1=Yes, 2=No
IsSCC1(
	void
	)
{
	LOCAL BYTE	req_vol[] =
	{
		0xF0,	// Exclusive Communications
		0x41, 	// Manufactures ID (Roland)
		0x10,	// Device ID (Unit #17)
		0x42,	// Model ID (SCC-1)
		0x11,	// Command ID (RQ1)
		0x40, 0x00, 0x00, // Get Master Volume
		0x00, 0x00, 0x03, // Size
		0x3D, 	// Checksum
		0xF7	// End of Exclusive Communications
	};
	int		j;
	BYTE	msg[ 200 ];
	int		mlen = 0;
	BYTE	data = 0;

	for ( j = 0; j < 2000; j++ )
		j++;
	memcpy( cmd, req_vol, sizeof( req_vol ) );
	MpuWriteUART( sizeof( req_vol ) );
/*
* Wait for Sysex Event
*/
	while ( data != 0xF7 && mlen < sizeof( msg ) && ! kbhit() )
	{
		for ( j = 0; j < MPU_TIMEOUT; j++ )
		{
			if ( (  ( BYTE ) inp( MpuStatusCmdPort ) & MPU_DATA_NOT_AVAIL ) == 0 )
				break;
		}
		if ( j == MPU_TIMEOUT )
			return 2;

		data = ( BYTE ) inp( MpuDataPort );
//		printf( "IsSCC1: %02xh\n", data );
		if ( mlen == 0 && data != 0xF0 )
			continue;

		msg[ mlen++ ] = data;
	}
	return 1;
}
#endif

/*==========================================================================*
* MpuBoardInstalled - Detect if card is present
*===========================================================================*/
PRIVATE INT	/* Returns: 1=Installed, 0=Can't find card.	*/
MpuBoardInstalled(
	VOID
	)
{
	int		j;

	inp( MpuDataPort );
	outp( MpuStatusCmdPort, MPU_CMD_RESET );

	for ( j = 0; j < MPU_TIMEOUT; j++ )
	{
		if ( (  ( BYTE ) inp( MpuStatusCmdPort ) & MPU_NOT_READY ) == 0 )
			break;
	}
	if ( j < MPU_TIMEOUT )
	{
		outp( MpuStatusCmdPort, MPU_CMD_RESET );
		for ( j = 0; j < MPU_TIMEOUT; j++ )
		{
			if ( ( ( BYTE ) inp( MpuStatusCmdPort ) & MPU_DATA_NOT_AVAIL ) == 0 )
			{
				if ( ( BYTE ) inp( MpuDataPort ) == MPU_ACK )
				{
					return TRUE;
				}
			}
		}
		return TRUE;
	}
	return FALSE;
}


/*--------------------------------------------------------------------------*
* Init - Initialize Driver
*---------------------------------------------------------------------------*/
PRIVATE DMX_STATUS
MpuInit(
	VOID
	)
{
	int			port;

	if ( fOverride == FALSE )
	{
		port = 0;
		if ( MPU_Detect( &port, NULL ) != 0 )
		{
			ready = DS_NOT_READY;
			return RS_BOARD_NOT_FOUND;
		}
	}
	else
	{
		if ( ! MpuBoardInstalled() )
		{
			ready = DS_NOT_READY;
			return RS_BOARD_NOT_FOUND;
		}
	}
/*
* Clear Port
*/
	inp( MpuDataPort );

	MpuSendCmd( ( BYTE ) MPU_UART_MODE );
	MpuPlaying = 1;
	MpuReset();

	ready = DS_READY;
	return RS_OKAY;
}



/*--------------------------------------------------------------------------*
* DeInit - Reset Wave Blaster
*---------------------------------------------------------------------------*/
PRIVATE VOID 
MpuDeInit(
	void
	)
{
	INT		channel;
	LOCAL BYTE	stop_genmidi[] =
	{
		0xF0,	// Exclusive Communications
		0x7E, 	// "non real time"           
		0x7F,	// Device ID (ALL)
		0x09,	// sub ID #1 = General MIDI message
		0x02,	// sub ID #2 = General MIDI OFF
		0xF7	// End of Exclusive Communications
	};

	if ( MpuPlaying )
	{
//		memcpy( cmd, reset_cmd, sizeof( reset_cmd ) );
//		MpuWriteUART( sizeof( reset_cmd ) );

		cmd[ 2 ] = 0;
		for ( channel = 0; channel < 16; channel++ )
		{
			cmd[ 0 ] = ( BYTE )( 0xB0 | channel );
			cmd[ 1 ] = 0x78;
			MpuWriteUART( 3 );

			cmd[ 1 ] = 0x7B;
			MpuWriteUART( 3 );

			cmd[ 1 ] = 0x79;
			MpuWriteUART( 3 );
		}
		memcpy( cmd, stop_genmidi, sizeof( stop_genmidi ) );
		MpuWriteUART( sizeof( stop_genmidi ) );

		MpuSendCmd( ( BYTE ) MPU_CMD_RESET );
		MpuPlaying = 0;
	}
	ready = DS_NOT_READY;
}


/****************************************************************************
* MpuEnterBusy - MPU Entry into critical section of code, only if not busy.
*****************************************************************************/
PUBLIC int	// returns: 0 = OK, 1 = Busy
MpuEnterBusy(
	void
	)
{
	++mpu_semaphore;
	if ( mpu_semaphore > 1 )
	{
		--mpu_semaphore;
		return 1;
	}
	return 0;
}

/****************************************************************************
* MpuLeave - Leave critical section of code.
*****************************************************************************/
PUBLIC void
MpuLeave(
	void
	)
{
	if ( mpu_semaphore )
		--mpu_semaphore;
}

PUBLIC DMX_MUS MpuMus = 
{
	MpuInit,
	MpuDeInit,
	MpuNoteOff,
	MpuNoteOn,
	MpuPitchBend,
	MpuCommand,
	MpuEnterBusy,
	MpuLeave
};


/****************************************************************************
* MPU_SetCard - Changes card properties.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*****************************************************************************/
PUBLIC VOID
MPU_SetCard(
    int         iPort           // INPUT : Port Address of Card
    )
{
	MpuDataPort 		= iPort;
	MpuStatusCmdPort 	= iPort + 1;
	fOverride = TRUE;
}

/****************************************************************************
* MPU_Detect - Detects MPU-401 Card and returns type of MIDI
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = No MPU-401 Port Found.
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
MPU_Detect(
    int         *iPort,    	// IN/OUT: Port Address of Card 0=AutoDetect
	int			*cType		// OUTPUT: Card Type: 0=GENMIDI 1=SCC-1
    )
{
	LOCAL int   Port[] = { 0x330, 0x300 };
	int			j;

	if ( cType )
		*cType = 0;

	if ( iPort != NULL && *iPort != 0 )
	{
		MpuDataPort = *iPort;
		MpuStatusCmdPort = *iPort + 1;
    	if ( MpuBoardInstalled() )
	    	return 0;
	}
	else
	{
		for ( j = 0; j < ASIZE( Port ); j++ )
		{
			MpuDataPort = Port[ j ];
			MpuStatusCmdPort = Port[ j ] + 1;
    		if ( MpuBoardInstalled() )
			{
				if ( iPort )
					*iPort = Port[ j ];
	    		return 0;
			}
		}
	}
	return 1;
}
