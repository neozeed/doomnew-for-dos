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
vc_dQSound		dd	?
vchan_s ends

clip	label byte
INCLUDE	CLIP.INC

ddSamples	dd	?
ddChannels	dd	?
ddMixBuffer	dd	?
ddQuotient	dd	?	
ddSampsToGo	dd	?

	 .CODE

	PUBLIC dmxClipInterleave
	PUBLIC dmxClipSeperate
	PUBLIC dmxMix8bitExpStereo
	PUBLIC dmxMix8bitStereo
	PUBLIC dmxMix8bitMono
	PUBLIC dmx16to8bit

;=========================================================================
; Entry:
;	EDI	: Destination Buffer
;	ESI	: Source Buffer
;	ECX	: Number of Samples
; Modifies:
;	EAX,ECX,ESI,EDI,FLAGS
;=========================================================================
dmxClipInterleave	proc
	push	ebx
	xor	eax,eax
	mov	ebx,eax
	shr	ecx,2
CLIP_I:
REPT	2
	mov	ax,[esi]
	mov	bx,[esi+2]
	add	esi,4
	mov	al,clip[eax]
	mov	ah,clip[ebx]
	mov	[edi],ax
	add	edi,2
ENDM
	dec	ecx
	jnz	CLIP_I
	pop	ebx
	ret
	endp

;=========================================================================
; Entry:
;	EDI	: Destination Buffer
;	ESI	: Source Buffer
;	ECX	: Number of Samples
; Modifies:
;	EAX,EBX,ECX,EDX,ESI,EDI,FLAGS
;=========================================================================
dmxClipSeperate	proc
	xor	eax,eax
	shr	ecx,3
	mov	edx,ecx
	mov	ebx,esi
CLIP_S:
REPT	4
	mov	ax,[esi]
	add	esi,4
	mov	al,clip[eax]
	mov	[edi],al
	inc	edi
ENDM
	dec	ecx
	jnz	CLIP_S

	cmp	edx,ecx
	je	CLIP_EXIT
	
	xchg	ecx,edx
	mov	esi,ebx
	add	esi,2
	jmp	short CLIP_S

CLIP_EXIT:
	ret
	endp

;=========================================================================
; Entry: 
;	ESI	: Virtual Channel Structure(s)
;	EDX	: Number of Channels
;	EDI	; Mixing Buffer
;	ECX	: DMA Block Size
; Modifies:
;	EAX,EBX,ECX,EDX,ESI,EDI
;=========================================================================
dmxMix8bitExpStereo	proc
	push	ebp
	mov	ebp,esi
;
; Save for later use
;
	mov	ddChannels,edx
	mov	ddMixBuffer,edi
	mov	ddSamples,ecx
;
; Initialize mix buffer to silence
;
	mov	eax,04000400h
	rep	stosd

	align	4
ES_ChkChannel:
;
; If dSamples != 0 then mix channel into mix buffer
;
	mov	ecx,ds:[ebp].vc_dSamples
	or	ecx,ecx
	jnz	ES_MixSamples

	add	ebp,type vchan_s
	dec	ddChannels
	jnz	ES_ChkChannel
	jmp	ES_Exit

ES_MixSamples:
	mov	eax,ddSamples
	mov	ddSampsToGo,eax

ES_LoopPatch:
	cmp	ecx,eax		; Samples <= ddSamples
	jbe	ES_SizeOk

	mov	ecx,eax		; Samples = ddSamples
ES_SizeOk:
	sub	ddSampsToGo,ecx	
	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume TranESation Table

	sub	ds:[ebp].vc_dSamples,ecx ; Mark how much we are using.

	mov	esi,ds:[ebp].vc_dWavPtr	; Load Waveform Data
	mov	edi,ddMixBuffer

	mov	edx,ds:[ebp].vc_dMogCorRem
	mov	ebx,ds:[ebp].vc_dMogQuotient
	mov	ddQuotient,ebx
ES_Mix:
	push	ebp
	mov	ebp,ds:[ebp].vc_dPhaseShift

	align	4
