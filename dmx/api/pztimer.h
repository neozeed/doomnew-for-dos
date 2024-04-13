/*===========================================================================
* PZTIMER.H - Perfromance Timer
*----------------------------------------------------------------------------
*                CONFIDENTIAL & PROPRIETARY INFORMATION 			     
*----------------------------------------------------------------------------
*             Copyright (C) 1994 Digital Expressions, Inc. 		     
*                        All Rights Reserved.		   		     
*----------------------------------------------------------------------------
* "This software is furnished under a license and may be used only 	     
* in accordance with the terms of such license and with the inclusion 	     
* of the above copyright notice. This software or any other copies 	     
* thereof may not be provided or otherwise made available to any other 	     
* person. No title to and ownership of the software is hereby transfered."   
*----------------------------------------------------------------------------
*                     Written by Paul J. Radek
*---------------------------------------------------------------------------*/

void ZTimerOn( void );
void ZTimerOff( void );
unsigned long ZGetTime( void );

#ifdef __386__
#pragma aux ZTimerOn "_*"
#pragma aux ZTimerOff "_*"
#pragma aux ZGetTime "_*" modify [EAX];
#endif

