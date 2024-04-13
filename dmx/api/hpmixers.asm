;============================================================================
; MIXERS.ASM - Realtime Digital Mixers
;----------------------------------------------------------------------------
;                CONFIDENTIAL & PROPRIETARY INFORMATION 			     
;----------------------------------------------------------------------------
;             Copyright (C) 1993 Digital Expressions, Inc. 		     
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
; $Log:   F:/DMX/API/VCS/MIXERS.ASV  $
;  
;     Rev 1.1   28 Oct 1993 06:49:28   pjr
;  Removed unused mixers (old method)
;  
;  
;     Rev 1.0   28 Oct 1993 06:46:52   pjr
;  Initial revision.
;----------------------------------------------------------------------------
	.386
	.MODEL	FLAT,C
	.DATA

; EAX EDX EBX ECX	( ORDER )
; pragma aux funcname "_*" modify [regs used not used for passing]

vchan_s	struc
vc_dLoopSamples		dd	?
vc_dWavStartPtr		dd	?
vc_dSamples		dd	?
vc_dVolPtr		dd	?
vc_dWavPtr		dd	?
vc_dMogQuotient		dd	?
vc_dMogCorRem		dd	?
vc_dPhaseShift		dd	?
vchan_s ends

clip	label byte
INCLUDE	CLIP.INC
ddActive	dd	0

ddBlockSize	dd	?
ddDmaBlockPtr	dd	?
ddChannels	dd	?
ddMixBuffer	dd	?
ddQuotient	dd	?	
ddResidue	dd	?

	 .CODE

	PUBLIC dmxMix8bitExpStereo
	PUBLIC dmxMix8bitStereo
	PUBLIC dmxMix8bitMono
	PUBLIC dmx16to8bit

;=========================================================================
; Entry: 
;	EDI	: DMA Block Pointer
;	ECX	: DMA Block Size
;	ESI	: Virtual Channel Structure(s)
;	EDX	: Number of Channels
;	EBX	; Mixing Buffer
; Uses:
;=========================================================================
dmxMix8bitExpStereo	proc
	push	ebp
	mov	ebp,esi

	inc	ddActive
	cmp	ddActive,1
	ja	ES_Silence

	xor	eax,eax
	mov	ddBlockSize,ecx

	mov	ddDmaBlockPtr,edi
	mov	ddChannels,edx
	mov	ddMixBuffer,ebx

ES_Scan:
	cmp	ds:[esi].vc_dSamples,eax
	jne	ES_Fill

	add	esi,type vchan_s
	dec	edx
	jnz	ES_Scan

ES_Silence:
	mov	eax,80808080h
	shr	ecx,1
	rep	stosd
	jmp	ES_EXIT

ES_Fill:
	mov	edi,ebx
	mov	eax,04000400h
	rep	stosd

	align	4

ES_ChkChannel:
	mov	ecx,ds:[ebp].vc_dSamples
	or	ecx,ecx
	mov	eax,ddBlockSize
	jz	ES_NoSamples

	mov	ddResidue,eax

	cmp	ecx,eax		; Samples <= BlockSize ?
	jbe	ES_SizeOk

	mov	ecx,eax		; Samples = BlockSize
ES_SizeOk:
	sub	ddResidue,ecx	
	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume TranESation Table

	sub	ds:[ebp].vc_dSamples,ecx ; Mark how much we are using.

	mov	esi,ds:[ebp].vc_dWavPtr	; Load Waveform Data
	mov	edi,ddMixBuffer

	mov	edx,ds:[ebp].vc_dMogCorRem
	mov	ebx,ds:[ebp].vc_dMogQuotient
	mov	ddQuotient,ebx
ES_Mix:
	shr	ecx,2

	push	ebp
	mov	ebp,ds:[ebp].vc_dPhaseShift

	align	4
