/*===========================================================================
* MUSAPI.C  - Music API Services
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
* $Log:   F:/DMX/API/VCS/MUSAPI.C_V  $
*  
*     Rev 1.1   29 Oct 1993 00:07:16   pjr
*  Dereferenced music pointers.
*  
*     Rev 1.0   02 Oct 1993 15:08:12   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <process.h>
#include <setjmp.h>

#include "dmx.h"
#include "musplr.h"

typedef struct
{
	BYTE		*SongData;
	INT			ChainTo;
	INT			Free;
} MUS_SONG;

#define DEFAULT_CHANNEL_VOL	127
#define RAMP_TICK_RATE		50

LOCAL DMX_MUS		*Mus = NULL;		// Music Command Structure
LOCAL MUS_SONG		*Song;
LOCAL INT			MaxSongs;
LOCAL INT			pid1 = -1;
LOCAL INT			pid2 = -1;
LOCAL INT			MasterVolume = DEFAULT_CHANNEL_VOL;
LOCAL INT			LastMasterVolume;
LOCAL MUS_STATUS	SongStatus[ 2 ];
LOCAL WORD			SongsActive;			// Number of Active Songs

/*--------------------------------------------------------------------------*
* MIDI FILE CONVERSION STUFF
*---------------------------------------------------------------------------*/
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
LOCAL DMX_HEADER 	*RtmHdr;

LOCAL BYTE		*bRTM;
LOCAL size_t	nCtrlByte;
LOCAL WORD		wTickRate = 140;
LOCAL BOOL		fSecMood = FALSE;

LOCAL LONG		lLastMidiEvent = 0L;		// Timestamp of last event
LOCAL LONG		lRunningTime = 0L;
LOCAL LONG		lRunningTimeQ = 0L;
LOCAL LONG		lLastTempoChange = 0L;
LOCAL LONG		lLastTempoChangeQ = 0L;
LOCAL LONG		lDelay = 0;
LOCAL LONG		lTicksPerSecond;
LOCAL LONG		lTicksPerMeasure;
LOCAL WORD		wQTicks;
LOCAL size_t	nMusSize;
LOCAL size_t	nMusUsed;
LOCAL jmp_buf	jmpenv;

/*-------------------------------------------------------------------------
* GetWord - Read WORD from MIDI Stream.
*------------------------------------------------------------------------*/
PRIVATE WORD 
GetWord(
	BYTE	**ptr		// INPUT : MIDI Data
	)
{
	WORD			num;
	BYTE			lsbmsb[ 2 ];

	memcpy( lsbmsb, *ptr, 2 );
	*ptr += 2;
	num = lsbmsb[ 0 ];
	num = ( num << 8 ) + lsbmsb[ 1 ];
	return num;
}


