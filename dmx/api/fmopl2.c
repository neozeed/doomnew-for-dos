/*===========================================================================
* FM3812.C - FM Driver for Ad Lib & Compatibles 
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
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <conio.h>

#include "dmx.h"
#include "opl2.h"

#define DEPTH		0xBD

#define DEP_AM		0x80
#define DEP_VIB		0x40

#define MELODIC		0
#define PERCUSSIVE	1

#define ONE_OCTAVE	12

LOCAL OP2_BANK		Bank;

#include "freqtbl.h"

typedef struct POLY
{
	WORD			Voice;
	WORD			Channel;
	WORD			Note;
	WORD			TNote;
	WORD			Velocity;
	SHORT			PitchBend;
	WORD			Voice2;
	WORD			Patch;
} POLY;

typedef struct
{
	short			PitchBend;
	WORD			Volume;
	WORD			VolumeScale;
	WORD			Patch;
	WORD			Bank;
	BOOL			Mono;
	WORD			PanPot;
	WORD			Res3;
} CHANNEL;

LOCAL CHANNEL	ChanInfo[ 16 ];

#define MAX_VOICES	18

LOCAL INT		Port = 0x388;
LOCAL INT		PolyVoices;
LOCAL POLY		Poly[ MAX_VOICES ];
LOCAL INT		PolyActive[ MAX_VOICES ];
LOCAL INT		PolyActiveCnt;
LOCAL INT		PolyInactive[ MAX_VOICES ];
LOCAL INT		PolyInactiveCnt;
LOCAL INT		PolyCutOff;
LOCAL BYTE		InstNotes[ 175 ];

LOCAL INT       WaitStates  = 6;
LOCAL INT		PortIdxDelay = 6;
LOCAL INT		PortDtaDelay = 24;
LOCAL INT		MaxVoices = 9;
LOCAL INT		Opl3 = 0;
LOCAL INT		ready = DS_NOT_READY;
LOCAL INT		fm_semaphore = 0;
LOCAL BOOL		fOverride = FALSE;

typedef struct
{
	INT				Modifier;
	INT				Carrier;
} OP_CHAIN;

typedef struct
{
	OP_CHAIN	 	Op;
	OP2_VOICE		*Patch;
	WORD		 	CarKsl;
	WORD		 	ModKsl;
	WORD		 	ModVol;
	WORD			Port;
	BYTE			Chnl;
	BYTE			Reserved;
} FM_VOICE;

LOCAL FM_VOICE		OplVoice[ MAX_VOICES ];
LOCAL WORD			VoiceFreq[ MAX_VOICES ];
LOCAL BYTE			FeedBackCon[ MAX_VOICES ];
LOCAL BYTE			Stereo[ MAX_VOICES ];

LOCAL OP_CHAIN		MelodicChain[] =
{
	0x00,	0x03,	// Voice 1
	0x01,	0x04,	// Voice 2
	0x02,	0x05,	// Voice 3
	0x08,	0x0B,	// Voice 4
	0x09,	0x0C,	// Voice 5
	0x0A,	0x0D,	// Voice 6
	0x10,	0x13,	// Voice 7
	0x11,	0x14,	// Voice 8
	0x12,	0x15	// Voice 9
};

/*==========================================================================*
* FmOut - Write Data to Y262
*===========================================================================*/
PRIVATE VOID
FmOut(
	WORD		io_port,
  	WORD		addr,
	BYTE		data
	)
{
	int		j;

	outp( io_port, addr );
	for ( j = PortIdxDelay; j > 0; j-- )
		inp( io_port + 1 );

	outp( io_port + 1, data );
	for ( j = PortDtaDelay; j > 0; j-- )
		inp( io_port );
}

/*==========================================================================*
* FmOutPort0 - Write Data to FM1312 Chip
*===========================================================================*/
PRIVATE VOID
FmOutPort0(
  	WORD		addr,
	BYTE		data
	)
{
	int		j;

	outp( Port, addr );
	for ( j = PortIdxDelay; j > 0; j-- )
		inp( Port );

	outp( Port + 1, data );
	for ( j = PortDtaDelay; j > 0; j-- )
		inp( Port );
}

