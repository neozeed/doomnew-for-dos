/*===========================================================================
* GSSAPI.C - General Sound Set API
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
* $Log:   F:/DMX/API/VCS/GSSAPI.C_V  $
*  
*     Rev 1.2   07 Oct 1993 10:13:20   pjr
*  Fixed bug in play MIDI patch - it was not turning the prevoius note
*  off because the note was never set to anything. (like 60).
*  
*  
*  
*     Rev 1.1   06 Oct 1993 22:08:50   pjr
*  Fixed conflict of usage on gssPatch, created gssInst to resolve 
*  this fixed problem with GENMIDI Single Note Effect Instrument not
*  being properly set.
*  
*  Fixex problem with GSS_WAVE not clearing gssIdx to 0, this caused
*  it to not restart at the beginning if a current wave or patch was
*  currently playing.
*  
*  
*     Rev 1.0   02 Oct 1993 15:08:52   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <conio.h>

#include "dmx.h"

#define FX_CHANNEL	14

LOCAL DMX_MUS		*Mus = NULL;		// Music Command Structure
LOCAL INT	gssId;
LOCAL BYTE	*gssPtr;
LOCAL INT 	gssHandle;
LOCAL INT 	gssPriority;
LOCAL WORD	gssWave;
LOCAL WORD	gssIdx;
LOCAL WORD	gssVolume;
LOCAL WORD	gssPrevSamp;
LOCAL WORD	gssPrevNote;
LOCAL WORD	gssPatch;
LOCAL WORD	gssPanPot;
LOCAL WORD	gssPatch;
LOCAL WORD 	gssBank;
LOCAL WORD 	gssPitch;
LOCAL WORD  gssInst;
LOCAL WORD 	gssNote;
LOCAL WORD	gssSetOrigin = 0;

PRIVATE VOID
GSS_Service(
	VOID
	)
{
	WORD		note;
	WORD		bend;
	BYTE		bPrevColor;
	BYTE		s;

#ifndef PROD
	if ( DmxDebug & DMXBUG_GSSAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 7 );
	}
#endif
	if ( gssWave )
	{
		if ( gssIdx == 0 )
		{
			if ( gssPrevNote )
				Mus->NoteOff( FX_CHANNEL, gssPrevNote );
			gssPrevNote = 0;
			Mus->Command( FX_CHANNEL, CMD_PANPOT, gssPanPot );
			Mus->Command( FX_CHANNEL, CMD_BANK_SELECT, gssBank );
			Mus->Command( FX_CHANNEL, CMD_CHANGE_PROGRAM, gssInst );
			Mus->PitchBend( FX_CHANNEL, 127 );
		}
		else if ( gssIdx >= gssWave )
		{
			if ( gssPrevNote )
			{
				Mus->NoteOff( FX_CHANNEL, gssPrevNote );
				gssPrevNote = 0;
			}
			gssWave = 0;
#ifndef PROD
			if ( DmxDebug & DMXBUG_GSSAPI )
			{
				inp( 0x3da );
				outp( 0x3c0, 0x31 );
				outp( 0x3c0, bPrevColor );
			}
#endif
			return;
		}
		if ( gssSetOrigin )
		{
			gssSetOrigin = 0;
			Mus->Command( FX_CHANNEL, CMD_PANPOT, gssPanPot );
		}
		s = gssPtr[ gssIdx++ ];
		if ( gssPrevSamp != s )
		{
			if ( ! s && gssPrevNote )
			{
				Mus->NoteOff( FX_CHANNEL, gssPrevNote );
				gssPrevNote = 0;
			}
			else
			{
				note = 29 + ( ( s - 1 ) >> 1 );
				bend = ( WORD )( ( s & 1 ) ? 127 : 159 );
				if ( note == gssPrevNote )
					Mus->PitchBend( FX_CHANNEL, bend );
				else
				{
					Mus->NoteOff( FX_CHANNEL, gssPrevNote );
					Mus->PitchBend( FX_CHANNEL, bend );
					Mus->NoteOn( FX_CHANNEL, note, gssVolume );
					gssPrevNote = note;
				}
			}
			gssPrevSamp = s;
		}
	}
	else if ( gssPatch )
	{
		if ( gssIdx == 0 )
		{
			if ( gssPrevNote )
				Mus->NoteOff( FX_CHANNEL, gssPrevNote );
			Mus->Command( FX_CHANNEL, CMD_PANPOT, gssPanPot );
			Mus->Command( FX_CHANNEL, CMD_BANK_SELECT, gssBank );
			Mus->Command( FX_CHANNEL, CMD_CHANGE_PROGRAM, gssInst );
			Mus->PitchBend( FX_CHANNEL, gssPitch );
			Mus->NoteOn( FX_CHANNEL, gssNote, gssVolume );
			gssPrevNote = gssNote;
		}
		if ( ++gssIdx > gssPatch )
		{
			Mus->NoteOff( FX_CHANNEL, gssPrevNote );
			gssPrevNote = 0;
			gssPatch = 0;
		}
		else if ( gssSetOrigin )
		{
			gssSetOrigin = 0;
			Mus->Command( FX_CHANNEL, CMD_PANPOT, gssPanPot );
		}
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_GSSAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
}


/****************************************************************************
* GSS_Stop - Stop playing sample on PC Speaker.
*****************************************************************************/
PUBLIC void
GSS_Stop(
	int			handle
	)
{
	if ( handle == gssHandle )
	{
		if ( gssWave )
			gssIdx = gssWave;
		else if ( gssPatch )
			gssIdx = gssPatch;
	}
}

