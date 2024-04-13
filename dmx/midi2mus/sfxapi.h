/*===========================================================================
* SFXAPI.H - Sound Effects API
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
* $Log:   F:/DMX/INC/VCS/SFXAPI.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:20:52   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _SFXAPI_H
#define _SFXAPI_H

#define SFX_LOOP	0x0001
#define SFX_ABS		0x0002


/****************************************************************************
* SFX_Playing - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
SFX_Playing(
	SFX_HANDLE	handle		// INPUT : Handle received from play
	);

/****************************************************************************
* SFX_SetOrigin - Change origin of active patch.
*****************************************************************************/
PUBLIC void
SFX_SetOrigin(
	SFX_HANDLE	handle,		// INPUT : Handle received from play
	int			pitch,		// INPUT : 0..127..255
	int			x,			// INPUT : Left-Right Positioning
	int			volume		// INPUT : Volume Level 1..127
	);

/****************************************************************************
* SFX_StopPatch - Stop playing of active patch
*****************************************************************************/
PUBLIC void
SFX_StopPatch(
	SFX_HANDLE	handle		// INPUT : Handle received from play
	);

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
	);

#endif