/*==========================================================================*
* FmOutPort1 - Write Data to Y262
*===========================================================================*/
PRIVATE VOID
FmOutPort1(
  	WORD		addr,
	BYTE		data
	)
{
	int		j;

	outp( Port + 2, addr );
	for ( j = PortIdxDelay; j > 0; j-- )
		inp( Port );

	outp( Port + 3, data );
	for ( j = PortDtaDelay; j > 0; j-- )
		inp( Port );
}

/*==========================================================================*
* FmBoardInstalled - Detect if Adlib card is present
*===========================================================================*/
PRIVATE INT	/* Returns: 1=Installed, 0=Can't find card.	*/
FmBoardInstalled(
	VOID
	)
{
	int		j;
	int		s1;
	int		s2;
	int		type;

	FmOutPort0( ( WORD ) 4, ( BYTE ) 0x60 );	// MASK T1 & T2
	FmOutPort0( ( WORD ) 4, ( BYTE ) 0x80 );	// Reset IRQ

	s1 = inp( Port );			// Read Status Register

	FmOutPort0( ( WORD ) 2, ( BYTE ) 0xff );	// Set TIMER-1 Latch 
	FmOutPort0( ( WORD ) 4, ( BYTE ) 0x21 );	// Unmask & Start T1

	for ( j = 200; j > 0; j-- )
		inp( Port );			// Delay for at least 80 uSec for timer-1 overflow

	s2 = inp( Port );			// Read Status Register

	FmOutPort0( ( WORD ) 4, ( BYTE ) 0x60 );	// MASK T1 & T2
	FmOutPort0( ( WORD ) 4, ( BYTE ) 0x80 );	// Reset IRQ

	type = ( ( s1 & 0xE0 ) == 0 && ( s2 & 0xE0 ) == 0xC0 ? 2 : 0 );
	if ( type )
	{
		MaxVoices = 9;
/*
* Attempt to detect OPL3
* 0x388 = 0x06  = OPL2
* 0x388 = 0x00 && 0x38A == 0xAA = OPL2
* 0x388 = 0x00 && 0x38A == 0xFF = OPL3 !!
*/
		if ( ( DmxOption & USE_OPL3 ) != 0 
			&& inp( Port ) == 0
			&& inp( Port + 2 ) == 0xFF
			)
		{
			MaxVoices = 18;
			type = 3;
			Opl3 = 1;
		}
	}
	return type;
}

/*==========================================================================*
* FmLoadPatch - Load Patch into FM Voice
*===========================================================================*/
PRIVATE VOID
FmLoadPatch(
	WORD		Voice,	// INPUT : OPL Voice         		0..8
	OP2_VOICE	*Patch	// INPUT : OPL Patch 
	)
{
	FM_VOICE	*v = &OplVoice[ Voice ];
	OP2_CELL	*Op;
	INT			OpIdx;
	WORD		Port;

	if ( v->Patch != Patch )
	{
		Port = v->Port;
		v->Patch = Patch;
		OpIdx = v->Op.Carrier;
		Op = &Patch->Carrier;

		v->CarKsl = Op->bKeyScale | 0x3F;
		FmOut( Port, ( WORD )( 0x40 + OpIdx ), ( BYTE )( v->CarKsl ) );
		FmOut( Port, ( WORD )( 0x20 + OpIdx ), Op->bAmVibEgKsrMult );
		FmOut( Port, ( WORD )( 0x60 + OpIdx ), Op->bAttackDecay );
		FmOut( Port, ( WORD )( 0x80 + OpIdx ), Op->bSustainRelease );
		FmOut( Port, ( WORD )( 0xE0 + OpIdx ), Op->bWaveSelect );

		OpIdx = v->Op.Modifier;
		Op = &Patch->Modulator;

		if ( Op->bFeedBackConnect & 1 )
		{
			v->ModVol = ( BYTE )( 63 - Op->bVelocity );
			v->ModKsl = Op->bKeyScale | 0x3F;
		}
		else
		{
			v->ModVol = 0;
			v->ModKsl = Op->bKeyScale | Op->bVelocity;
		}
		FmOut( Port, ( WORD )( 0x40 + OpIdx ), ( BYTE ) v->ModKsl );
		FmOut( Port, ( WORD )( 0x20 + OpIdx ), Op->bAmVibEgKsrMult );
		FmOut( Port, ( WORD )( 0x60 + OpIdx ), Op->bAttackDecay );
		FmOut( Port, ( WORD )( 0x80 + OpIdx ), Op->bSustainRelease );
		FmOut( Port, ( WORD )( 0xE0 + OpIdx ), Op->bWaveSelect );

		FeedBackCon[ Voice ] = Op->bFeedBackConnect;
		FmOut( Port, ( WORD )( 0x00C0 + v->Chnl ),
			( BYTE )( Op->bFeedBackConnect | Stereo[ Voice ] ) );
	}
}

