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
#include <ctype.h>
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

BYTE	capture[ 64000 ];

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
main(
    void
	)
{
	int			irq,dma,addr;
	WORD		ver;
	int			rc;
	void		*drum;
	void		*racecar;
	void		*testing;
	int			h;		   
	int			pitch;
	int			panpot;
	int			volume;

	TSM_Install( 140 );
	atexit( TSM_Remove );

    printf( "Auto-detect (y/n) ?\n" );
    if ( getch() == 'y' )
    {
		if ( MV_Detect() == 0 )
			printf( "\nFound MEDIA VISION card.\n" );
#if 1
		else
		if ( GF1_Detect() == 0 )
			printf( "\nfound GRAVIS Ultrasound card.\n" );
#endif
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

	rc = DMX_Init( 140, 20, 0, AHW_ANY );
	if ( rc == 0 )
	{
		printf( "No DSP device installed.\n" );
		return;
	}
	WAV_PlayMode( 8, 22050 );
	drum = WAV_LoadPatch( "drum.dsp" );
	racecar = WAV_LoadPatch( "racecar.dsp" );
	testing = WAV_LoadPatch( "medium.dsp" );
	printf( "Keys:\n'w' < VOLUME > 'r'\n's' < PANPOT > 'f'\n'x' <  PITCH > 'v'\n" );
	printf( "1=Drum 2=Racecar 3=Testing123.\n" );
	printf( "ESC to quit.\n" );
	panpot = 128;
	volume = 127;
	pitch = 128;
	h = -1;

	for ( ;; )
	{
		switch ( tolower( kbread() ) )
		{
			default:
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

			case '1':
				h = SFX_PlayPatch( drum, pitch, panpot, volume, 0, 1 );
				continue;

			case '2':
				h = SFX_PlayPatch( racecar, pitch, panpot, volume, 0, 1 );
				continue;

			case '3':
				h = SFX_PlayPatch( testing, pitch, panpot, volume, 0, 1 );
				continue;
		}
		SFX_SetOrigin( h, pitch, panpot, volume );
        printf( "Pitch:%3d  Panpot:%3d  Volume:%3d\r", pitch, panpot, volume );
        fflush( stdout );
	}
}						  
