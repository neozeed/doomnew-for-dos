/*===========================================================================
* DMXAPI.C  - Digital/Music/Effects API General Maitenance Services
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
* $Log:   F:/DMX/API/VCS/DMXAPI.C_V  $
*  
*     Rev 1.1   02 Oct 1993 15:11:06   pjr
*  
*     Rev 1.0   30 Aug 1993 18:36:40   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <malloc.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dos.h>

#include "dmx.h"

EXTERN DMX_MUS 	AweMus;
EXTERN DMX_MUS 	MpuMus;
EXTERN DMX_MUS	FmMus;
EXTERN DMX_WAV	clWav;
EXTERN DMX_WAV	mvWav;
EXTERN DMX_MUS 	Gf1Mus;
EXTERN DMX_WAV	Gf1Wav;

GLOBAL DWORD	DmxDebug = DMXBUG_NONE;
GLOBAL DWORD	DmxOption = 0;
LOCAL int		TraceActive = 0;
GLOBAL BYTE		gPrevColor = 0;

LOCAL INT		HardWare = 0;
LOCAL DMX_WAV	*WavDrv;

#ifndef PROD
LOCAL BYTE			vidRows;
LOCAL WORD 			vidCols;
LOCAL WORD			vidRowSize;
LOCAL BYTE			TraceQue[ 65536 ];
LOCAL WORD			Queued = 0;
LOCAL INT			TraceFile = -1;
LOCAL volatile INT	FileBusy = 0;

void
DmxFlushTrace(
	void
	)
{
	if ( Queued && TraceFile != -1 )
	{
		FileBusy = 1;
		write( TraceFile, TraceQue, Queued );
		Queued = 0;
		FileBusy = 0;
	}
}


void
DmxTrace(
	char	*routine
	)
{
	LOCAL BYTE	*lastpos = NULL ;
	LOCAL int 	row = 0;
	LOCAL int 	col = 0;
	LOCAL BYTE	*vidram = ( BYTE * ) ( 0xB8000L );
	LOCAL BYTE	*keybit = ( BYTE * ) ( 0x00000417L );

	UINT		flags;
	unsigned	j,k;
	unsigned	pos;

	if ( ! TraceActive )
		return;

	if ( ! ( DmxDebug & DMXBUG_TRACE ) )
		return ;

	flags = PUSH_DISABLE();
	if ( DmxDebug & DMXBUG_TOFILE )
	{
		if ( FileBusy == FALSE )
		{
			for ( j = Queued; *routine != '\0' && j < sizeof( TraceQue ) - 16 ;)
			{
				TraceQue[ j++ ] = *routine++;
			}
			if ( *routine )
			{
				memcpy( &TraceQue[ j ], "\r\nQ-OVERFLOW", 12 );
			}
			if ( j < sizeof( TraceQue ) - 2 )
			{
				TraceQue[ j++ ] = '\r';
				TraceQue[ j++ ] = '\n';
				Queued = j;
			}
		}
	}
	else if ( ( *keybit & 16 ) != 0 )
	{
		if ( lastpos )
		{
			for ( j = 0; j < 25; j++ )
			{
				lastpos[ ( j << 1 ) + 1 ] = 0x07;
			}
			j <<= 1;
			lastpos[ j ] = ' ';
			lastpos[ j + 1 ] = 0x07;
		}
		pos = (row*vidRowSize)+(col*52);
		lastpos = vidram + pos;
		for ( j = 0; j < 25; j++ )
		{
			k = pos + ( j << 1 );
			if ( *routine )
				vidram[ k ] = *routine++;
			else
				vidram[ k ] = ' ';
			vidram[ k + 1 ] = 0x4F;
		}
		row++;
		if ( row >= vidRows )
		{
			row = 0;
			col++;
			if ( col >= vidCols )
				col = 0;
		}
		while ( ( inp( 0x201 ) & 0x30 ) != 0x30 )
			;
	}
	POP_DISABLE( flags );
}

void
pDmxTrace(
	char	*str,
	int		args,
	int		arg1,
	int		arg2,
	int		arg3,
	int		arg4,
	int		arg5,
	int		arg6
	)
{
	char	txt[ 80 ];
	int		len;
	int		arg[ 6 ];
	int		j;

	arg[ 0 ] = arg1;
	arg[ 1 ] = arg2;
	arg[ 2 ] = arg3;
	arg[ 3 ] = arg4;
	arg[ 4 ] = arg5;
	arg[ 5 ] = arg6;

	strcpy( txt, str );
	len = strlen( txt );
	for ( j = 0; j < args; j++ )
	{
		itoa( arg[ j ], &txt[ len ], 10 );
		if ( j < ( args - 1 ) )
		{
			strcat( txt, "," );
			len = strlen( txt );
		}
	}
	DmxTrace( txt );
}

#endif

void
DMX_InitTrace(
	void
	)
{
	char		*p;

	if ( TraceActive )
		return ;
	if ( ( p = getenv( "DMXTRACE" ) ) != NULL )
	{
		DmxDebug |= atoi( p );
#ifdef DEBUG
		if ( DmxDebug & DMXBUG_TRACE )
		{
			vidCols = * ( WORD * )( 0x44AL );
			vidRows = * ( BYTE * )( 0x484L );
			if ( vidRows == 0 )
				vidRows = 24;
			++vidRows;
			vidRowSize = vidCols * 2;
			vidCols /= 26;
		}
		if ( DmxDebug & DMXBUG_TOFILE )
		{
			TraceFile = open( "DMX.LOG", O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE );
			printf( "\nTRACING TO DMX.LOG\n" );
		}
#endif
	}
	TraceActive = 1;
}

void
DMX_DeInitTrace(
	void
	)
{
	if ( ! TraceActive )
		return;

	TraceActive = 0;
#ifdef DEBUG
	if ( TraceFile != -1 )
	{
		DmxFlushTrace();
		close( TraceFile );
		TraceFile = -1;
	}
#endif
}

/****************************************************************************
* DMX_DeInit - DeInitialize Sound System
*****************************************************************************/
PUBLIC VOID
DMX_DeInit(
	VOID
	)
{
	PCS_DeInit();
	GSS_DeInit();
	WAV_DeInit();
	MUS_DeInit();
	DMX_DeInitTrace();
}

