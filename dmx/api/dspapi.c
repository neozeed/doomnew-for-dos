/*===========================================================================
* DSPAPI.C - Digital Signal Processing Utilities
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
* $Log:$
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <limits.h>
#include <dos.h>

#include "dmx.h"
#include "dspapi.h"
#include "mixers.h"

// #define SCRNDUMP

#define MAX_CHANNELS	8

typedef struct
{
	INT			iLeftLevel;
	INT			iRightLevel;
	INT			iPhase;
	WORD		wSampleRate;
	DWORD		dWavLength;
	WAV_SAMPLE	*wdWavData;
	BYTE		*bPPV;
	VCHAN		*vChan;
	INT			iPriority;
	INT			iHandle;
} CHANNEL;

#define SILENCE         0x80
#define DMA_BUFSIZE     4096
#define DMA_11MONO		128
#define DMA_11STEREO	256
#define DMA_22MONO		256
#define DMA_22STEREO	512

typedef struct
{
	BYTE	quotient;
	BYTE	remainder;
} MIXRATE;

LOCAL MIXRATE	mixrates[ 256 ] =
{
	{0,  64},	{0,  64},	{0,  65},	{0,  66},
	{0,  66},	{0,  67},	{0,  68},	{0,  69},
	{0,  69},	{0,  70},	{0,  71},	{0,  72},
	{0,  72},	{0,  73},	{0,  74},	{0,  75},
	{0,  76},	{0,  76},	{0,  77},	{0,  78},
	{0,  79},	{0,  80},	{0,  81},	{0,  82},
	{0,  83},	{0,  83},	{0,  84},	{0,  85},
	{0,  86},	{0,  87},	{0,  88},	{0,  89},
	{0,  90},	{0,  91},	{0,  92},	{0,  93},
	{0,  94},	{0,  95},	{0,  96},	{0,  97},
	{0,  98},	{0,  99},	{0, 100},	{0, 102},
	{0, 103},	{0, 104},	{0, 105},	{0, 106},
	{0, 107},	{0, 108},	{0, 110},	{0, 111},
	{0, 112},	{0, 113},	{0, 114},	{0, 116},
	{0, 117},	{0, 118},	{0, 119},	{0, 121},
	{0, 122},	{0, 123},	{0, 125},	{0, 126},
	{0, 128},	{0, 129},	{0, 130},	{0, 132},
	{0, 133},	{0, 135},	{0, 136},	{0, 138},
	{0, 139},	{0, 141},	{0, 142},	{0, 144},
	{0, 145},	{0, 147},	{0, 149},	{0, 150},
	{0, 152},	{0, 153},	{0, 155},	{0, 157},
	{0, 159},	{0, 160},	{0, 162},	{0, 164},
	{0, 166},	{0, 167},	{0, 169},	{0, 171},
	{0, 173},	{0, 175},	{0, 177},	{0, 179},
	{0, 181},	{0, 183},	{0, 185},	{0, 187},
	{0, 189},	{0, 191},	{0, 193},	{0, 195},
	{0, 197},	{0, 199},	{0, 201},	{0, 203},
	{0, 206},	{0, 208},	{0, 210},	{0, 213},
	{0, 215},	{0, 217},	{0, 220},	{0, 222},
	{0, 224},	{0, 227},	{0, 229},	{0, 232},
	{0, 234},	{0, 237},	{0, 239},	{0, 242},
	{0, 245},	{0, 247},	{0, 250},	{0, 253},
	{1,   0},	{1,   2},	{1,   5},	{1,   8},
	{1,  11},	{1,  14},	{1,  17},	{1,  20},
	{1,  23},	{1,  26},	{1,  29},	{1,  32},
	{1,  35},	{1,  38},	{1,  41},	{1,  45},
	{1,  48},	{1,  51},	{1,  55},	{1,  58},
	{1,  61},	{1,  65},	{1,  68},	{1,  72},
	{1,  76},	{1,  79},	{1,  83},	{1,  87},
	{1,  90},	{1,  94},	{1,  98},	{1, 102},
	{1, 106},	{1, 110},	{1, 114},	{1, 118},
	{1, 122},	{1, 126},	{1, 130},	{1, 134},
	{1, 138},	{1, 143},	{1, 147},	{1, 151},
	{1, 156},	{1, 160},	{1, 165},	{1, 169},
	{1, 174},	{1, 179},	{1, 184},	{1, 188},
	{1, 193},	{1, 198},	{1, 203},	{1, 208},
	{1, 213},	{1, 218},	{1, 223},	{1, 229},
	{1, 234},	{1, 239},	{1, 245},	{1, 250},
	{2,   0},	{2,   5},	{2,  11},	{2,  16},
	{2,  22},	{2,  28},	{2,  34},	{2,  40},
	{2,  46},	{2,  52},	{2,  58},	{2,  64},
	{2,  71},	{2,  77},	{2,  83},	{2,  90},
	{2,  96},	{2, 103},	{2, 110},	{2, 117},
	{2, 123},	{2, 130},	{2, 137},	{2, 144},
	{2, 152},	{2, 159},	{2, 166},	{2, 173},
	{2, 181},	{2, 188},	{2, 196},	{2, 204},
	{2, 212},	{2, 220},	{2, 227},	{2, 236},
	{2, 244},	{2, 252},	{3,   4},	{3,  13},
	{3,  21},	{3,  30},	{3,  38},	{3,  47},
	{3,  56},	{3,  65},	{3,  74},	{3,  83},
	{3,  93},	{3, 102},	{3, 111},	{3, 121},
	{3, 131},	{3, 141},	{3, 150},	{3, 160},
	{3, 171},	{3, 181},	{3, 191},	{3, 202},
	{3, 212},	{3, 223},	{3, 234},	{3, 245}
};

LOCAL INT		pcId;
LOCAL BYTE		*gDmaBuffer;
LOCAL BYTE		*gDmaHandle;
LOCAL int		PageMixed;
LOCAL int		Pages;
LOCAL int       PageSize;
LOCAL BOOL      fInitted = FALSE;
LOCAL BYTE		*gPanTables = NULL;
LOCAL BYTE		*gPanMemory = NULL;
LOCAL WORD		*gMixBuffer = NULL;
LOCAL CHNL_TYPE gType;
LOCAL int		gChannels;
LOCAL WORD		gSampleRate;
LOCAL int       gSamplesPerMix;
LOCAL VCHAN		gVirtChan[ MAX_CHANNELS ];
LOCAL CHANNEL	gChannel[ MAX_CHANNELS ];
LOCAL WORD      (*gDmaPC)(WORD) = NULL;
LOCAL VOID      (*gMixNotify)(BYTE*,WORD,WORD) = NULL;
LOCAL BOOL		fSyncPages = FALSE;
LOCAL BOOL 		fExpandedStereo = FALSE;

LOCAL BYTE 	gPanCoef[ 256 ] = 
{
    0x0E, 0x13, 0x19, 0x1D, 0x22, 0x26, 0x2A, 0x2F, 
    0x33, 0x37, 0x3B, 0x3F, 0x41, 0x45, 0x49, 0x4C, 
    0x50, 0x53, 0x55, 0x59, 0x5C, 0x5E, 0x61, 0x65, 
    0x67, 0x6A, 0x6C, 0x6F, 0x71, 0x74, 0x75, 0x78, 
    0x7A, 0x7D, 0x7F, 0x80, 0x83, 0x85, 0x86, 0x89, 
    0x8B, 0x8C, 0x8E, 0x90, 0x92, 0x93, 0x95, 0x96, 
    0x99, 0x9A, 0x9C, 0x9D, 0x9E, 0xA0, 0xA2, 0xA3, 
    0xA5, 0xA6, 0xA7, 0xA9, 0xAA, 0xAB, 0xAC, 0xAE, 
    0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB5, 0xB6, 0xB7, 
    0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC5, 0xC6, 
    0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCD, 
    0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD2, 0xD3, 
    0xD3, 0xD4, 0xD5, 0xD6, 0xD6, 0xD6, 0xD7, 0xD8, 
    0xD9, 0xD9, 0xD9, 0xDA, 0xDB, 0xDC, 0xDC, 0xDC, 
    0xDD, 0xDE, 0xDE, 0xDF, 0xDF, 0xE0, 0xE0, 0xE1, 
    0xE1, 0xE1, 0xE2, 0xE3, 0xE4, 0xE4, 0xE4, 0xE5, 
    0xE5, 0xE6, 0xE6, 0xE6, 0xE7, 0xE7, 0xE7, 0xE8, 
    0xE9, 0xE9, 0xE9, 0xE9, 0xEA, 0xEB, 0xEB, 0xEB, 
    0xEC, 0xEC, 0xEC, 0xEC, 0xED, 0xED, 0xED, 0xEE, 
    0xEE, 0xEF, 0xEF, 0xEF, 0xF0, 0xF0, 0xF0, 0xF0, 
    0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xF2, 0xF3, 
    0xF3, 0xF3, 0xF3, 0xF4, 0xF4, 0xF4, 0xF5, 0xF5, 
    0xF5, 0xF5, 0xF6, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 
    0xF7, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 
    0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 
    0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 
    0xFB, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 
    0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFE, 
    0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

LOCAL BYTE 	gExpPanCoef[ 256 ] = 
{
    0x21, 0x25, 0x29, 0x2D, 0x31, 0x34, 0x38, 0x3D, 
    0x3F, 0x43, 0x46, 0x4A, 0x4D, 0x50, 0x54, 0x56, 
    0x59, 0x5C, 0x5E, 0x61, 0x64, 0x66, 0x69, 0x6B, 
    0x6E, 0x71, 0x72, 0x74, 0x77, 0x79, 0x7B, 0x7D, 
    0x80, 0x81, 0x83, 0x86, 0x87, 0x89, 0x8B, 0x8D, 
    0x8E, 0x91, 0x92, 0x93, 0x95, 0x96, 0x99, 0x9A, 
    0x9B, 0x9D, 0x9E, 0x9F, 0xA1, 0xA3, 0xA4, 0xA5, 
    0xA6, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xB0, 
    0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 
    0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBD, 0xBE, 0xBF, 
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 
    0xC8, 0xC9, 0xC9, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 
    0xCE, 0xCE, 0xCF, 0xCF, 0xD0, 0xD1, 0xD2, 0xD2, 
    0xD3, 0xD3, 0xD4, 0xD4, 0xD5, 0xD6, 0xD7, 0xD7, 
    0xD7, 0xD8, 0xD9, 0xDA, 0xDA, 0xDA, 0xDB, 0xDB, 
    0xDC, 0xDD, 0xDD, 0xDD, 0xDE, 0xDF, 0xDF, 0xDF, 
    0xE0, 0xE0, 0xE1, 0xE2, 0xE2, 0xE2, 0xE3, 0xE3, 
    0xE4, 0xE4, 0xE4, 0xE5, 0xE5, 0xE5, 0xE6, 0xE6, 
    0xE7, 0xE7, 0xE7, 0xE8, 0xE8, 0xE8, 0xE9, 0xE9, 
    0xEA, 0xEA, 0xEA, 0xEB, 0xEB, 0xEB, 0xEB, 0xEC, 
    0xEC, 0xEC, 0xED, 0xED, 0xEE, 0xEE, 0xEE, 0xEE, 
    0xEF, 0xEF, 0xEF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF1, 
    0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 
    0xF2, 0xF3, 0xF3, 0xF3, 0xF3, 0xF4, 0xF4, 0xF4, 
    0xF4, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF6, 
    0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 
    0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 
    0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 
    0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 
    0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFC, 0xFC, 
    0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 
    0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD
};

#if 0
LOCAL BYTE 	gPanCoef[ 256 ] = 
{
    0x53, 0x55, 0x59, 0x5C, 0x5F, 0x61, 0x65, 0x68, 
    0x6B, 0x6D, 0x70, 0x72, 0x75, 0x78, 0x79, 0x7B, 
    0x7E, 0x80, 0x83, 0x84, 0x87, 0x89, 0x8A, 0x8D, 
    0x8F, 0x90, 0x93, 0x94, 0x96, 0x98, 0x9A, 0x9B, 
    0x9D, 0x9F, 0xA0, 0xA2, 0xA3, 0xA4, 0xA6, 0xA8, 
    0xA9, 0xAB, 0xAC, 0xAD, 0xAE, 0xB0, 0xB1, 0xB2, 
    0xB3, 0xB4, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 
    0xBD, 0xBE, 0xBF, 0xC1, 0xC2, 0xC3, 0xC4, 0xC4, 
    0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 
    0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD0, 0xD1, 0xD2, 
    0xD3, 0xD4, 0xD5, 0xD5, 0xD6, 0xD6, 0xD7, 0xD8, 
    0xD9, 0xD9, 0xDA, 0xDA, 0xDB, 0xDC, 0xDC, 0xDD, 
    0xDE, 0xDE, 0xDF, 0xDF, 0xE0, 0xE1, 0xE1, 0xE1, 
    0xE2, 0xE3, 0xE4, 0xE4, 0xE4, 0xE5, 0xE5, 0xE6, 
    0xE6, 0xE7, 0xE7, 0xE8, 0xE9, 0xE9, 0xE9, 0xEA, 
    0xEA, 0xEB, 0xEB, 0xEB, 0xEC, 0xEC, 0xEC, 0xED, 
    0xEE, 0xEE, 0xEE, 0xEF, 0xEF, 0xEF, 0xEF, 0xF0, 
    0xF1, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xF3, 
    0xF3, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5, 0xF5, 0xF5, 
    0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF7, 0xF8, 
    0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
    0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB, 
    0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 
    0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
};
#endif

PRIVATE VOID
dspCalcVolTable(
	BYTE		*VolTable,
	int			iDynamicRange
	)
{
	int			j;
	int			w_index;
	int			f_index;
	int			f_summing;
	int			k;

	k = - ( iDynamicRange / 2 );
	w_index = iDynamicRange >> 8;
	f_index = iDynamicRange & 255;
	f_summing = f_index >> 1;
	for ( j = 0; j < 256; j++ )
	{
		*VolTable++ = ( BYTE ) k;
		f_summing += f_index;
		k += ( w_index + ( f_summing >> 8 ) );
		f_summing &= 255;
	}
}

PRIVATE void
dspCalcTables(
	CHANNEL		*WavChnl
	)
{
	BYTE		*bTable;

	bTable = WavChnl->bPPV;
	if ( gType == DSP_STEREO_LR8 || gType == DSP_STEREO_LR8S )
	{
		dspCalcVolTable( bTable, WavChnl->iLeftLevel );
 		dspCalcVolTable( bTable + 256, WavChnl->iRightLevel );
		WavChnl->iPhase = -( WavChnl->iPhase );
	}
	else if ( gType == DSP_STEREO_RL8 )
	{
		dspCalcVolTable( bTable + 256, WavChnl->iLeftLevel );
 		dspCalcVolTable( bTable, WavChnl->iRightLevel );
	}
	else if ( gType == DSP_MONO_8 )
	{
		dspCalcVolTable( bTable,
			( WavChnl->iLeftLevel + WavChnl->iRightLevel ) >> 1 );
	}
}

PUBLIC VOID
dspCalcPanPosVol(
	int			panpot,    	// INPUT : 0=Left,127=Center,255=Right
	int			volume,   	// INPUT : 0=Silent..127=Full Volume
	int			*iLeft,		// OUTPUT: Left Volume Level
	int			*iRight,	// OUTPUT: Right Volume Level
	int			*iPhase		// OUTPUT: Phase Shift
	)
{
	if ( gType == DSP_MONO_8 )
	{
		*iLeft = *iRight = ( volume + 1 ) * 2;
		*iPhase = 0;
	}
	else
	{
		if ( panpot < 0 )
			panpot = 0;
		if ( panpot > 255 )
			panpot = 255;

		if ( fExpandedStereo )
		{
			*iLeft = ( gExpPanCoef[ 255 - panpot ] * volume ) / 127;
			*iRight = ( gExpPanCoef[ panpot ] * volume ) / 127;
			*iPhase = ( 16 - ( ( panpot + 1 ) >> 3 ) );
//			*iPhase = ( 16 - ( ( panpot + 1 ) >> 3 ) ) * ( 127 - volume ) / 127;
//			*iPhase = ( 16 - ( ( panpot + 1 ) >> 3 ) ) * volume / 127;
//			printf( "USING EXPANDED STEREO, PHASE %d PANPOT:%d VOL:%d\n\n", *iPhase, panpot, volume );
		}
		else
		{
			*iLeft = ( gPanCoef[ 255 - panpot ] * volume ) / 127;
			*iRight = ( gPanCoef[ panpot ] * volume ) / 127;
			*iPhase = 0;
//			printf( "USING NORMAL STEREO\n\n" );
		}
	}
}


/****************************************************************************
* dspAdvSetOrigin - Change origin of active patch.
* ---------------------------------------------------------------------------
* NOTE: for each X,VOLUME and PITCH the parameter will be ignored if it's
*       value == -1
*****************************************************************************/
PUBLIC VOID
dspAdvSetOrigin(
	int			handle,     // INPUT : Handle to Active Patch
	int			pitch,		// INPUT : 0..127..255
	int			left,     	// INPUT : X (l-r) 0=Left,127=Center,255=Right
	int			right,    	// INPUT : 0=Silent..127=Full Volume
	int			phase		// INPUT : Phase Shift
	)
{
	CHANNEL		*WavChnl;
	VCHAN		*vc;
	BYTE		*bWavStart;
	WORD		wScale;
	UINT		pflags;
	LONG		samples;

	pflags = PUSH_DISABLE();

	if ( ! dspIsPlaying( handle ) )
	{
		POP_DISABLE( pflags );
		return ;
	}

	WavChnl = &gChannel[ HIBYTE( handle ) ];

	WavChnl->iLeftLevel = left;
	WavChnl->iRightLevel= right;
	WavChnl->iPhase		= phase * gSampleRate / 22050;
	dspCalcTables( WavChnl );

	vc = WavChnl->vChan;

	bWavStart = ( BYTE * )( WavChnl->wdWavData ) + sizeof( WAV_SAMPLE ) + 16;
//	printf( "Old Phase=%d  New Phase=%d\n", vc->dPhaseShift, WavChnl->iPhase );
//	printf( "WS:%6LX  WP:%6LX\n",  );
//	fflush( stdout );
	if ( ( int )( vc->dPhaseShift ) > 0 )
		bWavStart -= vc->dPhaseShift;

	samples = ( LONG )( vc->bWavPtr - bWavStart );
	samples = ( LONG )( WavChnl->dWavLength - samples - 16 );
	if ( samples < 0 )
		samples = 0;

//	if ( WavChnl->iPhase > 0 )
//		vc->bWavPtr -= ( WavChnl->iPhase );

	vc->dPhaseShift = ( DWORD ) WavChnl->iPhase;

	wScale = ( WORD )( ( mixrates[ pitch ].quotient << 8 )
					+ mixrates[ pitch ].remainder );

	wScale = ( ( ( DWORD ) WavChnl->wSampleRate * wScale ) +
				( gSampleRate >> 1 ) ) / gSampleRate;


	vc->dMogCorRem	 = LOBYTE( wScale );
	vc->dMogQuotient = HIBYTE( wScale );

	vc->dSamples = ( samples << 8 ) / wScale;

	vc->dMogCorRem |= ( ( vc->dMogCorRem >> 1 ) << 8 );	// Init to Half Err

	if ( vc->bWavStartPtr )
	{
		vc->dLoopSamples = ( WavChnl->dWavLength << 8 ) / wScale;;
	}

	POP_DISABLE( pflags );
}