/*-------------------------------------------------------------------------
* GetLong - Read LONG value from MIDI Stream.
*------------------------------------------------------------------------*/
PRIVATE LONG 
GetLong(
	BYTE	**ptr		// INPUT : MIDI Data
	)
{
	LONG		num = 0L;
	WORD		i;
	BYTE		longval[ 4 ];

	memcpy( longval, *ptr, 4 );
	*ptr += 4;
	for ( i = 0; i < 4; i++ )
	{
		num = ( num << 8 ) + longval[ i ];
	}
	return num;
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
	lTicksPerSecond	  = 1000000L / ( luSec / wQTicks );
	lLastTempoChange  = lRunningTime;
	lLastTempoChangeQ = lRunningTimeQ;
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
	}
	else switch ( bEventType )
	{
		case MIDI_END_OF_TRACK:
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
			}
			break;
		}

		default:
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
	if ( nMusUsed >= nMusSize )
	{
		longjmp( jmpenv, -1 );
	}
	bRTM[ nMusUsed++ ] = bByte;
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

	if ( nCtrlByte == 0 )
	{
		lDelay = 0;
	}
	if ( lDelay > 0 )
	{
		lLastMidiEvent = lRunningTimeQ;
		bRTM[ nCtrlByte ] |= CB_TIME;
		while ( lDelay > 0 && wBytes < 5 )
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
	nCtrlByte = nMusUsed;
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
		v->bMappedVoice = ( BYTE ) RtmHdr->wPrimaryChannels++;
	}
	else
	{
		v->bMappedVoice = 10 + ( BYTE ) RtmHdr->wSecondaryChannels++;
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

		RtmWriteVoiceDelay();
		RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
		RtmWrite( ( BYTE ) ( CMD_CHANGE_PROGRAM ) );
		RtmWrite( ( BYTE ) ( v->bProgram ) );
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
	RtmWriteVoiceDelay();
	RtmWrite( v->bMappedVoice );

	bRTM[ nCtrlByte ] |= ( CS_NOTE_ON << 4 );
	v->Notes[ empty_slot ] = v->bNote = Note;
	v->lLastEventTime[ empty_slot ] = lRunningTimeQ;

	if ( v->bVelocity != Volume )
	{
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
			break;

		case 3:	/* CONTROL CHANGE */
			if ( ! v->fUsed )
			{
				NewVoice( v, bVoice );
			}
			CheckBend( v );
			switch ( *bMidiData )
			{
				case 0x00:	/* Bank Select */
				case 0x20:
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_BANK_SELECT ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x01:	/* Modulation	*/
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_MODULATION ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x07:	/* Volume */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_VOLUME ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x0a:	/* Pan/Balance */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_PANPOT ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x0b:	/* Expression */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_EXPRESSION ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x40:	/* Hold Off/On */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_HOLD1 ) );
					if ( *( bMidiData + 1 ) < 0x40 )
						RtmWrite( 0 );
					else
						RtmWrite( 0x7F );
					break;

				case 0x43:	/* Soft Off/On */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_SOFT ) );
					if ( *( bMidiData + 1 ) < 0x40 )
						RtmWrite( 0 );
					else
						RtmWrite( 0x7F );
					break;

				case 0x5b:	/* Reverb Depth */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_REVERB_DEPTH ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x5d:	/* Chorus Depth */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_CHORUS_DEPTH ) );
					RtmWrite( *( bMidiData + 1 ) );
					break;

				case 0x78:	/* All Sounds Off */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_ALL_SOUNDS_OFF ) );
					break;

				case 0x79: 	/* Reset All Controllers */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_RESET_CHANNEL ) );
					break;

				case 0x7B:	/* All Notes Off */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_ALL_NOTES_OFF ) );
					break;

				case 0x7E:	/* Mono Mode */
					RtmWriteVoiceDelay();
					RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND1 << 4 ) ) );
					RtmWrite( ( BYTE ) ( CMD_MONO ) );
					break;

				case 0x7F:	/* Poly Mode */
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
				v->bProgram = *bMidiData;
				RtmWriteVoiceDelay();
				RtmWrite( ( BYTE ) ( v->bMappedVoice | ( CS_COMMAND2 << 4 ) ) );
				RtmWrite( ( BYTE ) ( CMD_CHANGE_PROGRAM ) );
				RtmWrite( ( BYTE ) ( v->bProgram ) );
			}
			break;

		case 5:	/* CHANNEL PRESSURE */
			CheckBend( v );
			break;

		case 6:	/* PITCH BEND CHANGE */
		{
			WORD		wPitch = ( WORD )( *( bMidiData + 1 ) << 7 | *bMidiData );

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
	LONG			lTicks;

	lRunningTime = lTime;
	lTicks = lTime - lLastTempoChange;
	lRunningTimeQ = ( LONG ) ( lTicks * ( LONG ) wTickRate / lTicksPerSecond )
				  + lLastTempoChangeQ;
	lDelay = lRunningTimeQ - lLastMidiEvent;
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
* Midi2Mus - Convert MIDI format to DMX format (MUS)
*---------------------------------------------------------------------------*/
PUBLIC VOID *	// Returns pointer to MUS data
Midi2Mus(
	BYTE		*SongData
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
	BYTE			*bData;

	Tracks = NULL;
	bRTM = NULL;

	lLastMidiEvent = 0L;	
	lRunningTime = 0L;
	lRunningTimeQ = 0L;
	lLastTempoChange = 0L;
	lLastTempoChangeQ = 0L;
	lDelay = 0;

	if ( setjmp( jmpenv ) == 0 )
	{
		nMusSize = MAX_TRACK_SIZE;
		if ( ( bRTM = ( BYTE * ) malloc( nMusSize ) ) == NULL )
			longjmp( jmpenv, -1 );

		RtmHdr = ( DMX_HEADER * ) bRTM;
		nMusUsed = sizeof( DMX_HEADER );
		memset( bRTM, 0, nMusSize );
		nCtrlByte = 0;
		memset( Voice, 0, sizeof( VOICE ) * MAX_VOICES );
		for ( j = 0; j < MAX_VOICES; j++ )
		{
			Voice[ j ].wBend = MID_PITCH;
		}
	/*
	* Read Header
	*/
		memcpy( szChunkName, SongData, 4 );
		SongData += 4;
		if ( memcmp( szChunkName, "MThd", 4 ) )
			longjmp( jmpenv, -1 );

 		lChunkLen = GetLong( &SongData );
		wFormat = GetWord(  &SongData  );
		wTracks = GetWord(  &SongData );
		wQTicks = GetWord(  &SongData  );
		lTicksPerMeasure = wQTicks * 4;
	/*
	* Load Tracks
	*/
 		if ( ( Tracks = malloc( wTracks * sizeof( TRACK ) ) ) == NULL )
			longjmp( jmpenv, -1 );

		memset( Tracks, 0, wTracks * sizeof( TRACK ) );
		for ( Track = Tracks, wTrack = 0; wTrack < wTracks; wTrack++, Track++ )
		{
			memcpy( szChunkName, SongData, 4 );
			SongData += 4;
			if ( memcmp( szChunkName, "MTrk", 4 ) )
				longjmp( jmpenv, -1 );

			lChunkLen = GetLong( &SongData );
			if ( wFormat == 0 )
				lChunkLen += 3;

			Track->lSize = lChunkLen;
			Track->bData = SongData;
			SongData += lChunkLen;
			if ( wFormat == 0 )
				memcpy( Track->bData + lChunkLen - 3, sEndOfTrack, 3 );

			GetDelay( &Track->bData, &Track->lTime );
			Track->bStatus = *Track->bData;
		}
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

		memcpy( RtmHdr->cSignature, "MUS\x1A", 4 );
		RtmHdr->wMusicSize	 = nMusUsed - sizeof( DMX_HEADER );
		RtmHdr->wMusicOffset = sizeof( DMX_HEADER );

		bRTM = realloc( bRTM, nMusUsed );
		free( Tracks );
		return bRTM;
	}
	if ( Tracks )
		free( Tracks );
	if ( bRTM )
		free( bRTM );
	return NULL;
}

/*--------------------------------------------------------------------------*
* Music Playback Functions
*---------------------------------------------------------------------------*/

/****************************************************************************
* PlrResetCounters - Reset Counters for givin song.
*****************************************************************************/
PRIVATE VOID
PlrResetCounters(
	MUS_STATUS	*ms
	)
{
	DMX_HEADER	*hdr;
	WORD		channel;
	CHNL_STATE	*cs;

	hdr = ( DMX_HEADER * ) ms->SongData;

	ms->NextEvent		= hdr->wMusicOffset;
	ms->Delay 			= 0L;
	ms->CurrentMeasure	= 0L;
	ms->ActionsFlag     |= AF_ACTIVE_MUS;
	ms->ActionsFlag		&= ( ~AF_START & ~AF_STOP & ~AF_STOP_NOW );

	if ( ms->CurrentLevel > MasterVolume )
		ms->CurrentLevel = MasterVolume;

	if ( ! ( ms->ActionsFlag & AF_RAMP_VOLUME ) )
		ms->CurrentLevel = MasterVolume;

	for ( channel = 0; channel <= 15; channel++ )
	{
		if ( channel == ms->TotalChannels )
			channel = 15;

		cs = &ms->State[ channel ];

		cs->CurrentVolume 	= 100;
		cs->Volume			= 100;
		cs->Bank 			= 0;
		cs->Modulation 		= 0;
		cs->PanPot 			= 64;
		cs->Expression 		= 0;
		cs->ReverbDepth 	= 40;
		cs->ChorusDepth 	= 0;
		cs->Hold 			= 0;
		cs->Soft 			= 0;
		cs->PitchBend		= 128;
		
		if ( channel < ms->ActiveChannels ||
			( channel == 15 && ms->BaseChannel == 0 )
		   )
		{
			WORD	chnl = channel + ms->BaseChannel;
			BYTE	lvl;

			Mus->Command( chnl, CMD_RESET_CHANNEL, ( BYTE ) 0 );


			lvl = ( BYTE ) ms->State[ chnl ].CurrentVolume;
			if ( lvl > (BYTE) ms->CurrentLevel )
				lvl = (BYTE) ms->CurrentLevel;
			if ( lvl > (BYTE) MasterVolume )
				lvl = ( BYTE ) MasterVolume;
			Mus->Command( chnl, CMD_VOLUME, lvl );
		}
	}
}

/****************************************************************************
* PlrStartSong - Start Playing Song
*****************************************************************************/
PUBLIC VOID
PlrStartSong(
	WORD		MUS_Index
	)
{
	MUS_STATUS	*ms;
	MUS_STATUS	*ms2;

	ms = &SongStatus[ MUS_Index ];

	if ( SongsActive )
	{
		ms2 = &SongStatus[ ( MUS_Index == 0 ? 1 : 0 ) ];

		if ( ms2->ActiveChannels > MAX_SPLIT )
			ms2->ActiveChannels = MAX_SPLIT;

		ms->BaseChannel = ( ms2->BaseChannel == 0 ? 7 : 0 );
		ms->ActiveChannels = ( ms->TotalChannels < MAX_SPLIT ?
												ms->TotalChannels : MAX_SPLIT );
	}
	else
	{
		ms->ActiveChannels	= ms->TotalChannels;
		ms->BaseChannel 	= 0;
	}
	SongsActive++;
	PlrResetCounters( ms );
}

/****************************************************************************
* PlrSilenceSong - Silence any notes playing for Song.
*****************************************************************************/
PRIVATE VOID
PlrSilenceSong(
	MUS_STATUS		*ms
	)
{
	WORD			Chnls;
	WORD			Channel;
	WORD			ChanXlat;

	Chnls = ms->ActiveChannels;
	for ( Channel = 0; Channel <= 15; Channel++ )
	{
		if ( Channel == Chnls )
			ChanXlat = Channel = 15;
		else if ( ms->BaseChannel == 0 )
			ChanXlat = Channel;
		else
		{
			Channel += ms->BaseChannel;
			ChanXlat = ( Channel >= 14 ? Channel - 14 : Channel );
		}
		Mus->Command( ChanXlat, ( BYTE ) CMD_ALL_NOTES_OFF, ( BYTE ) 0 );
		Mus->Command( ChanXlat, ( BYTE ) CMD_ALL_SOUNDS_OFF, ( BYTE ) 0 );
	}
}

/****************************************************************************
* PlrPauseSong - Pause playing of Song.
*****************************************************************************/
PRIVATE VOID
PlrPauseSong(
	WORD		MUS_Index
	)
{
	MUS_STATUS	*ms;

	ms = &SongStatus[ MUS_Index ];
	if ( ms->ActionsFlag & AF_PAUSE )
	{
		PlrSilenceSong( ms );
		ms->ActionsFlag &= ~AF_PAUSE;
		ms->ActionsFlag |= AF_PAUSED;
	}
}

/*--------------------------------------------------------------------------*
* PlrKillSong - Kill Active Song 
*---------------------------------------------------------------------------*/
PRIVATE VOID
PlrKillSong(
	WORD		MUS_Index
	)
{
	MUS_STATUS	*ms;
	CHNL_STATE	*cs;
	BYTE		chan_xlat;

	ms = &SongStatus[ MUS_Index ];

	if ( SongsActive )
		SongsActive--;

	ms->ActionsFlag &= ~( AF_ACTIVE_MUS|AF_STOP|AF_LOADED|AF_STOP_NOW|AF_PAUSED|AF_RAMP_VOLUME );

/*
* Turn of all notes associated with song
*/
	PlrSilenceSong( ms );
/*
* If there is still an active song, expand it's channels
*/
	if ( SongsActive )
	{
		ms = &SongStatus[ ( MUS_Index == 0 ? 1 : 0 ) ];
		chan_xlat = ( BYTE )( ms->BaseChannel + ms->ActiveChannels );
		while ( ms->ActiveChannels < ms->TotalChannels )
		{
			if ( chan_xlat >= 14 )
				chan_xlat -= 14;

			cs = &ms->State[ chan_xlat ];

			Mus->Command( chan_xlat, CMD_RESET_CHANNEL, 	0 );
			Mus->Command( chan_xlat, CMD_CHANGE_PROGRAM,	cs->Program );
 			Mus->Command( chan_xlat, CMD_BANK_SELECT, 		cs->Bank );
			Mus->Command( chan_xlat, CMD_MODULATION,		cs->Modulation );
			Mus->Command( chan_xlat, CMD_VOLUME,			cs->CurrentVolume );
			Mus->Command( chan_xlat, CMD_PANPOT,			cs->PanPot );
			Mus->Command( chan_xlat, CMD_EXPRESSION,		cs->Expression );
			Mus->Command( chan_xlat, CMD_REVERB_DEPTH,		cs->ReverbDepth );
			Mus->Command( chan_xlat, CMD_CHORUS_DEPTH,		cs->ChorusDepth );
			Mus->Command( chan_xlat, CMD_HOLD1,				cs->Hold );
			Mus->Command( chan_xlat, CMD_SOFT,				cs->Soft );
			if ( cs->PitchBend )
			{
				Mus->PitchBend( chan_xlat, cs->PitchBend );
			}
			chan_xlat++;
			ms->ActiveChannels++;
		}
	}
}

/****************************************************************************
* StartSong - Start Playing Song
*---------------------------------------------------------------------------*
* Returns:
*	0		Success
*	-1		Bad Handle
*	-2 		Player busy
*	
*****************************************************************************/
PRIVATE INT
StartSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		FromVolume,		// INPUT : Starting Volume 1..127, -1=Current
	INT		ToVolume,		// INPUT : Target Volume 0..127
	INT		TimeMs			// INPUT : Time in ms
	)
{
	DMX_HEADER	*hdr;
	MUS_STATUS	*ms;
	SHORT		levels;
	SHORT		ticks;
	INT			tmr_ms;
	int			j;
	int			k;

	if ( pid1 == -1 )
		return -1;

/*
* Verify Handle...
*/
	if ( Handle < 1 || Handle > MaxSongs ||
		 Song[ Handle - 1 ].SongData == NULL
	   )
		return -1;

/*
* Check if song is still loaded by finding a matching ID
*/
	for ( j = 0; j < MAX_ACTIVE; j++ )
	{
		if ( SongStatus[ j ].CurrentMusID == ( WORD ) Handle &&
		   ( SongStatus[ j ].ActionsFlag & AF_LOADED ) == AF_LOADED
		   )
		{
			ms = &SongStatus[ j ];
			break;
		}
	}
/*
* if song was not loaded ( indicated by j == MAX_ACTIVE ) then
* check SongsActive count if ok, then look for load area.
*/
	if ( j == MAX_ACTIVE )
	{
		if ( SongsActive == MAX_ACTIVE )
			return -2;

	/*
	* Find which status block is free
	*/
		for ( k = 0; k < MAX_ACTIVE; k++ )
		{
			if ( SongStatus[ k ].ActionsFlag & AF_LOADED )
				continue;
			else
				break;
		}
		hdr = ( DMX_HEADER * ) Song[ Handle - 1 ].SongData;
		ms = &SongStatus[ k ];
		ms->SongData 		= ( BYTE * ) hdr;
		ms->CurrentMusID	= ( WORD ) Handle;
		ms->CurrentLevel	= MasterVolume;
		ms->TotalChannels 	= hdr->wPrimaryChannels;
	}
	ms->ChainMusID		= ( WORD ) Song[ Handle - 1 ].ChainTo;
	ms->MaxVolumeLevel 	= ToVolume;

	if ( TimeMs )
	{
		tmr_ms = ( 1000 / RAMP_TICK_RATE );
		ticks = ( TimeMs / tmr_ms ) + 1;

		if ( FromVolume == -1 )
		{
			if ( ms->ActionsFlag & AF_ACTIVE_MUS )
				FromVolume = ms->CurrentLevel;
			else
				FromVolume = 0;
		}
		if ( ( levels = ToVolume - FromVolume ) < 0 )
			levels = -levels;
		if ( levels > 0 && ticks > 2 )
		{
			ms->RampLevelSum 	= levels / ticks;
			ms->RampLevelErr 	= levels % ticks;
			ms->RampLevelAcc	= -( ms->RampLevelErr >> 1 );
			ms->RampLevels 		= levels;
			ms->CurrentLevel 	= FromVolume;
			ms->RampTargetLevel	= ToVolume;
			ms->ActionsFlag |= AF_RAMP_VOLUME;
		}
		else
		{
			ms->CurrentLevel = ToVolume;
		}
	}
	else
	{
		ms->CurrentLevel = ToVolume;
	}
	if ( ( ms->ActionsFlag & AF_ACTIVE_MUS ) != AF_ACTIVE_MUS )
	{
		ms->ActionsFlag |= AF_LOADED;
		ms->ActionsFlag |= AF_START;
	}
	return 0;
}

