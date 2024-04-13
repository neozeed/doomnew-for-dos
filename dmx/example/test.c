/*===========================================================================
* TEST.C - Test DMX Sound System Lib.
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
#include <dos.h>
#include <ctype.h>

#include "dmx.h"
#include "joystick.h"

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

LOCAL BYTE		data[ 512*1024];

BYTE *
LoadFile(
	char		*filename,
	long		*length
	)
{
	int			fh;
	BYTE		*LoadedFile = NULL;
	long		len;

	if ( ( fh = open( filename, O_RDONLY | O_BINARY ) ) == -1 )
		return NULL;

	if ( ( len = filelength( fh ) ) > 0 )
	{
		if ( ( LoadedFile = malloc( ( unsigned )( len ) ) ) != NULL )
		{
			read( fh, ( char * ) LoadedFile, ( unsigned )( len ) );
			if ( length )
				*length = len;
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

				case 'W':
				case 'w':
					printf( "\nAWE_32 Mode\n" );
					HwMusFlags = AHW_AWE32;
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

INT
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
    printf( "Looking for %s\n", fname );
	if ( access( fname, 0 ) == -1 )
		return FALSE;
	if ( truename )
		strcpy( truename, fname );
	return TRUE;
}

int
Strobe(
	void
	)
{
	UINT		flags;
/*
* Check for Scroll-Lock KEY - Don't Active unless it is active
*/
	if ( ( *(BYTE *)( 0x0417L ) & 16 ) == 0 )
		return TS_OKAY;

	flags = PUSH_DISABLE();
	inp( 0x3da );
	outp( 0x3c0, 0x31 );
	outp( 0x3c0, 2 );   // Make border GREEN
/*
* Check for active Vertical Blanking 
*/
    if ( inp( 0x3da ) & 0x08 )
    {
    /*
    * Damn - missed it!
    */
	    inp( 0x3da );
	    outp( 0x3c0, 0x31 );
	    outp( 0x3c0, 4 );       // Make border RED
    }
    else
    {
    /*
    * Wait for Vertical Blanking
    */
        while ( ( inp( 0x3da ) & 0x08 ) == 0 )
            ;
    /*
    * Wait for Vertical Retrace to End
    */
        while ( inp( 0x3da ) & 0x08 )
            ;
	    inp( 0x3da );
	    outp( 0x3c0, 0x31 );
	    outp( 0x3c0, 0 );      // Make border BLACK
    }
	POP_DISABLE( flags );
	return TS_SYNC;
}

LOCAL BYTE * g_lowmem = 0L;
LOCAL BYTE * g_highmem = 0L;
LOCAL DWORD  lowmem     = 0;
LOCAL DWORD  memsize	= 0;

/***************************************************************************
GrabMem() - Allocates memory ALL memory < 640K
 ***************************************************************************/
VOID
GrabMem(
VOID
)
{
#define MEM_KEEP   ( 16 * 1024 )
#define MIN_MEMREQ ( 480 * 1024 )
#define MAX_HMEM   ( 1024 * 1024 )
#define REQ_HMEM   ( 128 * 1024 )
#define REQ_LMEM   ( 500 * 1024 )

   DWORD  segment;
	DWORD getmem     = 38400;

   // == GET highmem =============================
   memsize = MAX_HMEM;

   // == GET lowmem =============================
   while ( DosMemAlloc( getmem, &segment ) && getmem > 2 )
      getmem -= 2;

   lowmem = ( ( getmem - 1 ) * 16 );

   if ( lowmem > 4096 && segment != 0 )
   {
      g_lowmem = ( BYTE * )( segment << 4 );
	  printf( "\nLOWMEM: %05LX-%05LX\n", g_lowmem, g_lowmem + ( lowmem - 1 ) );
   }
   else
      printf ("Lowmem = NONE\n" );

   g_highmem = calloc ( memsize, 1 );

   if ( g_highmem == NUL && memsize > MEM_KEEP )
   {
      printf ("MemGet1 = %d\n", memsize );
      while ( g_highmem == NUL && memsize > 0 )
      {
         g_highmem = calloc ( memsize, 1 );
         memsize -= MEM_KEEP;
      }
      printf ("MemGet2 = %d\n", memsize );
   }

   if ( g_highmem )
   {
      printf ("Highmem = %d\n", memsize );
   }
   else
      printf ("Highmem = NUL\n" );

	if ( g_lowmem )
		memset( g_lowmem, 'L', lowmem );
	if ( g_highmem )
		memset( g_highmem, 'H', memsize );
}

void CheckRam( BYTE *data, char cset, DWORD size )
{
	DWORD		j;

	if ( data == NULL )
		return ;

	printf( "Checking %ld bytes for '%c'", size, cset );
	for ( j = 0; j < size; j++ )
	{
		if ( data[ j ] != cset )
		{
			DWORD		k;
			DWORD		len;

			printf( "\nMEMORY OVERWRITE OFFSET: %ld ", j );
			for  ( k = j; k < size && data[ k ] != cset; k++ )
				;
			printf( "FOR %ld BYTES\n", k - j );
			for ( len = 1; j < k; j++, len++ )
			{
				printf( "%02X%c", data[ j ], ( len % 32 ) ? ' ' : '\n' ); 
			}
			printf(" \n" );
			return ;
		}
	}
	printf( " - OK\n" );
}