/****************************************************************************
* dspSetOrigin - Change origin of active patch.
*****************************************************************************/
PUBLIC VOID
dspSetOrigin(
	int			handle,     // INPUT : Handle to Active Patch
	int			pitch,		// INPUT : 0..127..255
	int			x,     		// INPUT : X (l-r) 0=Left,127=Center,255=Right
	int			v    		// INPUT : 0=Silent..127=Full Volume
	)
{
	int			iLeft;
	int			iRight;
	int			iPhase;

	dspCalcPanPosVol( x, v, &iLeft, &iRight, &iPhase );
	dspAdvSetOrigin( handle, pitch, iLeft, iRight, iPhase );
}

/****************************************************************************
* dspStopPatch - Stop playing patch associated with handle.
*****************************************************************************/
PUBLIC VOID
dspStopPatch(
	int			handle
	)
{
	int			c;
	WORD		id;

	if ( handle >= 0 )
	{
		c = HIBYTE( handle );
		id = LOBYTE( handle );

		if ( LOBYTE( gChannel[ c ].iHandle ) == id )
		{
			gChannel[ c ].vChan->dLoopSamples = 0;
			gChannel[ c ].vChan->dSamples = 0;
		}
	}
}


/****************************************************************************
* dspIsPlaying - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
dspIsPlaying(
	int			handle
	)
{
	if ( handle >= 0 )
	{
		int		c = HIBYTE( handle );
		WORD	id = LOBYTE( handle );

		if ( LOBYTE( gChannel[ c ].iHandle ) == id &&
			 ( gChannel[ c ].vChan->dSamples != 0 ||
			   gChannel[ c ].vChan->dLoopSamples != 0 )
		   )
		{
			return 1;
		}
	}
	return 0;
}


/****************************************************************************
* dspAdvPlayPatch - Play Patch.
*****************************************************************************/
PUBLIC int		// Returns: -1=Failure, else good handle
dspAdvPlayPatch(
	void		*patch,		// INPUT : Patch to play
	int			pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int			left,    	// INPUT : 0=Silent..127=Full Volume
	int			right,   	// INPUT : 0=Silent..127=Full Volume
	int			phase,		// INPUT : Sample Delay -16=L..0=C..16=R
	int			flags,		// INPUT : Flags
	int			priority	// INPUT : Priority, 0=No Stealing 1..MAX_INT
	)
{
	WAV_SAMPLE	*wav = ( WAV_SAMPLE * ) patch;
	WORD		wScale;
	CHANNEL		*WavChnl;
	DWORD		dSamplesLeft;
	BYTE		bPrevColor;
	VCHAN		*vc;
	UINT		pflags;
	int			ChnlPri;
	int			ChnlPriIdx;
	int			j;
	int			k;

	if ( ! fInitted )
		return -1;

#ifndef PROD
	if ( DmxDebug & DMXBUG_DSPAPI )
	{
		UINT	flags;

		flags = PUSH_DISABLE();
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 4 );
		POP_DISABLE( flags );
	}