PRIVATE VOID
FmSetVoiceFreq(
	POLY		*p
	)
{
	short		freq_idx;
  	WORD		freq;
	WORD		Port;
	WORD		Chnl;
	WORD		Voice;
	WORD		oct;

	freq_idx = ( p->TNote << 5 ) +
				ChanInfo[ p->Channel ].PitchBend +
				p->PitchBend;

	oct = 0;
	if ( freq_idx < 0 )
		freq_idx = 0;
	else if ( freq_idx >= 284 )
	{
		freq_idx -= 284;
		oct = freq_idx / 384;
		if ( oct > 7 )
			oct = 7;
		freq_idx = ( freq_idx % 384 ) + 284;
	}
	freq = fm_freq[ freq_idx ] | ( oct << 10 );

	Voice = p->Voice;
	Port = OplVoice[ Voice ].Port;
	Chnl = OplVoice[ Voice ].Chnl;

	FmOut( Port, ( WORD )( 0x00A0 + Chnl ),
			( BYTE )( freq & 0x00FF ) );

	FmOut( Port, ( WORD )( 0x00B0 + Chnl ),
			( BYTE )( ( freq >> 8 ) | 0x20 ) );

	VoiceFreq[ Voice ] = freq;
}

/*--------------------------------------------------------------------------*
* FmClear - Clear FM Chip
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmClear(
	VOID
	)
{
	WORD		v;
	WORD		p;
	WORD		j;
/*
* Stop any notes that are on
*/
	for ( j = 0x40; j <= 0x55; j++ )
		FmOutPort0( j, ( BYTE ) 0x3F );

	for ( j = 0x60; j <= 0xF5; j++ )
		FmOutPort0( j, ( BYTE ) 0 );

	for ( j = 1; j < 0x40; j++ )
		FmOutPort0( j, ( BYTE ) 0 );

	FmOutPort0( ( WORD ) 4, ( BYTE ) 0x60 );	// MASK T1 & T2
	FmOutPort0( ( WORD ) 4, ( BYTE ) 0x80 );	// Reset IRQ
	FmOutPort0( ( WORD ) 1, ( BYTE ) 0x20 );	// Enable Waveform Select

	if ( Opl3 )
	{
		FmOutPort1( ( WORD ) 5, ( BYTE ) 0x01 );	// Turn on OPL-3
		for ( j = 0x40; j <= 0x55; j++ )
			FmOutPort1( j, ( BYTE ) 0x3F );

		for ( j = 0x60; j <= 0xF5; j++ )
			FmOutPort1( j, ( BYTE ) 0 );

		for ( j = 1; j < 0x40; j++ )
			FmOutPort1( j, ( BYTE ) 0 );
	}
	memset( InstNotes, 0, sizeof( InstNotes ) );
	memset( Poly, 0, sizeof( Poly ) );
	memset( Stereo, 0x30, sizeof( Stereo ) );
	PolyVoices = MaxVoices;
	PolyCutOff = PolyVoices - 2;
	PolyActiveCnt = 0;
	PolyInactiveCnt = PolyVoices;
	for ( j = 0; j < ( WORD ) PolyVoices; j++ )
	{
		v = j % 9;
		p = 0x388 + ( ( j / 9 ) * 2 );

		OplVoice[ j ].Op = MelodicChain[ v ];
		OplVoice[ j ].Patch = NULL;
		OplVoice[ j ].Port = p;
		OplVoice[ j ].Chnl = v;
		PolyInactive[ j ] = j;
		Poly[ j ].Voice = j;
		FmLoadPatch( j, NULL );
	}
	memset( ChanInfo, 0, sizeof( ChanInfo ) );
	for ( j = 0; j < 16; j++ )
	{
		ChanInfo[ j ].Volume = 127;
		ChanInfo[ j ].VolumeScale = 256;
		ChanInfo[ j ].PitchBend = 64;
		ChanInfo[ j ].PanPot = 0x30;
	}
	memset( VoiceFreq, 0, sizeof( VoiceFreq ) );
}