/*--------------------------------------------------------------------------*
*---------------------------------------------------------------------------*/
PRIVATE VOID
PlayMus(
	WORD		MUS_Index
	)
{
	MUS_STATUS	*ms;
	MUS_STATUS	*ms2;
	LONG		NewDelay;	
	WORD		events_processed;
	BYTE		*event;
	BYTE		cb;
	DMX_HEADER	*hdr;
	CHNL_STATE	*cs;
	BYTE		channel;
	BYTE		chan_xlat;
	BYTE		status;
	BYTE		note;
	BYTE		velocity;
	BYTE		pitch;
	BYTE		*setting;
	BYTE		cmd;

	ms = &SongStatus[ MUS_Index ];
	while ( ms->Delay == 0L )
	{
		event = ( BYTE * )( ms->SongData + ms->NextEvent );
		events_processed = 0;

		cb 		= event[ 0 ];
		channel = LONIBBLE(cb);
		cs	 	= &ms->State[ channel ];
		status	= HINIBBLE(cb) & 0x07;

		events_processed += 2;

		if ( ms->BaseChannel == 0 || channel == 15 )
			chan_xlat = channel;
		else
		{
			chan_xlat = ( BYTE )( channel + ms->BaseChannel );
			if ( chan_xlat >= 14 )
				chan_xlat -= 14;
		}
		switch ( status )
		{
			case CS_NOTE_OFF:
				if ( channel == 15 || channel < ( BYTE ) ms->ActiveChannels )
					Mus->NoteOff( chan_xlat, event[ 1 ] );
				break;

			case CS_NOTE_ON:
				note = event[ 1 ];
				if ( note & NB_VELOCITY )
				{
					cs->NoteVelocity = velocity = event[ 2 ];
					note &= ~( NB_VELOCITY );
					events_processed++;
				}
				else
				{
					velocity = cs->NoteVelocity;
				}
				if ( channel == 15 || channel < ( BYTE ) ms->ActiveChannels )
					Mus->NoteOn( chan_xlat, note, velocity );
				break;

			case CS_PITCH_BEND:
				cs->PitchBend = pitch = event[ 1 ];
				if ( channel == 15 || channel < ( BYTE ) ms->ActiveChannels )
					Mus->PitchBend( chan_xlat, pitch );
				break;

			case CS_COMMAND1:
				cmd = event[ 1 ];
				if ( cmd == CMD_MONO )
					cs->Mono = 1;
				else if ( cmd == CMD_POLY )
					cs->Mono = 0;

				if ( channel == 15 || channel < ( BYTE ) ms->ActiveChannels )
				{
					Mus->Command( chan_xlat, cmd, ( BYTE ) 0 );
				}
				break;

			case CS_COMMAND2:
				cmd 	= event[ 1 ];
				status	= event[ 2 ];
				events_processed++;

				if ( cmd < ( BYTE ) CMD_ALL_SOUNDS_OFF )
				{
					setting = ( BYTE * ) cs;
					*( setting + cmd ) = status;
				}
				if ( channel == 15 || channel < ( BYTE ) ms->ActiveChannels )
				{
					if ( cmd == CMD_VOLUME )
					{
						if ( status > ( BYTE ) MasterVolume )
							status = ( BYTE ) MasterVolume;

						if ( status > ( BYTE ) ms->CurrentLevel )
							status = ( BYTE ) ms->CurrentLevel;

						ms->State[ channel ].CurrentVolume = status;
					}
					Mus->Command( chan_xlat, cmd, status );
					if ( cmd == CMD_RESET_CHANNEL )
					{
						status = ( BYTE ) ms->State[ channel ].CurrentVolume;
						if ( status > (BYTE) ms->CurrentLevel )
							status = (BYTE) ms->CurrentLevel;
						if ( status > (BYTE) MasterVolume )
							status = ( BYTE ) MasterVolume;

						Mus->Command( chan_xlat, CMD_VOLUME, status );
					}
				}
				break;

			case CS_END_OF_MEASURE:
				events_processed--;	// Fix assumption of 2 bytes being processed
				ms->CurrentMeasure++;
				break;

			case CS_END_OF_DATA:
				cb = 0;				// No time following command byte
				events_processed--;	// Fix assumption of 2 bytes being processed
				if ( ! ms->ChainMusID )
				{
					PlrKillSong( MUS_Index );	// Stop playing song
					return;
				}
				if ( ms->ChainMusID == ms->CurrentMusID )
				{
					hdr = ( DMX_HEADER * ) ms->SongData;

					PlrResetCounters( ms );
					event = ( BYTE * )( ms->SongData + ms->NextEvent );
					events_processed = 0;
					continue;
				}
				else
				{
					PlrKillSong( MUS_Index );	// Stop playing song
					MUS_Index ^= 1;			// Get OTHER Index
					ms2 = &SongStatus[ MUS_Index ];
					if ( ( ms2->ActionsFlag & AF_LOADED ) == AF_LOADED &&
						 ( ms2->ActionsFlag & AF_ACTIVE_MUS ) != AF_ACTIVE_MUS &&
						 ms2->CurrentMusID == ms->ChainMusID
						)
					{
						PlrStartSong( MUS_Index );
					}
					else
					{
						StartSong( ms->ChainMusID, -1, 127, 0 );
					}
				}
				return;

		}	// End switch( status )
		ms->NextEvent += ( LONG ) events_processed;
		if ( cb & CB_TIME )
		{
			event = ( BYTE * )( ms->SongData + ms->NextEvent );
		 	events_processed = 1;
			cb = event[ 0 ];
			NewDelay = ( LONG )( cb & ~TB_EXTENDED );
			if ( cb & TB_EXTENDED )
			{
				do
				{
					NewDelay <<= 7;
					cb = event[ events_processed++ ];
					NewDelay |= ( LONG )( cb & ~TB_EXTENDED );
				}
				while ( cb & TB_EXTENDED );
			}
			ms->Delay = NewDelay;
			ms->NextEvent += ( LONG ) events_processed;
		}
	}
}

