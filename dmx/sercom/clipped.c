

/* FROM ASIFIRST */
/*
    p->break_delay = as_brkdly[ port ];
    p->aswmodem = as_wmodem[ port ];
    p->aswtime = as_wtime[ port ];
    p->asrtime = as_rtime[ port ];
*/
/*
*  Transfer shared-hardware i/o addr and bitmask to
*  the array of structures TABLEPORT                  
*                                                      
*  If the port is NOT on a shared port, then the bitmask
*  is set with high order 8 bits ones to cause         
*  a non-recognition to occur in the interrupt.        
*/
/*
    gChnl[ port ].as_shbits = as_shbmask[ port ];
    gChnl[ port ].as_shport = as_shioad[ port ];
    if ( !as_shioad[ port ] )
        as_shbmask[ port ] |= 0xff00;
*/

/*
; ISR.ASM        4.00A  November 21, 1992
;
; Contains:     pcdigi
;               psdigi
;               UseInterruptCounters
;               _asigetc()
;               _asiputc()
;               _asipekc()
;            	   _asiprime()
;
; This module contains the ISR for the GSCI driver, as well as a considerable
; amount of supporting code.  It grew monolithic and sloppy over the years,
; although some trimming has taken place recently.
;

; April 24, 1992  3.20F : Over the last year or so, we began noticing a
;       problem that only shows up on clone/oddball UART chips running
;       on fast 386 and up machines.  With that particular hardware
;       setup, if your system was receiving multiple interrupt types
;       simultaneously, we would sometimes see the transmit interrupts
;       die.  In other words, the transmit register was empty, but no
;       interrupt would be generated, so the buffer would fill up, and
;       no more characters would be sent.  Apparently these chips
;       have problems when receving multiple interrupts simultaneously.
;       The modem status interrupt seems to be the one that causes the
;       trouble, although I have not proven this to my satisfaction.
;       The solution is to check for the THRE bit being set even when
;       the TX interrupt has not been generated.  If the THRE bit is set,
;       we go ahead and transmit a character.  Note that this problem
;       has *never* occurred on a legitimate NSC 8250/16450/16550, so
;       it may truly be a hardware problem.  The software fix works
;       perfectly, however.  If you don't want to use the fix, you
;       can turn it off by defining GF_RISKY in this file.
;
;       UARTS known to have this problem:
;
;               WINBOND W86C450, we don't have these here.
;               WINBOND W86C451, ditto
;               WINBOND W86C453, ditto
;               CIC83747AZ, we have these on hand
;               UM82C452, we have these on hand
;
;
;GF_RISKY=1     ;Turn this on to bypass the fix
;*/
        INCLUDE MODEL.EQU
        INCLUDE _GSCI.EQU

        .DATA

        PUBLIC  injmptl

;
; Interrupt dispatching for the GSCI is now done via the two routines
; found in GSCIDISP.ASM, _GSCIIsaDispatcher and _GSCIMcaDispatcher.
; These routines make psdigi and pcdigi obsolete, as well as all the
; helper routines found here.  Multiport boards have their own dedicated
; dispatchers.  The old stuff will still work if needed, and will
; work with people's 3.2 programs.  The code is all slated for removal
; in 4.x, however.  Because it is obsolete, I am not going to add
; any documentation for this release.

;*******************START OF OBSOLETE FUNCTION BLOCK************************
        IF      KEEP_OBSOLETE_FUNCTIONS
        PUBLIC  inttbl
        PUBLIC  pcdigi
        PUBLIC  psdigi
        PUBLIC  ihdptr
        PUBLIC  serv0

ihdptr  DW      OFFSET pcdigi


;==>--  This table contains the starting addresses of all interrupt
;       service routines.
;
MAKEOFFSET  MACRO NUMBER
            DW OFFSET serv&NUMBER
            ENDM

inttbl     LABEL WORD

PortNumber = 0
        REPT    MAX_PORT
        MAKEOFFSET %PortNumber
PortNumber = PortNumber+1
        ENDM
        ENDIF