PUBLIC VOID
FmProgInst(
	WORD			InstIdx,
	OP2_INST		*Inst
	)
{
	INT		j;

	for ( j = 0; j < PolyVoices; j++ )
	{
		OplVoice[ j ].Patch = NULL;
	}
	if ( InstIdx < 175 )
	{
		memcpy( &Bank.Instrument[ InstIdx ], Inst, sizeof( OP2_INST ) );
	}
}


/*--------------------------------------------------------------------------*
* FmChangePatch - Change Patch for Channel
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmChangePatch(
	WORD		Channel,	// INPUT : Channel            	0..15
	WORD		NewPatch	// INPUT : Patch to Change to 	0..127
	)
{
	if ( Channel < 15 )
	{
		ChanInfo[ Channel ].Patch = NewPatch;
	}
}

/*--------------------------------------------------------------------------*
* FmReset - Reset FM Chip
*---------------------------------------------------------------------------*/
PRIVATE VOID 
FmReset(
	void
	)
{
	FmClear();
	FmOutPort0( ( WORD ) 8, ( BYTE ) 0x40 );	// Tempered Scale
	if ( Opl3 )
	{
		FmOutPort1( ( WORD ) 5, ( BYTE ) 0x01 );	// Turn on OPL-3 
	}
}


/*--------------------------------------------------------------------------*
* FmTransposeNote - Transpose Note Up or Down 'n' Semi-Tones and pull note 
*                   back into range if required.
*---------------------------------------------------------------------------*/
PRIVATE WORD		/* Returns: Transposed Note		*/
FmTransposeNote(
	WORD		Note,				// INPUT : Note to Transpose
	INT		TransShift		// INPUT : Transpose Shift	in Semi-Tones
	)
{
	INT		iNote = ( INT ) Note;

	iNote += TransShift;

	if ( iNote < 0 )
	{
		do
		{
			iNote += ONE_OCTAVE;
		}
		while ( iNote < 0 );
	}
	else if ( iNote > 95 )
	{
		do
		{
			iNote -= ONE_OCTAVE;
		}
		while ( iNote > 95 );
	}
	return ( BYTE )( iNote );
}

/*--------------------------------------------------------------------------*
* FmSetVoiceVolume - Set Volume for Melodic Voice 
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmSetVoiceVolume(
 	WORD		Channel,	// INPUT : Channel to Note On		0..15
	WORD		Voice,	// INPUT : FM Voice					0..Poly
	WORD		Velocity // INPUT : Velocity of Note		0..127
	)
{
	FM_VOICE		*v;
	BYTE			bVol;
	BYTE			bModVol;
	WORD			Volume;

	Volume = ( VolTable[ Velocity ] * ChanInfo[ Channel ].VolumeScale ) >> 9;
	bVol = ( BYTE )( 63 - Volume );

	v = &OplVoice[ Voice ];
	if ( ( v->CarKsl & 0x3F ) != bVol )
	{
		v->CarKsl = ( v->CarKsl & 0xc0 ) | bVol;
	  	FmOut( v->Port, ( WORD )( 0x0040 + v->Op.Carrier ), ( BYTE )( v->CarKsl ) );
		if ( v->ModVol )
		{
			bModVol = 63 - v->ModVol;

			if ( bModVol < bVol )
				bVol = ( v->ModKsl & 0xc0 ) |  bVol;
			else
				bVol = ( v->ModKsl & 0xc0 ) | bModVol;

			if ( bVol != v->ModKsl )
			{
		  		FmOut( v->Port, ( WORD )( 0x0040 + v->Op.Modifier ), bVol );
				v->ModKsl = bVol;
			}
		}
  	}
}

PRIVATE VOID
FmPolyNoteOff(
	INT			PolyIdx
	)
{
	INT			Idx = PolyActive[ PolyIdx ];
	INT			Voice = Poly[ Idx ].Voice;
	INT			j;

	FmOut( OplVoice[ Voice ].Port,
		( WORD )( 0x00B0 + OplVoice[ Voice ].Chnl ),
		( BYTE )( VoiceFreq[ Voice ] >> 8 ) );

	VoiceFreq[ Voice ] = 0;
/*
* Unlink voice from active list
*/
	if ( PolyActiveCnt < 0 )
		PolyActiveCnt = 1;

	PolyActiveCnt--;
	for ( j = PolyIdx; j < PolyActiveCnt; j++ )
		PolyActive[ j ] = PolyActive[ j + 1 ];

	PolyInactive[ PolyInactiveCnt++ ] = Idx;
}

