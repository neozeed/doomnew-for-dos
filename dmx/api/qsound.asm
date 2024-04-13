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
	xor	eax,eax
	shr	ecx,2
CLIP_I:
REPT	4
	mov	ax,[esi]
	add	esi,2
	mov	al,clip[eax]
	mov	[edi],al
	inc	edi
ENDM
	dec	ecx
	jnz	CLIP_I
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
; for ( ;; )
; {
	mov	ecx,ds:[ebp].vc_dSamples
	or	ecx,ecx
	jnz	ES_MixSamples
; 	if ( channel->dSamples == 0 )
;	{
		add	ebp,type vchan_s
;		channel++;
		dec	ddChannels
;		if ( --ddChannels != 0 )
		jnz	ES_ChkChannel	
			; continue
;		else
		jmp	ES_Exit
			; break
;	}
ES_MixSamples:
	mov	eax,ddSamples
	mov	ddSampsToGo,eax
;	ddSampsToGo = ddSamples
;	LoopSamples = channel->dSamples
ES_LoopSamples:
	cmp	ecx,eax	
	jbe	ES_SizeOk
; 	if ( LoopSamples > ddSampsToGo )
; 	{
		mov	ecx,eax
;		LoopSamples = ddSampsToGo
; 	}
ES_SizeOk:
	sub	ddSampsToGo,ecx
;	ddSampsToGo -= LoopSamples

	mov	eax,ds:[ebp].vc_dVolPtr
;	volptr = channel->dVolPtr

	sub	ds:[ebp].vc_dSamples,ecx
;	channel->dSamples -= LoopSamples

	mov	esi,ds:[ebp].vc_dWavPtr
;	wavptr = channel->dWavPtr

	mov	edi,ddMixBuffer
;	mixptr = ddMixBuffer

	mov	edx,ds:[ebp].vc_dMogCorRem
	mov	ebx,ds:[ebp].vc_dMogQuotient
	mov	ddQuotient,ebx

	push	ebp
	cmp	ds:[ebp].vc_dQSound,0
	jz	ES_MIX
;	if ( channel->QSound ) 
;	{
		mov	ebp,ecx	

		mov	ecx,eax
		add	ecx,512			; qvolptr = volptr+512
;		do 
;		{
			align	4
		; Format: [S, SQ], [S, SQ] ...
QS_MixBlock:
			mov	al,[esi]	   	; s = *wavptr
			mov	cl,[esi+1]		; sq = *(wavptr+1)
			movsx	ebx,byte ptr [eax] 	; tb = volptr[ s ]	(left)
			add	[edi],bx	   	; *mixptr += tb
			movsx	ebx,byte ptr [ecx]	; tb = qvolptr[ sq ]	(qleft)
			add	[edi],bx	   	; *mixptr += tb

			add	dh,dl			; mog += mogidx
			mov	ebx,ddQuotient
			adc	ebx,0
			shr	ebx,1
			add	esi,ebx			; wavptr += ( Quotient + ( mog >> 8 ) );

			movsx	ebx,byte ptr [eax+256]	; tb = volptr[ s + 256 ] (right)
			add	[edi+2],bx		; *(mixptr+1) = tb
			movsx	ebx,byte ptr [ecx+256]	; tb = qvolptr[ sq ]	 (qright)
			add	[edi+2],bx		; *(mixptr+1) = tb
			add	edi,4		   	; mixptr += 2
			dec	ebp
			jnz	QS_MixBlock
; 		} while ( --LoopSamples > 0 )
;	}
;	else ( phase shift )
;	{
ES_Mix:
		mov	ebp,ds:[ebp].vc_dPhaseShift

		align	4
;		do
;		{
ES_MixBlock:
			mov	al,[esi]	   	; s = *wavptr
			movsx	ebx,byte ptr [eax] 	; tb = volptr[ s ]	(left)
			add	[edi],bx	   	; *mixptr += tb
	
			mov	al,[esi+ebp]	   	; s = *(wavptr+PhaseShift)

			add	dh,dl			; mog += mogidx
			adc	esi,ddQuotient		; wavptr += ( Quotient + ( mog >> 8 ) );

			movsx	ebx,byte ptr [eax+256]	; tb = volptr[ s + 256 ] (right)
			add	[edi+2],bx		; *(mixptr+1) = tb
			add	edi,4		   	; mixptr += 2
			dec	ecx			
			jnz	ES_MixBlock
; 		} while ( --LoopSamples > 0 )
;	}
	pop	ebp
	mov	ds:[ebp].vc_dMogCorRem,edx
;
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
		jmp	ES_LoopSamples
;		goto ES_LoopSamples
;	}

ES_NoSamples:
	mov	ds:[ebp].vc_dWavPtr,esi
	add	ebp,type vchan_s
;	channel++;
	dec	ddChannels
;	if ( --ddChannels != 0 )
	jnz	ES_ChkChannel
;		 continue
;	break
; }
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

	cmp	ecx,eax		; Samples <= ddSamples
	jbe	SL_SizeOk

	mov	ecx,eax		; Samples = ddSamples
SL_SizeOk:
	sub	ddSampsToGo,ecx
	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume Translation Table

	sub	ds:[ebp].vc_dSamples,ecx; Mark how much we are using.
	mov	esi,ds:[ebp].vc_dWavPtr	; Load Waveform Data
	mov	edi,ddMixBuffer

	mov	edx,ds:[ebp].vc_dMogCorRem
	mov	ebx,ds:[ebp].vc_dMogQuotient
	push	ebp
	mov	ebp,ebx

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
	pop	ebp

	mov	ds:[ebp].vc_dMogCorRem,edx
;---------------------------------------
; All Samples Done ?
;---------------------------------------
	cmp	ds:[ebp].vc_dSamples,ecx 
	jne	SL_NoSamples

;---------------------------------------
; Is the patch looping ?
;---------------------------------------
	mov	eax,ds:[ebp].vc_dLoopSamples
	or	eax,eax
	jz	SL_NoSamples

;----- Loop Patch -----
	mov	ecx,ddSampsToGo
	mov	ds:[ebp].vc_dSamples,eax
	mov	esi,ds:[ebp].vc_dWavStartPtr
	jecxz	SL_NoSamples

	cmp	eax,ecx		; Samples <= Mix Space ?
	jae	SL_NotPartial

	mov	ecx,eax		; Samples = Mix Space

SL_NotPartial:
	sub	ddSampsToGo,ecx
	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume Translation Table
	sub	ds:[ebp].vc_dSamples,ecx ; Mark how much we are using.
	jmp	short SL_MixBlock

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
; All Samples Done ?
;---------------------------------------
	cmp	ds:[ebp].vc_dSamples,ecx
	jne	ML_NoSamples

;---------------------------------------
; Is the patch looping ?
;---------------------------------------
	mov	eax,ds:[ebp].vc_dLoopSamples
	or	eax,eax
	jz	ML_NoSamples

;----- Loop Patch -----
	mov	ecx,ddSampsToGo
	mov	ds:[ebp].vc_dSamples,eax
	mov	esi,ds:[ebp].vc_dWavStartPtr
	jecxz	ML_NoSamples

	cmp	eax,ecx		; Samples <= Mix Space ?
	jae	ML_NotPartial

	mov	ecx,eax		; Samples = Mix Space

ML_NotPartial:
	sub	ddSampsToGo,ecx
	mov	eax,ds:[ebp].vc_dVolPtr	; Load Left Volume Translation Table
	sub	ds:[ebp].vc_dSamples,ecx ; Mark how much we are using.
	jmp	short ML_MixBlock


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


