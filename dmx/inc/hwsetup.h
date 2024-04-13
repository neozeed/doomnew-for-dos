/*===========================================================================
* HWSETUP.H
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
* $Log:   F:/DMX/INC/VCS/HWSETUP.H_V  $
*  
*     Rev 1.1   17 Oct 1993 00:23:04   pjr
*  Modified calling convention for AL_Detect
*  
*  
*     Rev 1.0   02 Oct 1993 15:19:14   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

/****************************************************************************
* SB_SetCard - Set card to specific configuration.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*****************************************************************************/
PUBLIC VOID
SB_SetCard(
	int		iBaseAddr,		// INPUT : Base Address of Board
	int		iIrq,			// INPUT : Irq of Board
	int		iDma			// INPUT : DMA Channel
	);

/****************************************************************************
* SB_Detect - Detects SB Series Card and returns best guess of how it is
*             configured.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*	2 = Could not allocate DMA buffer
*	3 = Could not detect interrupt
*	4 = Could not identify DMA channel
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
SB_Detect(
	int		*iBaseAddr,		// OUTPUT: Base Address of Board
	int		*iIrq,			// OUTPUT: Irq of Board
	int		*iDma,			// OUTPUT: DMA Channel
	WORD	*wVersion		// OUTPUT: Card Version
	);

/****************************************************************************
* AL_SetCard - Changes card properties.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*****************************************************************************/
PUBLIC VOID
AL_SetCard(
    int         iWaitStates,    // INPUT : # of I/O waitstates
	void		*BankData		// INPUT : (opt) Ptr to Bank Data
    );

/****************************************************************************
* AL_Detect - Detects Ad Lib Card and returns current waitstates.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
AL_Detect(
    int         *iWaitStates,   // OUTPUT: # of waitstates
	int			*iType			// OUTPUT: Board Type
    );

/****************************************************************************
* MPU_SetCard - Changes card properties.
*
* NOTE:
* 	Call this BEFORE DMX_Init
*****************************************************************************/
PUBLIC VOID
MPU_SetCard(
    int         iPort           // INPUT : Port Address of Card
    );

/****************************************************************************
* MPU_Detect - Detects MPU-401 Card and returns type of MIDI
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = No MPU-401 Port Found.
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
MPU_Detect(
    int         *iPort,    	// IN/OUT: Port Address of Card 0=AutoDetect
	int			*cType		// OUTPUT: Card Type: 0=GENMIDI 1=SCC-1
    );


/****************************************************************************
* GF1_Detect - Detects Gravis Ultrasound
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
GF1_Detect(
	void
	);

/****************************************************************************
* GF1_SetMap - Sets MAP for gf1 to read from.
*****************************************************************************/
PUBLIC void
GF1_SetMap(
	char		*gf1ini_map,
	int			map_size
	);

/****************************************************************************
* MV_Detect - Detects Media Vision Chip based Cards
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
MV_Detect(
	void
	);

/****************************************************************************
* ENS_Detect - Detects Ensoniq Sound Scape
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
ENS_Detect(
	VOID
	);

/****************************************************************************
* CODEC_Detect - Detects MSWSS, Compaq Business Audio and other CODEC based
*                audio cards.
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*	2 = Could not allocate DMA buffer
*	3 = Could not detect interrupt or DMA
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
CODEC_Detect(
	int		*iBaseAddr,		// IN/OUT: -2=Scan, -1=Default, >0=User Specified
	int		*iDma			// IN/OUT: -2=Scan, -1=Default, >=0 User Specified
	);

