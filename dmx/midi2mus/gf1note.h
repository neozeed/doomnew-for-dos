/***************************************************************************
*	NAME:  GF1NOTE.H
**	COPYRIGHT:
**	"Copyright (c) 1991,1992, by FORTE
**
**       "This software is furnished under a license and may be used,
**       copied, or disclosed only in accordance with the terms of such
**       license and with the inclusion of the above copyright notice.
**       This software or any other copies thereof may not be provided or
**       otherwise made available to any other person. No title to and
**       ownership of the software is hereby transfered."
****************************************************************************
*  CREATION DATE: 07/01/92
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	07/01/92		Original
***************************************************************************/

#define	ADDR_HIGH(x) ((WORD)(((x)>>7)&0x1fffL))
#define	ADDR_LOW(x)  ((WORD)(((x)&0x7fL)<<9))
#define VIBRATO_RESOLUTION 187
#define SUSTAIN_POINT	   2
#define RANGE_MASTER	   255U
#define MIN_VOLUME	   1
#define MAX_VOLUME	   127