#endif
/*
* Look for inactive channel...
* The channel allocation is processed in the following order:
*	1) An inactive channel is found.
*	2) A channel with a lower priority (lowest found) is stolen
*		a) if more than one channel is at the lowest priority, then
*		   the channel that has played the longest is stolen.
* 	3) If all channels are at the same priority level as our new voice:
*	   	a) if an active voice matches ours, then restart it.
*		b) steal the channel that has played the longest.
*	4) If the above rules can't be satisfied, don't play the voice.
*/
	ChnlPri = 0;
	ChnlPriIdx = 0;
	dSamplesLeft = ULONG_MAX;

	for ( j = 0; j < gChannels; j++ )
	{
		WavChnl = &gChannel[ j ];
		vc = WavChnl->vChan;
		if ( vc->dSamples == 0 && vc->dLoopSamples == 0 )
		{
			ChnlPriIdx = j;
			goto FOUND_CHANNEL;
		}

		if ( WavChnl->iPriority > ChnlPri )
		{
			ChnlPri = WavChnl->iPriority;
			ChnlPriIdx = j;
		}
		else if ( WavChnl->iPriority == ChnlPri && vc->dLoopSamples == 0 &&
				  vc->dSamples < dSamplesLeft
				)
		{
			dSamplesLeft = vc->dSamples;
			ChnlPriIdx = j;
		}
	}
