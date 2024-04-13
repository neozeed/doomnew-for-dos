/*===========================================================================
* PLAYMUS.C - MIDI Music Player
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
* $Log:   F:/DMX/API/VCS/PLAYMUS.C_V  $
*  
*     Rev 1.0   02 Oct 1993 15:12:34   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

/******************************************************************************
* Sample Music Player
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>

#include "dmx.h"

#define F1		315
#define F2		316
#define F3		317
#define F4		318
#define F5		319
#define F6		320
#define F7		321
#define F8		322
#define F9		323
#define F10		324
#define F11		389
#define F12		390

LOCAL WORD		HwFlags = AHW_ANY;
LOCAL WORD		Tempo = 140;

BYTE *
LoadFile(
	char		*filename,
	long		*length
	)
{
	int			fh;
	BYTE		*LoadedFile = NULL;

	if ( ( fh = open( filename, O_RDONLY | O_BINARY ) ) == -1 )
		return NULL;

	if ( ( *length = filelength( fh ) ) > 0 )
	{
		if ( ( LoadedFile = malloc( ( unsigned )( *length ) ) ) != NULL )
		{
			read( fh, ( char * ) LoadedFile, ( unsigned )( *length ) );
		}
	}
	close( fh );
	return LoadedFile;
}

int
kbread( void )
{
	int		key;

	key = getch();
	if ( key == 0x00 || key == 0xE0 )
		key = getch() | 0x100;
	return key;
}


int		a,b,c;

int proc_stub( int j )
{
	return j * 10;
}

void proc_a( void )
{
	int		j;

	inp( 0x3da );
	outp( 0x3c0, 0x31 );
	outp( 0x3c0, 0x32 );

	++a;

	for ( j = 0; j < 8192; j++ )
	{
		proc_stub( proc_stub( j ) );
	}
	inp( 0x3da );
	outp( 0x3c0, 0x31 );
	outp( 0x3c0, 0 );
}

void proc_b( void )
{
	inp( 0x3da );
	outp( 0x3c0, 0x31 );
	outp( 0x3c0, 0x33 );

	++b;

	inp( 0x3da );
	outp( 0x3c0, 0x31 );
	outp( 0x3c0, 0x00 );
}
void proc_c( void )
{
	inp( 0x3da );
	outp( 0x3c0, 0x31 );
	outp( 0x3c0, 0x34 );

	++c;

	inp( 0x3da );
	outp( 0x3c0, 0x31 );
	outp( 0x3c0, 0x00 );
}


void
CheckArgs(
	int		argc,
	char	*argv[]
	)
{
	int		j;

	for ( j = 1; j < argc; j++ )
	{
		if ( *argv[ j ] == '-' )
		{
			switch ( argv[ j ][ 1 ] )
			{
				case 'A':
				case 'a':
					HwFlags = AHW_ADLIB;
					printf( "\nAdLib Mode\n" );
					break;

				case 'U':
				case 'u':
					printf( "\nGravis Ultra Sound Mode\n" );
					HwFlags = AHW_ULTRA_SOUND;
					break;
					
				case 'M':
				case 'm':
					printf( "\nMPU-401 Mode\n" );
					HwFlags = AHW_MPU_401;
					break;

				case 'T':
				case 't':
					Tempo = (WORD) atoi( &argv[ j ][2] );
					printf( "\nNew Tempo: %d\n", Tempo );
					break;
			}
		}
	}
	
}

BYTE *
GetFile(
	char		*filename
	)
{
	BYTE		*filedata;
	long		flen;

	if ( ( filedata = LoadFile( filename, &flen ) ) == NULL ||
		  	flen <= sizeof( DMX_HEADER ) ||
		  	memcmp( filedata, "MUS", 3 ) != 0 
			)
	{
		printf( "\nError loading file: %s\n", filename );
		return NULL;
	}
	return filedata;
}

main(
	int		argc,
	char	*argv[]
	)
{
	BYTE		*filedata;
	char		*map;
	int			size;
	int			fh;
	INT			rc;
	INT			sh[ 9 ];
	INT			songs;
	INT	 		ActiveSong = 0;
	INT			j;
	INT			Rate;
	int			port;
	int			h;

	if ( argc < 2 )
	{
		printf( "Usage: PLAYMUS [options] <MUSfile>\n" );
		printf( "Options:\n" );
		printf( "    -a     AdLib Mode\n" );
		printf( "    -m     MPU-401 Mode\n" );
		printf( "    -u     Gravis Ultrasound Mode\n" );
		printf( "    -tnnn  Set Tempo where 'nnn' is new tempo (e.g. -t280)\n" );
		printf( "\n" );
		return;
	}
	printf( "Initializing System...\n" );
	CheckArgs( argc, argv );
 	if ( HwFlags == AHW_ANY )
	{
		printf( "\nAuto-detect mode\n" );
	}
	port = 0;
	if ( MPU_Detect( &port, NULL ) == 0 )
		printf( "MPU-401 Detected @ %03Xh\n", port );

	if ( AL_Detect( NULL, &j ) == 0 )
		printf( "Yamaha OPL%d Detected\n", j );

	if ( GF1_Detect() == 0 )
	{
		printf( "Gravis UltraSound Detected\n" );
/*
		if ( ( fh = open( "DMXGUS.INI", O_RDONLY | O_BINARY ) ) == -1 )
		{
			printf( "Can't open DMXMAP.INI" );
			return;
		}
		size = (int) lseek( fh, 0L, SEEK_END );
		if ( ( map = malloc( size ) ) == NULL )
		{
			close( fh );
			printf( "no memory :-(\n" );
			return;
		}
		lseek( fh, 0L, SEEK_SET );
		read( fh, map, size );
		close( fh );

		GF1_SetMap( map, size );
		memset( map, 0, size );	// test to make sure it isn't being used.
		free( map );
*/
	}

	TSM_Install( Tempo );
	atexit( TSM_Remove );
	a = b = c = 0;

