#include <stdio.h>
#include <string.h>

#include "dmx.h"
#include "mixers.h"
#include "pztimer.h"

BYTE		target[ 1024 ];
WORD		mixbuf[ 4096 ];
BYTE		volume[ 1024 ];
BYTE		wavform[ 1024 ];

char *
bytetohex(
	unsigned char	byte,
	char			*str
	)
{
	char			*p = str;

	*p = ( byte >> 4 ) + '0';
	if ( *p > '9' )
		*p += ( 'A' - ':' );
    ++p;
	*p = ( byte & 0x0F ) + '0';
	if ( *p > '9' )
		*p += ( 'A' - ':' );
    ++p;
    *p = '\0';
	return str;
}

void dump( char *title, BYTE *ptr, int size )
{
	int		j;
	int		k;

	printf( "\n\n[%s]\n", title );
	for ( j = 0; j < size; )
	{
		printf( "%04X %05d : ", j, j );
		for ( k = 0; k < 16 && ( j + k < size ); k++ )
		{
			printf( "%02X ", ptr[ k ] );
			if ( k == 7 )
				printf( "ú " );
		}
		j += 16;
		printf( "\n" );
		ptr += k;
	}
}

main( void )
{
	VCHAN	channel[ 4 ];
	int		j, k;
	DWORD	total;

	memset( &channel, 0, sizeof( channel ) );
	for ( j = 0; j < 4; j++ )
	{
		channel[j].dMogQuotient = 1;
    	channel[j].bVolPtr = (BYTE*)(( DWORD )( volume + 255 ) & ~255 );
	}

	memset( target, 0xff, sizeof( target ) );
	memset( mixbuf, 0xff, sizeof( mixbuf ) );
	memset( volume, 0x11, sizeof( volume ) );
	memset( wavform, 0, sizeof( wavform ) );
	total = 0;
	for ( j = 0; j < 32; j++ )
	{
		for ( k = 0; k < 4; k++ )
		{
			channel[ k ].bWavPtr = wavform;
			channel[ k ].bWavStartPtr = wavform;
			channel[ k ].dSamples = sizeof( wavform );
		}
		ZTimerOn();
		dmxMix8bitStereo( channel, 4, mixbuf, 256 );
		dmxClipInterleave( target, mixbuf, 512 );
		ZTimerOff();
		total += ZGetTime();
	}
	printf( "Mix required %u æs\n", total / 32 );
//	dump( "TARGET", target, sizeof( target ) );
//	dump( "MIX BUFFER", mixbuf, sizeof( mixbuf ) );
//	dump( "VOLUME", volume, sizeof( volume ) );
}