/*
* All Wave Channels are currently active, look for the lowest priority
* channel and steal it away if it's priority is less that ours.
*/
	if ( ChnlPri < priority || ChnlPri == 0 )
	{
		/* All Channels busy...	*/
#ifndef PROD
		if ( DmxDebug & DMXBUG_DSPAPI )
		{
			inp( 0x3da );
			outp( 0x3c0, 0x31 );
			outp( 0x3c0, bPrevColor );
		}
#endif
		return -1;
	}
	if ( ChnlPri == priority && gChannel[ ChnlPriIdx ].wdWavData != wav )
	{
	/*
	* We already know which would be the ideal to steal if they are all
	* unique, but the last check is for one with the SAME waveform.
	*/
		for ( k = 0; k < gChannels; k++ )
		{
			if ( gChannel[ k ].wdWavData == wav &&
		 		 gChannel[ k ].vChan->dLoopSamples == 0
				 )
			{
				ChnlPriIdx = k;
				break;
			}
		}
	}

FOUND_CHANNEL:
/*
* Setup Channel for Playback
*/
	WavChnl = &gChannel[ ChnlPriIdx ];
	vc = WavChnl->vChan;

	pflags = PUSH_DISABLE();

	WavChnl->wdWavData	= wav;
	WavChnl->dWavLength = wav->WaveLen - 32;
	WavChnl->wSampleRate= wav->Rate;
	WavChnl->iPriority	= priority;
	WavChnl->iHandle	= ( WavChnl->iHandle + 1 ) & 255;
	WavChnl->iLeftLevel = left;
	WavChnl->iRightLevel= right;
	WavChnl->iPhase		= phase * gSampleRate / 22050;

	dspCalcTables( WavChnl );

	memset( vc, 0, sizeof( VCHAN ) );
	vc->bVolPtr = WavChnl->bPPV;

	wScale = ( WORD )( ( mixrates[ pitch ].quotient << 8 )
					   + mixrates[ pitch ].remainder );

	wScale = ( ( ( DWORD ) WavChnl->wSampleRate * wScale ) +
				( gSampleRate >> 1 ) ) / gSampleRate;

	vc->dMogCorRem	 = LOBYTE( wScale );
	vc->dMogQuotient = HIBYTE( wScale );

	vc->dMogCorRem |= ( ( vc->dMogCorRem >> 1 ) << 8 );	// Init to Half Err
	vc->bWavPtr = ( BYTE * ) patch + sizeof( WAV_SAMPLE ) + 16;
	if ( WavChnl->iPhase > 0 )
		vc->bWavPtr -= ( WavChnl->iPhase );
	vc->dPhaseShift = ( DWORD ) WavChnl->iPhase;
