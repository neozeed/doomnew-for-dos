/*===========================================================================
* PLAYWAV.C - Play Waveform(s)
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
* $Log:   F:/DMX/API/VCS/PLAYWAV.C_V  $
*  
*     Rev 1.1   11 Oct 1993 05:54:32   pjr
*  Added option to auto-detect card.
*  
*  
*     Rev 1.0   02 Oct 1993 14:49:00   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>

#include "dmx.h"

#include "video.h"

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

LOCAL WORD		HwMusFlags = AHW_ANY;
LOCAL WORD		HwWavFlags = AHW_ANY;
LOCAL WORD		Tempo = 140;
LOCAL WORD		wSampleRate = 22050;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
* Video support variables
** - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int         video_mode;
int         va_background = VA_WHITE | VA_ON_BLUE;
int         va_inverse    = VA_BLACK | VA_ON_GRAY;
int         va_alert      = VA_WHITE | VA_ON_RED;
int         va_wait       = VA_BLUE  | VA_ON_CYAN;


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

char *erm[] =
{
	"",
	"could not find card address (port)\n",
	"could not allocate DMA buffer\n",
	"could not detect interrupt\n",
	"could not identify DMA channel\n"
};

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
					HwMusFlags = AHW_ADLIB;
					printf( "\nAdLib Mode\n" );
					break;

				case 'U':
				case 'u':
					printf( "\nGravis Ultra Sound Mode\n" );
					HwWavFlags = HwMusFlags = AHW_ULTRA_SOUND;
					break;
					
				case 'S':
				case 's':
					printf( "\nSoundBlaster Mode\n" );
					HwMusFlags = AHW_ADLIB;
					HwWavFlags = AHW_SOUND_BLASTER;
					break;

				case 'P':
				case 'p':
					printf( "\nPro Audio Spectrum Mode\n" );
					HwMusFlags = AHW_ADLIB;
					HwWavFlags = AHW_MEDIA_VISION;
					break;
				
				case 'M':
				case 'm':
					printf( "\nMPU-401 Mode\n" );
					HwMusFlags = AHW_MPU_401;
					break;

				case 'T':
				case 't':
					Tempo = (WORD) atoi( &argv[ j ][2] );
					printf( "\nNew Tempo: %d\n", Tempo );
					break;

				case 'R':
				case 'r':
					wSampleRate = (WORD) atoi( &argv[ j ][2] );
					printf( "\nNew Sample Rate: %d\n", wSampleRate );
					break;
			}
		}
	}
}

BOOL
FileExists(
	char		*filename,
	char		*extension,
	char		*truename
	)
{
	char		fname[ 80 ];
	char		*ptr;

	strcpy( fname, filename );
	if ( ( ptr = strrchr( fname, '.' ) ) != NULL )
		*ptr = '\0';
	strcat( fname, extension );
	if ( access( fname, 0 ) == -1 )
		return FALSE;
	if ( truename )
		strcpy( truename, fname );
	return TRUE;
}

BYTE *
GetFile(
	char		*filename
	)
{
	char		fname[ 80 ];
	char		*ptr;
	BYTE		*filedata;
	long		flen;

	strcpy( fname, filename );
	if ( ( ptr = strrchr( fname, '.' ) ) != NULL )
		*ptr = '\0';
	strcat( fname, ".MUS" );
	printf( "\nLoading %s ", fname );
	if ( ( filedata = LoadFile( fname, &flen ) ) == NULL ||
		  	flen <= sizeof( DMX_HEADER ) ||
		  	memcmp( filedata, "MUS", 3 ) != 0 
			)
	{
		printf( "\nError loading file: %s\n", fname );
		return NULL;
	}
	return filedata;
}

int
Strobe(
	void
	)
{
	UINT		flags;
	BYTE		bPrevColor;

/*
* Check for Scroll-Lock KEY
*/
	if ( ( *(BYTE *)( 0x0417L ) & 16 ) == 0 )
		return TS_OKAY;

	flags = PUSH_DISABLE();
	inp( 0x3da );
	outp( 0x3c0, 0x31 );
	bPrevColor = inp( 0x3c1 ) + 1;
	outp( 0x3c0, bPrevColor & 0x1F );
	POP_DISABLE( flags );

	return TS_OKAY;
}