VOID RdJoyStick( void )
{
 	ReadJoystick( 0x03 );
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
	BYTE		chk;
    int         master_clock;
    int         vrhz;
	int			rd_joy;
	int			exp_stereo = FALSE;

	memset( data, 'D', sizeof( data ) );
	if ( argc < 2 )
	{
		printf( "Usage: PLAYMUS [options] <MUSfile>\n" );
		printf( "Options:\n" );
		printf( "    -a     AdLib Mode\n" );
		printf( "    -w     AWE 32 Mode\n" );
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

    vrhz = TSM_TimeVR( 0x3da );
    printf( "Video Retrace is %dhz\n", vrhz );

    Tempo = vrhz * 2;
	master_clock = vrhz * 4;
    master_clock += 25; /* Like the timing on a car - just a bit advanced */

	TSM_Install( master_clock ); 
	atexit( TSM_Remove );

	if ( TSM_NewService(
			Strobe,		/* Function to call 	*/
			vrhz,	    /* @ what rate in hz	*/
			32768,		/* High Priority		*/
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
	dspExpandedStereo( exp_stereo );

	rd_joy = ! DMX_SetIdleCallback( RdJoyStick );
	WAV_PlayMode( 8, wSampleRate );
	GrabMem();
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
* Loading WAV Files
*/
	printf( "Loading WAV files...\n" );
	for ( j = 1, waves = 0; j < argc && waves < 10; j++ )
	{
		if ( argv[ j ][ 0 ] == '-' )
			continue;

		if ( FileExists( argv[ j ], ".WAV", fname ) )
		{
			wav[ waves ] = WAV_LoadWavFile( fname );
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
	songs = 0;
	for ( j = 1; j < argc && songs < 10; j++ )
	{
		if ( argv[ j ][ 0 ] == '-' )
			continue;

		if ( FileExists( argv[ j ], ".MUS", fname ) )
		{
			filedata = LoadFile( fname, NULL );

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
				printf( "Loaded %s\n", 	fname );
			}
		}
	}
/*
* Loading MIDI Files
*/
	printf( "Loading MIDI files...\n" );
	for ( j = 1; j < argc && songs < 10; j++ )
	{
		if ( argv[ j ][ 0 ] == '-' )
			continue;

		if ( FileExists( argv[ j ], ".MID", fname ) )
		{
			filedata = LoadFile( fname, NULL );

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
				printf( "Loaded %s\n", 	fname );
			}
		}
	}
	if ( songs )
		MUS_ChainSong( sh[ songs - 1 ], sh[ 0 ] );

	printf( "Keys:\n'q' < VOLUME > 'e'\n'a' < PANPOT > 'd'\n'z' <  PITCH > 'c'\n" );
	printf( "ESC to quit.\n" );
	panpot = 128;
	volume = 127;
	pitch = 128;
	MUS_PlaySong( sh[ ActiveSong ], 127 );
	chk = 'L';

	for ( ;; )
	{
		while ( ! kbhit() )
		{
			if ( rd_joy )
	 			ReadJoystick( 0x03 );

//			printf( "X1:%4d Y1%4d  X2:%4d Y2:%4d B:%d\n",
//				joyX1, joyY1, joyX2, joyY2, joyButtons );		

			if ( MUS_QrySongPlaying( sh[ ActiveSong ] ) )
				printf( "                                        MUS_ACTIVE  \r" );
			else
				printf( "                                        MUS_INACTIVE\r" );
	        fflush( stdout );
		}
		if ( g_lowmem && g_lowmem[ 464 ] != chk )
		{
			UINT	flags;

//			*(BYTE *)( 0x0417L ) &= ~16;

			flags = PUSH_DISABLE();
			inp( 0x3da );
			outp( 0x3c0, 0x31 );
			outp( 0x3c0, 0x04 );
			POP_DISABLE( flags );

			chk = g_lowmem[ 464 ];
			CheckRam( g_lowmem, 'L', lowmem );
		}
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
					h = SFX_PlayPatch( wav[ key ], pitch, panpot, volume, 0, 1 );
/*
					h = WAV_AdvPlayPatch( wav[ key ], pitch,
									255 - panpot, panpot,
									16 - ( ( panpot + 1 ) >> 3 ),
									0, 1 );
*/
				}
				continue;

			case 27:
				printf( "Quiting...\n" );
				DMX_DeInit();
				TSM_Remove();
				CheckRam( data, 'D', sizeof( data ) );
				CheckRam( g_lowmem, 'L', lowmem );
				CheckRam( g_highmem, 'H', memsize );
				return;

			case '+':
				MUS_SetMasterVolume( MUS_GetMasterVolume() + 1 );
				continue;

			case '-':
				MUS_SetMasterVolume( MUS_GetMasterVolume() - 1 );
				continue;

			case '`':
				exp_stereo ^= 1;
				dspExpandedStereo( exp_stereo );
				printf( "\nEXPANDED %s\n", ( exp_stereo ) ? "ON" : "OFF" );
				continue;

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
		SFX_SetOrigin( h, pitch, panpot, volume );
/*
		WAV_AdvSetOrigin( h, pitch, 
						255 - panpot, panpot,
						16 - ( ( panpot + 1 ) >> 3 ) );
*/
        printf( "Pitch:%3d  Panpot:%3d  Volume:%3d\r", pitch, panpot, volume );
        fflush( stdout );
	}
}						  
