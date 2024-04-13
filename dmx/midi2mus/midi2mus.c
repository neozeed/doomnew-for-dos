/*===========================================================================
* MIDI2MUS.C - MIDI to MUS format procesor
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
* $Log:   F:/DMX/MIDI2MUS/VCS/MIDI2MUS.C_V  $
*  
*     Rev 1.1   29 Oct 1993 00:02:36   pjr
*  Fixed quantinization 
*  Changed default from 280 ticks to 140 ticks per sec.
*  
*  
*     Rev 1.0   01 Sep 1993 16:44:10   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <malloc.h>

#include "dmx.h"
#include "dmxmus.h"

#define MID_PITCH			0x80

/* Some MIDI codes. */
#define MIDI_STATUS        	0x80
#define MIDI_NR_CHANS      	16
#define MIDI_SYSEX_F0      	0xf0
#define MIDI_SYSEX_F7      	0xf7

#define MIDI_META          	0xff
#define MIDI_TRACK_NAME	   	0x03
#define MIDI_END_OF_TRACK  	0x2f
#define MIDI_TEMPO         	0x51
#define MIDI_SMPTE		   	0x54
#define MIDI_TIME_SIG	   	0x58

#define MIDI_SEQ_SPECIFIC  	0x7f

#define MAX_VOICE_SIZE	   	16380
#define MAX_TRACK_SIZE     	65000
#define MAX_INSTRUMENTS	   	1000
#define MAX_MAP_SIZE	   	128
#define MAX_VOICES		   	16

typedef struct
{
	BYTE		BanksUsed;
	BYTE		Bank[ 15 ];
} PATCH_DESC;

LOCAL PATCH_DESC	PatchesUsed[ 189 ];

typedef struct 
{
	LONG		lSize;
	LONG		lTime;
	BYTE		*bData;
	BYTE		bStatus;
} TRACK;

typedef struct
{
	short		Notes[ 10 ];
	BYTE		bVelocity;
	WORD		wBend;
	WORD		wLastBend;
	BOOL		fBendQueued;
	BOOL		fInBend;
	BYTE		bNote;
	BYTE		bProgram;
	BYTE		bBank;
	BYTE		bInstrument;
	BYTE		bMappedVoice;
	LONG		lLastEventTime[ 10 ];
	LONG		lLastBend;
	BOOL		fUsed;
} VOICE;

LOCAL VOICE			Voice[ MAX_VOICES ];
LOCAL DMX_HEADER	RtmHdr;
LOCAL BYTE			bInstUsed[ 100 ];

LOCAL BYTE		*bRTM;
LOCAL BYTE		*bCtrlByte;
LOCAL WORD		wRTMSize;
LOCAL WORD		wRTMUsed;
LOCAL WORD		wTickRate = 140;
LOCAL BYTE		PercVoice = 9;
LOCAL BOOL		fVerbose = FALSE;
LOCAL BOOL		fSecMood = FALSE;
LOCAL BOOL 		fQuiet = FALSE;

LOCAL double	fTimeCorrection = 1.0;
LOCAL LONG		lLastMidiEvent = 0L;		// Timestamp of last event
LOCAL LONG		lRunningTime = 0L;
LOCAL LONG		lRunningTimeQ = 0L;
LOCAL LONG		lLastTempoChange = 0L;
LOCAL LONG		lLastTempoChangeQ = 0L;
LOCAL LONG		lDelay = 0;
LOCAL LONG		lTicksPerMeasure;
LOCAL WORD		wQTicks;


/*---------------------------------------------------------------------------
* MarkUsed - Mark Patch as being 'USED'
*---------------------------------------------------------------------------*/
PRIVATE VOID
MarkUsed(
	BYTE		Channel,// Channel 
	BYTE		Note,	// Note Played
	BYTE		Bank,	// Bank for Patch
	BYTE		Patch	// Patch
	)
{
	BYTE		j;

	if ( Channel == 15 )
	{
		Bank = Patch;
		Patch = Note + 100;
	}
	for ( j = 0; j < PatchesUsed[ Patch ].BanksUsed && j < 15; j++ )
	{
		if ( PatchesUsed[ Patch ].Bank[ j ] == Bank )
			return;
	}
	if ( j < 15 )
	{
		PatchesUsed[ Patch ].Bank[ j ] = Bank;
		PatchesUsed[ Patch ].BanksUsed++;
	}
}		

/*-------------------------------------------------------------------------
* GetWord - Read WORD from MIDI Stream.
*------------------------------------------------------------------------*/
PRIVATE WORD 
GetWord(
	int		midi_fh		// INPUT : Midi File Handle
	)
{
	WORD			num;
	BYTE			lsbmsb[ 2 ];

	read( midi_fh, lsbmsb, 2 );
	num = lsbmsb[ 0 ];
	num = ( num << 8 ) + lsbmsb[ 1 ];
	return num;
}


