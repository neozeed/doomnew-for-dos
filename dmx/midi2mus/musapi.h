/*===========================================================================
* MUSAPI.H  - Music API Service Prototypes
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
* $Log:   F:/DMX/INC/VCS/MUSAPI.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:18:48   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#ifndef _MUSAPI_H
#define _MUSAPI_H

/****************************************************************************
* MUS_PlaySong - Play Song
*****************************************************************************/
PUBLIC INT
MUS_PlaySong(
	INT		Handle,			// INPUT : Handle to Song
	INT		Volume			// INPUT : Volume Level of Song 1..127
	);

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
	);

/****************************************************************************
* MUS_FadeInSong - Fade Song into Foreground
*****************************************************************************/
PUBLIC INT
MUS_FadeInSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		TimeMs			// INPUT : Time in ms
	);

/****************************************************************************
* MUS_FadeOutSong - Fade Song into Foreground
*****************************************************************************/
PUBLIC INT
MUS_FadeOutSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		TimeMs			// INPUT : Time in ms
	);

/****************************************************************************
* MUS_RegisterSong - Register Song for Usage with API 
*****************************************************************************/
PUBLIC INT		// Returns: Handle to Song, -1 = Bad
MUS_RegisterSong(
	BYTE	*SongData			// INPUT : Memory Area song is loaded into
	);

/****************************************************************************
* MUS_UnregisterSong - UnRegister Song for Usage from API 
*****************************************************************************/
PUBLIC INT
MUS_UnregisterSong(
	INT		handle			// INPUT : Handle to Kill
	);

/****************************************************************************
* MUS_QrySongPlaying - Returns Song Play Status
*****************************************************************************/
PUBLIC INT
MUS_QrySongPlaying(
	INT		Handle			// INPUT : Handle to Song
	);

/****************************************************************************
* MUS_StopSong - Stop playing song.
*****************************************************************************/
PUBLIC INT
MUS_StopSong(
	INT		Handle			// INPUT : Handle to Song
	);

/****************************************************************************
* MUS_PauseSong - Pause playing of Song.
*****************************************************************************/
PUBLIC INT
MUS_PauseSong(
	INT		Handle			// INPUT : Handle to Song
	);

/****************************************************************************
* MUS_ResumeSong - Resume playing of paused Song.
*****************************************************************************/
PUBLIC INT
MUS_ResumeSong(
	INT		Handle			// INPUT : Handle to Song
	);

/****************************************************************************
* MUS_ChainSong - At end of play, play this next song...
*****************************************************************************/
PUBLIC INT
MUS_ChainSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		To					// INPUT : Chain To Song (-1 unchains)
	);

/****************************************************************************
* MUS_GetMasterVolume - Returns Master Volume Setting 1..127
*****************************************************************************/
PUBLIC INT		// Returns volume range of 1..127
MUS_GetMasterVolume(
	void
	);

/****************************************************************************
* MUS_SetMasterVolume - Sets Master Volume 1..127
*****************************************************************************/
PUBLIC VOID
MUS_SetMasterVolume(
	INT		VolumeLevel		// INPUT : Range (Soft) 1..127 (Loud)
	);

/****************************************************************************
* MUS_FadeToNextSong - Fade out current song, and play next chained song.
*****************************************************************************/
PUBLIC INT
MUS_FadeToNextSong(
	INT		Handle,			// INPUT : Handle to Song
	INT		TimeMs			// INPUT : Time in ms
	);

/****************************************************************************
* MUS_Init - Initialize Music Sound System
*****************************************************************************/
PUBLIC INT
MUS_Init(
	DMX_MUS		*MusDriver,	// INPUT : Driver for Music
	int			rate,		// INPUT : Periodic Rate for Music
	int			max_songs	// INPUT : Max # Registered Songs
	);

/****************************************************************************
* MUS_DeInit - DeInitialize Music Sound System
*****************************************************************************/
PUBLIC VOID
MUS_DeInit(
	VOID
	);

#endif
