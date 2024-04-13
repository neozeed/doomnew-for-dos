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
* $Log:   F:/DMX/API/VCS/PCDMA.C_V  $
*  
*     Rev 1.0   02 Oct 1993 15:03:36   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <conio.h>
#include <dos.h>

#include "dmx.h"
#include "pcdma.h"

/*
*	LowDMA
*/
#define PORT_DMA1_Wr_MASKFLAG       0x0A
#define PORT_DMA1_Wr_MODE           0x0B
#define PORT_DMA1_Wr_CLEARFLIPFLOP  0x0C

/*
*   HighDMA
*/
#define PORT_DMA2_Wr_MASKFLAG       0xD4
#define PORT_DMA2_Wr_MODE           0xD6
#define PORT_DMA2_Wr_CLEARFLIPFLOP  0xD8

/*
*	DMA command values
*/
#define MODE_DMA_WriteTransfer      0x04    /* Device writes to memory */
#define MODE_DMA_ReadTransfer       0x08    /* Device reads from memory */
#define MODE_DMA_AutoInitEnable     0x10
#define MASK_DMA_Set                0x04
#define MASK_DMA_Clear				(~MASK_DMA_Set)

typedef struct
{
    BYTE    PageReg;
    BYTE    AddxReg;
    BYTE    CountReg;
} DMAREGS;
  
LOCAL DMAREGS DmaPorts[ 8 ] =
{
    { 0x87,	0,		1	},
    { 0x83,	2,		3	},
    { 0,	0,		0	},                // Channel 2 not used
    { 0x82,	6,		7	},
    { 0,	0,		0	},                // Channel 4 not used
    { 0x8B,	0xC4,	0xC6},
    { 0x89,	0xC8,	0xCA},
    { 0x8A,	0xCC,	0xCE}
};


PUBLIC BYTE
PC_GetDMAPage(
	BYTE		*DmaPtr			// INPUT : DMA Buffer 
	)
{
	DWORD		dwlAddr;

	dwlAddr = ( DWORD ) DmaPtr;
	return (BYTE)( dwlAddr >> 16 );
}

PUBLIC WORD
PC_GetDMAOffset(
	BYTE		*DmaPtr			// INPUT : DMA Buffer 
	)
{
	DWORD		dwlAddr;

	dwlAddr = ( DWORD ) DmaPtr;
	return (WORD)( dwlAddr & 0x0000FFFFl );
}

PUBLIC WORD
PC_GetDMA2Offset(
	BYTE		*DmaPtr			// INPUT : DMA Buffer 
	)
{
	DWORD		dwlAddr;

	dwlAddr = ( DWORD ) DmaPtr;
	dwlAddr >>= 1;
	return (WORD)( dwlAddr & 0x0000FFFFl );
}

#ifdef __386__
int
DosMemAlloc( WORD size, UINT *segment )
{
    union REGS r;

	memset( &r, 0, sizeof( r ) );
    r.x.eax = 0x100; 			/* DOS allocate DOS memory */
    r.x.ebx = ( DWORD ) size;	/* Number of paragraphs requested */
    int386 (0x31, &r, &r);

    if (r.x.cflag)  /* Failed */
	    return -1;

	*segment = ( WORD ) r.x.eax;
	return 0;
}

void
DosMemFree( UINT segment )
{
    union REGS r;

#if 0
	memset( &r, 0, sizeof( r ) );
    r.x.eax = 0x0101;			/* DPMI free DOS memory */
    r.x.edx = ( DWORD ) segment;
    int386 (0x31, &r, &r);
#endif
}

#else
#define DosMemAlloc(a,b)	_dos_allocmem((a),(b))
#define DosMemFree(a)		_dos_freemem((a))
#endif

PUBLIC void
PC_FreeDMABuffer(
	BYTE		*DmaPtr			// INPUT : Dma Buffer to Free
	)
{
	DWORD		dwlAddr;
#if 0
	if ( DmaPtr != NULL )
	{
		dwlAddr = ( DWORD ) DmaPtr;
		DosMemFree( (UINT)( dwlAddr >> 4 ) );
	}
#endif
}

#define MAX_TRIES	5

PUBLIC BYTE *
PC_AllocDMABuffer(
	WORD		wDmaSize		// INPUT : Size in bytes of DMA
	)
{
	WORD		wParas;
	UINT		uSegment[ MAX_TRIES ];
	UINT		uMCB;
	DWORD		dwlAddr;
	DWORD		dwlStart;
	int			j;

	wParas = ( wDmaSize >> 4 ) + 2;

	for ( j = 0; j < MAX_TRIES; j++ )
	{
		uMCB = 0;
		if ( DosMemAlloc( wParas, &uMCB ) != 0 )
			break;
		else
		{
			uSegment[ j ] = uMCB;
			dwlAddr = dwlStart = ( DWORD ) uMCB << 4;
			dwlStart = ( dwlStart + ( DWORD ) wDmaSize ) & 0xFFFF0000L;
			if ( dwlStart == ( dwlAddr & 0xFFFF0000L ) )
			{
				for ( --j; j >= 0; j-- )
					DosMemFree( uSegment[ j ] );
				return ( void * ) dwlAddr;
			}
		}
	}
	for ( --j; j >= 0; j-- )
		DosMemFree( uSegment[ j ] );

	return NULL;
}