;*******************END OF OBSOLETE FUNCTION BLOCK************************

; The really important code in this module consists of the four
; ISR handlers for each of the four different types of 8250 interrupts.
; The various dispatchers all scan the 8250 IIR to see what sort of
; interrupt is in progress, then call one of the four routines based
; on the pointer found in this little table.  That makes this little
; table *very* important, as well as the four routines it points to.

;==>--  Table for interrupt service routines
;
injmptl DD      asicms   ; case 0 modem status
        DD      asictx   ; case 2 transmit register empty
        DD      asicrx   ; case 4 receive character ready
        DD      asicrs   ; case 6 special condition (line status)


        .CODE


;
;  void UseInterruptCounters( flag )
;
;  ARGUMENTS
;
;   int flag :  This flag determines whether the GSCI interrupt counters
;               get turned on or not.
;
;  DESCRIPTION
;
;   Each of the four GSCI interrupt types can increment a counter in the
;   PORT_TABLE structure.  When debugging this can be a very useful
;   thing, particularly when combined with DumpPortStatus() code.
;   See DOSTERM.C for an example.  Turning on the counters is done by
;   redirecting the four interrupt pointers in ihdptr[], to point to
;   an earlier instruction that increments the counter.  This makes it
;   a 0 cost item if it isn't used.
;
;  SIDE EFFECTS
;
;  The interrupt handler pointer is redirected.
;
;  RETURNS
;
;  Nothing.
;
;  AUTHOR
;
;   SM          November 21, 1992
;
;  MODIFICATIONS
;
;  November 21, 1992     4.00A : Initial release
;

ifndef  VGFD

                        PUBLIC  UseInterruptCounters
UseInterruptCounters    PROC    USES DS,useicnt:WORD

else    ;VGFD
                        PUBLIC  UseInterruptCounters1
UseInterruptCounters1   PROC    USES DS,useicnt:WORD

endif   ;VGFD

                        IF      @DataSize EQ 2
                        assume  ds:@data
                        mov     dx,@data
                        mov     ds,dx
                        ENDIF
                        mov     ax,useicnt
                        cmp     ax,0
                        je      unload

load:                   mov     word ptr injmptl, offset asicms_cnt
                        mov     word ptr injmptl + 2, seg asicms_cnt
                        mov     word ptr injmptl + 4, offset asictx_cnt
                        mov     word ptr injmptl + 6, seg asictx_cnt
                        mov     word ptr injmptl + 8, offset asicrx_cnt
                        mov     word ptr injmptl + 10, seg asicrx_cnt
                        mov     word ptr injmptl + 12, offset asicrs_cnt
                        mov     word ptr injmptl + 14, seg asicrs_cnt
                        jmp     short done

unload:                 mov     word ptr injmptl, offset asicms
                        mov     word ptr injmptl + 2, seg asicms
                        mov     word ptr injmptl + 4, offset asictx
                        mov     word ptr injmptl + 6, seg asictx
                        mov     word ptr injmptl + 8, offset asicrx
                        mov     word ptr injmptl + 10, seg asicrx
                        mov     word ptr injmptl + 12, offset asicrs
                        mov     word ptr injmptl + 14, seg asicrs
                        jmp     short done

done:                   ret
                        ASSUME  DS:NOTHING

ifndef  VGFD
UseInterruptCounters    ENDP
else    ;VGFD
UseInterruptCounters1   ENDP
endif   ;VGFD

;;;endif   ;VGFD


;
;  asicms( PORT_TABLLE *p )
;
;  ARGUMENTS
;
;   p  :  A pointer to the GSCI port structure.  This is passed in ds:si.
;
;  DESCRIPTION
;
;   This is the ISR function that handles modem status interrupts.  It
;   has to update bits in the port structure, and then manage handshaking.
;   Note that like the other three ISR handlers, this routine has an
;   alternate entry point that increments the modem status interrupt counter
;   in the port structure.  This routine is called by any one of the
;   GSCI front end handlers in *DISP.ASM.
;
;  SIDE EFFECTS
;
;  Lots.
;
;  RETURNS
;
;  Nothing.
;
;  AUTHOR
;
;   SM          November 21, 1992
;
;  MODIFICATIONS
;
;  November 21, 1992     4.00A : Initial release
;