/*--------------------------------------------------------------------------*
* Service - Service Song Interrupts
*---------------------------------------------------------------------------*/
// PRIVATE VOID
PUBLIC int
ServicePlayer(
	VOID
	)
{
	MUS_STATUS	*ms0;
	MUS_STATUS	*ms1;
	BYTE		bPrevColor;

#ifndef PROD
	if ( DmxDebug & DMXBUG_MUSAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3e );
	}
#endif
	if ( Mus->Enter )
	{
		if ( Mus->Enter() )
		{
			if ( DmxDebug & DMXBUG_MUSAPI )
			{
				inp( 0x3da );
				outp( 0x3c0, 0x31 );
				outp( 0x3c0, bPrevColor );
			}
			return TS_BUSY;
		}
	}
	ms0 = SongStatus;
	ms1 = ms0 + 1;

	if ( ms0->ActionsFlag & AF_STOP_NOW )
		PlrKillSong( 0 );

	if ( ms1->ActionsFlag & AF_STOP_NOW )
		PlrKillSong( 1 );

	if ( ms0->ActionsFlag & AF_START )
		PlrStartSong( 0 );

	if ( ms1->ActionsFlag & AF_START )
		PlrStartSong( 1 );

	if ( ms0->ActionsFlag & AF_PAUSE )
		PlrPauseSong( 0 );

	if ( ms1->ActionsFlag & AF_PAUSE )
		PlrPauseSong( 1 );

	if ( ( ms0->ActionsFlag & ( AF_ACTIVE_MUS | AF_PAUSED ) ) == AF_ACTIVE_MUS )
	{
		if ( ms0->Delay > 0L )
			ms0->Delay--;
		if ( ms0->Delay == 0L )
			PlayMus( 0 );
	}

	if ( ( ms1->ActionsFlag & ( AF_ACTIVE_MUS | AF_PAUSED ) ) == AF_ACTIVE_MUS )
	{
		if ( ms1->Delay > 0L )
			ms1->Delay--;
		if ( ms1->Delay == 0L )
			PlayMus( 1 );
	}
	if ( Mus->Leave )
	{
		Mus->Leave();
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_MUSAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
	return TS_OKAY;
}
	