/*
* Note: by setting VCHAN.dSamples to > 0 we are informing the background
* 		DSP service routine that it is okay to play this channel.
*/
	vc->dSamples = ( WavChnl->dWavLength << 8 ) / wScale;

	if ( flags & SFX_LOOP )
	{
		vc->bWavStartPtr = vc->bWavPtr;
		vc->dLoopSamples = vc->dSamples;
	}
	POP_DISABLE( pflags );

#ifndef PROD
	if ( DmxDebug & DMXBUG_DSPAPI )
	{
		pflags = PUSH_DISABLE();
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
		POP_DISABLE( pflags );
	}
#endif
	return ( ChnlPriIdx << 8 ) + WavChnl->iHandle;
}

/****************************************************************************
* dspPlayPatch - Play Patch.
*****************************************************************************/
PUBLIC int		// Returns: -1=Failure, else good handle
dspPlayPatch(
	void		*patch,		// INPUT : Patch to play
	int			pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int			x,     		// INPUT : X (l-r) 0=Left,127=Center,255=Right
	int			v,    		// INPUT : 0=Silent..127=Full Volume
	int			flags,		// INPUT : Flags
	int			priority	// INPUT : Priority, 0=No Stealing 1..MAX_INT
	)
{
	int			iLeft;
	int			iRight;
	int			iPhase;

	dspCalcPanPosVol( x, v, &iLeft, &iRight, &iPhase );
	return dspAdvPlayPatch( patch, pitch, iLeft, iRight, iPhase, flags, priority );
}