ES_MixBlock:
	mov	al,[esi]	   	; 4 / 1 Get Left Sample Source 
	movsx	ebx,byte ptr [eax] 	; 4 / 1 TranESate Left Source
	add	[edi],bx	   	; 7 / 3 Mix Left Audio

	mov	al,[esi+ebp]	   	; 4 / 1 Get Right Sample Source 
	add	dh,dl			; 2 / 1 MOG 
	adc	esi,ddQuotient		; 2 / 1 Bump to next sample 
	movsx	ebx,byte ptr [eax+256]	; 4 / 1 TranESate Right Source
	add	[edi+2],bx		; 7 / 3 Mix Right Audio
	add	edi,4		   	; 2 / 1 Bump Target Index
	dec	ecx			; 2 / 1 Adjust Remaining Count
					;-----------------------------
					; 45/17 Clocks Per Sample
	jnz	ES_MixBlock		; 7 / 3

	pop	ebp
	mov	ds:[ebp].vc_dMogCorRem,edx
;---------------------------------------
	cmp	ddSampsToGo,0
	jz	ES_NoSamples
	mov	ecx,ds:[ebp].vc_dLoopSamples
	or	ecx,ecx
	jz	ES_NoSamples
; 	if ( ddSampsToGo && channel->dLoopSamples )
;	{ 
;----- Loop Patch -----
		mov	ds:[ebp].vc_dSamples,ecx
		mov	esi,ds:[ebp].vc_dWavStartPtr
		mov	ds:[ebp].vc_dWavPtr,esi
		
		mov	eax,ddSampsToGo
		jmp	ES_LoopPatch
;		goto ES_LoopSamples
;	}

ES_NoSamples:
	mov	ds:[ebp].vc_dWavPtr,esi
	add	ebp,type vchan_s
	dec	ddChannels
	jnz	ES_ChkChannel

ES_Exit:
	pop	ebp
	ret
	endp


;=========================================================================
; Entry: 
;	ESI	: Virtual Channel Structure(s)
;	EDX	: Number of Channels
;	EDI	; Mixing Buffer
;	ECX	: DMA Block Size
; Modifies:
;	EAX,EBX,ECX,EDX,ESI,EDI
;=========================================================================
dmxMix8bitStereo	proc
	push	ebp
	mov	ebp,esi
;
; Save for later use
;
	mov	ddChannels,edx
	mov	ddMixBuffer,edi
	mov	ddSamples,ecx
;
; Initialize mix buffer to silence
;
	mov	eax,04000400h
	rep	stosd

	align	4
SL_ChkChannel:
;
; If dSamples != 0 then mix channel into mix buffer
;
	mov	ecx,ds:[ebp].vc_dSamples
	or	ecx,ecx
	jnz	SL_MixChannel

	add	ebp,type vchan_s
	dec	ddChannels
	jnz	SL_ChkChannel
	jmp	SL_Exit

SL_MixChannel:
	mov	eax,ddSamples
	mov	ddSampsToGo,eax
SL_LoopPatch:
	cmp	ecx,eax		; Samples <= ddSamples
	jbe	SL_SizeOk
; LoopSamples = channel->dSamples
; if ( (ecx)LoopSamples > (eax)ddSampsToGo )
; {
		mov	ecx,eax		
; 		LoopSamples = ddSampsToGo
; }
SL_SizeOk:
	sub	ddSampsToGo,ecx
;	ddSampesToGo -= LoopSamples

	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume Translation Table
	sub	ds:[ebp].vc_dSamples,ecx; Mark how much we are using.
	mov	esi,ds:[ebp].vc_dWavPtr	; Load Waveform Data
	mov	edi,ddMixBuffer

	mov	edx,ds:[ebp].vc_dMogCorRem
	mov	ebx,ds:[ebp].vc_dMogQuotient
	push	ebp
	mov	ebp,ebx
;	do 
;	{
		align	4
SL_MixBlock:
		mov	al,[esi]	; 4 / 1 Get Sample Source 
		add	dh,dl		; 2 / 1 MOG 
		adc	esi,ebp		; 2 / 1 Bump to next sample 
		movsx	ebx,byte ptr [eax] ; 4 / 1 Translate Left Source
		add	[edi],bx	; 7 / 3 Mix Left Audio
		movsx	ebx,byte ptr [eax+256]	; 4 / 1 Translate Right Source
		add	[edi+2],bx	; 7 / 3 Mix Right Audio
		add	edi,4		; 2 / 1 Bump Target Index
		dec	ecx		; 2 / 1 Adjust Remaining Count
					;-----------------------------
					; 36/14 Clocks Per Sample
		jnz	SL_MixBlock	; 7 / 3
;	} while ( --LoopSamples )

	pop	ebp

	mov	ds:[ebp].vc_dMogCorRem,edx