ES_MixBlock:
REPT	4
	mov	al,[esi]	   	; 4 / 1 Get Left Sample Source 
	movsx	ebx,byte ptr [eax] 	; 4 / 1 TranESate Left Source
	add	[edi],bx	   	; 7 / 3 Mix Left Audio

	mov	al,[esi+ebp]	   	; 4 / 1 Get Right Sample Source 
	add	dh,dl			; 2 / 1 MOG 
	adc	esi,ddQuotient		; 2 / 1 Bump to next sample 
	movsx	ebx,byte ptr [eax+256]	; 4 / 1 TranESate Right Source
	add	[edi+2],bx		; 7 / 3 Mix Right Audio
	add	edi,4		   	; 2 / 1 Bump Target Index
ENDM
	dec	ecx			; 2 / 1 Adjust Remaining Count
					;-----------------------------
					; 45/17 Clocks Per Sample
	jnz	ES_MixBlock		; 7 / 3
	pop	ebp
	jmp	ES_MixDone
		
ES_MixDone:
	align	4
	mov	ds:[ebp].vc_dMogCorRem,edx

;---------------------------------------
; All Samples Done ?
;---------------------------------------
	cmp	ds:[ebp].vc_dSamples,ecx 
	jne	ES_NoSamples

;---------------------------------------
; Is the patch looping ?
;---------------------------------------
	mov	eax,ds:[ebp].vc_dLoopSamples
	or	eax,eax
	jz	ES_NoSamples

;----- Loop Patch -----
	mov	ecx,ddResidue
	mov	ds:[ebp].vc_dSamples,eax
	mov	esi,ds:[ebp].vc_dWavStartPtr
	jecxz	ES_NoSamples

	cmp	eax,ecx		; Samples <= Mix Space ?
	jae	ES_NotPartial

	mov	ecx,eax		; Samples = Mix Space

ES_NotPartial:
	sub	ddResidue,ecx
	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume TranESation Table
	sub	ds:[ebp].vc_dSamples,ecx ; Mark how much we are using.
	jmp	ES_MixBlock

ES_NoSamples:
	mov	ds:[ebp].vc_dWavPtr,esi
	add	ebp,type vchan_s
	dec	ddChannels
	jnz	ES_ChkChannel

	mov	esi,ddMixBuffer
	mov	ecx,ddBlockSize
	shl	ecx,1
	mov	edi,ddDmaBlockPtr
	xor	eax,eax

	shr	ecx,2
ES_CLIP:
REPT	4
	mov	ax,[esi]
	add	esi,2
	mov	al,clip[eax]
	mov	[edi],al
	inc	edi
ENDM
	dec	ecx
	jnz	ES_CLIP

ES_EXIT:
	dec	ddActive
	pop	ebp
	ret
	endp


;=========================================================================
; Entry: 
;	EDI	: DMA Block Pointer
;	ECX	: DMA Block Size
;	ESI	: Virtual Channel Structure(s)
;	EDX	: Number of Channels
;	EBX	; Mixing Buffer
; Uses:
;=========================================================================
dmxMix8bitStereo	proc
	pushfd	
	cli
	push	ebp
	mov	ebp,esi

	inc	ddActive
	cmp	ddActive,1
	ja	SL_Silence

	xor	eax,eax
	mov	ddBlockSize,ecx

	mov	ddDmaBlockPtr,edi
	mov	ddChannels,edx
	mov	ddMixBuffer,ebx

SL_Scan:
	cmp	ds:[esi].vc_dSamples,eax
	jne	SL_Fill

	add	esi,type vchan_s
	dec	edx
	jnz	SL_Scan

SL_Silence:
	mov	eax,80808080h
	shr	ecx,1
	rep	stosd
	jmp	SL_EXIT

SL_Fill:
	mov	edi,ebx
	mov	eax,04000400h
	rep	stosd

	align	4

SL_ChkChannel:
	mov	ecx,ds:[ebp].vc_dSamples
	or	ecx,ecx
	mov	eax,ddBlockSize
	jz	SL_NoSamples

	mov	ddResidue,eax

	cmp	ecx,eax		; Samples <= BlockSize ?
	jbe	SL_SizeOk

	mov	ecx,eax		; Samples = BlockSize

SL_SizeOk:
	sub	ddResidue,ecx	; Residue = # of Samples NOT filled in block
	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume Translation Table

	sub	ds:[ebp].vc_dSamples,ecx ; Mark how much we are using.

	mov	esi,ds:[ebp].vc_dWavPtr	; Load Waveform Data
	mov	edi,ddMixBuffer

	mov	edx,ds:[ebp].vc_dMogCorRem
	mov	ebx,ds:[ebp].vc_dMogQuotient
	mov	ddQuotient,ebx