/****************************************************************************
* dspNextPage - Advance to next mixing page
*****************************************************************************/
PUBLIC void
dspNextPage(
	void
	)
{
	TSM_SyncService( pcId );
}

/****************************************************************************
* dspGetDmaBuffer - Get DMA Buffer Address
*****************************************************************************/
PUBLIC BYTE *
dspGetDmaBuffer(
	void
	)
{
	return gDmaHandle;
}

/****************************************************************************
* dspGetPageSize - Get DMA Page Size
*****************************************************************************/
PUBLIC WORD
dspGetPageSize(
	void
	)
{
	if ( fSyncPages )
		return	PageSize;
	else
		return PageSize * Pages;
}

/****************************************************************************
* dspGetDmaSize - Get Size of DMA Buffer
*****************************************************************************/
PUBLIC WORD
dspGetDmaSize(
	void
	)
{
	return PageSize * Pages;
}

DWORD	micros;
int		ztimed = 0;

#include "tohex.c"

/****************************************************************************
* dspMixPage - Call mix engine to merge waveforms together.
*****************************************************************************/
PRIVATE int
dspMixPage(
	void
	)
{
	BYTE		bPrevColor;
	BYTE        *dmaBuffer;
	WORD		DmaPage;
    WORD        Offset;
	int			j;
    int         rc = TS_OKAY;

#ifndef PROD
	if ( DmxDebug & DMXBUG_MIXERS )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 5 );