/****************************************************************************
* PlrTrimVolume - Trim Volume on Active Channels
*****************************************************************************/
PRIVATE VOID
PlrTrimVolume( 
	WORD		MUS_Index
	)
{
	MUS_STATUS	*ms;
	WORD		volume;
	WORD		chan_xlat;
	WORD		channel;
	WORD		chnls;

	ms = &SongStatus[ MUS_Index ];

	chnls = ( BYTE ) ms->ActiveChannels;
	for ( channel = 0; channel <= 15; channel++ )
	{
		if ( channel == chnls )
			chan_xlat = channel = 15;
		else if ( ms->BaseChannel == 0 )
			chan_xlat = channel;
		else
		{
			channel += ms->BaseChannel;
			chan_xlat = ( channel >= 14 ? channel - 14 : channel );
		}
		volume = LastMasterVolume;
		if ( ms->ActionsFlag & AF_RAMP_VOLUME )
		{
			if ( volume > ( WORD ) ms->CurrentLevel )
				volume = ms->CurrentLevel;
		}
		if ( channel != 15 )
		{
			if ( volume > ( WORD ) ms->State[ channel ].Volume )
				volume = ms->State[ channel ].Volume;
		}
		if ( volume != ms->State[ channel ].CurrentVolume )
		{
			ms->State[ channel ].CurrentVolume = ( BYTE ) volume;
			Mus->Command( chan_xlat, CMD_VOLUME, volume );
		}
	}
}