/****************************************************************************
* GSS_Play - Play sample on PC Speaker.
*****************************************************************************/
PUBLIC int
GSS_Play(
	void	*patch,		// INPUT : Patch to play
	int		x,			// INPUT : Left-Right Positioning
	int		pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int		volume,		// INPUT : Volume Level 1..127
	int		priority	// INPUT : Priority Level: 0 = lowest 
	)
{
	WORD		fxType = *( ( WORD * ) patch );
	GSS_WAVE	*wav;
	GSS_PATCH	*ptc;

	if ( Mus == NULL )
		return -1;

	if ( ( gssWave || gssPatch ) && priority < gssPriority )
		return -1;

	if ( gssWave || gssPatch )
	{
		gssPatch = gssWave = 0;
	}
	else
		gssPrevNote = 0;

	gssPriority = priority;

	gssVolume = volume;
	gssPanPot = x >> 1;
  	if ( fxType == FMT_GSS_WAVE )
	{
		wav = ( GSS_WAVE * ) patch;
		gssIdx = gssPrevSamp = 0;
		gssPtr = ( BYTE * ) patch + sizeof( GSS_WAVE );
		gssBank = wav->Bank;
		gssInst = wav->Patch;
		gssWave = wav->WaveLen;
	}
	else if ( fxType == FMT_GSS_PATCH )
	{
		ptc = ( GSS_PATCH * ) patch;
		gssBank  = ptc->Bank;
		gssPitch = pitch;
		gssNote  = ptc->Note;
		gssIdx 	 = 0;
		gssInst  = ptc->Patch;
		gssPatch = ptc->Duration;
	}
	gssHandle = ++gssHandle & 255;
	return gssHandle;
}

/****************************************************************************
* GSS_SetOrigin - Change origin of active patch.
*****************************************************************************/
PUBLIC void
GSS_SetOrigin(
	int			handle,		// INPUT : Handle received from play
	int			x,			// INPUT : Left-Right Positioning
	int			volume		// INPUT : Volume Level 1..127
	)
{
	if ( Mus == NULL || handle != gssHandle )
		return;

	gssPanPot = x >> 1;
	gssVolume = volume;
}

/****************************************************************************
* GSS_Playing - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
GSS_Playing(
	int		handle
	)
{
	if ( Mus == NULL )
		return -1;

	if ( ( gssWave || gssPatch ) && handle == gssHandle )
		return 1;
	else
		return 0;
}

/****************************************************************************
* GSS_Init - Initialize PC Speaker for 140hz sample playback
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
GSS_Init(
	DMX_MUS		*MusDriver	// INPUT : Driver for Music
	)
{
	gssWave = 0;
	gssPatch = 0;
	if ( ( gssId = TSM_NewService( GSS_Service, 140, 1, 0 ) ) >= 0 )
	 	Mus = MusDriver;
	return gssId;
}

/****************************************************************************
* GSS_DeInit - Removes PC Speaker logic from Timer Chain
*****************************************************************************/
PUBLIC void
GSS_DeInit(
	void
	)
{
	TSM_DelService( gssId );
	if ( gssWave || gssPatch )
		Mus->NoteOff( FX_CHANNEL, gssPrevNote );
}
