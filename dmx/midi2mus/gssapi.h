/*===========================================================================
* GSSAPI.H - General Sound Set API
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
* $Log:   F:/DMX/INC/VCS/GSSAPI.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:19:34   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _GSSAPI_H
#define _GSSAPI_H

/****************************************************************************
* GSS_Stop - Stop playing patch on audio card.
*****************************************************************************/
PUBLIC void
GSS_Stop(
	int			handle
	);

/****************************************************************************
* GSS_Play - Play patch on audio card.
*****************************************************************************/
PUBLIC int
GSS_Play(
	void	*patch,		// INPUT : Patch to play
	int		x,			// INPUT : Left-Right Positioning
	int		pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int		volume,		// INPUT : Volume Level 1..127
	int		priority	// INPUT : Priority Level: 0 = lowest 
	);

/****************************************************************************
* GSS_SetOrigin - Change origin of active patch.
*****************************************************************************/
PUBLIC void
GSS_SetOrigin(
	int			handle,		// INPUT : Handle received from play
	int			x,			// INPUT : Left-Right Positioning
	int			volume		// INPUT : Volume Level 1..127
	);

/****************************************************************************
* GSS_Playing - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
GSS_Playing(
	int		handle
	);

/****************************************************************************
* GSS_Init - Initialize GSS for 140hz sample playback
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
GSS_Init(
	DMX_MUS		*MusDriver	// INPUT : Driver for Music
	);

/****************************************************************************
* GSS_DeInit - Removes PC Speaker logic from Timer Chain
*****************************************************************************/
PUBLIC void
GSS_DeInit(
	void
	);

#endif
