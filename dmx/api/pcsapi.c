/*===========================================================================
* PCSAPI.C - PC Speaker API
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
* $Log:   F:/DMX/API/VCS/PCSAPI.C_V  $
*  
*     Rev 1.0   02 Oct 1993 15:08:26   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#include <conio.h>

#include "dmx.h"

LOCAL INT	pcId;
LOCAL BYTE	*pcPtr;
LOCAL INT 	pcHandle;
LOCAL INT	pcSound;
LOCAL INT	pcIdx;
LOCAL WORD	pcPrevFreq;
LOCAL INT 	pcPriority;
LOCAL WORD	pcFreq[ 128 ] =
{
	0,		// [  0]   NO SOUND Note #   Pitch
	0x1AA2,	// [  1]  175Hz		F	 29	 128
	0x19E4,	// [  2]  180Hz				 191
	0x1931,	// [  3]  185Hz		F#	 30	 128
	0x1887,	// [  4]  190Hz				 191
	0x17C7,	// [  5]  196Hz		G	 31	 128
	0x1712,	// [  6]  202Hz				 191
	0x1668,	// [  7]  208Hz		G#	 32	 128
	0x15C7,	// [  8]  214Hz				 191
	0x152F,	// [  9]  220Hz		A	 33	 128
	0x149F,	// [ 10]  226Hz				 191
	0x1400,	// [ 11]  233Hz		A#	 34	 128
	0x136B,	// [ 12]  240Hz				 191
	0x12DE,	// [ 13]  247Hz		B	 35	 128
	0x1259,	// [ 14]  254Hz				 191
	0x11CA,	// [ 15]  262Hz		C	 36	 128
	0x1153,	// [ 16]  269Hz				 191
	0x10D3,	// [ 17]  277Hz		C#	 37	 128
	0x105A,	// [ 18]  285Hz				 191
	0x0FDA,	// [ 19]  294Hz		D	 38	 128
	0x0F6E,	// [ 20]  302Hz				 191
	0x0EFC,	// [ 21]  311Hz		D#	 39	 128
	0x0E90,	// [ 22]  320Hz				 191
	0x0E1F,	// [ 23]  330Hz		E	 40	 128
	0x0DBF,	// [ 24]  339Hz				 191
	0x0D5A,	// [ 25]  349Hz		F	 41	 128
	0x0CFB,	// [ 26]  359Hz				 191
	0x0C98,	// [ 27]  370Hz		F#	 42	 128
	0x0C3B,	// [ 28]  381Hz				 191
	0x0BE3,	// [ 29]  392Hz		G	 43	 128
	0x0B90,	// [ 30]  403Hz				 191
	0x0B3B,	// [ 31]  415Hz		G#	 44	 128
	0x0AEA,	// [ 32]  427Hz				 191
	0x0A97,	// [ 33]  440Hz		A  	 45	 128
	0x0A49,	// [ 34]  453Hz				 191
	0x0A00,	// [ 35]  466Hz		A#	 46	 128
	0x09B5,	// [ 36]  480Hz				 191
	0x096F,	// [ 37]  494Hz		B	 47	 128
	0x092C,	// [ 38]  508Hz				 191
	0x08E9,	// [ 39]  523Hz		C	 48	 128
	0x08A5,	// [ 40]  539Hz				 191
	0x0869,	// [ 41]  554Hz		C#	 49	 128
	0x0829,	// [ 42]  571Hz				 191
	0x07F0,	// [ 43]  587Hz		D	 50	 128
	0x07B7,	// [ 44]  604Hz				 191
	0x077E,	// [ 45]  622Hz		D#	 51	 128
	0x0748,	// [ 46]  640Hz				 191
	0x0712,	// [ 47]  659Hz		E	 52	 128
	0x06DD,	// [ 48]  679Hz				 191
	0x06AD,	// [ 49]  698Hz		F	 53	 128
	0x067B,	// [ 50]  719Hz				 191
	0x064C,	// [ 51]  740Hz		F#	 54	 128
	0x061D,	// [ 52]  762Hz				 191
	0x05F1,	// [ 53]  784Hz		G	 55	 128
	0x05C6,	// [ 54]  807Hz				 191
	0x059B,	// [ 55]  831Hz		G#	 56	 128
	0x0573,	// [ 56]  855Hz				 191
	0x054B,	// [ 57]  880Hz		A  	 57	 128
	0x0524,	// [ 58]  906Hz				 191
	0x0500,	// [ 59]  932Hz		A#	 58	 128
	0x04DA,	// [ 60]  960Hz				 191
	0x04B7,	// [ 61]  988Hz		B	 59	 128
	0x0495,	// [ 62] 1017Hz				 191
	0x0474,	// [ 63] 1046Hz		C	 60	 128
	0x0453,	// [ 64] 1077Hz				 191
	0x0433,	// [ 65] 1109Hz		C#	 61	 128
	0x0415,	// [ 66] 1141Hz				 191
	0x03F7,	// [ 67] 1175Hz		D	 62	 128
	0x03DA,	// [ 68] 1209Hz				 191
	0x03BF,	// [ 69] 1244Hz		D#	 63	 128
	0x03A3,	// [ 70] 1281Hz				 191
	0x0389,	// [ 71] 1318Hz		E	 64	 128
	0x036F,	// [ 72] 1357Hz				 191
	0x0356,	// [ 73] 1397Hz		F	 65	 128
	0x033D,	// [ 74] 1438Hz				 191
	0x0326,	// [ 75] 1480Hz		F#	 66	 128
	0x030F,	// [ 76] 1523Hz				 191
	0x02F8,	// [ 77] 1568Hz		G	 67	 128
	0x02E3,	// [ 78] 1614Hz				 191
	0x02CE,	// [ 79] 1661Hz		G#	 68	 128
	0x02B9,	// [ 80] 1710Hz				 191
	0x02A5,	// [ 81] 1760Hz		A  	 69	 128
	0x0292,	// [ 82] 1811Hz				 191
	0x0280,	// [ 83] 1864Hz		A#	 70	 128
	0x026D,	// [ 84] 1919Hz				 191
	0x025C,	// [ 85] 1975Hz		B	 71	 128
	0x024A,	// [ 86] 2033Hz				 191
	0x023A,	// [ 87] 2093Hz		C	 72	 128
	0x0229,	// [ 88] 2154Hz				 191
	0x021A,	// [ 89] 2217Hz		C#	 73	 128
	0x020A,	// [ 90] 2282Hz				 191
	0x01FB,	// [ 91] 2349Hz		D	 74	 128
	0x01ED,	// [ 92] 2418Hz				 191
	0x01DF,	// [ 93] 2489Hz		D#	 75	 128
	0x01D1,	// [ 94] 2562Hz				 191
	0x01C4,	// [ 95] 2637Hz		E	 76	 128
	0x01B7,	// [ 96] 2714Hz				 191
	0x01AB,	// [ 97] 2793Hz		F	 77	 128
	0x019F,	// [ 98] 2875Hz				 191
	0x0193,	// [ 99] 2959Hz		F#	 78	 128
	0x0187,	// [100] 3046Hz				 191
	0x017C,	// [101] 3135Hz		G	 79	 128
	0x0171,	// [102] 3227Hz				 191
	0x0167,	// [103] 3322Hz		G#	 80	 128
	0x015C,	// [104] 3419Hz				 191
	0x0153,	// [105] 3519Hz		A  	 81	 128
	0x0149,	// [106] 3622Hz				 191
	0x013F,	// [107] 3729Hz		A#	 82	 128
	0x0136,	// [108] 3838Hz				 191
	0x012E,	// [109] 3950Hz		B	 83	 128
	0x0125,	// [110] 4066Hz				 191
	0x011D,	// [111] 4185Hz		C	 84	 128
	0x0114,	// [112] 4308Hz				 191
	0x010D,	// [113] 4434Hz		C#	 85	 128
	0x0105,	// [114] 4564Hz				 191
	0x00FD,	// [115] 4698Hz		D	 86	 128
	0x00F6,	// [116] 4835Hz				 191
	0x00EF,	// [117] 4977Hz		D#	 87	 128
	0x00E8,	// [118] 5123Hz				 191
	0x00E2,	// [119] 5273Hz		E	 88	 128
	0x00DB,	// [120] 5427Hz				 191
	0x00D5,	// [121] 5587Hz		F	 89	 128
	0x00CF,	// [122] 5750Hz				 191
	0x00C9,	// [123] 5919Hz		F#	 90	 128
	0x00C3,	// [124] 6092Hz				 191
	0x00BE,	// [125] 6271Hz		G	 91	 128
	0x00B8,	// [126] 6454Hz				 191
	0x00B3,	// [127] 6643Hz		G#	 92	 128
};										 

PRIVATE VOID
PCS_Service(
	VOID
	)
{
	WORD	freq;
	BYTE	bPrevColor;

#ifndef PROD
	if ( DmxDebug & DMXBUG_PCSAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 6 );
	}
#endif
	if ( pcSound )
	{
		if ( pcIdx == pcSound )
		{
			pcSound = 0;
			pcPrevFreq = 0xFFFF;
			outp( 0x61, inp( 0x61 ) & ~3 );
		}
		else
		{
			freq = pcFreq[ pcPtr[ pcIdx++ ] ];

			if ( pcPrevFreq != freq )
			{
				if ( ! freq )
					outp( 0x61, inp( 0x61 ) & ~3 );
				else
				{
					outp( 0x43, 0xb6 );
					outp( 0x42, LOBYTE( freq ) );
					outp( 0x42, HIBYTE( freq ) );

					outp( 0x61, inp( 0x61 ) | 3 );
				}
				pcPrevFreq = freq;
			}
		}
	}
#ifndef PROD
	if ( DmxDebug & DMXBUG_PCSAPI )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
}


/****************************************************************************
* PCS_Stop - Stop playing sample on PC Speaker.
*****************************************************************************/
PUBLIC void
PCS_Stop(
	int			handle
	)
{
	if ( pcSound && handle == pcHandle )
	{
 		pcSound = 0;
		outp( 0x61, inp( 0x61 ) & ~3 );
	}
}