/*-------------------------------------------------------------------------
* GetLong - Read LONG value from MIDI Stream.
*------------------------------------------------------------------------*/
PRIVATE LONG 
GetLong(
	int		midi_fh		// INPUT : Midi File Handle
	)
{
	LONG		num = 0L;
	WORD		i;
	BYTE		longval[ 4 ];

	read( midi_fh, longval, 4 );
	for ( i = 0; i < 4; i++ )
	{
		num = ( num << 8 ) + longval[ i ];
	}
	return num;
}

/*-------------------------------------------------------------------------
*   Reads a variable length value from the MIDI file data and advances the
   data pointer.
*-------------------------------------------------------------------------*/
PRIVATE LONG 
GetVarNum(
	int		midi_fh		// INPUT : Midi File Handle
	)
{
	LONG		value;
	BYTE		vlb;

	read( midi_fh, &vlb, 1 );
	if ( ( value = vlb ) & 0x80 )
	{
		value &= 0x7f;
		do
		{
			read( midi_fh, &vlb, 1 );
			value = ( value << 7 ) + ( vlb & 0x7f );
		} while ( vlb & 0x80 );
	}
	return value;
}

/*-------------------------------------------------------------------------
* GetEventLength - Read LONG value from MIDI Stream.
*------------------------------------------------------------------------*/
PRIVATE WORD 
GetEventLength(
   BYTE     	*bData,     // INPUT : MIDI Data Stream
   LONG     	*lLength    // OUTPUT: Event Length
   )
{
   LONG     lValue = 0L;
   WORD     wBytesUsed = 1;
   BYTE     bByte;

   if ( ( lValue = ( LONG )( *bData++ ) ) & 0x80 )
   {
      lValue &= 0x7F;
      do
      {
         lValue = ( lValue << 7 ) + ( ( bByte = *bData++ ) & 0x7F );
         ++wBytesUsed;
      }
      while ( bByte & 0x80 );
   }
   *lLength = lValue;
   return wBytesUsed;
}

/*---------------------------------------------------------------------------
* MidiSystemExclusiveEvent - Processes a Midi System Exclusive Event
*---------------------------------------------------------------------------*/
PRIVATE WORD 
MidiSystemExclusiveEvent(
   BYTE        bMidiStatus,   // INPUT : Running Midi Status
   BYTE        **bData        // INPUT : Midi Data Ptr
   )
{
   BYTE        *bMidiData = *bData;
   WORD        wBytesUsed = 0;
   LONG        lEventLength;

	wBytesUsed = GetEventLength( bMidiData, &lEventLength );
	if ( fVerbose )
		printf( "MIDI SYSTEM EXCLUSIVE EVENT - LEN: %d", wBytesUsed );
	wBytesUsed += ( WORD ) lEventLength;

	*bData = ( bMidiData + wBytesUsed );
	return wBytesUsed;
}

/*---------------------------------------------------------------------------
* SetTempo - Change the tempo for all tracks.
*---------------------------------------------------------------------------*/
PRIVATE VOID 
SetTempo(
	LONG		luSec			// INPUT : Micro Seconds per 1 Q-Note Tick
	)
{
	LONG		TicksPerSec;
	double		fPrevCorrection;

	if ( fVerbose )
		printf( "\nSetTempo: %d Ticks (%ldæs) per Quarter Note.", wQTicks, luSec );

	TicksPerSec = 1000000L / ( luSec / wQTicks );
	lLastTempoChange  = lRunningTime;
	lLastTempoChangeQ = lRunningTimeQ;
	fTimeCorrection = ( double ) wTickRate / ( double ) TicksPerSec;
	if ( fVerbose )
		printf( " \nTPS: %ld, Corrective: %f ", TicksPerSec, fTimeCorrection );
}