/*--------------------------------------------------------------------------*
* FmNoteOff - Turn Note OFF
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmNoteOff(
	WORD		Channel,	// INPUT : Channel 					0..15
	WORD		Note		// INPUT : Note To Kill     		0..107
	)
{
	POLY			*p;
	INT			j;

#ifndef PROD
	pDmxTrace( "NOTE OFF ", 4, Channel, Note, PolyActiveCnt, PolyInactiveCnt, 0, 0 );
#endif
	for ( j = 0; j < PolyActiveCnt; )
	{
		p = &Poly[ PolyActive[ j ] ];
	 	if ( p->Channel == Channel && p->Note == Note )
		{
			FmPolyNoteOff( j );
			continue;
		}
		j++;
	}
}

/*--------------------------------------------------------------------------*
* FmAllNotesOff - Turn Notes OFF for given channel
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmAllNotesOff(
	WORD		Channel	// INPUT : Channel 					0..15
	)
{
	POLY		*p;
	INT			j;

	for ( j = 0; j < PolyActiveCnt; )
	{
		p = &Poly[ PolyActive[ j ] ];
	 	if ( p->Channel == Channel )
		{
			FmPolyNoteOff( j );
			continue;
		}
		j++;
	}
}

/*--------------------------------------------------------------------------*
* FmPolyVoiceOn - Turn Poly Voice On
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmPolyVoiceOn(
 	WORD		Channel,	// INPUT : Channel to Note On		0..15
	WORD		Note,		// INPUT : Note To Play      		0..107
	WORD		Velocity,	// INPUT : Velocity of Note
	OP2_INST	*inst,		// INPUT : Instrument
	WORD		Voice2		// INPUT : Play 2nd Voice
	)
{
	POLY	  	*p;
	INT		  	j;
	INT		  	PolyIdx;
	WORD	  	FmVoice;
	OP2_VOICE 	*Voice;

	if ( PolyInactiveCnt == 0 )
	{
		return;
	}
/*
* Remove Last Unused Voice from Inactivity List
*/
	PolyIdx = PolyInactive[ 0 ];
	PolyInactiveCnt--;
	for ( j = 0; j < PolyInactiveCnt; j++ )
		PolyInactive[ j ] = PolyInactive[ j + 1 ];
/*
* Place voice into active list
*/
	PolyActive[ PolyActiveCnt++ ] = PolyIdx;
