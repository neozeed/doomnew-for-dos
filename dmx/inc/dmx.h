/*===========================================================================
* DMX.H - Digital & Music Structures & Definitions
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
* $Log:   F:/DMX/INC/VCS/DMX.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:21:16   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _DMX_H
#define _DMX_H

#include "types.h"

#define SFX_HANDLE		long


#define MAX_CMDS		8
#define MAX_DRIVERS		6

typedef enum
{
	RS_OKAY,
	RS_BOARD_NOT_FOUND,
	RS_BANK_NOT_FOUND,
	RS_BANK_NOT_VALID,
	RS_NOT_ENOUGH_MEMORY,
	RS_NO_FREE_INTERRUPTS,
	RS_DRIVER_ALREADY_LOADED,
	RS_INVALID_DRIVER,
	RS_DUPLICATE_DRIVER,
	RS_INVALID_CLOCK_RATE,
	RS_NO_DRIVERS_SPECIFIED,
	RS_DRIVER_NOT_FOUND,
	RS_FAIL
} DMX_STATUS;

/*
* Driver 'GetStatus' Return Values:
*/
#define DS_NOT_READY	0x0000
#define DS_READY		0x0001

//--------------------------------------
// Driver "TYPE" Definition:
//--------------------------------------
#define DRV_MIDI		0x0001
#define DRV_WAVE		0x0002
#define DRV_TIMER		0x0004
#define DRV_USER		0x0008

#define DRV_RECORD		0x1000
#define DRV_PATCHES		0x2000
#define DRV_OPL2		0x0100
#define DRV_OPL3		0x0200
#define DRV_OPL4		0x0300
#define DRV_GENMIDI		0x0400
#define DRV_WAVETBLE	0x0500

#define DRV_EXE_PENDING	0x8000

//--------------------------------------
// INTERRUPT SERVICES:
//--------------------------------------
#define DMX_RESET			0x00
#define DMX_START_TMR		0x01
#define DMX_STOP_TMR		0x02
#define DMX_CALLBACK_ON		0x03
#define DMX_CALLBACK_OFF	0x04

//--------------------------------------
// SYSTEM INCLUDES:
//--------------------------------------
#include "dmxmus.h"
#include "musapi.h"
#include "pcsapi.h"
#include "gssapi.h"
#include "dmxwav.h"
#include "dmxapi.h"
#include "tsmapi.h"
#include "sfxapi.h"
#include "pcints.h"
#include "pcdma.h"
#include "dmxsfx.h"
#include "hwsetup.h"
#endif