/*---------------------------------------------------------------------------
* MidiMetaEvent - Processes a Midi Meta Event
*---------------------------------------------------------------------------*/
PRIVATE WORD 
MidiMetaEvent(
	BYTE 		*bMidiStatus,  // INPUT : Running Midi Status
	BYTE 		**bData        // INPUT : Midi Data Ptr
	)
{
	CHAR		szText[ 128 + 1 ];
	BYTE		*bMidiData = *bData;
	WORD		wBytesUsed;
	LONG		lEventLength;
	LONG		lClocksPerMeasure;
	BYTE		bEventType;

	bEventType = *bMidiData;
	wBytesUsed = GetEventLength( bMidiData + 1, &lEventLength ) + 1;
	bMidiData += wBytesUsed;
	wBytesUsed += ( WORD ) lEventLength;

	if ( bEventType >= 0x01 && bEventType <= 0x0f )
	{
		memcpy( szText, bMidiData, ( WORD ) lEventLength );
		szText[ lEventLength ] = '\0';
		if ( fVerbose )	printf( "Meta %02x: %s", bEventType, szText );
	}
	else switch ( bEventType )
	{
		case MIDI_END_OF_TRACK:
			if ( fVerbose )
				printf( "\n**** END OF TRACK ****\n " );
			*bMidiStatus = bEventType;
			break;

		case MIDI_TEMPO:
		{
			LONG		luSec = 0L;
		
			while( lEventLength > 0 )
			{
				lEventLength--;
				luSec <<= 8;
				luSec += ( LONG )( *bMidiData++ );
			}
			SetTempo( luSec );
			break;
		}

		case MIDI_SMPTE:
		{
			if ( fVerbose )
			{
				printf( "SMPTE:  %2d:%02d:%02d  %3d.%d",
					bMidiData[ 0 ],
					bMidiData[ 1 ],
					bMidiData[ 2 ],
					bMidiData[ 3 ],
					bMidiData[ 4 ] );
			}
			break;
		}

		case MIDI_TIME_SIG:
		{
			WORD	dnom[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
			WORD	bpm;

			if ( bMidiData[ 1 ] < ASIZE( dnom ) )
			{
				bpm = (WORD)( bMidiData[ 0 ] ) * 256;
				lClocksPerMeasure = ( ( bpm / dnom[ bMidiData[ 1 ] ] ) * 96 ) / 256;
				lTicksPerMeasure = lClocksPerMeasure * wQTicks / 24;
				if ( fVerbose )
				{
					printf( "Time Signature  %d/%d Time  %ld Clocks  %ld Ticks\n",
						bMidiData[ 0 ],
						dnom[ bMidiData[ 1 ] ],
						lClocksPerMeasure,
						lTicksPerMeasure
						);
				}
			}
			else
			{
				printf( "Denom to large in time signature.\n" );
			}
			break;
		}

		default:
			if ( fVerbose )	printf( "Ignoring Meta Event Type: %02x, Len:%ld", bEventType, lEventLength );
			break;
	}
	bMidiData += lEventLength;
	*bData = bMidiData;
	return wBytesUsed;
}

/*---------------------------------------------------------------------------
* RtmWrite - Write a byte into the RTM Buffer
*---------------------------------------------------------------------------*/
PRIVATE VOID 
RtmWrite(
   BYTE 				bByte  		// INPUT : Byte to Write
	)
{
	if ( wRTMUsed >= wRTMSize )
	{
		printf( "\nERROR: MUS Buffer full.\n" );
		exit( 20 );
	}
	*( bRTM + wRTMUsed++ ) = bByte;
}

/*---------------------------------------------------------------------------
* RtmWriteDelay - Write Long Delay
*---------------------------------------------------------------------------*/
PRIVATE VOID 
RtmWriteVoiceDelay(
	VOID
	)
{
	BYTE				bDelayBytes[ 5 ];
	WORD				wBytes = 0;

	if ( bCtrlByte == NULL )
	{
		bCtrlByte = bRTM;
		lDelay = 0;
	}
	if ( lDelay )
	{
		lLastMidiEvent = lRunningTimeQ;
		*bCtrlByte |= CB_TIME;
		while ( lDelay )
		{
			bDelayBytes[ wBytes ] = ( BYTE )( lDelay & 0x0000007FL );
			lDelay >>= 7;
			if ( wBytes )
				bDelayBytes[ wBytes ] |= TB_EXTENDED;
			wBytes++;
		}
		while ( wBytes > 0 )
		{
			RtmWrite( bDelayBytes[ --wBytes ] );
		}
	}
	bCtrlByte = bRTM + wRTMUsed;
}

/*---------------------------------------------------------------------------
* RtmGetDelay - Return Elapsed Delay Ticks
*---------------------------------------------------------------------------*/
PRIVATE LONG 
RtmGetDelay(
	VOID
	)
{
/*
	LONG				lDelay;
	LONG				lPastTime;

	if ( bCtrlByte == NULL )
		lPastTime = lRunningTime;
	else
		lPastTime = lLastTime;

	lDelay = lRunningTime - lPastTime;
	lDelay = ( LONG )( ( double ) lDelay / fTimeCorrection );
*/
	return lDelay;
}

/*---------------------------------------------------------------------------
* NewVoice - Assign new Voice to List
*---------------------------------------------------------------------------*/
PRIVATE VOID
NewVoice(
	VOICE			*v,			// INPUT : Free Voice
	BYTE			MidiVoice	// INPUT : Midi Voice to Map
	)
{
	short			j;

	for ( j = 0; j < 10; j++ )
		v->Notes[ j ] = -1;

	v->fUsed = TRUE;
  	v->bProgram = 255;
	if ( MidiVoice == 9 || MidiVoice == 15 )
	{
		if ( fSecMood == FALSE )
			v->bMappedVoice = 15;
		else
			v->bMappedVoice = MidiVoice;
	}
	else if ( MidiVoice < 9 || fSecMood == FALSE )
	{
		v->bMappedVoice = ( BYTE ) RtmHdr.wPrimaryChannels++;
	}
	else
	{
		v->bMappedVoice = 10 + ( BYTE ) RtmHdr.wSecondaryChannels++;
	}
}

/*---------------------------------------------------------------------------
* NoteOff - Turn Note Off
*---------------------------------------------------------------------------*/
PRIVATE VOID
NoteOff(
	VOICE		*v,		// INPUT : Voice
	BYTE		Note		// INPUT : Note to Turn OFF
	)
{
	short		j;

	if ( v->fUsed )
	{
		for ( j = 0; j < 10; j++ )
		{
			if ( ( BYTE ) ( v->Notes[ j ] ) == Note && v->lLastEventTime[ j ] != lRunningTimeQ )
			{
				if ( fVerbose )	printf( "NOTE OFF: %d  ", Note );
				RtmWriteVoiceDelay();
				RtmWrite( ( BYTE )( v->bMappedVoice | ( CS_NOTE_OFF << 4 ) ) );
				RtmWrite( ( BYTE )( Note ) );
				v->Notes[ j ] = -1;
			}
		}
	}
}


/*---------------------------------------------------------------------------
* NoteOn - Turn Note On
*---------------------------------------------------------------------------*/
PRIVATE VOID
NoteOn(
	VOICE		*v,		// INPUT : Voice
	BYTE		Note,	// INPUT : Note to Turn On
	BYTE		Volume	// INPUT : Volume Level
	)
{
	short		j;
	short		notes_on;
	short		empty_slot;

	if ( v->bProgram == 255 )
	{
		v->bProgram = 0;
		v->bBank = 0;

		MarkUsed( v->bMappedVoice, Note, 0, 0 );
		RtmWriteVoiceDelay();
		RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
		RtmWrite( ( BYTE ) ( CMD_CHANGE_PROGRAM ) );
		RtmWrite( ( BYTE ) ( v->bProgram ) );
	}
	else
	{
		MarkUsed( v->bMappedVoice, Note, v->bBank, v->bProgram );
	}
	notes_on = 0;
	for ( j = 9; j >= 0; j-- )
	{
		if ( v->Notes[ j ] == -1 )
			empty_slot = j;
		else
			notes_on++;
	}
	if ( notes_on == 10 )
		return;

	notes_on++;
	if ( fVerbose )	printf( "NOTE (%d): %d   VELOCITY: %d", notes_on, Note, Volume );
	RtmWriteVoiceDelay();
	RtmWrite( v->bMappedVoice );
	*bCtrlByte |= ( CS_NOTE_ON << 4 );
	v->Notes[ empty_slot ] = v->bNote = Note;
	v->lLastEventTime[ empty_slot ] = lRunningTimeQ;

	if ( v->bVelocity != Volume )
	{
		if ( fVerbose ) printf( "*" );
		RtmWrite( ( BYTE )( Note | NB_VELOCITY ) );
		RtmWrite( Volume );
		v->bVelocity = Volume;
	}
	else
	{
		RtmWrite( ( BYTE )( Note ) );
	}
}

PRIVATE VOID
CheckBend(
	VOICE		*v
	)
{
	if ( v->fInBend )
	{
		v->fInBend = FALSE;
		if ( v->fBendQueued )
		{
			v->fBendQueued = FALSE;
			v->wBend = v->wLastBend;
			if ( fVerbose )	printf( "*8-Bit: %d  ", v->wBend );
			RtmWriteVoiceDelay();
			RtmWrite( ( BYTE )( v->bMappedVoice | ( CS_PITCH_BEND << 4 ) ) );
			RtmWrite( ( BYTE ) v->wBend );
		}
	}
}

/*---------------------------------------------------------------------------
* MidiEvent - Processes a Midi Event
*---------------------------------------------------------------------------*/
PRIVATE WORD 
MidiEvent(
   BYTE        bMidiStatus,  	// INPUT : Running Midi Status
   BYTE        **bData        // INPUT : Midi Data Ptr
   )
{
	static WORD 	wEventLength[ 7 ] = { 2, 2, 2, 2, 1, 1, 2 };
	BYTE			*bMidiData = *bData;
	BYTE			bVoice = bMidiStatus & 0x0F;
	VOICE			*v = &Voice[ bVoice ];

	if ( fVerbose )	printf( "³ CHANNEL:%2d ³ %2d ³ ", bVoice, ( v->fUsed ) ? v->bMappedVoice : -1 );

	bMidiStatus = ( bMidiStatus >> 4 ) & 0x07;
	switch ( bMidiStatus )
	{
		case 0:	/* NOTE OFF */
			CheckBend( v );
			NoteOff( v, *bMidiData );
			break;

		case 1:	/* NOTE ON */
			if ( *( bMidiData + 1 ) == 0 )		/* 0 Volume = Note OFF */
			{
				CheckBend( v );
				NoteOff( v, *bMidiData );
			}
			else
			{
				if ( ! v->fUsed )
					NewVoice( v, bVoice );
				else
				{
					NoteOff( v, *bMidiData );
				}
				CheckBend( v );
				NoteOn( v, *bMidiData, *( bMidiData + 1 ) );
			}
			break;

		case 2:	/* POLYPHONIC KEY PRESSURE */
			CheckBend( v );
			if ( fVerbose )	printf( "POLYPHONIC KEY PRESSURE: %d, %d", *bMidiData, *( bMidiData + 1 ) );
			break;

		case 3:	/* CONTROL CHANGE */
			if ( fVerbose )	printf( "CONTROL CHANGE  %02x, %02x ", *bMidiData, *( bMidiData + 1 ) );
			if ( ! v->fUsed )
			{
				NewVoice( v, bVoice );
			}
			CheckBend( v );
			switch ( *bMidiData )
			{
				case 0x00:	/* Bank Select */
				case 0x20:
					if ( fVerbose ) printf( "BANK SELECT" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_BANK_SELECT ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x01:	/* Modulation	*/
					if ( fVerbose ) printf( "MODULATION" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_MODULATION ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x07:	/* Volume */
					if ( fVerbose ) printf( "VOLUME" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_VOLUME ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x0a:	/* Pan/Balance */
					if ( fVerbose ) printf( "PANPOT" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_PANPOT ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x0b:	/* Expression */
					if ( fVerbose ) printf( "EXPRESSION" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_EXPRESSION ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x40:	/* Hold Off/On */
					if ( fVerbose ) printf( "HOLD1" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_HOLD1 ) );
					if ( *( bMidiData + 1 ) < 0x40 )
						RtmWrite( 0 );
					else
						RtmWrite( 0x7F );
					break;

				case 0x43:	/* Soft Off/On */
					if ( fVerbose ) printf( "SOFT" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_SOFT ) );
					if ( *( bMidiData + 1 ) < 0x40 )
						RtmWrite( 0 );
					else
						RtmWrite( 0x7F );
					break;

				case 0x5b:	/* Reverb Depth */
					if ( fVerbose ) printf( "REVERB DEPTH" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_REVERB_DEPTH ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x5d:	/* Chorus Depth */
					if ( fVerbose ) printf( "CHORUS DEPTH" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_CHORUS_DEPTH ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x78:	/* All Sounds Off */
					if ( fVerbose ) printf( "ALL SOUNDS OFF" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_ALL_SOUNDS_OFF ) );
					break;

				case 0x79: 	/* Reset All Controllers */
					if ( fVerbose ) printf( "RESET ALL CONTROLLERS" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_RESET_CHANNEL ) );
					break;

				case 0x7B:	/* All Notes Off */
					if ( fVerbose ) printf( "ALL NOTES OFF" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_ALL_NOTES_OFF ) );
					break;

				case 0x7E:	/* Mono Mode */
					if ( fVerbose ) printf( "MONO" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_MONO ) );
					break;

				case 0x7F:	/* Poly Mode */
					if ( fVerbose ) printf( "POLY" );
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_POLY ) );
					break;
			}
			break;

		case 4:	/* PROGRAM CHANGE */
			if ( ! v->fUsed )
			{
				NewVoice( v, bVoice );
			}
			CheckBend( v );
			if ( v->bProgram != *bMidiData )
			{
				if ( fVerbose )	printf( "PROGRAM CHANGE: %d", *bMidiData );
//				if ( v->bMappedVoice == 15 )
//					*bMidiData = 0;

				v->bProgram = *bMidiData;
				RtmWriteVoiceDelay();
				RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
				RtmWrite( ( BYTE ) ( CMD_CHANGE_PROGRAM ) );
				RtmWrite( ( BYTE ) ( v->bProgram ) );
			}
			break;

		case 5:	/* CHANNEL PRESSURE */
			CheckBend( v );
			if ( fVerbose )	printf( "CHANNEL PRESSURE: %d, %d", *bMidiData, *( bMidiData + 1 ) );
			break;

		case 6:	/* PITCH BEND CHANGE */
		{
			WORD		wPitch = ( WORD )( *( bMidiData + 1 ) << 7 | *bMidiData );

			if ( fVerbose ) printf( "PITCH BEND CHANGE: %d", wPitch );

			if ( ! v->fUsed )
				break;

			wPitch >>= 6;
			if ( v->fInBend == FALSE ||
				( v->wBend != wPitch && ( lRunningTimeQ - v->lLastBend ) > 0 ) )
			{
				v->fInBend = TRUE;
				v->fBendQueued = FALSE;
				v->wBend = wPitch;
				v->lLastBend = lRunningTimeQ;
				if ( fVerbose )	printf( " 8-Bit: %d ", v->wBend );
				RtmWriteVoiceDelay();
				RtmWrite( ( BYTE )( v->bMappedVoice | ( CS_PITCH_BEND << 4 ) ) );
				RtmWrite( ( BYTE ) v->wBend );
			}
			else
			{
				v->fBendQueued = TRUE;
				v->wLastBend = wPitch;
			}
			break;
		}

		case 7: /* System Realtime Message */
			if ( fVerbose ) 	printf( "REALTIME MESSAGE" );
			CheckBend( v );
			break;
	}
	*bData += wEventLength[ bMidiStatus ];
	return wEventLength[ bMidiStatus ];
}

