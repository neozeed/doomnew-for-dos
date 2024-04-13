/*===========================================================================
* DMXMUS.H - Music Communication Structure
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
* $Log:   F:/DMX/INC/VCS/DMXMUS.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:18:10   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _DMXMUS_H
#define _DMXMUS_H

/*
* Music Format:
*
*	|Ctrl Byte|[note|[pitch bend]]|[volume]|[time delay]
*/
/*
* Control Byte 
*/
#define CB_TIME				0x80
#define CB_STATUS			0x70
#define CB_CHANNEL			0x0F

/*
* Control Status ( from CB_STATUS mask )
*/
typedef enum
{
	CS_NOTE_OFF,
	CS_NOTE_ON,
	CS_PITCH_BEND,
	CS_COMMAND1,
	CS_COMMAND2,
	CS_END_OF_MEASURE,
	CS_END_OF_DATA
};
	
typedef enum
{
	CMD_CHANGE_PROGRAM,
	CMD_BANK_SELECT,
	CMD_MODULATION,
	CMD_VOLUME,
	CMD_PANPOT,
	CMD_EXPRESSION,
	CMD_REVERB_DEPTH,
	CMD_CHORUS_DEPTH,
	CMD_HOLD1,
	CMD_SOFT,
	CMD_ALL_SOUNDS_OFF,
	CMD_ALL_NOTES_OFF,
	CMD_MONO,
	CMD_POLY,
	CMD_RESET_CHANNEL,
	CMD_EVENT,
} CMD_TYPE;

/*
* Note Byte
*
*	 Bits
* 		7:	Velocity Follows
*	 6-0:	Note
*/
#define NB_VELOCITY			0x80
#define NB_NOTE				0x7F

/*
* Time Byte
*
*	 Bits
*		7: Extended Time Byte
*   6-0: Time Delay 0-127 256th second.
*/
#define TB_EXTENDED	0x80

typedef struct
{
	CHAR				cSignature[ 3 + 1 ]; 	/* M U S Eof */
	WORD				wMusicSize;
	WORD				wMusicOffset;
	WORD				wPrimaryChannels;
	WORD				wSecondaryChannels;
	WORD				wPatches;
	WORD				wReserved;
} DMX_HEADER;


#define MAX_CODES		384

typedef struct
{
	DMX_STATUS	( *Init )		( VOID );
	VOID		( *DeInit )		( VOID );
	VOID	 	( *NoteOff )	( WORD Channel, WORD Note );
	VOID		( *NoteOn )		( WORD Channel, WORD Note, WORD Velocity );
	VOID		( *PitchBend )	( WORD Channel, WORD Bend );
	VOID		( *Command )	( WORD Channel, WORD Cmd, WORD Data );
	INT			( *Enter )		( VOID );
	VOID		( *Leave )		( VOID );
} DMX_MUS;

#endif
