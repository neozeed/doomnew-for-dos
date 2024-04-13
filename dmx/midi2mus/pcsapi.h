/*===========================================================================
* PCSAPI.H - PC Speaker API
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
* $Log:   F:/DMX/INC/VCS/PCSAPI.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:20:16   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#ifndef _PCSAPI_H
#define _PCSAPI_H

/****************************************************************************
* PCS_Stop - Stop playing sample on PC Speaker.
*****************************************************************************/
PUBLIC void
PCS_Stop(
	int			handle
	);

/****************************************************************************
* PCS_Play - Play sample on PC Speaker.
*****************************************************************************/
PUBLIC int
PCS_Play(
	void	*patch,		// INPUT : Patch to play
	int		priority	// INPUT : Priority Level: 0 = lowest 
	);

/****************************************************************************
* PCS_Playing - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
PCS_Playing(
	int		handle
	);

/****************************************************************************
* PCS_Init - Initialize PC Speaker for 140hz sample playback
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
PCS_Init(
	void
	);

/****************************************************************************
* PCS_DeInit - Removes PC Speaker logic from Timer Chain
*****************************************************************************/
PUBLIC void
PCS_DeInit(
	void
	);

#endif
