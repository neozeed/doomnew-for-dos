/*===========================================================================
* MUSPLR.H - Music Player Interface
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
* $Log:   F:/DMX/INC/VCS/MUSPLR.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:19:24   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _MUSPLR_H
#define _MUSPLR_H

#define MAX_ACTIVE			2
#define MAX_SPLIT			7

typedef struct	// Structure elements must not change order | size.
{
	BYTE		Bank;
	BYTE		Program;
	BYTE		Modulation;
	BYTE		Volume;
	BYTE		PanPot;
	BYTE		Expression;
	BYTE		ReverbDepth;
	BYTE		ChorusDepth;
	BYTE		Hold;
	BYTE		Soft;
	BYTE		Mono;
	BYTE		NoteVelocity;
	BYTE		PitchBend;
	BYTE		Reserved1;
	BYTE		Reserved2;
	BYTE		CurrentVolume;
} CHNL_STATE;

typedef struct
{
	BYTE		*SongData;			// Where MUS data is stored.
	LONG		NextEvent;			// Next event within MUS data.
	LONG		Delay;				// Number of "Ticks" before next event.
	LONG		CurrentMeasure;		// Current Measure

//	WORD		Reserved;			// Reserved Word
	WORD		TotalChannels;		// Required # of Channels for MUS data.
	WORD		ActiveChannels;		// # of Channels ALLOWED for MUS data.
	WORD		BaseChannel;		// Channel Base (Mabye we should interleave?).

	WORD		CurrentMusID;		// ID of current MUS data.
	WORD		ChainMusID;			// ID of MUS to chain (0=none)
	WORD		ActionsFlag;		// Flags to indicate pending/active actions
	SHORT		MaxVolumeLevel;		// Max Volume Level for MUS data
	
	SHORT		CurrentLevel;		// Current Volume Level for MUS data
	SHORT		RampTargetLevel;	// Target Volume Level (when Ramp is Active)
	SHORT		RampLevels;			// Total # of Levels in Ramp
	SHORT		RampLevelSum;		// DDA Level to Increment by...
	SHORT		RampLevelErr;		// DDA Error
	SHORT		RampLevelAcc;		// DDA Error Accumulator
	CHNL_STATE	State[ 16 ];		// State for Channels used by THIS MUS data.
} MUS_STATUS;

/*
* ACTION FLAGS for MUS_STATUS.ActionsFlag
*/
#define AF_IDLE			0x0000
#define AF_ACTIVE_MUS	0x0001
#define AF_RAMP_VOLUME	0x0002
#define AF_PAUSE		0x0004
#define AF_PAUSED		0x0008
#define AF_START		0x0010
#define AF_STOP			0x0020
#define AF_STOP_NOW		0x0040
#define AF_LOADED		0x0080

#endif