asicms_cnt:
        inc     [si+mscount]
asicms: mov     cx,dx                   ;save 8250 base in cx for speed

;ifndef  VGFD

        add     dx,MSREG                ;dx = Modem status register
        in      al,dx                   ;read the reason we are here
        KILL_TIME

        test    al,TERI                 ;see if this is TERI
        jz      skip_x                  ;jump if not
        or      al,RINGIN               ;else set the ring indicator bit
skip_x:
        mov     dx,cx                   ;dx back to 8250 base
        mov     ah,al
        not     ah
        add     dx,INTER                ;dx=interrupt enable register
        or      [si+modem_stat],ax      ;update static status byte
        not     ah
        mov     bx,[si+wide_stat]       ;save status bits for wide track
        and     bl,0fh                  ;mask off bits 4-7
        and     ah,0f0h
        or      bl,ah
        mov     ah,al                   ;refresh ah with 8250 status
        mov     [si+wide_stat],bx
        or      [si+chst_bits],MODCHG   ;say we had a modem status change
        mov     bp,[si+chmode_bits]     ;let bp hold these for speed
        test    bp,IS_IGMSTAT+IS_IGALERT+IS_CTSMODE  ;**2.10 eliminate
                                        ;time-consuming test/jump sequences
;;      jz      mstout
        jnz     mst001
        jmp     mstout
mst001: test    bp,IS_IGMSTAT           ;see if we are ignoring modem
        jz      mstiga                  ;status changes?
        or      [si+chst_bits],ALERT    ;set alert bit if not
mstiga: test    bp,IS_IGALERT           ;are we ignoring alert conditions?
        jz      mstcts                  ;if so check for cts
        in      al,dx                   ;come here to stop tx & rx interrupts
        KILL_TIME
        and    [si+chst_bits],not TXIFLAG
        and    al,not ENRX
        out    dx,al
        KILL_TIME
        and     [si+chst_bits],not XCHRUN+RCHRUN  ;say interrupts both stopped
        or      [si+chst_bits],TXWALERT ;and we are waiting for alert
        jmp     short mstout
mstcts: test    bp,IS_CTSMODE           ;see if CTS flow control enabled?
        jz      mstout
        test    bp,IS_TXINT             ;see if transmit interrupts
        jz      mstout                  ;are enabled?
        test    ah,DCTS                 ;did CTS change?
        jz      mstout                  ;if so disable or enable tx interrupts
        test    ah,CTS                  ;did it just go true
        jz      stoptx                  ;if not stop xmit ints
        mov     dx,LSREG        ; now wait for tx holding to empty
        add     dx,cx
wloop3: in      al,dx
        KILL_TIME
        and     al,THRE         ; is tx holding empty?
        jz      wloop3
        RESTARTTX
        and     [si+chst_bits],not TXWCTS ;and not waiting for CTS
        jmp     short mstout                  ;leave modem status change
stoptx: in      al,dx                   ;come here to stop tx-interrupts
        KILL_TIME
        and    [si+chst_bits],not TXIFLAG
        and     [si+chst_bits],not XCHRUN       ;say tx interrupts stopped
        or      [si+chst_bits],TXWCTS           ;and we are waiting for CTS

;endif   ;VGFD

mstout: retf

;
;  asictx( PORT_TABLLE *p )
;
;  ARGUMENTS
;
;   p  :  A pointer to the GSCI port structure.  This is passed in ds:si.
;
;  DESCRIPTION
;
;   This is the ISR function that handles transmit interrupts.  It
;   has an easier job than most of the other interrupt handlers.  It
;   just has to stuff a character out the TX buffer if it seems
;   appropriate.
;
;  SIDE EFFECTS
;
;  Lots.
;
;  RETURNS
;
;  Nothing.
;
;  AUTHOR
;
;   SM          November 22, 1992
;
;  MODIFICATIONS
;
;
;   April 24, 1992  3.20F : I sometimes now call this routine without
;       knowing in advance that the transmitter is ready for another
;       character.  In order to accomodate that, I test for THRE upon
;       the entrance, and exit if I don't find it.
;
;  November 22, 1992     4.00A : Initial release
;

