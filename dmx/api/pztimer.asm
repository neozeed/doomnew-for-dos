;
; Uses the 8253 timer to time the performance of code that takes
; less than about 54 milliseconds to execute, with a resolution
; of better than 10 microseconds.
;
; Externally callable routines:
;
;  ZTimerOn: Starts the Zen timer, with interrupts disabled.
;
;  ZTimerOff: Stops the Zen timer, saves the timer count,
;	times the overhead code, and restores interrupts to the
;	state they were in when ZTimerOn was called.
;
; Note: If longer than about 54 ms passes between ZTimerOn and
;	ZTimerOff calls, the timer turns over and the count is
;	inaccurate. When this happens, an error message is displayed
;	instead of a count. The long-period Zen timer should be used
;	in such cases.
;
; Note: Interrupts *MUST* be left off between calls to ZTimerOn
;	and ZTimerOff for accurate timing.
;
; All registers, and all flags except the interrupt flag, are
; preserved by all routines. Interrupts are enabled and then disabled
; by ZTimerOn, and are restored by ZTimerOff to the state they were
; in when ZTimerOn was called.
;

	.386
	.MODEL	FLAT,C
;
; Base address of the 8253 timer chip.
;
BASE_8253		equ	40h
;
; The address of the timer 0 count registers in the 8253.
;
TIMER_0_8253		equ	BASE_8253 + 0
;
; The address of the mode register in the 8253.
;
MODE_8253		equ	BASE_8253 + 3

;
; The address of Operation Command Word 3 in the 8259 Programmable
; Interrupt Controller (PIC) (write only, and writable only when
; bit 4 of the byte written to this address is 0 and bit 3 is 1).
;
OCW3			equ	20h
;
; The address of the Interrupt Request register in the 8259 PIC
; (read only, and readable only when bit 1 of OCW3 = 1 and bit 0
; of OCW3 = 0).
;
IRR			equ	20h

	.DATA
OriginalFlags		db	?	;storage for upper byte of
					; FLAGS register when
					; ZTimerOn called
TimedCount		dw	?	;timer 0 count when the timer
					; is stopped
ReferenceCount		dw	?	;number of counts required to
					; execute timer overhead code
OverflowFlag		db	?	;used to indicate whether the
					; timer overflowed during the
					; timing interval

	.CODE
	public	ZTimerOn, ZTimerOff, ZGetTime
;
; Macro to delay briefly to ensure that enough time has elapsed
; between successive I/O accesses so that the device being accessed
; can respond to both accesses even on a very fast PC.
;
DELAY	macro
	jmp	$+2
	jmp	$+2
	jmp	$+2
	endm

;********************************************************************
; ZTimerOn - Routine called to start timing.
;********************************************************************
ZTimerOn	proc	near
;
; Save the context of the program being timed.
;
	push	eax
	pushfd
	pop	eax			;get flags so we can keep
					; interrupts off when leaving
					; this routine
	mov	[OriginalFlags],ah	;remember the state of the
					; Interrupt flag
	and	ah,0fdh 		;set pushed interrupt flag
					; to 0
	push	eax
;
; Turn on interrupts, so the timer interrupt can occur if it's
; pending.
;
	sti
;
; Set timer 0 of the 8253 to mode 2 (divide-by-N), to cause
; linear counting rather than count-by-two counting. Also
; leaves the 8253 waiting for the initial timer 0 count to
; be loaded.
;
	mov	al,00110100b		;mode 2
	out	MODE_8253,al
;
; Set the timer count to 0, so we know we won't get another
; timer interrupt right away.
; Note: this introduces an inaccuracy of up to 54 ms in the system
; clock count each time it is executed.
;
	DELAY
	sub	al,al
	out	TIMER_0_8253,al		;lsb
	DELAY
	out	TIMER_0_8253,al		;msb
;
; Wait before clearing interrupts to allow the interrupt generated
; when switching from mode 3 to mode 2 to be recognized. The delay
; must be at least 210 ns long to allow time for that interrupt to
; occur. Here, 10 jumps are used for the delay to ensure that the
; delay time will be more than long enough even on a very fast PC.
;
	rept 10
	jmp	$+2
	endm
;
; Disable interrupts to get an accurate count.
;
	cli
;
; Set the timer count to 2 again to start the timing interval.
;
	mov	al,00110100b		;set up to load initial
	out	MODE_8253,al		; timer count
	DELAY
	sub	al,al
	out	TIMER_0_8253,al		;load count lsb
	DELAY
	out	TIMER_0_8253,al		;load count msb
;
; Restore the context and return.
;
	popfd				;keeps interrupts off
	pop	eax
	ret

ZTimerOn	endp

