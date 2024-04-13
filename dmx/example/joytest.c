#include "joystick.h"


main()
{
	while ( ! kbhit() )
	{
 		ReadJoystick( 0x03 );
		printf( "X1:%4d Y1%4d  X2:%4d Y2:%4d B:%d\n",
			joyX1, joyY1, joyX2, joyY2, joyButtons );		
	}
}