/****************************************************************************
* DMX_Init - Initialize DMX Sound System
*****************************************************************************/
PUBLIC INT		// Returns: Hardware Flags
DMX_Init(
	int			tickrate,	// INPUT : Music Quantinization Rate
	int			max_songs,	// INPUT : Max # Registered Songs
	DWORD		mus_device,	// INPUT : Valid Device ID for Music Playback
	DWORD		sfx_device	// INPUT : Valid Device ID for Sound Effects
	)
{
	BOOL		fMus = FALSE;
	BOOL		fSfx = FALSE;
	BOOL		fWav = FALSE;
	int			combh = mus_device | sfx_device;
	char		*p;

	if ( ( p = getenv( "DMXOPTION" ) ) != NULL )
	{
		if ( strstr( p, "-opl3" ) )
			DmxOption |= USE_OPL3;
		if ( strstr( p, "-phase" ) )
			DmxOption |= USE_PHASE;
	}
	HardWare = 0;
	DMX_InitTrace();
/*
* Assign Devices ...
*/
	if ( combh & AHW_ULTRA_SOUND )
	{
		if ( ( sfx_device & AHW_ULTRA_SOUND ) && Gf1Wav.Init() == RS_OKAY )
		{
			WavDrv = &Gf1Wav;
			WAV_Init( &Gf1Wav );
			HardWare |= AHW_ULTRA_SOUND;
			fWav = TRUE;
		}
		if ( ( mus_device & AHW_ULTRA_SOUND ) && fMus == FALSE )
        {
		    if ( Gf1Mus.Init() == RS_OKAY )
		    {
			    if ( ( mus_device & AHW_ULTRA_SOUND ) && fMus == FALSE )
			    {
				    MUS_Init( &Gf1Mus, tickrate, max_songs );
				    fMus = TRUE;
			    }
			    if ( ( sfx_device & AHW_ULTRA_SOUND ) && fSfx == FALSE )
			    {
				    GSS_Init( &Gf1Mus );
				    fSfx = TRUE;
			    }
			    HardWare |= AHW_ULTRA_SOUND;
		    }
        }
	}
	if ( sfx_device & AHW_MEDIA_VISION )
	{
		if ( fWav == FALSE && mvWav.Init() == RS_OKAY )
		{
			WavDrv = &mvWav;
			WAV_Init( &mvWav );
			HardWare |= AHW_MEDIA_VISION;
			fWav = TRUE;
		}
	}
	if ( mus_device & AHW_AWE32 )
	{
		if ( fMus == FALSE && AweMus.Init() == RS_OKAY )
		{
			MUS_Init( &AweMus, tickrate, max_songs );
			GSS_Init( &AweMus );
			fMus = TRUE;
			HardWare |= AHW_AWE32;
		}
	}
	if ( combh & AHW_SOUND_BLASTER )
	{
		if ( sfx_device & AHW_SOUND_BLASTER )
		{
			if ( fWav == FALSE && clWav.Init() == RS_OKAY )
			{
				WavDrv = &clWav;
				WAV_Init( &clWav );
				HardWare |= AHW_SOUND_BLASTER;
				fWav = TRUE;
			}
		}
	}
	if ( combh & AHW_MPU_401 )
	{
		if ( MpuMus.Init() == RS_OKAY )
		{
			if ( ( mus_device & AHW_MPU_401 ) && fMus == FALSE )
			{
				MUS_Init( &MpuMus, tickrate, max_songs );
				fMus = TRUE;
			}
			if ( ( sfx_device & AHW_MPU_401 ) && fSfx == FALSE )
			{
				GSS_Init( &MpuMus );
				fSfx = TRUE;
			}
			HardWare |= AHW_MPU_401;
		}
	}
	if ( combh & AHW_ADLIB )
	{
		if ( ( fMus == FALSE || fSfx == FALSE ) && FmMus.Init() == RS_OKAY )
		{
			if ( ( mus_device & AHW_ADLIB ) && fMus == FALSE )
			{
				MUS_Init( &FmMus, tickrate, max_songs );
				fMus = TRUE;
			}
			if ( ( sfx_device & AHW_ADLIB ) && fSfx == FALSE )
			{
				GSS_Init( &FmMus );
				fSfx = TRUE;
			}
			HardWare |= AHW_ADLIB;
		}
	}
	if ( sfx_device & AHW_PC_SPEAKER )
	{
		PCS_Init();
		HardWare |= AHW_PC_SPEAKER;
	}
	return HardWare;
}


/****************************************************************************
* DMX_SetIdleCallback - Sets callback hook when timing can be acurate.
****************************************************************************/
PUBLIC int
DMX_SetIdleCallback(
	int		(*callback)( void )
	)
{
	if ( WavDrv )
	{
		return WavDrv->Callback( callback );
	}
	return 0;
}