;********************************************************************
; ZTimerOff - Routine called to stop timing and get count.
;********************************************************************
ZTimerOff proc	near
;
; Save the context of the program being timed.
;
	push	eax
	push	ecx
	pushfd
;
; Latch the count.
;
	mov	al,00000000b		;latch timer 0
	out	MODE_8253,al
;
; See if the timer has overflowed by checking the 8259 for a pending
; timer interrupt.
;
	mov	al,00001010b		;OCW3, set up to read
	out	OCW3,al			; Interrupt Request register
	DELAY
	in	al,IRR			;read Interrupt Request
					; register
	and	al,1			;set AL to 1 if IRQ0 (the
					; timer interrupt) is pending
	mov	[OverflowFlag],al	;store the timer overflow
					; status
;
; Allow interrupts to happen again.
;
	sti
;
; Read out the count we latched earlier.
;
	in	al,TIMER_0_8253		;least significant byte
	DELAY
	mov	ah,al
	in	al,TIMER_0_8253		;least significant byte
	xchg	ah,al
	neg	ax			;convert from countdown
					; remaining to elapsed
					; count
	mov	[TimedCount],ax
; Time a zero-length code fragment, to get a reference for how
; much overhead this routine has. Time it 16 times and average it,
; for accuracy, rounding the result.
;
	mov	[ReferenceCount],0
	mov	cx,16
	cli				;interrupts off to allow a
					; precise reference count
RefLoop:
	call	ReferenceZTimerOn
	call	ReferenceZTimerOff
	loop	RefLoop
	sti
	add	[ReferenceCount],8	;total + (0.5 * 16)
	mov	cl,4
	shr	[ReferenceCount],cl	;(total) / 16 + 0.5
;
; Restore original interrupt state.
;
	pop	eax			;retrieve flags when called
	mov	ch,[OriginalFlags]	;get back the original upper
					; byte of the FLAGS register
	and	ch,not 0fdh		;only care about original
					; interrupt flag...
	and	ah,0fdh			;...keep all other flags in
					; their current condition
	or	ah,ch			;make flags word with original
					; interrupt flag
	push	eax			;prepare flags to be popped
;
; Restore the context of the program being timed and return to it.
;
	popfd				;restore the flags with the
					; original interrupt state
	pop	ecx
	pop	eax
	ret

ZTimerOff endp

;--------------------------------------------------------------------
; Called by ZTimerOff to start timer for overhead measurements.
;--------------------------------------------------------------------
ReferenceZTimerOn	proc	near
;
; Save the context of the program being timed.
;
	push	eax
	pushfd		;interrupts are already off
;
; Set timer 0 of the 8253 to mode 2 (divide-by-N), to cause
; linear counting rather than count-by-two counting.
;
	mov	al,00110100b	;set up to load
	out	MODE_8253,al	; initial timer count
	DELAY
;
; Set the timer count to 0.
;
	sub	al,al
	out	TIMER_0_8253,al	;load count lsb
	DELAY
	out	TIMER_0_8253,al	;load count msb
;
; Restore the context of the program being timed and return to it.
;
	popfd
	pop	eax
	ret

ReferenceZTimerOn	endp

;--------------------------------------------------------------------
; Called by ZTimerOff to stop timer and add result to ReferenceCount
; for overhead measurements.
;--------------------------------------------------------------------
ReferenceZTimerOff proc	near
;
; Save the context of the program being timed.
;
	push	eax
	push	ecx
	pushfd
;
; Latch the count and read it.
;
	mov	al,00000000b		;latch timer 0
	out	MODE_8253,al
	DELAY
	in	al,TIMER_0_8253		;lsb
	DELAY
	mov	ah,al
	in	al,TIMER_0_8253		;msb
	xchg	ah,al
	neg	ax			;convert from countdown
					; remaining to amount
					; counted down
	add	[ReferenceCount],ax
;
; Restore the context of the program being timed and return to it.
;
	popfd
	pop	ecx
	pop	eax
	ret

ReferenceZTimerOff endp

;********************************************************************
; ZGetTime - Returns time in microseconds of last timed event.
;********************************************************************
ZGetTime proc near
	push	ebx
	push	edx
	sub	eax,eax
;
; Check for timer 0 overflow.
;
	cmp	[OverflowFlag],0
	jnz	TimeOver

	mov	ax,[TimedCount]
	sub	ax,[ReferenceCount]
;
; Convert count to microseconds by multiplying by .8381.
;
;	mov	dx,8381
;	mul	dx
;	mov	bx,10000
;	div	bx		;* .8381 = * 8381 / 10000

;	mov	edx,8381
;	mul	edx
;	mov	ebx,10000
;	div	ebx		;* .8381 = * 8381 / 10000
TimeOver:
	pop	edx
	pop	ebx
	ret
ZGetTime endp


	end
