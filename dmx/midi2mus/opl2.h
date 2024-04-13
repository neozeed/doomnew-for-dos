/*===========================================================================
* OPL2.H - Bank Storage Format
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
* $Log:   F:/DMX/INC/VCS/OPL2.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:17:32   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _OPL2_H
#define _OPL2_H

typedef struct
{
 	BYTE			bAmVibEgKsrMult;
 	BYTE			bAttackDecay;
 	BYTE			bSustainRelease;
	BYTE			bWaveSelect;
 	BYTE			bKeyScale;
 	BYTE			bVelocity;
	BYTE			bFeedBackConnect;
} OP2_CELL;

typedef struct
{
 	BYTE			bAmVibEgKsrMult;
 	BYTE			bAttackDecay;
 	BYTE			bSustainRelease;
	BYTE			bWaveSelect;
 	BYTE			bKeyScale;
 	BYTE			bVelocity;
	BYTE			bFeedBackConnect;
	BYTE			bReserved;
} OPL_CELL;

typedef struct
{
	OP2_CELL		Modulator;
	OP2_CELL		Carrier;
 	SHORT			sTranspose;
} OP2_VOICE;


typedef struct
{
	OPL_CELL		Modulator;
	OPL_CELL		Carrier;
 	SHORT			sTranspose;
	SHORT			sReserved;
} OPL_VOICE;

typedef struct 
{
 	WORD			fFNoteDVibVoc2;
 	BYTE			bPitchBend;
 	BYTE			bNote;
	OP2_VOICE	Voice1;
	OP2_VOICE	Voice2;
} OP2_INST;

typedef struct 
{
 	WORD			fFNoteDVibVoc2;
 	BYTE			bPitchBend;
 	BYTE			bNote;
	OPL_VOICE	Voice1;
	OPL_VOICE	Voice2;
} OPL_INST;

typedef struct
{
	BYTE				Signature[ 8 ];		// #OPL_II#
	OP2_INST			Instrument[ 175 ];
} OP2_BANK;

typedef struct
{
	BYTE				Signature[ 8 ];		// #OPL_II#
	OPL_INST			Instrument[ 175 ];
} OPL_BANK;

/*
* Flags for fFNoteDVibVoc2
*/
#define FLG_FIXED_NOTE		0x0001
#define FLG_DELAYED_VIB		0x0002
#define FLG_VOICE2			0x0004

#endif
