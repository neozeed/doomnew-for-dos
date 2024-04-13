/*===========================================================================
* DMXWAV.H - Waveform Communication Structure
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
* $Log:   F:/DMX/INC/VCS/DMXWAV.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:17:44   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _DMXWAV_H
#define _DMXWAV_H

#define WAV_FLG_LOOP		0x00000001

typedef struct
{
	DWORD				dLoopSamples;
	BYTE				*bWavStartPtr;
	DWORD				dSamples;
	BYTE				*bVolPtr;
	BYTE				*bWavPtr;
	DWORD				dMogQuotient;
	DWORD				dMogCorRem;
	DWORD				dPhaseShift;
	DWORD				dQSound;
} VCHAN;

typedef struct
{
	DMX_STATUS	( *Init )		( VOID );
	VOID		( *DeInit )		( VOID );
	VOID		( *PlayMode )	( int Channels, WORD wSampleRate );
	VOID		( *StopMode )	( VOID );
	int			( *Callback )	( void (*func)(void) );
} DMX_WAV;


/****************************************************************************
* WAV_Playing - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
WAV_Playing(
	int			handle
	);

/****************************************************************************
* WAV_AdvSetOrigin - Change origin of active patch.
*****************************************************************************/
PUBLIC VOID
WAV_AdvSetOrigin(
	int			handle,     // INPUT : Handle to Active Patch
	int			pitch,		// INPUT : 0..127..255
	int			left,     	// INPUT : 0=Silent..127=Full Volume
	int			right,    	// INPUT : 0=Silent..127=Full Volume
	int			phase		// INPUT : Phase Shift
	);

/****************************************************************************
* WAV_SetOrigin - Change origin of active patch.
*****************************************************************************/
PUBLIC void
WAV_SetOrigin(
	int			handle,
	int			pitch,		// INPUT : 0..127..255
	int			x,			// INPUT : Left-Right Positioning
	int			volume		// INPUT : Volume Level 1..127
	);

/****************************************************************************
* WAV_StopPatch - Stop playing of active sample.
*****************************************************************************/
PUBLIC void
WAV_StopPatch(
	int			handle
	);

/****************************************************************************
* WAV_AdvPlayPatch - Play Patch.
*****************************************************************************/
PUBLIC int		// Returns: -1=Failure, else good handle
WAV_AdvPlayPatch(
	void		*patch,		// INPUT : Patch to play
	int			pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int			left,    	// INPUT : 0=Silent..127=Full Volume
	int			right,   	// INPUT : 0=Silent..127=Full Volume
	int			phase,		// INPUT : Sample Delay -16=L..0=C..16=R
	int			flags,		// INPUT : Flags
	int			priority	// INPUT : Priority, 0=No Stealing 1..MAX_INT
	);

/****************************************************************************
* WAV_PlayPatch - Play Patch.
*****************************************************************************/
PUBLIC int		// Returns: -1=Failure, else good handle
WAV_PlayPatch(
	void		*patch,		// INPUT : Patch to play
	int			pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int			x,			// INPUT : Left-Right Positioning
	int			volume,		// INPUT : Volume Level 1..127
	int			flags,		// INPUT : Flags 
	int			priority	// INPUT : Priority, 0=Lowest
	);

/****************************************************************************
* WAV_LoadPatch - Loads patch from disk.
*****************************************************************************/
PUBLIC void *	// Returns: Ptr to Patch, NULL=Failure	
WAV_LoadPatch(
	char			*patch_name	// INPUT : Name of Patch File
	);

/****************************************************************************
* WAV_LoadWavFile - Loads patch from disk.
*****************************************************************************/
PUBLIC void *	// Returns: Ptr to Patch, NULL=Failure	
WAV_LoadWavFile(
	char			*wav_name	// INPUT : Name of Patch File
	);

/****************************************************************************
* WAV_PlayMode - Start Play mode on Audio Card
*****************************************************************************/
PUBLIC VOID
WAV_PlayMode(
	int		iChannels,		// INPUT : # of Channels 1..4
	WORD	wSampleRate
	);


/****************************************************************************
* WAV_Init - Initialize Digital Sound System.
*****************************************************************************/
PUBLIC INT
WAV_Init(
	DMX_WAV		*WavDriver
	);


/****************************************************************************
* WAV_DeInit - Terminate Digital Sound System.
*****************************************************************************/
PUBLIC void
WAV_DeInit(
	void
	);

#endif