;---------------------------------------
	cmp	ddSampsToGo,0
	jz	SL_NoSamples
	mov	ecx,ds:[ebp].vc_dLoopSamples
	or	ecx,ecx
	jz	SL_NoSamples
; 	if ( ddSampsToGo && channel->dLoopSamples )
;	{ 
;----- Loop Patch -----
		mov	ds:[ebp].vc_dSamples,ecx
		mov	esi,ds:[ebp].vc_dWavStartPtr
		mov	ds:[ebp].vc_dWavPtr,esi
		
		mov	eax,ddSampsToGo
		jmp	SL_LoopPatch
;		goto ES_LoopSamples
;	}

SL_NoSamples:
	mov	ds:[ebp].vc_dWavPtr,esi
	add	ebp,type vchan_s
	dec	ddChannels
	jnz	SL_ChkChannel

SL_Exit:
	pop	ebp
	ret
	endp


;=========================================================================
; Entry: 
;	ESI	: Virtual Channel Structure(s)
;	EDX	: Number of Channels
;	EDI	; Mixing Buffer
;	ECX	: DMA Block Size
; Modifies:
;	EAX,EBX,ECX,EDX,ESI,EDI
;=========================================================================
dmxMix8bitMono proc
	push	ebp
	mov	ebp,esi
;
; Save for later use
;
	mov	ddChannels,edx
	mov	ddMixBuffer,edi
	mov	ddSamples,ecx
;
; Initialize mix buffer to silence
;
	shr	ecx,1		; Init Mix Buffer to Silence
	mov	eax,04000400h
	rep	stosd

	align	4
ML_ChkChannel:
;
; If dSamples != 0 then mix channel into mix buffer
;
	mov	ecx,ds:[ebp].vc_dSamples
	or	ecx,ecx
	jnz	ML_MixChannel
	
	add	ebp,type vchan_s
	dec	ddChannels
	jnz	ML_ChkChannel
	jmp	ML_Exit

ML_MixChannel:
	mov	eax,ddSamples
	mov	ddSampsToGo,eax
ML_LoopPatch:
	cmp	ecx,eax		; Samples <= ddSamples ?
	jbe	ML_SizeOk

	mov	ecx,ddSamples
ML_SizeOk:
	sub	ddSampsToGo,ecx	

	mov	eax,ds:[ebp].vc_dVolPtr	; Load Volume Translation Table
	sub	ds:[ebp].vc_dSamples,ecx; Mark how much we are using.
	mov	esi,ds:[ebp].vc_dWavPtr	; Load Waveform Data
	mov	edi,ddMixBuffer

	mov	edx,ds:[ebp].vc_dMogCorRem
	mov	ebx,ds:[ebp].vc_dMogQuotient
	push	ebp
	mov	ebp,ebx

	align	4
ML_MixBlock:
	mov	al,[esi]	; 4 / 1 Get Sample Source 
	add	dh,dl		; 2 / 1 MOG 
	adc	esi,ebp		; 2 / 1 Bump to next sample 
	movsx	ebx,byte ptr [eax] ; 6 Translate Source Volume
	add	[edi],bx	; 7 / 3 Mix Audio
	add	edi,2		; 2 / 1 Bump Target Index
	dec	ecx		; 2 / 1 Adjust Remaining Count
				;-----------------------------
				; 25/14 Clocks Per Sample
	jnz	ML_MixBlock
	pop	ebp

	mov	ds:[ebp].vc_dMogCorRem,edx
;---------------------------------------
	cmp	ddSampsToGo,0
	jz	ML_NoSamples
	mov	ecx,ds:[ebp].vc_dLoopSamples
	or	ecx,ecx
	jz	ML_NoSamples
; 	if ( ddSampsToGo && channel->dLoopSamples )
;	{ 
;----- Loop Patch -----
		mov	ds:[ebp].vc_dSamples,ecx
		mov	esi,ds:[ebp].vc_dWavStartPtr
		mov	ds:[ebp].vc_dWavPtr,esi
		
		mov	eax,ddSampsToGo
		jmp	ML_LoopPatch
;		goto ES_LoopSamples
;	}

ML_NoSamples:
	mov	ds:[ebp].vc_dWavPtr,esi
	add	ebp,type vchan_s
	dec	ddChannels
	jnz	ML_ChkChannel

ML_Exit:
	pop	ebp
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

