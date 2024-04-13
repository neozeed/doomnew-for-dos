/*===========================================================================
* DMXAPI.H  - Digital/Music/Effects API General Maitenance Prototypes
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
* $Log:   F:/DMX/INC/VCS/DMXAPI.H_V  $
*  
*     Rev 1.1   06 Oct 1993 22:14:42   pjr
*  Removed AHW_CANVAS define
*  
*  
*     Rev 1.0   02 Oct 1993 15:20:46   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#ifndef _DMXAPI_H
#define _DMXAPI_H

EXTERN DWORD DmxOption;
EXTERN DWORD DmxDebug;	// Changes color of border upon execution
EXTERN BYTE	 gPrevColor;
#define DMXBUG_NONE			0				// NO FLASHES
#define DMXBUG_TSMAPI		0x00000001		// BLUE BORDER
#define DMXBUG_PCDMA		0x00000002		// GREEN
#define DMXBUG_PCINT		0x00000004		// CYAN
#define DMXBUG_DSPAPI		0x00000008		// RED
#define DMXBUG_MIXERS		0x00000010		// MAGENTA
#define DMXBUG_PCSAPI		0x00000020		// BROWN
#define DMXBUG_GSSAPI		0x00000040		// GREY
#define DMXBUG_MUSAPI		0x00000080		// YELLOW
#define DMXBUG_CARD			0x00000100		// WHITE
#define DMXBUG_MESSAGES		0x00001000		// Show Messages
#define DMXBUG_TRACE		0x00002000		// Show Trace
#define DMXBUG_TOFILE		0x00004000		// Trace to File
/*
* OPTIONS
*/
#define USE_PHASE			0x00000001L
#define USE_OPL3			0x00000002L

/*
* HARDWARE DEFINES
*/
#define AHW_ANY				0xFFFFFFFFL
#define AHW_PC_SPEAKER		0x00000001L
#define AHW_ADLIB			0x00000002L
#define AHW_AWE32			0x00000004L
#define AHW_SOUND_BLASTER	0x00000008L
#define AHW_MPU_401			0x00000010L
#define AHW_ULTRA_SOUND		0x00000020L
#define AHW_MEDIA_VISION	0x00000040L
#define AHW_FM_OPL3			0x00000080L
#define AHW_ENSONIQ 		0x00000100L
#define AHW_CODEC           0x00000200L

#ifndef PROD
PUBLIC void DmxTrace( char * );
PUBLIC void pDmxTrace( char	*, int, int, int, int, int, int, int );
#else
#define DmxTrace(__ignore) ((void)0)
#endif

/****************************************************************************
* DMX_Init - Initialize DMX Sound System
*****************************************************************************/
PUBLIC INT		// Returns: Hardware Flags
DMX_Init(
	int			tickrate,	// INPUT : Music Quantinization Rate
	int			max_songs,	// INPUT : Max # Registered Songs
	DWORD		mus_device,	// INPUT : Valid Device ID for Music Playback
	DWORD		sfx_device	// INPUT : Valid Device ID for Sound Effects
	);

/****************************************************************************
* DMX_DeInit - DeInitialize Sound System
*****************************************************************************/
PUBLIC VOID
DMX_DeInit(
	VOID
	);

/****************************************************************************
* DMX_SetIdleCallback - Sets callback hook when timing can be acurate.
****************************************************************************/
PUBLIC int		// returns: 0: Not Needed/Installed, 1=Callback Installed
DMX_SetIdleCallback(
	int		(*callback)( void )
	);

/****************************************************************************
* DmxOpenFile - Open DOS file for buffered reads
****************************************************************************/
PUBLIC VOID *       // Returns: File handle
DmxOpenFile(
    char            *filename,      // INPUT: Filename to open
    BYTE            *buffer,        // INPUT: Cache buffer
    UINT            size            // INPUT: Cache size    
    );

/****************************************************************************
* DmxReadFile - Read from file.
****************************************************************************/
PUBLIC UINT			// Returns: number of bytes read
DmxReadFile(
    void            *fh,        // INPUT: File Handle from DmxOpenFile
    VOID            *data,      // INPUT: Buffer to read into    
    UINT	        amount      // INPUT: Amount of data to read
    );

/****************************************************************************
* DmxCloseFile - ReadFile - Read from file.
****************************************************************************/
VOID
DmxCloseFile(
    void            *fh
    );

#endif

