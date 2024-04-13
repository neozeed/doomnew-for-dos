#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#include "serial.h"

void 
main( void )
{
	long	rc;
	int		key;
  
  rc = initSerial( 1, USE_16550, 1024, 2048 );
  if ( rc != SER_OK )
  {
  	printf( "Error %d on initSerial\n", rc );
    return;
  }
	do
	{
		while ( ! kbhit() )
		{
			while ( ! rxBuffEmpty() )
			{
				int	chr;

				chr = readSer();
				if ( chr < SER_OK )
					printf( "Error %d, on readSer\n", chr );
				else
					printf( "%c", chr );
			}
		  fflush( stdout );
		}
		key = getch();
		if ( ( key == 0 || key == 0xe0 ) && kbhit() )
			key = getch() | 0x100;
		if ( key < 0x100 )
		{
    	writeSer( key );
		}
	}
	while( key != 301 );	/* ALT_X */
}
