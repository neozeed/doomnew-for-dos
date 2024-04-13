/*===========================================================================
* DMXWAV.C
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
* $Log:   F:/DMX/API/VCS/WAVAPI.C_V  $
*  
*     Rev 1.0   02 Oct 1993 15:07:08   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dmx.h"
#include "dspapi.h"

PRIVATE DMX_WAV	*Wav = NULL;

/*----------------------------------------------------------------------*
*     Below are the Microsoft definitions of the WAVE format
*-----------------------------------------------------------------------*/
typedef char		FOURCC[ 4 ];

typedef struct CHUNK
{
	FOURCC		id;	  	/* chunk ID					   				*/
	DWORD		size; 	/* chunk size				   				*/
	FOURCC		Type; 	/* form type or list type	   				*/
} CHUNK;

typedef struct TAG
{
	FOURCC		id;	  	/* chunk ID									*/
	DWORD		size; 	/* chunk size								*/
} TAG;

typedef struct _MMCKINFO
{
	FOURCC		ckid;		  /* chunk ID							*/
	DWORD 		cksize;		  /* chunk size							*/
	FOURCC		fccType;	  /* form type or list type				*/
	DWORD 		dwDataOffset; /* offset of data portion of chunk	*/
	DWORD 		dwFlags;	  /* flags								*/
} MMCKINFO;

typedef struct waveformat_tag
{
	WORD  		wFormatTag;			/* Format Type				  	*/
	WORD  		nChannels;			/* Number of Channels		  	*/
	DWORD 		nSamplesPerSec;		/* Number of sampes per second	*/
	DWORD 		nAveBytesPerSec;	/* average data rate		  	*/
	WORD  		nBlockAlign;		/* block alignment			  	*/
} WAVEFORMAT;

typedef struct pcmwaveformat_tag
{
	WAVEFORMAT	wf;					/* general format information	*/
	WORD		wBitsPerSample;		/* number of bits per sample 	*/
} PCMWAVEFORMAT;
																							  	

/****************************************************************************
* WAV_Playing - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
WAV_Playing(
	int			handle
	)
{
	return dspIsPlaying( handle );
}

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
	)
{
	dspAdvSetOrigin( handle, pitch, left, right, phase );
}

/****************************************************************************
* WAV_SetOrigin - Change origin of active patch.
*****************************************************************************/
PUBLIC void
WAV_SetOrigin(
	int			handle,
	int			pitch,		// INPUT : 0..127..255
	int			x,			// INPUT : Left-Right Positioning
	int			volume		// INPUT : Volume Level 1..127
	)
{
	dspSetOrigin( handle, pitch, x, volume );
}


/****************************************************************************
* WAV_StopPatch - Stop playing of active sample.
*****************************************************************************/
PUBLIC void
WAV_StopPatch(
	int			handle
	)
{
	dspStopPatch( handle );
}

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
	)
{
	 return dspAdvPlayPatch( patch, pitch, left, right, phase, flags, priority );
}

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
	int			priority	// INPUT : Priority, 0=No Stealing 1..MAX_INT
	)
{
	return dspPlayPatch( patch, pitch, x, volume, flags, priority );
}

/****************************************************************************
* WAV_LoadPatch - Loads patch from disk.
*****************************************************************************/
PUBLIC void *	// Returns: Ptr to Patch, NULL=Failure	
WAV_LoadPatch(
	char			*patch_name	// INPUT : Name of Patch File
	)
{
	int			fh;
	WAV_SAMPLE	wav;
	BYTE		*mem;

	fh = open( patch_name, O_BINARY | O_RDONLY );
	if ( fh != -1 )
	{
		read( fh, &wav, sizeof( WAV_SAMPLE ) );
		if ( ( mem = malloc( ( size_t )( sizeof( WAV_SAMPLE ) +
										  wav.WaveLen ) ) ) == NULL )
			return NULL;

		memcpy( mem, &wav, sizeof( WAV_SAMPLE ) );
		read( fh, mem + sizeof( WAV_SAMPLE ), ( size_t ) wav.WaveLen );
		close( fh );
		return mem;
	}
	return NULL;
}


/****************************************************************************
* WAV_LoadWavFile - Loads patch from disk.
*****************************************************************************/
PUBLIC void *	// Returns: Ptr to Patch, NULL=Failure	
WAV_LoadWavFile(
	char			*wav_name	// INPUT : Name of Patch File
	)
{
	int				fh;
	WAV_SAMPLE		wav;
	BYTE			*mem;
	PCMWAVEFORMAT 	pcmwf;
	CHUNK			chunk;
	TAG	 			tag;
	long 			ofs;
	int				len;

	if ( ( fh = open( wav_name, O_BINARY | O_RDONLY ) ) == -1 )
		return NULL;

	len = read( fh, &chunk, sizeof( CHUNK ) );
	mem = NULL;
	while( len == sizeof( CHUNK ) )
	{
		if ( memcmp( chunk.Type, "WAVE", 4 ) != 0 )
		{
			lseek( fh, chunk.size, SEEK_CUR );
			len = read( fh, &chunk, sizeof( CHUNK ) );
			continue;
		}
		read( fh, &tag, sizeof( TAG ) );
		if ( memcmp( tag.id, "fmt ", 4 ) != 0 )
			break;
		ofs = tell( fh );
		read( fh, &pcmwf, sizeof( PCMWAVEFORMAT ) );
		lseek( fh, ofs + tag.size, SEEK_SET );
		read( fh, &tag, sizeof( TAG ) );

		if ( memcmp( tag.id, "data", 4 ) != 0 
			 || pcmwf.wBitsPerSample != 8
			 || pcmwf.wf.nChannels != 1
			 || pcmwf.wf.nBlockAlign != 1 
		   )
		{
			break;
		}
		memset( &wav, 0, sizeof( WAV_SAMPLE ) );
		wav.FxType 	= FMT_WAV_SAMPLE;
		wav.Rate 	= ( WORD ) pcmwf.wf.nSamplesPerSec;
		wav.WaveLen	= tag.size;

		if ( ( mem = malloc( ( size_t ) wav.WaveLen + sizeof( WAV_SAMPLE ) ) ) == NULL )
		{
			break;
		}
		read( fh, mem + sizeof( WAV_SAMPLE ), ( size_t ) tag.size );
		memcpy( mem, &wav, sizeof( WAV_SAMPLE ) );
	}
	close( fh );
	return mem;
}

/****************************************************************************
* WAV_PlayMode - Start Play mode on Audio Card
*****************************************************************************/
PUBLIC VOID
WAV_PlayMode(
	int		iChannels,		// INPUT : # of Channels 1..4
	WORD	wSampleRate
	)
{
#ifndef PROD
	DmxFlushTrace();
	DmxTrace( "WAV_PlayMode" );
#endif
	if ( Wav )
	{
		Wav->PlayMode( iChannels, wSampleRate );
	}
}

/****************************************************************************
* WAV_Init - Initialize Digital Sound System.
*****************************************************************************/
PUBLIC INT
WAV_Init(
	DMX_WAV		*WavDriver
	)
{
	Wav = WavDriver;
	return 0;
}


/****************************************************************************
* WAV_DeInit - Terminate Digital Sound System.
*****************************************************************************/
PUBLIC void
WAV_DeInit(
	void
	)
{
	if ( Wav )
	{
		Wav->DeInit();
		Wav = NULL;
	}
}