/*
* Keep Note Info in Poly Record
*/
	p = &Poly[ PolyIdx ];

	FmVoice = p->Voice;
	if ( Voice2 )
	{
		p->PitchBend = ( short )( inst->bPitchBend >> 1 ) - 64;
		Voice = &inst->Voice2;
	}
	else
	{
		Voice = &inst->Voice1;
		p->PitchBend = 0;
	}
	p->Channel	= Channel;
	p->Note		= Note;
	p->Velocity = Velocity;
	p->Voice2	= Voice2;
  	p->Patch 	= ChanInfo[ Channel ].Patch;

	Stereo[ FmVoice ] = ChanInfo[ Channel ].PanPot;
	if ( Channel == 15 )
	{
		FmLoadPatch( FmVoice, Voice );
		if ( ( inst->fFNoteDVibVoc2 & FLG_FIXED_NOTE ) != 0 )
			Note = inst->bNote;
		else
			Note = 60;
	}
	else
	{
		FmLoadPatch( FmVoice, Voice );
		if ( ( inst->fFNoteDVibVoc2 & FLG_FIXED_NOTE ) != 0 )
			Note = inst->bNote;
		else
			Note = FmTransposeNote( Note, ( INT ) Voice->sTranspose );
	}
	p->TNote = Note;
	FmSetVoiceVolume( Channel, FmVoice, Velocity );
	FmSetVoiceFreq( p );
}


/*--------------------------------------------------------------------------*
* FmNoteOn - Turn Note ON
*---------------------------------------------------------------------------*/
PRIVATE VOID 
FmNoteOn(
 	WORD		Channel,	// INPUT : Channel to Note On		0..15
	WORD		Note,		// INPUT : Note To Play      		0..107
	WORD		Velocity	// INPUT : Velocity of Note		1..127
	)
{
	POLY		*p;
	OP2_INST	*inst;
	WORD 		Patch;
	INT			j;

	if ( ChanInfo[ Channel ].Mono )
		FmAllNotesOff( Channel );

	if ( Channel == 15 )
	{
		if ( Note < 35 || Note > 81 )
			return;

		Patch = Note + 93;
	}
	else
	{
		Patch = ChanInfo[ Channel ].Patch;
	}
	inst = &Bank.Instrument[ Patch ];

#ifndef PROD
	pDmxTrace( "NOTE ON   CH NT ACNT ICNT  ", 4, Channel, Note, PolyActiveCnt, PolyInactiveCnt, 0, 0 );
#endif
/*
* Steal any voices if nessesary...
*/
	while ( ( PolyVoices - PolyActiveCnt ) < 1 )
	{
		int	perfered = 0;
		int chnl = 0;

		for ( j = 0; j < PolyActiveCnt; j++ )
		{
			p = &Poly[ PolyActive[ j ] ];
			if ( p->Channel >= chnl || p->Voice2 )
			{
				perfered = j;
				chnl = p->Channel;
			}
		}
#ifndef PROD
		p = &Poly[ PolyActive[ perfered ] ];
		pDmxTrace( "   STEAL ", 5, perfered, p->Channel, p->Note, Channel, Note, 0 );
		for ( j = 0; j < PolyActiveCnt; j++ )
		{
			p = &Poly[ PolyActive[ j ] ];
			pDmxTrace( "   Active ", 3, j, p->Channel, p->Note, 0, 0, 0 );
		}
#endif
		FmPolyNoteOff( perfered );
	}
/*
* We now have a free voice to play our note...
*/
	FmPolyVoiceOn( Channel, Note, Velocity, inst, ( WORD ) 0 );

	if ( ( PolyVoices - PolyActiveCnt ) >= 1 
		&& ( inst->fFNoteDVibVoc2 & FLG_VOICE2 ) != 0 )
	{
	/*
	* Only play the 2nd part when we have an idle voice
	*/
		FmPolyVoiceOn( Channel, Note, Velocity, inst, ( WORD ) 1 );
	}
}

/*--------------------------------------------------------------------------*
* PitchBend - Perform 
*---------------------------------------------------------------------------*/
PRIVATE VOID 
FmPitchBend(
	WORD		Channel,	// INPUT : Channel to Note On	  0..15
	WORD		Bend		// INPUT : Pitch Bend           0..255 127=Normal
	)
{
	INT			pia[ MAX_VOICES ];
	INT			pa[ MAX_VOICES ];
	POLY		*p;
	INT			j;
	INT			k;
	INT			active;
/*
* Bend Range: 0 = 2 full semitones down, 255 = 2 full semitones up
*             127 = no change
*/
	ChanInfo[ Channel ].PitchBend = ( short )( Bend >> 1 );

	for ( active = k = j = 0; j < PolyActiveCnt; j++ )
	{
		p = &Poly[ PolyActive[ j ] ];
		if ( p->Channel == Channel )
		{
			pa[ active++ ] = PolyActive[ j ];
			FmSetVoiceFreq( p );
		}
		else
		{
			pia[ k++ ] = PolyActive[ j ];
		}
	}
	for ( j = 0; j < active; j++ )
	{
		pia[ k++ ] = pa[ j ];
	}
	memcpy( PolyActive, pia, sizeof( PolyActive ) );
}

