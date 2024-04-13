#include <stdio.h>
#include "types.h"
#include "sercom.h"

main()
{
	int 	rc;
	int		key;
	int		port = COM1;

	rc = serOpenPort( port, ASINOUT|USE_16550, 2048, 1024, 
				 38400L, 8, 1, P_NONE, 3, TRIGGER_04 );

	if ( rc != SER_OKAY )
	{
		printf( "Error %d, opening communications port.\n" );
		return;
	}
	do
	{
		while ( ! kbhit() )
		{
			while ( ! isrxempty( port ) )
			{
				int	chr;

				chr = serGetChr( port );
				if ( chr < SER_OKAY )
					printf( "Error %d, on serGetChr\n" );
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
			serPutChr( port, key );
		}
	}
	while( key != 301 );	/* ALT_X */
	serClosePort( port );
}