asictx_cnt:
        inc     [si+txcount]
asictx:

;ifndef  VGFD

        test   [si+chst_bits],TXIFLAG
        jz     stopxm

        add     dx,LSREG                ;Test for THRE, exit if it is not set
        in      al,dx
        KILL_TIME
        sub     dx,LSREG
        test    al,THRE
        jnz     asictx1
        retf
asictx1:
         mov    cx,[si+tx_16550_limit]          ; can transmit up to 16 bytes
txloop: test    [si+chst_bits],TXEMPTY          ;see if TX buffer empty
        jnz     stopxm                          ;if transmit is empty stop it
        test    [si+chst_bits],TXWXON           ;tx waiting for xon?
        jnz     txout
        lea     di,[si+tx_cell]                 ;else go get next character
        push    cx
        cli                                     ;3.11
        call    _remove_char                          ;to al
        pop     cx
        jnz     txnomt                          ;if not empty take jump
        or      [si+chst_bits],TXEMPTY          ;say it is now empty
txnomt: and     [si+chst_bits],not TXFULL
        sti                                     ;3.11
        mov     dx,[si+base_8250]               ;dx=8250 base address
        out     dx,al                           ;send character
        KILL_TIME
        jmp     short txout
stopxm:
        and    [si+chst_bits],not XCHRUN+TXIFLAG ;say not running any more
        retf

txout:  test   [si+chmode_bits], IS_16550  ; Don't loop if not a 16550
        jz     txdone
;
; If we have are using 16550 FIFOS and RX XON/XOFF handshaking,
; I don't fill the FIFO.  Handshaking gets fouled up if it has to wait
; forever to stuff the XOFF out to the UART THRE.
;
        test   [si+chmode_bits], IS_RX_XOFFMODE ; Don't loop if handshaking
        jnz    txdone
        loop   txloop

;endif   ;VGFD

txdone: retf

;
;  asicrx( PORT_TABLLE *p )
;
;  ARGUMENTS
;
;   p  :  A pointer to the GSCI port structure.  This is passed in ds:si.
;
;  DESCRIPTION
;
;   This is the ISR function that handles receiver interrupts.  It
;   has gotten complicated and ugly for a couple of reasons.  First, it
;   has to be able to handle all of the checking and updating needed
;   to handle hardware and software handshaking.  Second, it needs to
;   handle the asirchk() stuff, meaning it has to scan incoming characters
;   for matches.
;
;  SIDE EFFECTS
;
;  Lots.
;
;  RETURNS
;
;  Nothing.
;
;  AUTHOR
;
;   SM          November 22, 1992
;
;  MODIFICATIONS
;
;
;  November 22, 1992     4.00A : Initial release
;

asicrx_cnt:
        inc     [si+rxcount]
asicrx: mov     cx,dx                           ;save 8250 base in cx

;ifndef  VGFD

        in      al,dx                           ;clear condition
        KILL_TIME
        inc     [si+rx_accum]                   ;increment accumulator
        mov     bp,[si+chmode_bits]             ;Get the mode bits in bp for speed
        test    [si+chst_bits],RXFULL           ;see if rx buffer is full
        jz      rx0                             ;if not full take jump
        or      [si+chst_bits],RXOVFLOW         ;else say overflow
        jmp     rxrts
rx0:    test    bp,IS_IGCD+IS_IGDSR+IS_IGALERT+IS_TX_XOFFMODE+IS_RCHKING ;**2.10
;;;     jz      rx5                             ;**2.10 skip numerous test
        jnz     rx0a                            ;jump sequences if features
        jmp     rx5                             ;are not used