SL_Mix:
	shr	ecx,2
	align	4
SL_MixBlock:
REPT 	4
	mov	al,[esi]	; 4 / 1 Get Sample Source 
	add	dh,dl		; 2 / 1 MOG 
	adc	esi,ddQuotient	; 2 / 1 Bump to next sample 
	movsx	ebx,byte ptr [eax] ; 4 / 1 Translate Left Source
	add	[edi],bx	; 7 / 3 Mix Left Audio
	movsx	ebx,byte ptr [eax+256]	; 4 / 1 Translate Right Source
	add	[edi+2],bx	; 7 / 3 Mix Right Audio
	add	edi,4		; 2 / 1 Bump Target Index
ENDM
	dec	ecx		; 2 / 1 Adjust Remaining Count
				;-----------------------------
				; 36/14 Clocks Per Sample
	jnz	SL_MixBlock	; 7 / 3

SL_MixDone:
	align	4
	mov	ds:[ebp].vc_dMogCorRem,edx
COMMENT $
;;---------------------------------------
;; All Samples Done ?
;;---------------------------------------
;	cmp	ds:[ebp].vc_dSamples,ecx ; vc_dSamples == 0?
;	jne	SL_NoSamples
;
;;---------------------------------------
;; Is the patch looping ?
;;---------------------------------------
;	mov	eax,ds:[ebp].vc_dLoopSamples
;	or	eax,eax
;	jz	SL_NoSamples
;
;;----- Loop Patch -----
;	mov	ecx,ddResidue
;	mov	ds:[ebp].vc_dSamples,eax
;	mov	esi,ds:[ebp].vc_dWavStartPtr
;	jecxz	SL_NoSamples
;
;	cmp	eax,ecx		; Samples <= Mix Space ?
;	jae	SL_NotPartial
;
;	mov	ecx,eax		; Samples = Mix Space
;
;SL_NotPartial:
;	sub	ddResidue,ecx
;	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume Translation Table
;	sub	ds:[ebp].vc_dSamples,ecx ; Mark how much we are using.
;	shr	ecx,2
;	jmp	SL_MixBlock
$ ; END OF COMMENT 
SL_NoSamples:
	mov	ds:[ebp].vc_dWavPtr,esi
	add	ebp,type vchan_s
	dec	ddChannels
	jnz	SL_ChkChannel

; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
; SKIP CLIPPING
; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;	jmp	short SL_EXIT

	mov	esi,ddMixBuffer
	mov	ecx,ddBlockSize
	shl	ecx,1
	mov	edi,ddDmaBlockPtr
	xor	eax,eax
	shr	ecx,2
SL_CLIP:
REPT	4
	mov	ax,[esi]
	add	esi,2
	mov	al,clip[eax]
	mov	[edi],al
	inc	edi
ENDM
	dec	ecx
	jnz	SL_CLIP

SL_EXIT:
	dec	ddActive
	pop	ebp
	popfd
	ret
	endp


;=========================================================================
; Entry: 
;	EDI	: DMA Block Pointer
;	ECX	: DMA Block Size
;	ESI	: Virtual Channel Structure(s)
;	EDX	: Number of Channels
;	EBX	; Mixing Buffer
; Uses:
;=========================================================================
dmxMix8bitMono proc
	pushfd	
	cli
	push	ebp
	mov	ebp,esi

	mov	ddBlockSize,ecx
	shr	ecx,1		; Init Mix Buffer to Silence
	inc	ddActive

	cmp	ddActive,1
	ja	ML_Silence
	

	mov	ddDmaBlockPtr,edi
	mov	ddChannels,edx
	mov	ddMixBuffer,ebx

	xor	eax,eax
ML_Scan:
	cmp	ds:[esi].vc_dSamples,eax
	jne	ML_Fill

	add	esi,type vchan_s
	dec	edx
	jnz	ML_Scan

