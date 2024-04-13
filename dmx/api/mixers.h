/*===========================================================================
* MIXERS.H  - Digital Mixers
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
* $Log:   F:/DMX/API/VCS/MIXERS.H_V  $
*  
*     Rev 1.0   28 Oct 1993 06:49:24   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/

PUBLIC VOID
dmxClipInterleave(
	BYTE		*ouput,		/* Clipped Output 			*/
	WORD		*mixbuf,	/* Mixed Audio 				*/
	int			iSamples	/* Samples to mix			*/
	);

PUBLIC VOID
dmxClipSeperate(
	BYTE		*ouput,		/* Clipped Output 			*/
	WORD		*mixbuf,	/* Mixed Audio 				*/
	int			iSamples	/* Samples to mix			*/
	);

PUBLIC VOID
dmxMix8bitExpStereo( 
	VCHAN 		*vChanList,	/* Virtual Channel List		*/
	int			iListDepth,	/* Depth of VC List			*/
	WORD		*wMixBuf,	/* Mixing Buffer			*/
	int			iBlkSize	/* DMA Block Size			*/
	);

PUBLIC VOID
dmxMix8bitStereo( 
	VCHAN 		*vChanList,	/* Virtual Channel List		*/
	int			iListDepth,	/* Depth of VC List			*/
	WORD		*wMixBuf,	/* Mixing Buffer			*/
	int			iBlkSize	/* DMA Block Size			*/
	);

PUBLIC VOID
dmxMix8bitMono(
	VCHAN 		*vChanList,	/* Virtual Channel List		*/
	int			iListDepth,	/* Depth of VC List			*/
	WORD		*wMixBuf,	/* Mixing Buffer			*/
	int			iBlkSize	/* DMA Block Size			*/
	);

PUBLIC VOID
dmx16to8bit(
	VOID		*bDestPtr,	/* 8 bit buffer				*/
	VOID		*wSrcPtr,	/* 16 bit data				*/
	UINT		samples		/* Number of samples		*/
	);

#ifdef __386__
#pragma aux dmxMixer "_*" \
			parm [ESI] [EDX] [EDI] [ECX] modify [EAX EBX]

#pragma aux pdmxMixer "*" \
			parm [ESI] [EDX] [EDI] [ECX] modify [EAX EBX]

#pragma aux ( dmxMix8bitExpStereo, dmxMixer );
#pragma aux ( dmxMix8bitStereo, dmxMixer );
#pragma aux ( dmxMix8bitMono, dmxMixer );

#pragma aux dmxClipper "_*" \
			parm [EDI] [ESI] [ECX] modify [EAX EBX EDX];

#pragma aux ( dmxClipInterleave, dmxClipper );
#pragma aux ( dmxClipSeperate, dmxClipper );

#pragma aux dmx16to8bit "_*" \
			parm [EDI] [ESI] [ECX] modify [EAX];

#endif