rx0a:   test    bp,IS_IGCD                      ;are we ignoring CD
        jz      rx1                             ;if so take jump
        test    [si+wide_stat],RLSD             ;see if it is active
        jnz     rx1
        jmp     rxout
;;;     jz      rxout                           ;if not get out
rx1:    test    bp,IS_IGDSR                     ;are we ignoring DSR?
        jz      rx2                             ;if so don't test it
        test    [si+wide_stat],DSR              ;is it active
        jnz     rx2
        jmp     rxout
;;;     jz      rxout                           ;if not get out
rx2:    test    bp,IS_IGALERT                   ;are we ignoring alert?
        jz      rx3                             ;if so get out
        test    [si+chst_bits],ALERT            ;see if alert set
        jz      rx3
        jmp     rxout
;;;     jnz     rxout                           ;if so get out
rx3:    test    bp,IS_TX_XOFFMODE               ;see if transmitter in xon/xoff mode
        jz      rx4                             ;if not skip around
        add     dx,INTER                        ;dx=interrupt enable register
        cmp     al,byte ptr[si+stop_xmt]        ;is this the stop character
        jnz     rx3a                            ;if not check for on character
        and    [si+chst_bits],not TXIFLAG
        or      [si+chst_bits],TXWXON           ;set status bit
        and     [si+chst_bits],not XCHRUN       ;say transmit not running
        jmp     rxout                           ;get out
rx3a:   cmp     al,byte ptr[si+start_xmt]       ;is this the start character
        jnz     rx4                             ;if not take jump
        test    bp,IS_TXINT                     ;should tx interrupts be on
        jz      rx4                             ;if not don't re-enable them
        mov     dx,LSREG        ; now wait for tx holding to empty
        add     dx,cx
wloop5: in      al,dx
        KILL_TIME
        and     al,THRE         ; is tx holding empty?
        jz      wloop5
        RESTARTTX
        and     [si+chst_bits],not TXWXON       ;reset status bit
        jmp     rxout                           ;get out
rx4:    test    bp,IS_RCHKING                   ;are we rchking
        jz      rx5                             ;if not take quick way out
        lea     di,[si+chkchr_1]                ;start with #1
        mov     cx,3                            ;and check 3 characters
rchlop: mov     bx,[di]                         ;read value from structure
        test    bx,RCHKENABLED                  ;is this position checking
        jz      rchnom                          ;if not do next
        cmp     al,bl                           ;do characters match
        jnz     rchnom
        and     bh,high RCHKVAL                 ;mask out all but mode
        jnz     rx4a
        jmp     rxout
;;;     jz      rxout                           ;if discarding just get out
rx4a:   or      [di],RCHKFLAG                   ;1&2 both set flags so set now
        dec     bh
        jnz     rx5
        jmp     rxout
;;;     jz      rxout                           ;if 1 discard
;;;     jmp     rx5
rchnom: add     di,2                            ;check next character
        loop    rchlop                          ;till all 3 checked
rx5:    lea     di,[si+rx_cell]                 ;put character in queue
        cli                                     ;3.11
        and     [si+chst_bits],not RXEMPTY      ;say rx not empty
        mov     ah,byte ptr[si+wide_stat]
        and     byte ptr[si+wide_stat],0f0h     ;**2.04
        call    _insert_char
        jnz     rnfull                          ;if not full
        or      [si+chst_bits],RXFULL           ;else say full
rnfull: sti                                     ;3.11
        test    bp,IS_RX_XOFFMODE+IS_RTSCONTROL ;**2.10 eliminate test/jumps
;;;     jz      rxout                           ;**2.10 and get out if
        jnz     cxorts                          ;possible
        jmp     short rxout
cxorts: test    bp,IS_RX_XOFFMODE                ;Receiver in xon/xoff mode?
        jz      rxrts                           ;if not skip this
        test    [si+chst_bits],XOFFSENT         ;has it already been sent?
        jnz     rxrts
        mov     ax,[si+rx_count]                ;get rx character count
        cmp     ax,[si+rx_hiwater]              ;see if below high water mark
        jnb     rxsxof
        jmp     short rxout                           ;if so don't send xoff