/****************************************************************************
* PlrRampVolume - Ramp Volume on Active Channels
*****************************************************************************/
PRIVATE BOOL
PlrRampVolume( 
	WORD		MUS_Index
	)
{
	MUS_STATUS	*ms;
	MUS_STATUS	*ms2;
	SHORT		bump;
	WORD		flags;

	ms = &SongStatus[ MUS_Index ];
	
	if ( ms->CurrentLevel >= ( SHORT ) LastMasterVolume &&
		 ms->RampTargetLevel >= ( SHORT ) LastMasterVolume
		)
	{
		ms->ActionsFlag &= ~AF_RAMP_VOLUME;
		return FALSE;
	}

	bump = ms->RampLevelSum;	
   	ms->RampLevelAcc += ms->RampLevelErr;
	if ( ms->RampLevelAcc >= ms->RampLevels )
	{
		bump++;
		ms->RampLevelAcc -= ms->RampLevels;
	}
	if ( bump )
	{
		if ( ms->RampTargetLevel < ms->CurrentLevel )
		{
			ms->CurrentLevel -= bump;
			if ( ms->CurrentLevel < 0 )
				ms->CurrentLevel = 0;
			if ( ms->CurrentLevel == 0 )
			{
				flags = ms->ActionsFlag;
				PlrKillSong( MUS_Index );

                if ( ! ( flags & AF_STOP ) )
                {
					MUS_Index ^= 1;			// Get OTHER Index
					ms2 = &SongStatus[ MUS_Index ];
					if ( ( ms2->ActionsFlag & AF_LOADED ) == AF_LOADED &&
						 ( ms2->ActionsFlag & AF_ACTIVE_MUS ) != AF_ACTIVE_MUS &&
						 ms2->CurrentMusID == ms->ChainMusID
						)
					{
						PlrStartSong( MUS_Index );
					}
					else
					{
						StartSong( ms->ChainMusID, -1, 127, 0 );
					}
                }
				return TRUE;
			}
			if ( ms->CurrentLevel == ms->RampTargetLevel )
				ms->ActionsFlag &= ~AF_RAMP_VOLUME;
			PlrTrimVolume( MUS_Index );
			return TRUE;
		}
		ms->CurrentLevel += bump;
		if ( ms->CurrentLevel >= ms->MaxVolumeLevel )
		{
			ms->CurrentLevel = ms->MaxVolumeLevel;
			ms->ActionsFlag &= ~AF_RAMP_VOLUME;
		}
		if ( ms->CurrentLevel >= ( SHORT ) LastMasterVolume )
		{
			ms->CurrentLevel = LastMasterVolume;
			ms->ActionsFlag &= ~AF_RAMP_VOLUME;
		}
		PlrTrimVolume( MUS_Index );
		return TRUE;
	}
	return FALSE;
}


/*--------------------------------------------------------------------------*
* Fader - Volume Fade Control
*---------------------------------------------------------------------------*/
PUBLIC INT
ServiceFader(
	VOID
	)
{
	BOOL		fMasterVolChg;
	WORD		Midx;
	BYTE		bPrevColor;

#ifndef PROD
	if ( DmxDebug & DMXBUG_MUSAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3e );
	}
#endif
	if ( Mus->Enter )
	{
		if ( Mus->Enter() )
		{
#ifndef PROD
			if ( DmxDebug & DMXBUG_MUSAPI )
			{
				inp( 0x3da );
				outp( 0x3c0, 0x31 );
				outp( 0x3c0, bPrevColor );
			}
#endif
			return TS_BUSY;
		}
	}

	if ( LastMasterVolume != MasterVolume )
	{
		LastMasterVolume = MasterVolume;
		fMasterVolChg = TRUE;
	}
	else
		fMasterVolChg = FALSE;

	for (  Midx = 0; Midx < 2; Midx++ )
	{
		if ( SongStatus[ Midx ].ActionsFlag & AF_ACTIVE_MUS )
		{
			if ( SongStatus[ Midx ].ActionsFlag & AF_RAMP_VOLUME )
			{
				if ( ! PlrRampVolume( Midx ) && fMasterVolChg )
					PlrTrimVolume( Midx );
			}
			else if ( fMasterVolChg )
			{
				PlrTrimVolume( Midx );
			}
		}
	}
	if ( Mus->Leave )
	{
		Mus->Leave();
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_MUSAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
	return TS_OKAY;
}


