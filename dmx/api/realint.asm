;============================================================================
; REALINT.ASM - Realmode Interrupt Handler
;----------------------------------------------------------------------------
;                CONFIDENTIAL & PROPRIETARY INFORMATION 			     
;----------------------------------------------------------------------------
;           Copyright (C) 1993,94 Digital Expressions, Inc. 		     
;                        All Rights Reserved.		   		     
;----------------------------------------------------------------------------
; "This software is furnished under a license and may be used only 	     
; in accordance with the terms of such license and with the inclusion 	     
; of the above copyright notice. This software or any other copies 	     
; thereof may not be provided or otherwise made available to any other 	     
; person. No title to and ownership of the software is hereby transfered."   
;----------------------------------------------------------------------------
;                     Written by Paul J. Radek
;----------------------------------------------------------------------------
; $Log:$
;----------------------------------------------------------------------------
	.386

_TEXT16 SEGMENT BYTE PUBLIC USE16 'CODE'
	ASSUME	cs:_TEXT16

	PUBLIC	rmInt_
	PUBLIC	rmEnd_
	PUBLIC	rmFixup_

;=========================================================================

rmInt_	proc
	jmp	short @@f
rmFlg_	db	0
@@f:
	push	ax		; Save register contents

	mov	al,0bh		; OCW3 - Read In-Service  Chip #2
	out	0A0h,al
	jmp	$+2
	in	al,0A0h
rmFixup_ label byte
	or	rmFlg_,al

	mov	al,20h
	out	0A0h,al		; EOI CHIP #2
	out	20h,al		; EOI CHIP #1

	pop	ax
	iret
rmEnd_	label	byte
rmInt_	endp
_TEXT16	ends
;=========================================================================
	end