rxsxof: mov     cx,[si+base_8250]
        mov     dx,LSREG
        add     dx,cx
        or      [si+chst_bits],XOFFSENT         ;say XOFF sent
xoflop: in      al,dx                           ;wait till transmitter empty
        KILL_TIME
        and     al,THRE
        jz      xoflop
        mov     dx,cx
        mov     ax,[si+stop_rem_xmt]            ;get character to send
        out     dx,al                           ;and send it out
        KILL_TIME
rxrts:  test    bp,IS_RTSCONTROL                ;are we controlling RTS?
        jz      rxout
        test    [si+chst_bits],RTSACTIVE        ;is RTS asserted?
        jz      rxout                           ;if not then nothing to do
        mov     cx,[si+rx_count]                ;get rx character count
        cmp     cx,[si+rts_hiwater]
        jb      rxout                           ;if rx_count<rts_hiwater
        and     [si+chst_bits],not RTSACTIVE    ;say RTS de-asserted
        mov     dx,[si+base_8250]               ;and de-assert RTS
        add     dx,MCREG
        in      al,dx
        KILL_TIME
        and     al,not RTS
        out     dx,al
        KILL_TIME
rxout:  test    bp, IS_16550         ;If not a 16550, no more chars
        jz      rxdone
        mov     cx,[si+base_8250]
        mov     dx,LSREG
        add     dx,cx
        in      al,dx
        KILL_TIME
        mov      dx,cx
        test     al,1                ;See if any data is ready
        jz       rxdone
        jmp      asicrx
rxdone:

;endif   ;VGFD

        retf

;
;  asicrs( PORT_TABLLE *p )
;
;  ARGUMENTS
;
;   p  :  A pointer to the GSCI port structure.  This is passed in ds:si.
;
;  DESCRIPTION
;
;   This is the ISR function that handles line status interrupts.  It
;   is pretty simple, since the only thing it has to do is update some
;   bits in the GSCI port structure.  If alert flag checking is enabled,
;   it might also have to disable RX and TX interrupts.
;
;  SIDE EFFECTS
;
;  Lots.
;
;  RETURNS
;
;  Nothing.
;
;  AUTHOR
;
;   SM          November 22, 1992
;
;  MODIFICATIONS
;
;
;  November 22, 1992     4.00A : Initial release
;

asicrs_cnt:
        inc     [si+lscount]
asicrs: mov     cx,dx
        add     dx,LSREG                ;point to line status
        in      al,dx                   ;read the reason we are here
        KILL_TIME
        mov     ah,al
        not     ah
        mov     dx,INTER
        add     dx,cx                   ;dx = interrupt enable register
        or      [si+line_stat],ax       ;update static status byte
        not     ah
        mov     bx,[si+wide_stat]       ;save status bits for wide track
        and     bl,0f0h                 ;set bits 0-3 to 0
        and     ah,0eh                  ;strip unwanted bits in ah
        or      bl,ah                   ;and merge into bl
        mov     [si+wide_stat],bx       ;and save it back
        or      [si+chst_bits],LINERRG  ;say we had a line status change
        mov     bp,[si+chmode_bits]     ;let bp hold these for speed
        test    bp,IS_IGRCVER           ;see if we are ignoring modem
        jz      lstout                  ;status changes?
        or      [si+chst_bits],ALERT    ;set alert bit if not
        test    bp,IS_IGALERT           ;are we ignoring alert conditions?
        jz      lstout                  ;if so get out
        in      al,dx                   ;come here to stop tx & rx interrupts
        KILL_TIME
        and    al,not ENRX
        out    dx,al
        KILL_TIME
        and    [si+chst_bits],not TXIFLAG
        out     dx,al
        KILL_TIME
        and     [si+chst_bits],not XCHRUN+RCHRUN  ;say interrupts both stopped
        or      [si+chst_bits],TXWALERT ;and we are waiting for alert
lstout: retf