/*--------------------------------------------------------------------------*
* ChangeVolume - Change Volume of ALL Notes on Channel
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmChangeVolume(
	WORD		Channel,	// INPUT : Channel           	  0..15
	WORD		Volume	// INPUT : Volume               0..127
	)
{
	POLY		*p;
	INT			j;

	if ( ChanInfo[ Channel ].Volume != Volume )
	{
		ChanInfo[ Channel ].Volume = Volume;
		ChanInfo[ Channel ].VolumeScale = ( VolTable[ Volume ] + 1 ) << 1;
		for ( j = 0; j < PolyActiveCnt; j++ )
		{
			p = &Poly[ PolyActive[ j ] ];
			if ( p->Channel == Channel )
			{
				FmSetVoiceVolume( Channel, p->Voice, p->Velocity );
			}
		}
	}
}

/*--------------------------------------------------------------------------*
* FmChangePanPot - Change Pan Positioning for Channel
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmChangePanPot(
	WORD		Channel,	// INPUT : Channel	0..15
	WORD		PanPot		// INPUT : Pan Pot	0..63 64 65..127
	)
{
	POLY		*p;
	INT			j;
	BYTE		mask;
	BYTE		pan;

	if ( ! Opl3 )
		return ;

	if ( PanPot <= 48 )
		pan = 0x20;
	else if ( PanPot >= 96 )
		pan = 0x10;
	else
		pan = 0x30;

	if ( ChanInfo[ Channel ].PanPot != pan )
	{
		ChanInfo[ Channel ].PanPot = ( WORD ) pan;
		for ( j = 0; j < PolyActiveCnt; j++ )
		{
			p = &Poly[ PolyActive[ j ] ];
			if ( p->Channel == Channel )
			{
				FM_VOICE	*v = &OplVoice[ p->Voice ];

				Stereo[ p->Voice ] = pan;
				mask = FeedBackCon[ p->Voice ];
				FmOut( v->Port, ( WORD )( 0x00C0 + v->Chnl ), mask | pan );
			}
		}
	}
}


/*--------------------------------------------------------------------------*
* Command - Perform Command Logic
*---------------------------------------------------------------------------*/
PRIVATE VOID 
FmCommand(
	WORD		Channel,	// INPUT : Channel           	  0..15
	WORD		CmdCode,	// INPUT : Command Code
	WORD		CmdData	// INPUT : Command Related Data
	)
{
	switch ( CmdCode )
	{
		case CMD_CHANGE_PROGRAM:
			FmChangePatch( Channel, CmdData );
			break;

		case CMD_BANK_SELECT:
			ChanInfo[ Channel ].Bank = CmdData;
			break;

		case CMD_VOLUME:
			FmChangeVolume( Channel, CmdData );
			break;

		case CMD_PANPOT:
			FmChangePanPot( Channel, CmdData );
			break;

		case CMD_HOLD1:
		case CMD_SOFT:
			break;

		case CMD_ALL_SOUNDS_OFF:
		case CMD_ALL_NOTES_OFF:
			FmAllNotesOff( Channel );
			break;

		case CMD_RESET_CHANNEL:
			FmAllNotesOff( Channel );
			ChanInfo[ Channel ].PitchBend = 64;
			ChanInfo[ Channel ].PanPot = 0x30;
			ChanInfo[ Channel ].Mono = FALSE;
			break;

		case CMD_MONO:
			ChanInfo[ Channel ].Mono = TRUE;
			break;

		case CMD_POLY:
			ChanInfo[ Channel ].Mono = FALSE;
			break;

		default:
			break;
	}
}