//		DmxTrace( "> dspMixPage" );
	}
#endif
	if ( gDmaPC != NULL )
	{
		char		str[ 80 ];
		WORD		ofs;

		ofs = gDmaPC( DMA_BUFSIZE );
		DmaPage = ( ofs / PageSize ) + 1;
//		DmaPage = ( gDmaPC( DMA_BUFSIZE ) / PageSize ) + 1;
		if ( DmaPage >= Pages )
			DmaPage = 0;

		strcpy( str, "MixPg: " );
		wordtohex( ofs, &str[ 7 ] );
		str[ 11 ] = ' ';
		wordtohex( DmaPage, &str[ 12 ] );
		str[ 16 ] = ' ';
		wordtohex( PageMixed, &str[ 17 ] );
		DmxTrace( str );

        Offset = DmaPage * PageSize;
		dmaBuffer = gDmaBuffer + Offset;

		if ( DmaPage != PageMixed )
		{
			for ( j = 0; j < gChannels; j++ )
			{
				if ( gVirtChan[ j ].dSamples )
					break;
			}
			if ( j < gChannels )
			{
    			if ( gType == DSP_MONO_8 )
    				dmxMix8bitMono( gVirtChan, gChannels, gMixBuffer, gSamplesPerMix );
				else if ( fExpandedStereo )
    				dmxMix8bitExpStereo( gVirtChan, gChannels, gMixBuffer, gSamplesPerMix );
    			else	 
    				dmxMix8bitStereo( gVirtChan, gChannels, gMixBuffer, gSamplesPerMix );
				
				if ( gType == DSP_STEREO_LR8S )
					dmxClipSeperate( dmaBuffer, gMixBuffer, gSamplesPerMix * 2 );
				else
					dmxClipInterleave( dmaBuffer, gMixBuffer, 
								( gType == DSP_MONO_8 ? gSamplesPerMix : gSamplesPerMix * 2 ));
			}
			else
			{
				memset( dmaBuffer, 0x80, PageSize );
			}
#ifdef SCRNDUMP
/* DEBUG STUFF - DUMP TO SCREEN
*/
		    {
			    BYTE	*vidram = ( BYTE * ) ( 0xB8000L );
			    char	str[ 200 ];
			    char	*hex = "0123456789ABCDEF";
			    BYTE	*ptr;
			    int		j;
			    int		c;
			    int		k;

			    ptr = dmaBuffer;
			    for ( j = 0; j < PageSize; j+=16 )
			    {
				    memset( str, 0x1F, sizeof( str ) );
				    for ( c = k = 0; k < 32; k++, c+=6 )
				    {
					    str[ c ] = hex[ *ptr >> 4 ];
					    str[ c+2 ] = hex[ *ptr & 0x0F ];
					    str[ c+4 ] = ' ';
					    ptr++;
				    }
				    memcpy( vidram, str, c );
				    vidram += 264;
			    }
		    }
#endif
			PageMixed = DmaPage;

            if ( gMixNotify )
            {
                gMixNotify( dmaBuffer, PageSize, Offset );
            }
		}
        else
        {
            rc = TS_BUSY;
        }
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_MIXERS )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
//		DmxTrace( "< dspMixPage" );
	}
#endif
	return rc;
}

/****************************************************************************
* dspExpandedStereo - Sets Expanded Stereo ON/OFF
*****************************************************************************/
PUBLIC int		// returns: previous setting 0=Off, 1=On
dspExpandedStereo(
	int			mode			// INPUT : 1=On,0=Off
	)
{
	int			cmode = fExpandedStereo;

	fExpandedStereo = ( mode ? 1 : 0 );
	return cmode;
}