/****************************************************************************
* PlrReset - Reset Music Sound Player
*****************************************************************************/
PRIVATE VOID
PlrReset(
	VOID
	)
{
	SongStatus[ 0 ].ActionsFlag		= AF_IDLE;
	SongStatus[ 0 ].CurrentMusID	= 0;
	SongStatus[ 0 ].ChainMusID		= 0;
	SongStatus[ 1 ].ActionsFlag		= AF_IDLE;
	SongStatus[ 1 ].CurrentMusID	= 0;
	SongStatus[ 1 ].ChainMusID		= 0;
	SongsActive = 0;
	LastMasterVolume = MasterVolume = DEFAULT_CHANNEL_VOL;
}

/****************************************************************************
* MUS_PlaySong - Play Song
*****************************************************************************/
PUBLIC INT
MUS_PlaySong(
	INT		Handle,			// INPUT : Handle to Song
	INT		Volume			// INPUT : Volume Level of Song 1..127
	)
{
#ifdef DEBUG
	DmxFlushTrace();
#endif
	if ( pid1 == -1 )
		return -1;

	return StartSong( Handle, -1, Volume, 0 );
}

/****************************************************************************
* MUS_RampVolume - Ramp Song from one volume level to another, if the song
*				   is not playing it will be started from the beginning.
*				   A target volume of 0 indicates to stop the song once the
*				   ramp is complete. 
*****************************************************************************/
PUBLIC INT
MUS_RampVolume(
	INT		Handle,			// INPUT : Handle to Song
	INT		FromVolume,		// INPUT : Starting Volume 1..127, -1=Current
	INT		ToVolume,		// INPUT : Target Volume 0..127
	INT		TimeMs			// INPUT : Time in ms
	)
{
	return StartSong( Handle, FromVolume, ToVolume, TimeMs );
}

/****************************************************************************
* MUS_FadeInSong - Fade Song into Foreground
*****************************************************************************/
PUBLIC INT
MUS_FadeInSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		TimeMs			// INPUT : Time in ms
	)
{
#ifdef DEBUG
	DmxFlushTrace();
#endif
	return StartSong( Handle, 0, 127, TimeMs );
}

/****************************************************************************
* MUS_FadeOutSong - Fade Song into Foreground
*****************************************************************************/
PUBLIC INT
MUS_FadeOutSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		TimeMs			// INPUT : Time in ms
	)
{
	MUS_STATUS	*ms;
	INT			j;

#ifdef DEBUG
	DmxFlushTrace();
#endif
	if ( Handle < 1 || Handle > MaxSongs || Song[ Handle - 1 ].SongData == NULL )
		return -1;

	for ( j = 0; j < MAX_ACTIVE; j++ )
	{
		ms = &SongStatus[ j ];
		if ( ms->CurrentMusID == ( WORD ) Handle &&
  		 	 ms->ActionsFlag & AF_ACTIVE_MUS
			)
		{
			StartSong( Handle, -1, 0, TimeMs );
			if ( ms->ActionsFlag & AF_RAMP_VOLUME )
				ms->ActionsFlag |= AF_STOP;
			else
				ms->ActionsFlag |= AF_STOP_NOW;
			break;
		}
	}
	return 0;
}

/****************************************************************************
* MUS_QrySongPlaying - Returns Song Play Status
*****************************************************************************/
PUBLIC INT
MUS_QrySongPlaying(
	INT		Handle
	)
{
	INT		j;

#ifdef DEBUG
	DmxFlushTrace();
#endif

	if ( pid1 == -1 )
		return FALSE;

	if ( Handle < 1 || Handle > MaxSongs || Song[ Handle - 1 ].SongData == NULL )
	{
		return FALSE;
	}

	for ( j = 0; j < MAX_ACTIVE; j++ )
	{
		if ( SongStatus[ j ].CurrentMusID == ( WORD ) Handle &&
 	 	   ( SongStatus[ j ].ActionsFlag & AF_ACTIVE_MUS ) == AF_ACTIVE_MUS 
		   )
		{
			return TRUE;
		}
	}
	return FALSE;
}

/****************************************************************************
* MUS_GetMasterVolume - Returns Master Volume Setting 1..127
*****************************************************************************/
PUBLIC INT		// Returns volume range of 1..127
MUS_GetMasterVolume(
	void
	)
{
	return ( INT ) MasterVolume;
}

/****************************************************************************
* MUS_SetMasterVolume - Sets Master Volume 1..127
*****************************************************************************/
PUBLIC VOID
MUS_SetMasterVolume(
	INT			VolumeLevel		// INPUT : Range (Soft) 1..127 (Loud)
	)
{
#ifdef DEBUG
	DmxFlushTrace();
#endif
	if ( pid1 == -1 )
		return;

	if ( VolumeLevel >= 0 && VolumeLevel <= 127 )
	{
		MasterVolume = ( WORD ) VolumeLevel;
	}
}

/****************************************************************************
* MUS_StopSong - Stop playing song.
*****************************************************************************/
PUBLIC INT
MUS_StopSong(
	INT		Handle			// INPUT : Handle to Song
	)
{
	INT		j;

	if ( Handle < 1 || Handle > MaxSongs || Song[ Handle - 1 ].SongData == NULL )
		return -1;

	for ( j = 0; j < MAX_ACTIVE; j++ )
	{
		if ( SongStatus[ j ].CurrentMusID == ( WORD ) Handle &&
		 	 SongStatus[ j ].ActionsFlag & AF_ACTIVE_MUS
		   )
		{
			SongStatus[ j ].ActionsFlag |= AF_STOP_NOW;
			ServicePlayer();
			return 0;
		}
	}
	return -1;
}

/****************************************************************************
* MUS_PauseSong - Pause playing of Song.
*****************************************************************************/
PUBLIC INT
MUS_PauseSong(
	INT		Handle			// INPUT : Handle to Song
	)
{
	INT		j;

	if ( pid1 == -1 )
		return -1;

	if ( Handle < 1 || Handle > MaxSongs || Song[ Handle - 1 ].SongData == NULL )
		return -1;

	for ( j = 0; j < MAX_ACTIVE; j++ )
	{
		if ( SongStatus[ j ].CurrentMusID == ( WORD ) Handle &&
		 	 SongStatus[ j ].ActionsFlag & AF_ACTIVE_MUS
		   )
		{
			SongStatus[ j ].ActionsFlag |= AF_PAUSE;
			return 0;
		}
	}
	return -1;
}