/*-------------------------------------------------------------------------
* GetDelay - Get Event Delay based upon Current Tempo.
*------------------------------------------------------------------------*/
PRIVATE WORD 
GetDelay(
	BYTE        **bData,		// INPUT : Midi Data Ptr
	LONG		*lTicks			// OUTPUT: Delay Ticks
	)
{
	WORD		wBytesUsed;

	wBytesUsed = GetEventLength( *bData, lTicks );
	*bData += wBytesUsed;
	return wBytesUsed;
}

PRIVATE VOID
CalcDelay(
	LONG			lTime
	)
{
	double			fCorrect;
	LONG			lTicks;

	lRunningTime = lTime;
	lTicks = lTime - lLastTempoChange;
	fCorrect = ( double ) lTicks * fTimeCorrection;
	lRunningTimeQ = ( long ) fCorrect + lLastTempoChangeQ;
	lDelay = lRunningTimeQ - lLastMidiEvent;

	if ( fVerbose )
	{
		printf( "\nRT:%5ld RTQ:%5ld DLY:%3ld  ", lRunningTime,
			lRunningTimeQ, lDelay );
	}
}

/*---------------------------------------------------------------------------
* GetNextDelay - Get Next Delay in order of Tracks
*---------------------------------------------------------------------------*/
PRIVATE TRACK * 
GetNextDelay(
	TRACK			*Track,
	TRACK			*Tracks,
	WORD			wTracks
	)
{
	WORD			wTrack;
	LONG			lTicks;

	if ( Track )
	{
		if ( Track->bStatus == MIDI_END_OF_TRACK )
		{
			lRunningTime = Track->lTime;
			Track->lTime = 0x7FFFFFFFL;
		}
		else
		{
			GetDelay( &( Track->bData ), &lTicks );
			Track->lTime += lTicks;
		}
	}
/*
*	Find earliest time in array.  This determines which track
*	contains the next event.
*/
	for ( Track = Tracks, wTrack = 1; wTrack < wTracks; wTrack++ )
	{
		if ( ( Tracks + wTrack )->lTime < Track->lTime &&
			  ( Tracks + wTrack )->bStatus != MIDI_END_OF_TRACK
			)
		{
			Track = Tracks + wTrack;
		}
	}
	if ( Track->bStatus == MIDI_END_OF_TRACK )
	{
		if ( fVerbose )
			printf( "\n=======  END OF TRACK =======\n" );
		if ( ( lRunningTime % lTicksPerMeasure ) != 0 )
		{
			lRunningTime += (lTicksPerMeasure - (lRunningTime % lTicksPerMeasure) );
		}
		CalcDelay( lRunningTime );
		return NULL;
	}
	CalcDelay( Track->lTime );
	return Track;
}