/****************************************************************************
* dspInit - Initialize DSP Utilities
*****************************************************************************/
PUBLIC int      // returns: 0=Success, 1=Fail, Not enough memory
dspInit(
    CHNL_TYPE   eType,          // INPUT : Physical properties of channel
    WORD        SampleRate,     // INPUT : Sample Rate for Playback
    int         Channels,       // INPUT : Max # channels allowed
    WORD        (*GetPlayCnt)(  // INPUT : Get current playback position
                    WORD            // PASSBACK: DMA Buffer Size
                    ),
    VOID        (*MixNotify)(   // INPUT : Mix Complete Notification
                    BYTE *,         // PASSBACK; Mixed Data
                    WORD,           // PASSBACK: amount mixed.
                    WORD            // PASSBACK: position
                    ),
	BOOL		SyncPages	    // INPUT : T/F Sync. Pages with DMA Int.
    )
{
    int             j;
	CHANNEL         *c;
	WORD	        offset;
    WORD            pan_size;
    WORD            mix_size;
	size_t			memsize;

    if ( GetPlayCnt == NULL )
        return -1;

	if ( DmxOption & USE_PHASE )
		fExpandedStereo = 1;

	gType = eType;

    gDmaPC      = GetPlayCnt;
    gMixNotify  = MixNotify;
	fSyncPages  = SyncPages;

	PageMixed = 1;

	if ( SampleRate == 11025 )
		PageSize = DMA_11MONO;
	else
		PageSize = DMA_22MONO;

    gSamplesPerMix = PageSize;

    if ( eType != DSP_MONO_8 )
		PageSize += PageSize;

    gSampleRate = SampleRate;

	Pages = DMA_BUFSIZE / PageSize;
	if ( ( gDmaHandle = PC_AllocDMABuffer( DMA_BUFSIZE ) ) == NULL )
	{
		return RS_NOT_ENOUGH_MEMORY;
	}
#ifdef __386__
    gDmaBuffer = gDmaHandle;
#else
    gDmaBuffer = Addr32Bit( gDmaHandle );
#endif
    memset( gDmaBuffer, ( BYTE ) SILENCE, DMA_BUFSIZE );

	if ( Channels > MAX_CHANNELS )
        Channels = MAX_CHANNELS;
	if ( Channels < 1 )
		Channels = 1;

    gChannels   = Channels;

	memset( gVirtChan, 0, sizeof( gVirtChan ) );
	memset( gChannel, 0, sizeof( gChannel ) );
	offset = 0;

    if ( eType == DSP_MONO_8 )
    {
		memsize = ( 256 * 1 * MAX_CHANNELS ) + 512;
        pan_size = 256;
        mix_size = PageSize * sizeof( WORD );
    }
	else
    {
		memsize = ( 256 * 2 * MAX_CHANNELS ) + 512;
        pan_size = 512;
        mix_size = ( PageSize * 2 ) * sizeof( WORD );
    }
	if ( ( gPanMemory = malloc( memsize ) ) == NULL ||
		 ( gMixBuffer = malloc( mix_size ) ) == NULL
	   )
	{
		if ( gPanMemory )
	        free( gPanMemory );
		PC_FreeDMABuffer( gDmaHandle );
		gDmaHandle = NULL;
		gDmaBuffer = NULL;
        gPanMemory = NULL;
		return RS_NOT_ENOUGH_MEMORY;
	}
    gPanTables = (BYTE*)(( DWORD )( gPanMemory + 255 ) & ~255 );

	for ( j = 0; j < MAX_CHANNELS; j++ )
	{
		c = &gChannel[ j ];

		c->vChan 	= &gVirtChan[ j ];
		c->bPPV 	= gPanTables + offset;
		c->iHandle	= -1;

        offset += pan_size;
	}
    fInitted = TRUE;
	pcId = TSM_NewService( dspMixPage, ( fSyncPages ? 1 : 0 ), 1, 0 );
	if ( pcId < 0 )
	{
		return -1;
	}
    return 0;
}


/****************************************************************************
* dspDeInit - Deinitialize DSP Utilities
*****************************************************************************/
PUBLIC void
dspDeInit(
    void
    )
{
    int         j;

    if ( ! fInitted )
        return ;

	TSM_DelService( pcId );
    fInitted = FALSE;
	for ( j = 0; j < gChannels; j++ )
	{
        gChannel[ j ].vChan->dSamples = 0;
    }

    if ( gPanMemory )
    {
        free( gPanMemory );
        gPanMemory = NULL;
    }
    if ( gMixBuffer )
    {
        free( gMixBuffer );
        gMixBuffer = NULL;
    }
	if ( gDmaHandle != NULL )
	{
		PC_FreeDMABuffer( gDmaHandle );
		gDmaHandle = NULL;
		gDmaBuffer = NULL;
	}
}
