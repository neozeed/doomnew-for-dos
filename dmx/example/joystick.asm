	.386
	.MODEL	FLAT,C
	.DATA
joyX1		dd	0
joyY1		dd	0
joyX2		dd	0
joyY2		dd	0
joyButtons	dd	0

	.CODE

	PUBLIC	ReadJoystick
	PUBLIC	joyX1
	PUBLIC	joyY1
	PUBLIC	joyX2
	PUBLIC	joyY2
	PUBLIC	joyButtons

; Uses EAX, EDI, EBX, ECX, EDX
ReadJoystick	proc
	xor	eax,eax
	mov	ebx,eax
	mov	joyX1,eax
	mov	joyY1,eax
	mov	joyX2,eax
	mov	joyY2,eax

	mov	ecx,3FFh
	mov	edx,201h
	pushfd

	cli
	out	dx,al
	in	al,dx

	not	al
	shr	eax,4
	mov	joyButtons,eax

LR_1:
	in	al,dx
	and	eax,edi
	jz	LR_3

	rcr	al,1
	adc	joyX1,ebx
	rcr	al,1
	adc	joyY1,ebx
	rcr	al,1
	adc	joyX2,ebx
	rcr	al,1
	adc	joyY2,ebx
	loop	LR_1
LR_2:
	mov	eax,-1
LR_3:
	popfd
	inc	eax
	ret
ReadJoystick	endp
	end