/****************************************************************************
* MUS_ResumeSong - Resume playing of paused Song.
*****************************************************************************/
PUBLIC INT
MUS_ResumeSong(
	INT		Handle			// INPUT : Handle to Song
	)
{
	INT		j;

	if ( pid1 == -1 )
		return -1;

	if ( Handle < 1 || Handle > MaxSongs || Song[ Handle - 1 ].SongData == NULL )
		return -1;

	for ( j = 0; j < MAX_ACTIVE; j++ )
	{
		if ( SongStatus[ j ].CurrentMusID == ( WORD ) Handle &&
		 	 SongStatus[ j ].ActionsFlag & AF_ACTIVE_MUS
		   )
		{
			SongStatus[ j ].ActionsFlag &= ~AF_PAUSED;
			return 0;
		}
	}
	return -1;
}

/****************************************************************************
* MUS_ChainSong - At end of play, play this next song...
*****************************************************************************/
PUBLIC INT
MUS_ChainSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		To				// INPUT : Chain To Song
	)
{
#ifdef DEBUG
	DmxFlushTrace();
#endif
	if ( pid1 == -1 )
		return -1;

	if ( Handle < 1 || Handle > MaxSongs || Song[ Handle - 1 ].SongData == NULL )
		return -1;

	if ( To < -1 || To > MaxSongs || Song[ To - 1 ].SongData == NULL )
		return -1;

	if ( To < 0 )
		To = 0;

	Song[ Handle - 1 ].ChainTo = To;
	return 0;
}


/****************************************************************************
* MUS_FadeToNextSong - Fade out current song, and play next chained song.
*****************************************************************************/
PUBLIC INT
MUS_FadeToNextSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		TimeMs			// INPUT : Time in ms
	)
{
    int         j;
	MUS_STATUS	*ms;

#ifdef DEBUG
	DmxFlushTrace();
#endif
	if ( pid1 == -1 )
		return -1;

	if ( Handle < 1 || Handle > MaxSongs || Song[ Handle - 1 ].SongData == NULL )
		return -1;


	for ( j = 0; j < MAX_ACTIVE; j++ )
	{
		ms = &SongStatus[ j ];
		if ( ms->CurrentMusID == ( WORD ) Handle &&
  		 	 ms->ActionsFlag & AF_ACTIVE_MUS
			)
		{
			StartSong( Handle, -1, 0, TimeMs );
			break;
		}
	}
	return 0;
}


/****************************************************************************
* MUS_RegisterSong - Register Song for Usage with API 
*****************************************************************************/
PUBLIC INT		// Returns: Handle to Song, -1 = Bad
MUS_RegisterSong(
	BYTE	*SongData		// INPUT : Memory Area song is loaded into
	)
{
	int			j;
	int			is_midi;

#ifdef DEBUG
	DmxFlushTrace();
#endif

	if ( pid1 < 0 )
		return -1;

	if ( memcmp( SongData, "MThd", 4 ) == 0 )
	{
		if ( ( SongData = Midi2Mus( SongData ) ) == NULL )
			return -1;
		is_midi = 1;
	}
	else
		is_midi = 0;

	for ( j = 0; j < MaxSongs; j++ )
	{
		if ( Song[ j ].SongData == NULL )
		{
			Song[ j ].SongData = SongData;
			Song[ j ].ChainTo = 0;
			Song[ j ].Free = is_midi;
			return j + 1;
		}
	}
	return -1;
}

/****************************************************************************
* MUS_UnregisterSong - UnRegister Song from Usage by API 
*****************************************************************************/
PUBLIC INT
MUS_UnregisterSong(
	INT		handle		// INPUT : Handle to Kill
	)
{
	MUS_SONG		*ms;

	if ( handle < 1 || handle > MaxSongs || Song[ handle - 1 ].SongData == NULL )
		return -1;

	if ( MUS_QrySongPlaying( handle ) )
		MUS_StopSong( handle );

	ms = &Song[ handle - 1 ];
	if ( ms->Free )
		free( ms->SongData );
	ms->SongData = NULL;
	return 0;
}


/****************************************************************************
* MUS_DeInit - DeInitialize Sound System
*****************************************************************************/
PUBLIC VOID
MUS_DeInit(
	VOID
	)
{
	if ( Song != NULL )
	{
		free( Song );
		Song = NULL;
	}
	if ( pid1 >= 0 )
	{
		TSM_DelService( pid1 );
		pid1 = -1;
		if ( pid2 >= 0 )
		{
			TSM_DelService( pid2 );
			pid2 = -1;
		}
		Mus->DeInit();
	}
}


/****************************************************************************
* MUS_Init - Initialize Music Sound System
*---------------------------------------------------------------------------*
* Returns:
*	0		Success
*	-1		Failure, Driver not found.
*	-2 		Failure, Bad Driver
*	
*****************************************************************************/
PUBLIC INT
MUS_Init(
	DMX_MUS		*MusDriver,	// INPUT : Driver for Music
	int			rate,		// INPUT : Periodic Rate for Music
	int			max_songs	// INPUT : Max # Registered Songs
	)
{
	Mus = MusDriver;

	if ( ( Song = calloc( max_songs, sizeof( MUS_SONG ) ) ) == NULL )
		return -1;

	wTickRate = rate;
	MaxSongs = max_songs;
	PlrReset();
	lTicksPerSecond = wTickRate;
	pid1 = pid2 = -1;
	if ( ( pid1 = TSM_NewService( ServicePlayer, rate, 10, 0 ) ) == -1 ||
	     ( pid2 = TSM_NewService( ServiceFader, RAMP_TICK_RATE, 9, 0 ) ) == -1
		)
	{
		MUS_DeInit();
		return -1;
	}
	return 0;
}