ML_Silence:
	mov	eax,80808080h
	shr	ecx,1
	rep	stosd
	jmp	ML_EXIT

ML_Fill:
	mov	edi,ebx
	mov	eax,04000400h
	rep	stosd

	align	4

ML_ChkChannel:
	mov	ecx,ds:[ebp].vc_dSamples
	or	ecx,ecx
	mov	eax,ddBlockSize
	jz	ML_NoSamples

	mov	ddResidue,eax

	cmp	ecx,eax		; Samples <= BlockSize ?
	jbe	ML_SizeOk

	mov	ecx,ddBlockSize
ML_SizeOk:
	sub	ddResidue,ecx	

	mov	eax,ds:[ebp].vc_dVolPtr	; Load Volume Translation Table

	sub	ds:[ebp].vc_dSamples,ecx	; Mark how much we are using.

	mov	esi,ds:[ebp].vc_dWavPtr	; Load Waveform Data
	mov	edi,ddMixBuffer

	mov	edx,ds:[ebp].vc_dMogCorRem
	mov	ebx,ds:[ebp].vc_dMogQuotient
	mov	ddQuotient,ebx

	xor	ebx,ebx
	shr	ecx,2
	align	4

ML_MixBlock:
REPT 	4
	mov	al,[esi]	; 4 / 1 Get Sample Source 
	add	dh,dl		; 2 / 1 MOG 
	adc	esi,ddQuotient	; 2 / 1 Bump to next sample 
	movsx	ebx,byte ptr [eax] ; 6 Translate Source Volume
	add	[edi],bx	; 7 / 3 Mix Audio
	add	edi,2		; 2 / 1 Bump Target Index
ENDM
	dec	ecx		; 2 / 1 Adjust Remaining Count
				;-----------------------------
				; 25/14 Clocks Per Sample
	jnz	ML_MixBlock

	mov	ds:[ebp].vc_dMogCorRem,edx

;;---------------------------------------
;; All Samples Done ?
;;---------------------------------------
;	cmp	ds:[ebp].vc_dSamples,ecx
;	jne	ML_NoSamples
;
;;---------------------------------------
;; Is the patch looping ?
;;---------------------------------------
;	mov	eax,ds:[ebp].vc_dLoopSamples
;	or	eax,eax
;	jz	ML_NoSamples
;
;;----- Loop Patch -----
;	mov	ecx,ddResidue
;	mov	ds:[ebp].vc_dSamples,eax
;	mov	esi,ds:[ebp].vc_dWavStartPtr
;	jecxz	ML_NoSamples
;
;	cmp	eax,ecx		; Samples <= Mix Space ?
;	jae	ML_NotPartial
;
;	mov	ecx,eax		; Samples = Mix Space
;
;ML_NotPartial:
;	sub	ddResidue,ecx
;	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume Translation Table
;	sub	ds:[ebp].vc_dSamples,ecx ; Mark how much we are using.
;	shr	ecx,2
;	jmp	ML_MixBlock
;
;
ML_NoSamples:
	mov	ds:[ebp].vc_dWavPtr,esi
	add	ebp,type vchan_s
	dec	ddChannels
	jnz	ML_ChkChannel

; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
; SKIP CLIPPING
; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;	jmp	short ML_EXIT

	mov	esi,ddMixBuffer
	mov	ecx,ddBlockSize
	mov	edi,ddDmaBlockPtr
	xor	eax,eax

	shr	ecx,2
	align	4
ML_CLIP:
REPT	4
	mov	ax,[esi]
	add	esi,2
	mov	al,clip[eax]
	mov	[edi],al
	inc	edi
ENDM
	dec	ecx
	jnz	ML_CLIP
ML_EXIT:	
	dec	ddActive
	pop	ebp
	popfd
	ret
	endp

;=========================================================================
; Entry: 
;	ESI	: Source Waveform Data (16 bit)
;	EDI	: Target Waveform Data (8 bit)
;	ECX	: Samples
;=========================================================================
dmx16to8bit proc
	jecxz	NO_CONV
	align 	4
CONV:
	lodsw
	mov	al,ah
	stosb
	loop	CONV
NO_CONV:
	ret
	endp

;=========================================================================
	end