main(
	int		argc,
	char	*argv[]
	)
{
	int			irq,dma,addr;
	WORD		ver;
	int			rc;
	int			port;
	char		fname[ 80 ];
	void		*wav[ 10 ];
	int			waves;
	int			h;		   
	int			pitch;
	int			panpot;
	int			volume;
	int			key;
	INT			sh[ 9 ];
	INT			songs;
	INT	 		ActiveSong = 0;
	INT			j;
	BYTE		*filedata;
/*
* Set video mode and draw screen
*/
    video_mode = vioGetMode();
    if ( video_mode < 2 || video_mode > 7 ||
       ( video_mode > 3 && video_mode < 7 ) )
    {
        video_mode = 3;
        vioSetMode( video_mode );
    }
    else if ( video_mode == 7 || video_mode == 2 )
    {
        va_background = VA_GRAY | VA_BLACK;
        va_alert = VA_BLACK | VA_ON_GRAY | VA_BLINK;
        va_wait  = VA_BLACK | VA_ON_GRAY;
    }
    vioClearWindow( 0, 0, vidRows - 1, vidCols - 1, va_background );
    vioClearWindow( 0, 0, 0, vidCols - 1, va_inverse );
    vioDrawStrAt( 0, 1, "DMX Example - Copyright (C) 1994 Digital Expressions, Inc."  );
	vioDrawStrAt( 2, 1, " MIDI DEVICE" );
	vioDrawStrAt( 3, 1, "컴컴컴컴컴컴�" );
	vioDrawStrAt( 4, 1, "[ ] Ad Lib" );
	vioDrawStrAt( 5, 1, "[ ] AWE 32" );
	vioDrawStrAt( 6, 1, "[ ] MPU-401" );
	vioDrawStrAt( 7, 1, "[ ] Ultrasound" );
	vioDrawStrAt( 8, 1, "[ ] NONE" );

    getch();

	if ( argc < 2 )
	{
		printf( "Usage: PLAYMUS [options] <MUSfile>\n" );
		printf( "Options:\n" );
		printf( "    -a     AdLib Mode\n" );
		printf( "    -m     MPU-401 Mode\n" );
		printf( "    -p     Pro Audio Spectrum Mode\n" );
		printf( "    -s     SoundBlaster Mode\n" );
		printf( "    -u     Gravis Ultrasound Mode\n" );
		printf( "    -tnnn  Set Tempo where 'nnn' is new tempo (e.g. -t280)\n" );
		printf( "    -r###  Sample Rate where '###' is the new sample rate\n" );
		printf( "\n" );
		return;
	}
	printf( "Initializing System...\n" );
	TSM_Install( Tempo );
	atexit( TSM_Remove );

	if ( TSM_NewService(
			Strobe,		/* Function to call 	*/
			5,			/* @ what rate in hz	*/
			0,			/* 0=High Priority		*/
			0			/* Don't pause			*/
			) == -1 )
		printf( "Error installing Strobe service.\n" );
	else
		printf( "PRESS SCROLL-LOCK to ACTIVATE STROBE!\n" );

	CheckArgs( argc, argv );
 	if ( HwMusFlags == AHW_ANY )
	{
    	printf( "Auto-detect (y/n) ?\n" );
    	if ( getch() == 'y' )
    	{
			port = 0;
			if ( MPU_Detect( &port, NULL ) == 0 )
				printf( "MPU-401 Detected @ %03Xh\n", port );

			if ( AL_Detect( NULL, &j ) == 0 )
				printf( "Yamaha OPL%d Detected\n", j );

			if ( GF1_Detect() == 0 )
				printf( "\nfound GRAVIS Ultrasound card.\n" );
		}
	}
	if ( HwWavFlags == AHW_ANY )
	{
		if ( MV_Detect() == 0 )
			printf( "\nFound MEDIA VISION card.\n" );
		else if ( GF1_Detect() == 0 )
			printf( "\nfound GRAVIS Ultrasound card.\n" );
		else
		{
			addr = dma = -1;
			irq = 0;
	    	if ( ( rc = SB_Detect( &addr, &irq, &dma, &ver ) ) == 0 )
		    	printf( "\nCard is default is Port:%xh Irq:%d  DMA:%d\n", addr, irq, dma );

	    	printf( "Looking for a SOUND BLASTER card...\n" );
	    	addr = irq = dma = -1;
	    	if ( ( rc = SB_Detect( &addr, &irq, &dma, &ver ) ) == 0 )
	    	{
				printf( "\nFound SoundBlaster card\n" );
		    	printf( "\nADDR:%03xh IRQ:%d DMA:%d  Ver:%2x.%02x\n",
			    	addr, irq, dma, HIBYTE(ver),LOBYTE(ver) );

		    	SB_SetCard( addr, irq, dma );
	    	}
	    	else
	    	{
		    	printf( "Sound Blaster not found - %s", erm[ rc ] );
		    	return;
	    	}
		}
	}
	printf( "\nInitializing System...\n" );

	rc = DMX_Init( Tempo, 20, HwMusFlags, HwWavFlags );
	if ( rc == 0 )
	{
		printf( "No playback device found.\n" );
		return;
	}
	WAV_PlayMode( 8, wSampleRate );
/*
* Loading DSP Files
*/
	printf( "Loading DSP files...\n" );
	for ( j = 1, waves = 0; j < argc && waves < 10; j++ )
	{
		if ( argv[ j ][ 0 ] == '-' )
			continue;

		if ( FileExists( argv[ j ], ".DSP", fname ) )
		{
			wav[ waves ] = WAV_LoadPatch( fname );
			if ( wav[ waves ] != NULL )
			{
				++waves;
				printf( "KEY %d = %s\n", waves, fname );
			}
		}
	}
/*
* Loading MUS Files
*/
	printf( "Loading MUS files...\n" );
	for ( j = 1, songs = 0; j < argc && songs < 10; j++ )
	{
		if ( argv[ j ][ 0 ] == '-' )
			continue;

		if ( FileExists( argv[ j ], ".MUS", fname ) )
		{
			filedata = GetFile( fname );

			if ( filedata != NULL )
			{
				sh[ songs ] = MUS_RegisterSong( filedata );
				if ( sh[ songs ] == -1 )
				{
					free( filedata );
					continue;
				}
				++songs;
				if ( songs > 1 )
					MUS_ChainSong( sh[ songs - 2 ], sh[ songs - 1 ] );
				else
				   	MUS_ChainSong( sh[ songs - 1 ], sh[ 0 ] );
				printf( "Loaded %s\n", 	fname );
			}
		}
	}
	printf( "Keys:\n'q' < VOLUME > 'e'\n'a' < PANPOT > 'd'\n'z' <  PITCH > 'c'\n" );
	printf( "ESC to quit.\n" );
	panpot = 128;
	volume = 127;
	pitch = 128;
	h = MUS_PlaySong( sh[ ActiveSong ], 127 );

	for ( ;; )
	{
		switch ( ( key = tolower( kbread() ) ) )
		{
			default:
				if ( key >= '0' && key <= '9' )
				{
					key -= '1';
					if ( key < 0 )
						key = 9;
					if ( key + 1 > waves )
						continue;
//					h = SFX_PlayPatch( wav[ key ], pitch, panpot, volume, 0, 1 );
					h = WAV_AdvPlayPatch( wav[ key ], pitch,
									255 - panpot, panpot,
									16 - ( ( panpot + 1 ) >> 3 ),
									0, 1 );
				}
				continue;

			case 27:
				printf( "Quiting...\n" );
				DMX_DeInit();
				TSM_Remove();
				return;

			case 'q':	// Full Off
				if ( volume > 0 )
                {
			   	    volume = 0;
                    break;
                }
                continue;

			case 'w':
				if ( volume > 0 )
				{
					--volume;
                    break;
				}
				continue;

			case 'e':
                if ( volume != 64 )
                {
				    volume = 64;
                    break;
                }
				continue;

			case 'r':
				if ( volume < 127 )
				{
					++volume;
                    break;
				}
				continue;

			case 't':
				if ( volume < 127 )
                {
				    volume = 127;
                    break;
                }
				continue;


			case 'a':
                if ( panpot > 0 )
                {
                    panpot = 0;
                    break;
                }
                continue;

			case 's':
				if ( panpot > 0 )
				{
					--panpot;
                    break;
				}
				continue;

			case 'd':
                if ( panpot != 128 )
                {
                    panpot = 128;
                    break;
                }
				continue;

			case 'f':
				if ( panpot < 255 )
				{
					++panpot;
                    break;
				}
				continue;

			case 'g':
                if ( panpot < 255 )
                {
                    panpot = 255;
                    break;
                }
                continue;

			case 'z':
                if ( pitch > 0 )
                {
                    pitch = 0;
                    break;
                }
                continue;

			case 'x':
				if ( pitch > 0 )
				{
					--pitch;
                    break;
				}
				continue;

			case 'c':
                if ( pitch != 128 )
                {
                    pitch = 128;
                    break;
                }
                continue;

			case 'v':
				if ( pitch < 255 )
				{
					++pitch;
                    break;
				}
				continue;

			case 'b':
                if ( pitch != 255 )
                {
                    pitch = 255;
                    break;
                }
                continue;
		}
//		SFX_SetOrigin( h, pitch, panpot, volume );

		WAV_AdvSetOrigin( h, pitch, 
						255 - panpot, panpot,
						16 - ( ( panpot + 1 ) >> 3 ) );

        printf( "Pitch:%3d  Panpot:%3d  Volume:%3d\r", pitch, panpot, volume );
        fflush( stdout );
	}
}						  
