/*===========================================================================
* DMXSFX.H 
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
* $Log:   F:/DMX/INC/VCS/DMXSFX.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:18:58   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#ifndef _DMXSFX_H
#define _DMXSFX_H

typedef enum 
{
	FMT_PCS_WAVE,		// PC Speaker Wave
	FMT_GSS_WAVE,		// General Sound System Wave
	FMT_GSS_PATCH,		// General Sound System Patch
	FMT_WAV_SAMPLE		// Wave Sample
} FX_TYPE;

typedef struct
{
	WORD	FxType;		// FMT_PSS_WAVE
	WORD	WaveLen;	// Length in bytes of PC Wave data
} PCS_WAVE;

typedef struct
{
	WORD	FxType;		// FMT_GSS_WAVE
	WORD	WaveLen;	// Length in bytes of GSS Wave data
	WORD	Bank;		// MIDI Bank
	WORD	Patch;		// MIDI Patch #
} GSS_WAVE;

typedef struct
{
	WORD	FxType;		// FMT_GSS_PATCH
	WORD	Bank;		// MIDI Bank
	WORD	Patch;		// MIDI Patch #
	WORD	Note;		// MIDI Note # 0..107
	WORD	Duration;	// Length of note in 140 ticks
	WORD	Reserved;	// Padding
} GSS_PATCH;

typedef struct
{
	WORD	FxType;		// FMT_WAV_SAMPLE
	WORD	Rate;		// Recorded Sample Rate (11025..44100)
	DWORD	WaveLen;	// Length in bytes of WAV Sample
} WAV_SAMPLE;

#endif