//	TSM_NewService( proc_a, 5, 20, 0 );
//	TSM_NewService( proc_b, 35, 25,	0 );
//	TSM_NewService( proc_c, 35, 25, 0 );

	rc = DMX_Init( Tempo, 20, HwFlags, 0 );
//	rc = DMX_Init( Tempo, 20, 0, 0 );
	if ( rc == 0 )
	{
		printf( "No MIDI device installed.\n" );
		return;
	}
#if 0
Again:
	h = MUS_RegisterSong( GetFile( "E1M1" ) );
   	MUS_ChainSong( h, h );
	rc = MUS_PlaySong( h, 127 );
	printf( "Playing E1M1.MUS\n" );
	getch();
	MUS_StopSong( rc );
	MUS_UnregisterSong( h );
	h = MUS_RegisterSong( GetFile( "E1M6" ) );
   	MUS_ChainSong( h, h );
	rc = MUS_PlaySong( h, 127 );
	printf( "Playing E1M6.MUS\n" );
	if ( getch() == ' ' )
	{
		MUS_UnregisterSong( h );
		goto Again;
	}
	MUS_UnregisterSong( h );
#endif

	for ( j = 1, songs = 0; j < argc && songs < 9; j++ )
	{
		if ( argv[ j ][ 0 ] == '-' )
			continue;

		filedata = GetFile( argv[ j ] );
		if ( filedata == NULL )
			continue;

		if ( ( sh[ songs++ ] = MUS_RegisterSong( filedata ) ) < 0 )
		{
			printf( "\nError registering song.\n" );
		}
		if ( songs > 1 )
		{
			MUS_ChainSong( sh[ songs - 2 ], sh[ songs - 1 ] );
		}
	}
	if ( songs == 0 )
	{
		printf( "\nNo valid songs specified.\n" );
		return;
	}
   	MUS_ChainSong( sh[ songs - 1 ], sh[ 0 ] );
	h = MUS_PlaySong( sh[ ActiveSong ], 127 );
	printf( "\nPlaying song \n" );
	printf( "\n<ESC>     Quit" );
	printf( "\n<F1..F4>  Fade In/Out" );
	printf( "\n<P>       Pause Song" );
	printf( "\n<R>       Resume Song" );
	printf( "\n<S>       Stop Song" );
	printf( "\n<+,->     Adjust Master Volume" );
	if ( songs > 1 )
		printf( "\n<1..%d>   Select Song", songs );
	printf( "\n<Space> Play Again.\n" );
	for ( ;; )
	{
		Rate = 0;
//		while ( ! kbhit() )
//		{
//			printf( "A:%d  B:%d  C:%d\n", a, b, c );
//		}

		switch ( kbread() )
		{
			default:
				continue;

			case 27:
				break;

			case '+':
				MUS_SetMasterVolume( MUS_GetMasterVolume() + 1 );
				continue;

			case '-':
				MUS_SetMasterVolume( MUS_GetMasterVolume() - 1 );
				continue;

			case ' ':
				if ( ! MUS_QrySongPlaying( sh[ ActiveSong ] ) )
				{
					MUS_PlaySong( sh[ ActiveSong ], 127 );
					printf( "Playing song %d\n", ActiveSong + 1 );
				}
				continue;

			case 'P':
			case 'p':
				if ( MUS_QrySongPlaying( sh[ ActiveSong ] ) )
				{
					MUS_PauseSong( sh[ ActiveSong ] );
					printf( "Song %d Paused.\n", ActiveSong+1 );
				}
				continue;

			case 'R':
			case 'r':
				if ( MUS_QrySongPlaying( sh[ ActiveSong ] ) )
				{
					MUS_ResumeSong( sh[ ActiveSong ] );
					printf( "Song %d Resumed.\n", ActiveSong+1 );
				}
				continue;

			case 'S':
			case 's':
				MUS_StopSong( sh[ ActiveSong ] );
#if 0
				/* Unregister then Register New Song */
				MUS_Chain();
				MUS_Play( sh[ ActiveSong ] );
#endif
				printf( "Song %d Stoped.\n", ActiveSong+1);
				continue;

			case F1:	Rate++;
			case F2:	Rate++;
			case F3:	Rate++;
			case F4:	Rate++;
				if ( MUS_QrySongPlaying( sh[ ActiveSong ] ) )
				{
					if ( songs > 1 )
						MUS_FadeToNextSong( sh[ ActiveSong ], Rate * 1000 );
					else
						MUS_FadeOutSong( sh[ ActiveSong ], Rate * 1000 );
					printf( "Fading out song %d\n", ActiveSong + 1 );
				}
				else
				{
					MUS_FadeInSong( sh[ ActiveSong ], Rate * 1000 );
					printf( "Fading in song %d\n", ActiveSong + 1 );
				}
				continue;

			case '9':
				if ( songs >= 9 )
				{
					ActiveSong = 8;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
			case '8':
				if ( songs >= 8 )
				{
					ActiveSong = 7;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
			case '7':
				if ( songs >= 7 )
				{
					ActiveSong = 6;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
			case '6':
				if ( songs >= 6 )
				{
					ActiveSong = 5;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
			case '5':
				if ( songs >= 5 )
				{
					ActiveSong = 4;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
			case '4':
				if ( songs >= 4 )
				{
					ActiveSong = 3;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
			case '3':
				if ( songs >= 3 )
				{
					ActiveSong = 2;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
			case '2':
				if ( songs >= 2 )
				{
					ActiveSong = 1;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
			case '1':
				if ( songs >= 1 )
				{
					ActiveSong = 0;
					printf( "Song %d selected.\n", ActiveSong + 1 );
				}
				continue;
		}
		break;
	}
	DMX_DeInit();
	return;
}
