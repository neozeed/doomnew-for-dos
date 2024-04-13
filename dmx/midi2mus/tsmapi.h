/*===========================================================================
* TSMAPI.H - Provide Timer Driven Execution Services
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
* $Log:   F:/DMX/INC/VCS/TSMAPI.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:19:56   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#ifndef _TSMAPI_H
#define _TSMAPI_H

#define TS_OKAY		0
#define TS_BUSY		1
#define TS_SYNC     2

/****************************************************************************
* TSM_Active - Callback that bumps the master tick count, when TSM_Service
*			   is busy.
*****************************************************************************/
PUBLIC VOID
TSM_Active(
	VOID
	);

/****************************************************************************
* TSM_ResumeService - Resume execution of service procedure.
*****************************************************************************/
void
TSM_ResumeService(
	INT			ServiceId			// INPUT: Service ID to Resume
	);

/****************************************************************************
* TSM_PauseService - Pause execution of service procedure.
*****************************************************************************/
void
TSM_PauseService(
	INT			ServiceId			// INPUT: Service ID to Pause
	);

/****************************************************************************
* TSM_SyncService - Synchronize Service (call on next timer tick) with
*                   current event.
*****************************************************************************/
void
TSM_SyncService(
	INT			ServiceId			// INPUT: Service ID to Reset Counters
	);

/****************************************************************************
* TSM_NewService - Assign procedure to be service at given frequency.
*****************************************************************************/
INT		// Returns: Service ID >= 0, -1 = Failed    
TSM_NewService(
	int			( *SProc )( void ), 	// INPUT: Ptr to Service Procedure
	INT			Frequency,          	// INPUT: Frequency in Hz
	INT			Priority,           	// INPUT: Scheduler Pri. HIGH=255..0=LOW
	INT			Pause               	// INPUT: Wait for Resume to Start Service
	);

/****************************************************************************
* TSM_Delay - Wait until the specified amount of time elapses.
*****************************************************************************/
void
TSM_Delay(
	DWORD			ms		// INPUT : Amount of time in milliseconds
	);

/****************************************************************************
* TSM_Clock - Returns the amount of time elapsed in milliseconds since
*             TSM_Install was called.
*****************************************************************************/
DWORD
TSM_Clock(
	VOID
	);

/****************************************************************************
* TSM_DelService - Delete the service procedure from the timer manager.
*****************************************************************************/
void
TSM_DelService(
	INT			ServiceId			// INPUT: Service ID to Delete
	);

/****************************************************************************
* TSM_ChangeFreq - Change Frequecny of Exection of given Service.
*****************************************************************************/
PUBLIC void DYNAMIC
TSM_ChangeFreq(
	INT			ServiceId,			// INPUT: Service ID to Change
	INT			Frequency          	// INPUT: Frequency in Hz
	);

/****************************************************************************
* TSM_ResetCount - Reset service invocation counter.
*****************************************************************************/
void
TSM_ResetCount(
	INT			ServiceId			// INPUT: Service ID to Reset Counters
	);


/****************************************************************************
* TSM_Install - Install Timer Service Manager.
*****************************************************************************/
BOOL			// Returns : True - Installed OK, FALSE - Failed to Install
TSM_Install(
	UINT			MasterFrequency		// INPUT: Frequency in Hz
	);


/****************************************************************************
* TSM_Remove - Remove Timer Service Manager.
*****************************************************************************/
void
TSM_Remove(
	void
	);

/****************************************************************************
* TSM_TimeVR - Time Vertical Retrace
*****************************************************************************/
int     // returns frequency in hz
TSM_TimeVR(
    int     port        // INPUT : Video Port Address
    );

#endif
