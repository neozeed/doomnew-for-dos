/*===========================================================================
* TYPES.H - Define Data Types 
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
* $Log:   F:/DMX/INC/VCS/TYPES.H_V  $
*  
*     Rev 1.0   02 Oct 1993 15:20:06   pjr
*  Initial revision.
*---------------------------------------------------------------------------*/
#ifndef _TYPES_H
#define _TYPES_H

#ifdef __386__
#pragma off (check_stack)
#endif

#define HINIBBLE(a)			(BYTE)((BYTE)(a)>>4)
#define LONIBBLE(a)			(BYTE)((BYTE)(a)&0x0F)
#define HIBYTE(a)  			(BYTE)((WORD)(a)>>8)
#define LOBYTE(a)  			(BYTE)((WORD)(a)&0x00FF)
#define HIWORD(a)  			(WORD)((DWORD)(a)>>16)
#define LOWORD(a)  			(WORD)((DWORD)(a)&0x0000FFFFL)

#define Addr20Bit(a)	(void*)(((DWORD)(HIWORD(a))<<4)+LOWORD(a))
#define Addr32Bit(a)	(void*)((((DWORD)(a)&0x000FFFF0L)<<12)|\
							    ((DWORD)(a)&0x0000000FL))
#ifdef __386__
void set_es(void);
#pragma aux set_es = "push ds", "pop es" 

#define FAR
#define NEAR
#define SEG_TO_FLAT_ADDR(a)	Addr20Bit(a)
#define INTERRUPT			__interrupt __far
#else
#define FAR					__far
#define NEAR				__near
#define SEG_TO_FLAT_ADDR(a)	(a)
#define INTERRUPT			__cdecl __interrupt __far
#define set_es()
#endif

#define EXTERN				extern
#define LOCAL  				static
#define PRIVATE				static
#define GLOBAL
#define PUBLIC
#define DYNAMIC				FAR __loadds 
#define TSMCALL
#define SPECIAL
#define MACRO
#define NUL                     (VOID *)0
#define EMPTY                   -1

#define FALSE					0
#define TRUE					1

typedef int						BOOL;
typedef int						BOOLEAN;

typedef unsigned char 			BYTE;
typedef unsigned short			WORD;
typedef unsigned long 			DWORD;

typedef void 					VOID;
typedef char 					CHAR;
typedef short					SHORT;
typedef unsigned short			USHORT;
typedef long 					LONG;
typedef unsigned long			ULONG;
typedef int						INT;
typedef unsigned int			UINT;

#define ASIZE(a)				(sizeof(a)/sizeof((a)[0]))

#define FDIV2(a)            ( ( a ) >> 1 )
#define FDIV4(a)            ( ( a ) >> 2 )
#define FDIV8(a)            ( ( a ) >> 3 )
#define FDIV16(a)           ( ( a ) >> 4 )
#define FDIV32(a)           ( ( a ) >> 5 )
#define FDIV64(a)           ( ( a ) >> 6 )
#define FDIV128(a)          ( ( a ) >> 7 )

#define FMUL2(a)            ( ( a ) << 1 )
#define FMUL4(a)            ( ( a ) << 2 )
#define FMUL8(a)            ( ( a ) << 3 )
#define FMUL16(a)           ( ( a ) << 4 )
#define FMUL32(a)           ( ( a ) << 5 )
#define FMUL64(a)           ( ( a ) << 6 )
#define FMUL128(a)          ( ( a ) << 7 )

int random ( int );
#define random( x ) ( rand() % x )

#ifdef __386__
UINT PUSH_DISABLE( void );
#pragma aux PUSH_DISABLE =	\
	"pushfd" 	\
	"pop	eax" \
	"cli"

void POP_DISABLE( UINT flgs );
#pragma aux POP_DISABLE = \
	"push	eax" \
	"popfd" \
	parm [eax]
#endif


#endif
