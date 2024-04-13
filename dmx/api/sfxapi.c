/*===========================================================================
* SFXAPI.C - Sound Effects API
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
* $Log:   F:/DMX/API/VCS/SFXAPI.C_V  $
*  
*     Rev 1.0   02 Oct 1993 15:07:30   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include "dmx.h"

/****************************************************************************
* SFX_Playing - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
SFX_Playing(
	SFX_HANDLE	handle		// INPUT : Handle received from play
	)
{
#ifdef DEBUG
	DmxFlushTrace();
#endif

	switch ( HIWORD( handle ) )
	{
		case FMT_PCS_WAVE:
			return PCS_Playing( LOWORD( handle ) );

		case FMT_GSS_WAVE:
		case FMT_GSS_PATCH:
			return GSS_Playing( LOWORD( handle ) );

		case FMT_WAV_SAMPLE:
			return WAV_Playing( LOWORD( handle ) );
	}
	return 0;
}

/****************************************************************************
* SFX_SetOrigin - Change origin of active patch.
*****************************************************************************/
PUBLIC void
SFX_SetOrigin(
	SFX_HANDLE	handle,		// INPUT : Handle received from play
	int			pitch,		// INPUT : 0..127..255
	int			x,			// INPUT : Left-Right Positioning
	int			volume		// INPUT : Volume Level 1..127
	)
{
	char		str[ 80 ];

	switch ( HIWORD( handle ) )
	{
		case FMT_GSS_WAVE:
		case FMT_GSS_PATCH:
			GSS_SetOrigin( LOWORD( handle ), x, volume );
			break;

		case FMT_WAV_SAMPLE:
			WAV_SetOrigin( LOWORD( handle ), pitch, x, volume );
			break;
	}
}


/****************************************************************************
* SFX_StopPatch - Stop playing of active patch
*****************************************************************************/
PUBLIC void
SFX_StopPatch(
	SFX_HANDLE	handle		// INPUT : Handle received from play
	)
{
	switch ( HIWORD( handle ) )
	{
		case FMT_PCS_WAVE:
			PCS_Stop( LOWORD( handle ) );
			break;

		case FMT_GSS_WAVE:
		case FMT_GSS_PATCH:
			GSS_Stop( LOWORD( handle ) );
			break;

		case FMT_WAV_SAMPLE:
			WAV_StopPatch( LOWORD( handle ) );
			break;
	}
}


/****************************************************************************
* SFX_PlayPatch - Play Patch.
*****************************************************************************/
PUBLIC SFX_HANDLE		// Returns: -1=Failure, else good handle
SFX_PlayPatch(
	void		*patch,		// INPUT : Patch to play
	int			pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int			x,			// INPUT : Left-Right Positioning
	int			volume,		// INPUT : Volume Level 1..127
	int			flags,		// INPUT : Flags 
	int			priority	// INPUT : Priority, 0=Lowest
	)
{
	WORD		fxType;
	int			handle = -1;
	char		str[ 80 ];

    if ( patch == NULL )
        return -1;

	fxType = * ( WORD * )( patch );
	switch ( fxType )
	{
		case FMT_PCS_WAVE:
			handle = PCS_Play( patch, priority );
			break;

		case FMT_GSS_WAVE:
		case FMT_GSS_PATCH:
			handle = GSS_Play( patch, x, pitch, volume, priority );
			break;

		case FMT_WAV_SAMPLE:
			handle = WAV_PlayPatch( patch, pitch, x, volume, flags, priority );
			break;
	}
	if ( handle >= 0 )
	{
		handle |= ( ( long ) fxType << 16 );
	}
	return handle;
}

