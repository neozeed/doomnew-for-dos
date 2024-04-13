/*===========================================================================
* WAV2DSP.C - Convert WAV files to DSP format
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
* $Log:$
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dmx.h"

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
																							  	
/*----------------------------------------------------------------------*
*                          CONVERSION CODE 
*-----------------------------------------------------------------------*/
int
main(
	int		argc,
	char	*argv[]
	)
{
	PCMWAVEFORMAT 	pcmwf;
	CHUNK			chunk;
	TAG	 			tag;
	long 			ofs;
	long			trim;
	BYTE			threshold = 2;
	int	 			fh;
	char			sfn[ 200 ];
	char 			ofn[ 200 ];
	int				j;
	BYTE 			*ptr;
	BYTE 			*buffer;
	WAV_SAMPLE		wav;
	BYTE			pad[ 16 ];

	printf( "WAV2DSP File Converter  v2.2\n" );
	printf( "Copyright (C) 1993,94 Digital Expressions Inc.\n" );
	printf( "All Rights Reserved.\n" );
	if ( argc < 2 )
	{
		printf( "\nUsage: WAV2DSP [options] <file.wav>\n" );
		printf(   "		-f##	Filter Noise from Head & Tail < +-##\n" );
		return 20;
	}
	for ( j = 1; j < argc; j++ )
	{
		if ( *argv[ j ] == '-' )
		{
			if ( argv[ j ][ 1 ] != 'f' )
			{
				printf( "-%c is not a valid option.\n" );
				return 20;
			}
			threshold = ( BYTE ) atoi( &argv[ j ][ 2 ] );
			printf( "Trim threshold: %d\n", threshold );
			continue;
		}
		break;
	}
	for ( ; j < argc; j++ )
	{
		strcpy( sfn, argv[ j ] );
		if ( ( ptr = strrchr( sfn, '.' ) ) == NULL || *(ptr+1) == '\\' )
			strcat( sfn, ".wav" );

		if ( ( fh = open( sfn, O_RDONLY | O_BINARY ) ) == -1 )
		{
			perror( "Cannot open wave file" );
			return 20;
		}
		read( fh, &chunk, sizeof( CHUNK ) );
		if ( memcmp( chunk.Type, "WAVE", 4 ) != 0 )
		{
			printf( "Unsupported Format\n" );
			return 20;
		}
		read( fh, &tag, sizeof( TAG ) );
		if ( memcmp( tag.id, "fmt ", 4 ) != 0 )
		{
			printf( "Could not find \"fmt \" tag.\n" );
			return 20;
		}
		ofs = tell( fh );
		read( fh, &pcmwf, sizeof( PCMWAVEFORMAT ) );
		lseek( fh, ofs + tag.size, SEEK_SET );
		read( fh, &tag, sizeof( TAG ) );
		if ( memcmp( tag.id, "data", 4 ) != 0 )
		{
			printf( "Could not find \"data\" tag.\n" );
			return 20;
		}
		printf( "Size of sample: %ld bytes\n", tag.size );
		if ( ( buffer = malloc( ( size_t ) tag.size ) ) == NULL )
		{
			printf( "Not enough memory to catch wave!\n" );
			return 20;
		}
		read( fh, buffer, ( size_t ) tag.size );
		close( fh );

		if ( pcmwf.wBitsPerSample != 8 )
		{
			printf( "Unsupported bits per sample, must be 8-Bit.\n" );
			return 20;
		}
		if ( pcmwf.wf.nChannels != 1 )
		{
			printf( "Stereo samples are not supported.\n" );
			return 20;
		}
		if ( pcmwf.wf.nBlockAlign != 1 )
		{
			printf( "Invalid block alignment.\n" );
			return 20;
		}
		printf( "   Sample rate: %ld bytes\n", pcmwf.wf.nSamplesPerSec );
		memset( &wav, 0, sizeof( WAV_SAMPLE ) );
		wav.FxType 	= FMT_WAV_SAMPLE;
		wav.Rate 	= ( WORD ) pcmwf.wf.nSamplesPerSec;
		wav.WaveLen	= tag.size;

		if ( ( ptr = strrchr( sfn, '\\' ) ) == NULL )
			ptr = sfn;
		else
			++ptr;

		strcpy( ofn, ptr );
		if ( ( ptr = strrchr( ofn, '.' ) ) )
			*ptr = '\0';

		strupr( ofn );
		strcat( ofn, ".DSP" );

		fh = open( ofn, O_CREAT|O_TRUNC|O_RDWR|O_BINARY, S_IREAD|S_IWRITE );
		if ( fh == -1 )
		{
			perror( "Cannot open output file" );
			return 20;
		}
		trim = 0;
		if ( threshold )
		{
			BYTE	min = 128 - threshold;
			BYTE	max = 128 + threshold;

			while ( wav.WaveLen > 0  )
			{
				if ( *buffer > min && *buffer < max )
				{
					++buffer;
					wav.WaveLen--;
					++trim;
					continue;
				}
				break;
			}
			if ( wav.WaveLen )
				ptr = buffer + ( wav.WaveLen - 1 );

			while ( wav.WaveLen > 0 )
			{
				if ( *ptr > min && *ptr < max )
				{
					--ptr;
					wav.WaveLen--;
					++trim;
					continue;
				}
				break;
			}
			printf( "       Trimmed: %ld bytes\n", trim );
		}
		wav.WaveLen += ( sizeof( pad ) * 2 );
		write( fh, &wav, sizeof( WAV_SAMPLE ) );
		memset( pad, *buffer, sizeof( pad ) );
		write( fh, pad, sizeof( pad ) );
		write( fh, buffer, ( size_t ) wav.WaveLen - ( sizeof( pad ) * 2 ) );
		memset( pad, buffer[ wav.WaveLen - ( sizeof( pad ) * 2 ) - 1 ], sizeof( pad ) );
		write( fh, pad, sizeof( pad ) );
		close( fh );
	}
	return 0;
}