/*---------------------------------------------------------------------------
* ConvertMidiFile - Converts Midi File to Game Music Format (RTM)
*---------------------------------------------------------------------------*/
PRIVATE VOID 
ConvertMidiFileToRTM(
	int				midi_fh		// INPUT : Midi File Handle
	)
{
	BYTE			sEndOfTrack[ 3 ] = { 0, 0xFF, 0x2f };
	char			szChunkName[ 4 ];
	LONG			lChunkLen;
	WORD			wFormat;
	WORD			wTracks;
	WORD			j;
	WORD			wTrack;
	TRACK			*Track;
	TRACK			*Tracks;
	BYTE           *bData;

	if ( ( bRTM = ( BYTE * ) malloc( ( size_t ) MAX_TRACK_SIZE + 10 ) ) == NULL )
	{
		printf( "\rNot enough memory.\n" );
		exit( 20 );
	}
	wRTMSize = MAX_TRACK_SIZE;
	wRTMUsed = 0;
	bCtrlByte = bRTM;

	memset( bRTM, 0, wRTMSize );
	bCtrlByte = NULL;

	memset( Voice, 0, sizeof( VOICE ) * MAX_VOICES );
	for ( j = 0; j < MAX_VOICES; j++ )
	{
		Voice[ j ].wBend = MID_PITCH;
	}
/*
* Read Header
*/
	read( midi_fh, szChunkName, 4 );
	if ( memcmp( szChunkName, "MThd", 4 ) )
	{
		printf( "\rInvalid Midi File.\n" );
		exit( 20 );
	}
 	lChunkLen = GetLong( midi_fh );
	wFormat = GetWord( midi_fh );
	wTracks = GetWord( midi_fh );
	wQTicks = GetWord( midi_fh );
	lTicksPerMeasure = wQTicks * 4;

	if ( ! fQuiet )
		printf( "\rFormat: %d  Tracks: %d  QTicks: %d\n", wFormat, wTracks, wQTicks );
/*
* Load Tracks
*/
 	if ( ( Tracks = malloc( wTracks * sizeof( TRACK ) ) ) == NULL )
	{
		printf( "\nError: not enough memory.\n" );
		exit( 20 );
	}
	memset( Tracks, 0, wTracks * sizeof( TRACK ) );
	for ( Track = Tracks, wTrack = 0; wTrack < wTracks; wTrack++, Track++ )
	{
		if ( ! fQuiet )
			printf( "\rReading Track: %d", wTrack + 1 );
		read( midi_fh, szChunkName, 4 );
		if ( memcmp( szChunkName, "MTrk", 4 ) )
		{
			printf( "\rInvalid Midi File.\n" );
			exit( 20 );
		}
		lChunkLen = GetLong( midi_fh );
		if ( lChunkLen > MAX_TRACK_SIZE )
		{
			printf( "\rTrack exceeds maximize size of %u\n", MAX_TRACK_SIZE );
			exit( 20 );
		}
		if ( wFormat == 0 )
		lChunkLen += 3;

		Track->lSize = lChunkLen;
		if ( ( bData = ( BYTE * ) malloc( ( size_t ) lChunkLen ) ) == NULL )
		{
			printf( "\nError: not enough memory.\n" );
			exit( 20 );
		}
		Track->bData = bData;
		read( midi_fh, bData, ( WORD ) lChunkLen );
		if ( wFormat == 0 )
			memcpy( bData + lChunkLen - 3, sEndOfTrack, 3 );

		GetDelay( &Track->bData, &Track->lTime );
		Track->bStatus = *Track->bData;
	}
	if ( ! fQuiet )
	{
		printf( "\r%d Tracks Read                   \n", wTracks );
		printf( "Converting Music" );
	}
	memset( &RtmHdr, 0, sizeof( DMX_HEADER ) );

	Track = GetNextDelay( NULL, Tracks, wTracks );
	while ( Track != NULL )
	{
		bData = Track->bData;

		if ( *bData & MIDI_STATUS )
			Track->bStatus = *bData++;

		switch ( Track->bStatus )
		{
			case MIDI_SYSEX_F0:
			case MIDI_SYSEX_F7:
				MidiSystemExclusiveEvent( Track->bStatus, &bData );
				break;

			case MIDI_META:
				MidiMetaEvent( &( Track->bStatus ), &bData );
				break;

			default:
				MidiEvent( Track->bStatus, &bData );
				break;
		}
		Track->bData = bData;
		Track = GetNextDelay( Track, Tracks, wTracks );
	}
	RtmWriteVoiceDelay();
	RtmWrite( ( BYTE )( CS_END_OF_DATA << 4 ) );
}