/****************************************************************************
* PCS_Play - Play sample on PC Speaker.
*****************************************************************************/
PUBLIC int
PCS_Play(
	void	*patch,		// INPUT : Patch to play
	int		priority	// INPUT : Priority Level: 0 = lowest 
	)
{
	PCS_WAVE	*pcs = ( PCS_WAVE * ) patch;
  
	if ( pcSound && priority < pcPriority )
		return -1;

	if ( pcSound )
	{
		pcSound = 0;
		outp( 0x61, inp( 0x61 ) & ~3 );
	}

	if ( ++pcHandle < 0 )
		pcHandle = 0;

	pcPriority = priority;
	pcIdx = pcPrevFreq = 0;
	pcPtr = ( BYTE * ) patch + sizeof( PCS_WAVE );
	pcSound = pcs->WaveLen;

	return pcHandle;
}

/****************************************************************************
* PCS_Playing - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
PCS_Playing(
	int		handle
	)
{
	if ( pcSound && handle == pcHandle )
		return 1;
	else
		return 0;
}

/****************************************************************************
* PCS_Init - Initialize PC Speaker for 140hz sample playback
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
PCS_Init(
	void
	)
{
	pcSound = 0;
	pcId = TSM_NewService( PCS_Service, 140, 1, 0 );
	return pcId;
}

/****************************************************************************
* PCS_DeInit - Removes PC Speaker logic from Timer Chain
*****************************************************************************/
PUBLIC void
PCS_DeInit(
	void
	)
{
	TSM_DelService( pcId );
	if ( pcSound )
	{
 		pcSound = 0;
		outp( 0x61, inp( 0x61 ) & ~3 );
	}
}
