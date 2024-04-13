/*===========================================================================
* PCDMA.C - Module for Handling DMA
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
* $Log:   F:/DMX/INC/VCS/PCDMA.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:17:12   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

#ifndef _PC_DMA_H
#define _PC_DMA_H

PUBLIC void
PC_FreeDMABuffer(
	BYTE		*DmaPtr			// INPUT : Dma Buffer to Free
	);

PUBLIC BYTE *
PC_AllocDMABuffer(
	WORD		wDmaSize		// INPUT : Size in bytes of DMA
	);

PUBLIC WORD
PC_GetDmaCount(
	BYTE        bDmaChannel		// INPUT : DMA Channel # {0,1,3,5,6,7}
	);

PUBLIC void
PC_StopDMA(
	BYTE        bDmaChannel 	// INPUT : DMA Channel # {0,1,3,5,6,7}
    );

PUBLIC void
PC_PgmDMA(
	BYTE        bDmaChannel,	// INPUT : DMA Channel # {0,1,3,5,6,7}
	BYTE		*DMAPtr,		// INPUT : DMA Buffer Address
	WORD		wcbDmaBlock,	// INPUT : # of bytes to transfer
	BOOL        fInput,			// INPUT : In / Out from PC
	BOOL        fAutoInit		// INPUT : Single / Autoinitialize
	);


#endif