/*---------------------------------------------------------------------------
* WriteRtm - Write out Real Time Music Format File
*---------------------------------------------------------------------------*/
PRIVATE VOID 
WriteRtm(
	char 			*szFile		// INPUT : File to Write To
	)
{
	WORD		wPatches = 0;
	int			patch_size;
	int			rtm_fh;
	int			j;

	if ( ! wRTMUsed )
	{
		printf( "\nNo music to save.\n" );
		exit( 20 );
	}
	if ( ! fQuiet )
		printf( "\nWriting MUS File..." );
	if ( ( rtm_fh = open( szFile, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC,
								S_IREAD | S_IWRITE ) ) == -1 )
	{
		printf( "\nError writing %s\n", szFile );
		exit( 20 );
	}
	patch_size = 0;
	for ( j = 0; j < ASIZE( PatchesUsed ); j++ )
	{
		if ( PatchesUsed[ j ].BanksUsed  )
		{
			WORD		b;

			wPatches++;
			patch_size += 2;	/* 1 Byte to Represent Patch #,
								*  1 Byte to Hold Bank Count */

			if ( PatchesUsed[ j ].BanksUsed == 1 &&
				 PatchesUsed[ j ].Bank[ 0 ] == 0 )
			{
				continue;	/* Bank 0 is default, don't count it */
			}
			for ( b = 0; b < PatchesUsed[ j ].BanksUsed; b++ )
			{
				patch_size++;
			}
		}
	}
	memcpy( RtmHdr.cSignature, "MUS\x1A", 4 );
	RtmHdr.wMusicOffset = sizeof( DMX_HEADER ) + patch_size;
	RtmHdr.wMusicSize	= wRTMUsed;
	RtmHdr.wPatches		= wPatches;
	write( rtm_fh, &RtmHdr, sizeof( DMX_HEADER ) );
	for ( j = 0; j < ASIZE( PatchesUsed ); j++ )
	{
		if ( PatchesUsed[ j ].BanksUsed  )
		{
			WORD		b;
			BYTE		PatchID = ( BYTE ) j;

			write( rtm_fh, &PatchID, sizeof( BYTE ) );

			if ( PatchesUsed[ j ].BanksUsed == 1 &&
				 PatchesUsed[ j ].Bank[ 0 ] == 0 )
			{
				b = 0;
				write( rtm_fh, &b, sizeof( BYTE ) );
				continue;	/* Bank 0 is default, don't count it */
			}
			write( rtm_fh, &PatchesUsed[ j ].BanksUsed, sizeof( BYTE ) );
			for ( b = 0; b < PatchesUsed[ j ].BanksUsed; b++ )
			{
				BYTE	BankID = ( BYTE ) PatchesUsed[ j ].Bank[ b ];

				write( rtm_fh, &BankID, sizeof( BYTE ) );
			}
		}
	}
	write( rtm_fh, bRTM, wRTMUsed );
	close( rtm_fh );
	if ( ! fQuiet )
	{
		printf( "\n%5u Ticks Per Second", wTickRate );
		printf( "\n%5u Bytes Used", wRTMUsed + RtmHdr.wMusicOffset );
		printf( "\n%5d Primary Channel%s Required.",
					RtmHdr.wPrimaryChannels, ( RtmHdr.wPrimaryChannels > 1 ) ? "s" : "" );
		printf( "\n%5d Secondary Channel%s Required.",
					RtmHdr.wSecondaryChannels, ( RtmHdr.wSecondaryChannels > 1 ) ? "s" : "" );
		lRunningTime = lRunningTimeQ / wTickRate;
		printf( "\nPlaying Time: %2ld:%02ld\n", lRunningTime / 60L, lRunningTime % 60L );
		printf( "\nPatches Used: " );
		for ( j = 0; j < ASIZE( PatchesUsed ); j++ )
		{
			if ( PatchesUsed[ j ].BanksUsed  )
			{
				WORD		b;

				printf( "\n %3d:", j + 1 );
				for ( b = 0; b < PatchesUsed[ j ].BanksUsed; b++ )
				{
					printf( " %d", PatchesUsed[ j ].Bank[ b ] + 1 );
				}
			}
		}
		printf( "\n" );
	}
}