/*--------------------------------------------------------------------------*
* FmLoadBank - Load Bank File into RAM
*---------------------------------------------------------------------------*/
PRIVATE VOID
FmLoadBank(
	VOID
	)
{
	int		fh;

	if ( fOverride == TRUE )
		return;

	if ( ( fh = open( "GENMIDI.OP2", O_BINARY | O_RDWR ) ) != -1 )
	{
		read( fh, ( char * ) &Bank, sizeof( OP2_BANK ) );
		close( fh );
	}
}


/*--------------------------------------------------------------------------*
* FmInit - Initialize Driver
*---------------------------------------------------------------------------*/
PRIVATE DMX_STATUS
FmInit(
	VOID
	)
{
	PortIdxDelay = WaitStates;
	PortDtaDelay = WaitStates * 4;
	fm_semaphore = 0;

	ready = DS_NOT_READY;
	if ( FmBoardInstalled() )
	{
		FmReset();
		FmLoadBank();
		return RS_OKAY;
	}
	return RS_BOARD_NOT_FOUND;
}

PRIVATE VOID
FmDeInit(
	void
	)
{
	ready = DS_NOT_READY;
	FmClear();
}

/****************************************************************************
* FmEnterBusy - FM Entry into critical section of code, only if not busy.
*****************************************************************************/
PUBLIC int	// returns: 0 = OK, 1 = Busy
FmEnterBusy(
	void
	)
{
	++fm_semaphore;
	if ( fm_semaphore > 1 )
	{
		--fm_semaphore;
		return 1;
	}
	return 0;
}

/****************************************************************************
* FmLeave - Leave critical section of code.
*****************************************************************************/
PUBLIC void
FmLeave(
	void
	)
{
	if ( fm_semaphore )
		--fm_semaphore;
}

PUBLIC DMX_MUS FmMus =
{
	FmInit,
	FmDeInit,
	FmNoteOff,
	FmNoteOn,
	FmPitchBend,
	FmCommand,
	FmEnterBusy,
	FmLeave
};

/****************************************************************************
* AL_SetCard - Changes card properties.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*****************************************************************************/
PUBLIC VOID
AL_SetCard(
    int         iWaitStates,    // INPUT : # of I/O waitstates
	void		*BankData		// INPUT : (opt) Ptr to Bank Data
    )
{
    WaitStates = iWaitStates;
	if ( BankData )
	{
		memcpy( &Bank, BankData, sizeof( OP2_BANK ) );
		fOverride = TRUE;
	}
}

/****************************************************************************
* AL_Detect - Detects Ad Lib Card and returns current waitstates.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
AL_Detect(
    int         *iWaitStates,   // OUTPUT: # of waitstates
	int			*iType			// OUTPUT: Board Type
    )
{
	int	 type;
	if ( ( type = FmBoardInstalled() ) )
    {
        if ( iWaitStates )
            *iWaitStates = WaitStates;

		if ( iType )
			*iType = type;

        return 0;
    }
    return 1;
}


#if 0
void main( void )
{
	WORD		Port;
	WORD		Chnl;
	short	  	freq_idx;
	WORD		freq;
	int			j;

	FmInit();
	FmLoadPatch( 0, &Bank.Instrument[ 109 ].Voice1 );
	FmSetVoiceVolume( 0, 0, 127 );

	Port = 0x388;
	Chnl = 0;
	freq_idx = 0;
	for ( freq_idx = 0; freq_idx < ASIZE( fm_freq ); freq_idx++ )
	{
		freq = fm_freq[ freq_idx ];
		FmOut( Port, ( WORD )( 0x00A0 + Chnl ), ( BYTE )( freq & 0x00FF ) );
		FmOut( Port, ( WORD )( 0x00B0 + Chnl ), ( BYTE )( ( freq >> 8 ) | 0x20 ) );
		for ( j = 0; j < 200; j++ )
		{
			inp( 0x389 );
			if ( kbhit() )
			{
				freq_idx = ASIZE( fm_freq );
				break;
			}
		}
		printf( "Index:%4d, Freq:%04x\r", freq_idx, freq );
	}
	FmDeInit();
}
	
#endif