PUBLIC void
PC_StopDMA(
	BYTE        bDmaChannel 	// INPUT : DMA Channel # {0,1,3,5,6,7}
    )
{
    if ( bDmaChannel >= 4 )
    {
        outp( PORT_DMA2_Wr_MASKFLAG, ( bDmaChannel & 0x03 ) | MASK_DMA_Set );
        outp( PORT_DMA2_Wr_CLEARFLIPFLOP, 0 );
    }
    else
    {
        outp( PORT_DMA1_Wr_MASKFLAG, bDmaChannel | MASK_DMA_Set );
        outp( PORT_DMA1_Wr_CLEARFLIPFLOP, 0 );
    }
}

PUBLIC WORD
PC_GetDmaCount(
	BYTE        bDmaChannel		// INPUT : DMA Channel # {0,1,3,5,6,7}
	)
{
    WORD        cnt2;
	WORD		cnt1;
	UINT		flags;
	int			j;

	flags = PUSH_DISABLE();
    for ( j = 0; j < 1; j++ )
    {
        outp( ( bDmaChannel >= 4 ) ?
			    PORT_DMA2_Wr_CLEARFLIPFLOP : PORT_DMA1_Wr_CLEARFLIPFLOP, 0 );
        cnt1 = ( WORD )( inp( DmaPorts[ bDmaChannel ].CountReg ) );
        cnt2 = ( WORD ) inp( DmaPorts[ bDmaChannel ].CountReg );
        if ( cnt2 != 0xFF && cnt1 != 0xFF )
        {
            break;
        }
    }
	POP_DISABLE( flags );
    cnt1 |= ( cnt2 << 8 );
	return cnt1;
}

PUBLIC void
PC_PgmDMA(
	BYTE        bDmaChannel,	// INPUT : DMA Channel # {0,1,3,5,6,7}
	BYTE		*DMAPtr,		// INPUT : DMA Buffer Address
	WORD		wcbDmaBlock,	// INPUT : # of bytes to transfer
	BOOL        fInput,			// INPUT : In / Out from PC
	BOOL        fAutoInit		// INPUT : Single / Autoinitialize
	)
{
    BYTE    bMode;
    WORD    wNumSamplesLess1;
	BYTE	bPage;
	BYTE	bPrevColor;
	WORD	wOffset;

#ifndef PROD
	if ( DmxDebug & DMXBUG_PCDMA )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 2 );
	}
#endif
	bPage = PC_GetDMAPage( DMAPtr );
	wOffset = PC_GetDMAOffset( DMAPtr );
  
    if ( bDmaChannel >= 4 )
    {
        wNumSamplesLess1 = ( wcbDmaBlock >> 1 ) - 1;
		wOffset = PC_GetDMA2Offset( DMAPtr );
  
        outp( PORT_DMA2_Wr_MASKFLAG, ( bDmaChannel & 0x03 ) | MASK_DMA_Set );
	    outp( DmaPorts[ bDmaChannel ].PageReg, bPage );
        outp( PORT_DMA2_Wr_CLEARFLIPFLOP, 0 );
    }
    else
    {
        wNumSamplesLess1 = wcbDmaBlock - 1;
  
        outp( PORT_DMA1_Wr_MASKFLAG, bDmaChannel | MASK_DMA_Set );
	    outp( DmaPorts[ bDmaChannel ].PageReg, bPage );
        outp( PORT_DMA1_Wr_CLEARFLIPFLOP, 0 );
    }
    outp( DmaPorts[ bDmaChannel ].AddxReg, LOBYTE( wOffset ) );
    outp( DmaPorts[ bDmaChannel ].AddxReg, HIBYTE( wOffset ) );
    outp( DmaPorts[ bDmaChannel ].CountReg, LOBYTE( wNumSamplesLess1 ) );
    outp( DmaPorts[ bDmaChannel ].CountReg, HIBYTE( wNumSamplesLess1 ) );
  
    bMode = bDmaChannel & 0x03;
    if ( fAutoInit )
        bMode |= MODE_DMA_AutoInitEnable;
    bMode |= (BYTE)( fInput ? MODE_DMA_WriteTransfer : MODE_DMA_ReadTransfer );
    outp( bDmaChannel >= 4 ? PORT_DMA2_Wr_MODE : PORT_DMA1_Wr_MODE, bMode );
  
    if ( bDmaChannel >= 4 )
        outp( PORT_DMA2_Wr_MASKFLAG, bDmaChannel & 0x03 );
    else
        outp( PORT_DMA1_Wr_MASKFLAG, ( bDmaChannel & MASK_DMA_Clear ) );

#ifndef PROD
	if ( DmxDebug & DMXBUG_PCDMA )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif

}