void Banner( void )
{
	printf( "\nMIDI to MUS Converter  Version 2.06\n" );
	printf( "Copyright (C) 1992,93 Digital Expressions Inc.\n" );
	printf( "All Rights Reserved.\n" );
}

/*---------------------------------------------------------------------------
* main - MIDI to Game Music Format Converter
*---------------------------------------------------------------------------*/
PUBLIC VOID
main(
	int			argc,
	char		*argv[]
	)
{
	char		filename[ 256 ];
	char		outfname[ 256 ];
	char 		*ptr;
	int			midi_fh;
	int			j;

	if ( argc < 2 )
	{
		Banner();
		fprintf( stderr, "Usage: midi2mus [options] <midifile>\n" );
		fprintf( stderr, "\nOPTIONS:  /v    - Verbose" );
		fprintf( stderr, "\n          /t### - Ticks Per Second (default:%d)", wTickRate );
		fprintf( stderr, "\n          /q    - Quiet Mode\n" );
		exit ( 1 );
	}
	for ( j = 1; j < argc - 1; j++ )
	{
		if ( argv[ j ][ 0 ] == '-' || argv[ j ][ 0 ] == '/' )
		{
			switch ( argv[ j ][ 1 ] )
			{
				case 's':
				case 'S':
					PercVoice = 9;
					break;

				case 'q':
				case 'Q':
					fQuiet = TRUE;
					break;

				case 'p':
				case 'P':
					PercVoice = 15;
					break;

				case 't':
				case 'T':
					if ( atoi( &argv[ j ][ 2 ] ) != 0 )
					{
						wTickRate = ( WORD )( atoi( &argv[ j ][ 2 ] ) );
					}
					break;

				case 'v':
				case 'V':
					fVerbose = TRUE;
					break;
			}
		}
	}
	if ( fQuiet == FALSE )
	{
		Banner();
		printf( "\nTicks Per Second: %d", wTickRate );
	}
	memset( PatchesUsed, 0, 128 );
	strcpy( filename, argv[ argc - 1 ] );
	strupr( filename );
	if ( ( ptr = strrchr( filename, '.' ) ) == NULL ||
		 *( ptr + 1 ) == '\\' )
	{
		strcat( filename, ".MID" );
	}
	if ( ( ptr = strrchr( filename, '\\' ) ) == NULL )
		ptr = filename;
	else
		++ptr;
	strcpy( outfname, ptr );
	if ( ( ptr = strrchr( outfname, '.' ) ) )
	{
		if ( *( ptr + 1 ) != '\\' )
			*ptr = '\0';
	}
	strcat( outfname, ".MUS" );

	if ( ( midi_fh = open( filename, O_RDONLY | O_BINARY ) ) == -1 )
	{
		printf( "\nError reading %s\n", filename );
		exit( 20 );
	}
	ConvertMidiFileToRTM( midi_fh );
	close( midi_fh );
	WriteRtm( outfname );
}

