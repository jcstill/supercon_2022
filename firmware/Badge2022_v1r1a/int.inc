
;***********************************************************************************
;****************************       T1 INTERRUPT       *****************************
;***********************************************************************************
T1Int:				; TIMER1 int, display & kbd 3.2 KHz
	push.s			; \
; OUT port service
	mov.b	Ram+0x0A,WREG	; OUT register if WrFlags,#1 = 0
	btsc	WrFlags,#1	; I/O port select
	mov.b	Ram+0xFA,WREG	; OUT register if WrFlags,#1 = 1
	mov.b	WREG,LATB	; OUT port
; split interrupt Dimmer timing to two timings (to give a total of 514)
	mov.b	Dimmer,WREG
	ze	w0,w0
	btsc	Flag,#13	; #13 toggle between int (0) - aux_int (1)
	bra	aux_int		; aux_int only turns off LEDs, for dimming
	bset	Flag,#13	; #13 toggle between int (0) - aux_int (1)
; calculate exponential dimmer step
	mul.uu	w0,w0,w2	; w2 =   0,   1,   4,   9,  16...196, 225
	cp0	w2
	ifnz
	add	#5,w2		; w2 =   0,   6,   9,  14,  21...201, 230
	add	#5,w2		; w2 =   5,  11,  14,  19,  26...206, 235
	sl	w2,w2		; w2 =  10,  22,  28,  38,  52...412, 470
; mov #300,w2 ;(samo za snimanje 60 fps)
	mov	w2,PR1
	mov	w2,PR1copy
; automatic Leds and Matrix off when ALT pressed
	  mov	  #0b0011,w0
	  btsc	  Flag,#6	  ; #6 set if ALT pressed (debounced)
	  and.b	  WrFlags
; if 10 minutes countdown=0 then go to sleep
	inc	BlinkCount	; +
	  btsc	  Flag,#14	  ; dim down in progress?
	  bra	  count625
	  btsc	  Flag,#15	  ; dim up in progress?
	  bra	  count625
	mov	BlinkCount,w0
	and	#0x3F,w0	; 4 KHz / 0x03F = 62.5 Hz
	  ifz
	  dec	  AutoOff_Hi	  ; 62.5 Hz decrement
	  bra	  nz,count625
; if AutoOff_Hi started from 37500, then 10 minutes here
	  mov	  #37500,w0
	  mov	  w0,AutoOff_Hi	  ; new 10 min cycle started
; decrement 10 minutes countdown, if=0 then go to sleep 
	  dec.b	  AutoOff
	  bra	  nz,count625
	  mov.b	  Dimmer,WREG
	  mov.b	  WREG,Temp_Dimmer
	  bset	  Flag,#14	; dim down request (AutoOff patience counter expired)
count625:
; inc main multiplex pointer
	inc	RowScan,WREG	; +
	and	#0x0F,w0
	mov	w0,RowScan	; RowScan = 0000...1111
; anode off
	clr.b	LATC		; LED anodes off
	clr.b	LATB+1		; LED anodes off
	bclr	LATC,#8		; 17th anode column
	bclr	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; decoder service
	sl	w0,#7,w1	; RowScan 3, 2, 1, 0 ---> A10, A9, A8, A7
	btsc	LATA,#1		; LATA,#1 is output for SAO (if it's not Tx)...
	bset	w1,#1		; ...restore it
	mov	w1,LATA		; set decoder 74HC138 inputs
; A16 History/standard mode select
	  mov	  History_A16,w3
	  btss	  Flag,#5	  ; #5 set in HISTORY mode
	mov	A16,w3
; main branch
	bra	w0		; main branch (w0=0...15)
;...
	bra	cathode0
	bra	cathode1
	bra	cathode2
	bra	cathode3
	bra	cathode4
	bra	cathode5
	bra	cathode6
	bra	cathode7
	bra	cathode8
	bra	cathode9
	bra	cathode10
	bra	cathode11
	bra	cathode12
	bra	cathode13
	bra	cathode14
	bra	cathode15

;------------------------------- MACRO -------------------------------
; Important Note for this Macro: 
; Ports LATA,#A7-#A10 are a,b,c,d inputs to the 4-to-16 decoder (2×74AHC138). These 
; outputs drive LED matrix, and also buttons. Outputs 0-6 are not used for buttons, 
; output 7 is for rightmost keys, output 8 for Mode and Carry, output 9 for Save and 
; Load, and so on. Input ; PORTA,#3 is for keys Mode, save, Clock... and so every 
; second key. Input Porta,#4 ; is for Carry, Load, Opcode8... and so every second 
; key. Keys On-Off and ALT are not in the keyboard matrix. Please see schematics.
.macro	keytest	keynum,AllowRpt1,AllowRpt2
; key connected to PORTA,#3 input
	btsc	BlinkCount,#4
	bra	6f		; if odd then skip
; debouncer and "just" condition detector
	sl	KeyRotors+\keynum*2
	mov	#0b1111110000000000,w0
	ior	KeyRotors+\keynum*2
	btsc	PORTA,#3
	bset	KeyRotors+\keynum*2,#0
	inc2	KeyRotors+\keynum*2,WREG ; test if keynum = 0bxxxx111111111110
	bra	nz,7f		; if not just pressed
	and	Anypress	; bit 0 will be reset if any key still depressed
	mov	#\keynum,w0
	mov	w0,Just		; just pressed
	sub	#4,w0		; now key numbers 1...13 for Opcode/OperX/OperY/DataIn...
	ifnc
	mov	#0,w0		; ...and 0 for all other keys
	mov.b	WREG,KeyReg	; SFR code of "just pressed" key
	bset	Flag,#1		; flag "key just pressed"
	bset.b	KeyStatus,#0	; SFR "key just pressed"
	mov	#15,w0		; 15×10 min autooff at keystroke
	mov.b	WREG,AutoOff
	mov	#37500,w0
	mov	w0,AutoOff_Hi
; test if Autorepeat for this key is allowed
	btss	AutorptFlag,#\AllowRpt1
	bra	8f		; if Autorepeat for this key not allowed
; yes, it's allowed
	mov	#autorpt_start,w0
	btsc	AutorptCount,#15 ; don't write to AutorptCount if it's already counting
	mov	w0,AutorptCount	; write only if it's negative
	bra	8f
6:
; key connected to PORTA,#4 input
; debouncer and "just" condition detector
	sl	KeyRotors+\keynum*2+2
	mov	#0b1111110000000000,w0
	ior	KeyRotors+\keynum*2+2
	btsc	PORTA,#4
	bset	KeyRotors+\keynum*2+2,#0
	inc2	KeyRotors+\keynum*2+2,WREG ; test if keynum = 0bxxxx111111111110
	bra	nz,7f		; if not just pressed
	and	Anypress	; bit 0 will be reset if any key still depressed
	mov	#\keynum+1,w0
	mov	w0,Just		; code of "just pressed" key
	sub	#4,w0		; now key numbers 1...13 for Opcode/OperX/OperY/DataIn...
	ifnc
	mov	#0,w0		; ...and 0 for all other keys
	mov.b	WREG,KeyReg	; SFR code of "just pressed" key
	bset	Flag,#1		; flag "key just pressed"
	bset.b	KeyStatus,#0	; SFR "key just pressed"
	mov	#15,w0		; 15×10 min autooff at keystroke
	mov.b	WREG,AutoOff
	mov	#37500,w0
	mov	w0,AutoOff_Hi
; test if Autorepeat for this key is allowed
	btss	AutorptFlag,#\AllowRpt2
	bra	8f		; if Autorepeat for this key not allowed
; yes, it's allowed
	mov	#autorpt_start,w0
	btsc	AutorptCount,#15 ; don't write to AutorptCount if it's already counting
	mov	w0,AutorptCount	; write only if it's negative
	bra	8f
7:
	and	Anypress	; bit 0 will be reset if any key still depressed
8:
.endm

;------------------------------- OPCODE BAR --------------------------- cathode 0
cathode0:
; spec test if any key pressed (needed for autorepeat)
	btsc	BlinkCount,#4
	bra	cont_cathode0	; test must be performed in every 2nd pass
	btsts	Anypress,#0	; test AND SET bit 0 (will be Z if any key pressed)
	bra	nz,NoAutoRpt	; if no key was pressed
; at least 1 key was pressed
	btsc	AutorptCount,#15
	bra	cont_cathode0	; if it's not set for counting (it's negative)
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	cont_cathode0	; if ALT pressed
	dec	AutorptCount
	bra	nz,cont_cathode0 ; if not yet 0x0000
; AutorptCount just reached 0, reinitialize autorpt process
	mov	#KeyRotors,w0
	repeat	#18-1
	setm	[w0++]		; reenable "just" condition
	mov	#autorpt_next,w0
	mov	w0,AutorptCount	; reinitialize autorpt process
	bra	cont_cathode0
NoAutoRpt:
	setm	AutorptCount	; forced -1 (only #autorpt_start allowed for writing)
cont_cathode0:
; extra anode 16
	btsc	w3,#b_clk_sch	; Extra bit 16: CLOCK (schematics) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	mov	Mode,w1
	cp.b	w1,#3
	bra	z,3f		; *** in PGM mode, no Sync Clock Page
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	alt0		; IF ALT pressed
3:
	  mov	  History_w9,w1	  ; HISTORY mode
	  mov	  [w1],w1	  ; HISTORY mode
	  lsr	  w1,#8,w1	  ; HISTORY mode
	  btss	  Flag,#5	  ; #5 set in HISTORY mode
	mov	Opcode,w1	; ***
	mov	#0,w0
	btsc	BlinkCount,#9	; blinking freq
	btss	BlinkFlag,#0	; 0 set = column Opcode blinks
	mov	#1,w0
	sl	w0,w1,w0	; shift 0000000000000001 for w1 bit positions
	  cp0	  Taskbar
	  ifnz
	  mov	  Taskbar,w0
	bra	go_disp0a
alt0:				; IF ALU pressed
	mov.b	Sync,WREG	; if Mode > 0 (SS Run Pgm)
	    cp0	    Mode
	    ifz
	    mov.b   Dimmer,WREG	; if Mode = 0 (ALU)
	ze	w0,w1
	mov	#1,w0
	sl	w0,w1,w0	; shift 0000000000000001 for w1 bit positions
go_disp0a:
	btsc	Flag,#3		; #3 self-check mode
	mov	Column1,w0
	bra	disp_w0_noblank
;------------------------------- OPERAND X BAR ------------------------ cathode 1
cathode1:
; extra anode 16
	btsc	w3,#b_exr	; Extra bit 16: EXR (instruction set)
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	mov	Mode,w1
	cp.b	w1,#3
	bra	z,3f		; *** in PGM mode, no Sync Clock Page
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	alt1		; IF ALT pressed
3:
	  mov	  History_w9,w1	  ; HISTORY mode
	  mov	  [w1],w1	  ; HISTORY mode
	  lsr	  w1,#4,w1	  ; HISTORY mode
	  btss	  Flag,#5	  ; #5 set in HISTORY mode
	mov	OperX,w1	; ***
	and	#0x0F,w1
	mov	#0,w0
	btsc	BlinkCount,#9	; blinking freq
	btss	BlinkFlag,#1	; 1 set = column Oper X blinks
	mov	#1,w0
	sl	w0,w1,w0	; shift 0000000000000001 for w1 bit positions
	  cp0	  Taskbar
	  ifnz
	  mov	  Taskbar,w0
	bra	go_disp0b
alt1:			; IF ALT pressed
	mov.b	Speed,WREG
	  btsc	  Flag,#5	  ; #5 set in HISTORY mode
	  mov.b	  History_Speed,WREG
	    cp0	    Mode
	    bra	    nz,1f
	    mov.b   SerCtrl,WREG	; if Mode = 0 (ALU)
	    bclr.b  w0,#3		; spec case: Buad Rate takes only 3 bits
1:
	ze	w0,w1
	mov	#1,w0
	sl	w0,w1,w0	; shift 0000000000000001 for w1 bit positions
go_disp0b:
	btsc	Flag,#3		; #3 self-check mode
	mov	Column2,w0
	bra	disp_w0
;------------------------------- OPERAND Y BAR ------------------------ cathode 2
cathode2:
; extra anode 16
	btsc	w3,#b_clk_key	; Extra bit 16: CLOCK (schematic) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	mov	Mode,w1
	cp.b	w1,#3
	bra	z,3f		; *** in PGM mode, no Sync Clock Page
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	alt2pressed	; IF ALT pressed
3:
	  mov	  History_w9,w1	  ; HISTORY mode
	  mov	  [w1],w1	  ; HISTORY mode
	  btss	  Flag,#5	  ; #5 set in HISTORY mode
	mov	OperY,w1
	and	#0x0F,w1
	mov	#1,w0
	sl	w0,w1,w0	; shift 0000000000000001 for w1 bit positions
	  cp0	  Taskbar
	  ifnz
	  mov	  Taskbar,w0
	bra	go_disp0c
alt2pressed:		; IF DATA IN pressed
	mov.b	Page,WREG
	  btsc	  Flag,#5	  ; #5 set in HISTORY mode
	  mov.b	  History_Page,WREG
	    cp0	    Mode
	    ifz
	    mov	    FlashAddr,w0 ; Alt pressed in ALU mode: get FlashAddr instead of OperY
	ze	w0,w1
	mov	#1,w0
	sl	w0,w1,w0	; shift 0000000000000001 for w1 bit positions
go_disp0c:
	btsc	Flag,#3		; #3 self-check mode
	mov	Column3,w0
	bra	disp_w0
;---------------------- SOURCE, PROG MEM ADDR ------------------------ cathode 3
cathode3:
; ALT key debouncer
	sl	KeyRotors+38
	btsc	PORTA,#2
	bset	KeyRotors+38,#0
	inc.b	KeyRotors+38,WREG ; test if keynum = 0bxxxxxxxx11111111
	ifz
	bclr	Flag,#6		; #6 set if ALT pressed (debounced)
	ifz
	bclr	KeyStatus,#3
	cp0.b	KeyRotors+38	; test if keynum = 0bxxxxxxxx00000000
	ifz
	bset	Flag,#6		; #6 set if ALT pressed (debounced)
	ifz
	bclr	KeyStatus,#3
	btsc	Flag,#3		; #3 self-check mode
	bra	no_in3		; no IN in self-check mode
; IN register service
	mov.b	PORTB,WREG
	lsr	w0,#4,w0
	and	w0,#0x0F,w0
	btss	WrFlags,#1	; I/O port select
	mov.b	WREG,Ram+0x0B	; IN register
	btsc	WrFlags,#1	; I/O port select
	mov.b	WREG,Ram+0xFB	; IN register
no_in3:
; extra anode 16
	btsc	w3,#b_noclk_sch ; Extra bit 16: INV CLOCK (schematic)
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	mov	#Rom,w1
	  mov	  History_w9,w0	  ; HISTORY mode
	  sub	  w0,w1,w0	  ; HISTORY mode
	  btsc	  Flag,#6	  ; #6 set if ALT pressed (debounced)
	  sl	  History_Ptr,WREG ; if ALT in History mode, then History_Ptr displayed
	  btss	  Flag,#5	  ; #5 set in HISTORY mode
	sub	w9,w1,w0
	lsr	w0,w1		; w1 = PC actual value
	  mov	  History_w12,w0  ; HISTORY mode
	  sl	  w0,#12,w0	  ; HISTORY mode
	  btss	  Flag,#5	  ; #5 set in HISTORY mode
	sl	w12,#12,w0	; w12 = SOURCE (OperY) contents
	    mov	    #0x0FFF,w3	    ; * * * * BUG FIX SEPT 21 2022
	    and	    w3,w1,w1	    ; * * * * BUG FIX SEPT 21 2022
;	cp0	Mode
;	ifnz			; ProgMemAddr (w9) is not displayed in ALU mode
	ior	w1,w0,w0
	btsc	Flag,#3		; #3 self-check mode
	mov	Row3,w0
	bra	disp_w0_noblank
;------------------------------- Vflag, ADDER, DEST ------------------- cathode 4
cathode4:
; extra anode 16
	btsc	w3,#b_data_inv	; Extra bit 16: DATA INVERSE (schematic) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	  btsc	  Flag,#5	  ; #5 set in HISTORY mode
	  bra	  History_cat4	  ; if HISTORY mode
	mov	Vflag,w0
	btss	Stack,#2
	bra	1f
	btsc	BlinkFlag,#6	; 6 set = Stack blinks
	btss	BlinkCount,#9	; blinking freq
	bset	w0,#3		; Stack LED bit
1:
	sl	w0,#8,w1	; 0000 SVVV 0000 0000
; Adder Sum & Carry
	sl	w12,#4,w0	; w0 = source × 16
	ior	w11,w0,w0	; w0 = source × 16 + dest
skr_cat4:
	sl	w0,#2,w0	; w0 = 4 × (source × 16 + dest)
	mov	#tbloffset(table_adder),w2
	add	w0,w2,w0	; w0 = #table_adder + 4 × (source × 16 + dest)
	btsc	w3,#b_cin	; Extra bit 16: C IN (schematic) 
	inc2	w0,w0		; if Cin set
	push	TBLPAG	   ; \\
	clr	TBLPAG
	tblrdl	[w0],w0		; 0000 0VZC AAAA CCCC
	pop	TBLPAG	   ; //
	and	#0x0FF,w0	; 0000 0000 AAAA CCCC
	ior	w0,w1,w0	; 0000 SVVV AAAA CCCC
	sl	w0,#4,w0	; SVVV AAAA CCCC 0000
	ior	w0,w11,w0	; SVVV AAAA CCCC DDDD (w11 = dest contents)
	btsc	Flag,#3		; #3 self-check mode
	mov	Row4,w0
	bra	disp_w0
History_cat4:			  ; HISTORY mode ............................
	  mov	  History_Vflag,w0
	  btsc	  History_Stack,#2
	  bset	  w0,#3		  ; Stack LED bit
	  sl	  w0,#8,w1	  ; 0000 SVVV 0000 0000
  ; Adder Sum & Carry
	  mov	  History_w12,w0
	  sl	  w0,#4,w0	  ; w0 = source × 16
	  ior	  History_w11,WREG ; w0 = source × 16 + dest
	  bra	  skr_cat4
;-------------------------------- Zflag, OR, AND, XOR ----------------- cathode 5
cathode5:
; extra anode 16
	btsc	w3,#b_cin	; Extra bit 16: C IN (schematic) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	  btsc	  Flag,#5	  ; #5 set in HISTORY mode
	  bra	  History_cat5
	mov	Zflag,w0	; 0000 0000 0000 0ZZZ
	btss	Stack,#1
	bra	1f
	btsc	BlinkFlag,#6	; 6 set = Stack blinks
	btss	BlinkCount,#9	; blinking freq
	bset	w0,#3		; 0000 0000 0000 SZZZ
1:
	sl	w0,#4,w0	; 0000 0000 SZZZ 0000
; Logic OR, AND and XOR
	ior	w11,w12,w1
	ior	w0,w1,w0	; 0000 0000 SZZZ OOOO
	sl	w0,#4,w0	; 0000 SZZZ OOOO 0000
	and	w11,w12,w1
	ior	w0,w1,w0	; 0000 SZZZ OOOO AAAA
	sl	w0,#4,w0	; SZZZ OOOO AAAA 0000
	xor	w11,w12,w1
	ior	w0,w1,w0	; SZZZ OOOO AAAA XXXX
	btsc	Flag,#3		; #3 self-check mode
	mov	Row5,w0
	bra	disp_w0
History_cat5:			  ; HISTORY mode ............................
	  mov	  History_Zflag,w0 ; 0000 0000 0000 0ZZZ
	  btsc	  History_Stack,#1
	  bset	  w0,#3		  ; 0000 0000 0000 SZZZ
	  sl	  w0,#4,w0	  ; 0000 0000 SZZZ 0000
  ; Logic OR, AND and XOR
	  mov	  History_w11,w2
	  mov	  History_w12,w3
	  ior	  w2,w3,w1
	  ior	  w0,w1,w0	  ; 0000 0000 SZZZ OOOO
	  sl	  w0,#4,w0	  ; 0000 SZZZ OOOO 0000
	  and	  w2,w3,w1
	  ior	  w0,w1,w0	  ; 0000 SZZZ OOOO AAAA
	  sl	  w0,#4,w0	  ; SZZZ OOOO AAAA 0000
	  xor	  w2,w3,w1
	  ior	  w0,w1,w0	  ; SZZZ OOOO AAAA XXXX
	  bra	  disp_w0
;-------------------------------- Cflag, ACCU OUT/TEMP/IN ------------- cathode 6
cathode6:
; test ON-OFF key
	sl	KeyRotors+36
	btsc	PORTC,#9
	bset	KeyRotors+36,#0
	inc2	KeyRotors+36,WREG ; test if keynum = 0bxxxxxxxx11111110
	bra	nz,1f
; ON-OFF pressed
	  btsc	  Flag,#14	  ; dim down in progress?
	  bra	  1f
	  btsc	  Flag,#15	  ; dim up in progress?
	  bra	  1f
	  mov.b	  Dimmer,WREG
	  mov.b	  WREG,Temp_Dimmer
	  bset	  Flag,#14	; dim down MAIN REQUEST (key ON/OFF pressed)
1:
; extra anode 16
	btsc	w3,#b_cena	; Extra bit 16: C ENABLE (schematic) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	  btsc	  Flag,#5	  ; #5 set in HISTORY mode
	  bra	  History_cat6
	mov	Cflag,w0
	btss	Stack,#0
	bra	1f
	btsc	BlinkFlag,#6	; 6 set = Stack blinks
	btss	BlinkCount,#9	; blinking freq
	bset	w0,#3		; Stack LED bit
1:
	sl	w0,#4,w0	; xxxx xxxx SCCC 0000
; Accumulator OUT / TEMP / IN
	ior	w0,w14,w0	; xxxx xxxx SCCC OOOO (Accu Out is w14)
	sl	w0,#4,w0	; xxxx SCCC OOOO 0000
	ior	Atemp,WREG	; xxxx SCCC OOOO TTTT
	sl	w0,#4,w0	; SCCC OOOO TTTT 0000
	ior	w0,w13,w0	; SCCC OOOO TTTT IIII (Accu In is w13)
	btsc	Flag,#3		; #3 self-check mode
	mov	Row6,w0
	bra	disp_w0
History_cat6:			  ; HISTORY mode ............................
	  mov	  History_Cflag,w0
	  btsc	  History_Stack,#0
	  bset	  w0,#3		  ; Stack LED bit
	  sl	  w0,#4,w0	  ; xxxx xxxx SCCC 0000
  ; Accumulator OUT / TEMP / IN
	  ior	  History_w14,WREG ; xxxx xxxx SCCC OOOO (Accu Out is w14)
	  sl	  w0,#4,w0	  ; xxxx SCCC OOOO 0000
	  ior	  History_Atemp,WREG ; xxxx SCCC OOOO TTTT
	  sl	  w0,#4,w0	  ; SCCC OOOO TTTT 0000
	  ior	  History_w13,WREG ; SCCC OOOO TTTT IIII (Accu In is w13)
	  bra	  disp_w0
;-------------------------------- PAGE, OPCODE, OPERX, OPERY ---------- cathode 7
cathode7:
; test keys 16, 17
	keytest	16,7,8
; extra anode 16
	btsc	w3,#b_sel	; Extra bit 16: SEL (bin/sel leds) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
	  btsc	  Flag,#5	; #5 set in HISTORY mode
	  bra	  History_cat7
	mov.b	Page,WREG	; xxxx xxxx xxxx PPPP
	sl	w0,#4,w0	; xxxx xxxx PPPP 0000
	mov	Mode,w1
	cp.b	w1,#3
	bra	z,3f		; *** in PGM mode, no Sync Clock Page
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	Alt7		; IF ALT pressed
; anodes 0...15
3:
	ior	Opcode,WREG	; xxxx xxxx PPPP OOOO
	sl	w0,#4,w0	; xxxx PPPP OOOO 0000
	ior	OperX,WREG	; xxxx PPPP OOOO XXXX
	sl	w0,#4,w0	; PPPP OOOO XXXX 0000
	ior	OperY,WREG	; PPPP OOOO XXXX YYYY
	bra	go_disp0d
Alt7:			; IF ALT pressed (not history) (Sync Clock Page)
	cp0	Mode
	bra	z,alt7alu
	ior.b	Sync,WREG	; xxxx xxxx PPPP SSSS
	sl	w0,#4,w0	; xxxx PPPP SSSS 0000
	ior.b	Speed,WREG	; xxxx PPPP SSSS SSSS
	sl	w0,#4,w0	; PPPP SSSS SSSS 0000
	ior.b	Page,WREG	; PPPP SSSS SSSS PPPP
go_disp0d:
	btsc	Flag,#3		; #3 self-check mode
	mov	Row7,w0
	bra	disp_w0
alt7alu:		; if ALT pressed in ALU mode (Dim Baud Flash)
	ior.b	Dimmer,WREG	; xxxx xxxx PPPP DDDD
	sl	w0,#4,w0	; xxxx PPPP DDDD 0000
	ior.b	SerCtrl,WREG	; xxxx PPPP DDDD EBBB
	  bclr	  w0,#3		; spec case: Buad Rate takes only 3 bits
	sl	w0,#4,w0	; PPPP DDDD 0BBB 0000
	ior.b	FlashAddr,WREG	; PPPP DDDD 0BBB FFFF
	bra	disp_w0
History_cat7:			  ; HISTORY mode ............................
	  btsc	  Flag,#6		; #6 set if ALT pressed (debounced)
	  bra	  History_PageClockDim	; IF ALT pressed
  ; anodes 0...15
	  mov.b	  History_Page,WREG	; xxxx xxxx xxxx PPPP
	  sl	  w0,#12,w0		; PPPP 0000 0000 0000
	  mov	  History_w9,w1		; Program Memory Address
	  ior	  w0,[w1],w0		; PPPP CCCC XXXX YYYY
	  bra	  disp_w0
History_PageClockDim:
	  mov.b	  History_Page,WREG	; xxxx xxxx xxxx PPPP
	  sl	  w0,#4,w0		; xxxx xxxx PPPP 0000
	  ior.b	  Dimmer,WREG		; xxxx xxxx PPPP SSSS
	  sl	  w0,#4,w0		; xxxx PPPP SSSS 0000
	  ior.b	  History_Speed,WREG	; xxxx PPPP SSSS CCCC
	  sl	  w0,#4,w0		; PPPP SSSS CCCC 0000
	  ior.b	  History_Page,WREG	; PPPP SSSS CCCC PPPP
	  bra	  disp_w0
;-------------------------------- MATRIX 0,1 -------------------------- cathode 8
cathode8:
; test keys 0, 1
	keytest	0,0,1
; extra anode 16
	btsc	w3,#b_pgm	; Extra bit 16: PGM (led) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	mov	#Ram+17,w1
	bra	disp_matrix
;-------------------------------- MATRIX 2,3 -------------------------- cathode 9
cathode9:
; test keys 2, 3
	keytest 2,2,3
; extra anode 16
	btss	w3,#b_run	; Extra bit 16: RUN (led) 
	bra	1f
	btsc	BlinkFlag,#2	; 2 set = LED Run blinks
	btss	BlinkCount,#9	; bit pos = blinking freq
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
1:
; anodes 0...15
	mov	#Ram+19,w1
	bra	disp_matrix
;-------------------------------- MATRIX 4,5 -------------------------- cathode 10
cathode10:
; test keys 4, 5
	keytest 4,4,5
; extra anode 16
	btss	w3,#b_ss	; Extra bit 16: SS (led) 
	bra	1f
	btsc	BlinkFlag,#3	; 3 set = LED SS blinks
	btss	BlinkCount,#9	; blinking freq
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
1:
; anodes 0...15
	mov	#Ram+21,w1
	bra	disp_matrix
;-------------------------------- MATRIX 6,7 -------------------------- cathode 11
cathode11:
	btsc	Flag,#3		; #3 self-check mode
	bra	no_in4		; no IN in self-check mode
; IN register service
	mov.b	PORTB,WREG
	lsr	w0,#4,w0
	and	w0,#0x0F,w0
	btss	WrFlags,#1	; I/O port select
	mov.b	WREG,Ram+0x0B	; IN register
	btsc	WrFlags,#1	; I/O port select
	mov.b	WREG,Ram+0xFB	; IN register
no_in4:
; test keys 6, 7
	keytest 6,5,5
; extra anode 16
	btsc	w3,#b_alu	;  Extra bit 16:ALU (led) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	mov	#Ram+23,w1
	bra	disp_matrix
;-------------------------------- MATRIX 8,9 -------------------------- cathode 12
cathode12:
; test keys 8, 9
	keytest 8,5,6
; extra anode 16
	btss	w3,#b_carry	; Extra bit 16: CARRY (key)
	bra	1f
	btsc	BlinkFlag,#4	; #4 = CARRY blinks
	btss	BlinkCount,#9	; blinking freq
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
1:
; anodes 0...15
	mov	#Ram+25,w1
	bra	disp_matrix
;-------------------------------- MATRIX A,B -------------------------- cathode 13
cathode13:
; test keys 10, 11
	keytest 10,6,6
; extra anode 16
	btss	w3,#b_save	; Extra bit 16: SAVE (key)
	bra	1f
	btsc	BlinkFlag,#5	; #5 = SAVE blinks
	btss	BlinkCount,#9	; blinking freq
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
1:
; anodes 0...15
	mov	#Ram+27,w1
	bra	disp_matrix
;-------------------------------- MATRIX C,D -------------------------- cathode 14
cathode14:
	mov	BlinkCount,w0
	mov	#0b110000,w1	; dim down/up speed
	and	w0,w1,w0
	bra	nz,no_dim_up
; dim down \\\\\\\\\\\\\\\\\\\\\
	btss	Flag,#14	; #14 set if dim down in progress
	bra	no_dim_down
	cp0.b	Dimmer
	ifz
	bclr	Flag,#14	; end of dim down
	bra	z,enter_sleep	; GO SLEEP -------------------------------------->
	dec.b	Dimmer
no_dim_down:
; dim up ///////////////////////
	btss	Flag,#15	; #15 set if dim up in progress
	bra	no_dim_up
	mov.b	Temp_Dimmer,WREG
	cp.b	Dimmer		; Dimmer - Temp_Dimmer
	ifc
	bclr	Flag,#15	; end of dim up
	ifnc
	inc.b	Dimmer
no_dim_up:
; test keys 12, 13
	keytest 12,6,7
; extra anode 16
	btss	w3,#b_load	; Extra bit 16: LOAD (key) 
	bra	1f
	btsc	BlinkFlag,#7	; #7 = LOAD blinks
	btss	BlinkCount,#9	; blinking freq
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
1:
; anodes 0...15
	mov	#Ram+29,w1
	bra	disp_matrix
;-------------------------------- MATRIX E,F -------------------------- cathode 15
cathode15:
; test keys 14, 15
	keytest 14,7,7
; anykey ---> Flag,#11 test
	btss	BlinkCount,#4
	bra	1f		; if odd pass
	bclr	Flag,#11	; #11 flag "any Key Pressed"
	btss	Anypress,#0	; bit 0 is reset if any key still depressed
	bset	Flag,#11	; #11 flag "any Key Pressed"
;	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
;	bset	Flag,#11	;WrFlags #11 flag "any Key Pressed"
	bclr.b	KeyStatus,#2	; user's flag "any key pressed"
	btsc	Flag,#11	; #11 flag "any Key Pressed" (will be moved to KeyStatus,#2)
	bset.b	KeyStatus,#2	; user's flag "any key pressed"
	bra	2f
1:
; test if last key is still pressed (service KeyStatus,#1)
	bclr.b	KeyStatus,#1	; user's flag "last key pressed"
	sl.b	KeyReg,WREG	; ×2
	and	#0x1F,w0
	bra	z,2f		; if last key = 0, 1, 2, 3, 4 (illegal keys)
	mov	#KeyRotors+8,w1
	add	w0,w1,w0	; debouncer for the last pressed key
	btss	[w0],#0
	bset.b	KeyStatus,#1	; user's flag "last key pressed"
2:
; extra anode 16
	btsc	w3,#b_bin	; Extra bit 16: BIN (bin/sel leds) 
	bset	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
; anodes 0...15
	mov	#Ram+31,w1
disp_matrix:
  ; History cathodes 8...15 conditional setup
	  btss	  Flag,#5	  ; #5 set in HISTORY mode
	  bra	  regular_15
  ; get bytes from unpacked history RAM pages
	  mov	  History_Visible,w1
	  btss	  Flag,#6	  ; #6 set if ALT pressed (debounced)
	  add	  #16,w1	  ; address for hist RAM page 0 is History_Visible+16
	  mov	  RowScan,w0
	  and	  #7,w0		  ; 8...15 ---> 0...7
	  sl	  w0,w0		  ; ×2
	  add	  w0,w1,w1
	  mov	  [w1],w0	  ; here's unpacked word ready for display ports
	  bra	  skr_15
regular_15:
	mov.b	Page,WREG
	  btsc	  Flag,#6	  ; #6 set if ALT pressed (debounced)
	  clr	  w0		  ; if ALT pressed, then page 0
	ze	w0,w0
	sl	w0,#4,w0	; w0 = 16 × Page
	add	w0,w1,w1	; w1 = #Ram + (16×Page) + 31
	sub	w1,#16,w2	; w2 = #Ram + (16×Page) + 15
; if >15 then wrap to 0
	mov	#Ram+255,w0
	cp	w1,w0
	ifc
	sub	#256,w1		; if >=Ram+256 then wrap to start of Ram
; get and stack RAM nibbles
	ze	[w1--],w0	; 0000 0000 0000 AAAA
	sl	w0,#4,w0	; 0000 0000 AAAA 0000
	ior.b	w0,[w2--],w0	; 0000 0000 AAAA CCCC
	sl	w0,#4,w0	; 0000 AAAA CCCC 0000
	ior.b	w0,[w1],w0	; 0000 AAAA CCCC BBBB
	sl	w0,#4,w0	; AAAA CCCC BBBB 0000
	ior.b	w0,[w2],w0	; AAAA CCCC BBBB DDDD
; blank matrix in mode 0 and mode 3
	mov	Mode,w2
	cp0	w2
	bra	nz,1f		; if Mode not ALU
; Mode = Dir
	btsc	Flag2,#8	; #8 set = first button pressed, Ver/Rev deleted
	bra	3f
; no buttons pressed yet (Ver/Rev mode)
	mov	RowScan,w2
	cp	w2,#13		; here are rows 10 and 11
	ifz
	mov	CHS1,w0		; if in Ver/Rev mode, show CHS
	cp	w2,#15		; here are rows 14 and 15
	ifz
	mov	CHS2,w0		; if in Ver/Rev mode, show CHS
	bra	skr_15		; if no buttons pressed yet, show Ver/Rev
3:
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	2f		; clear matrix data
; ALT pressed in ALU mode, anodes 7-15 show Files occupancy
	sl	RowScan,WREG
	and	#0x0F,w0
	mov	#Files,w1
	mov	[w0+w1],w0
1:
	cp	w2,#3		; if Mode = 3 (Run)...
	ifz
2:
	mov	#0,w0		; ...then clear matrix data
; end of blank matrix in mode 0 and mode 3
skr_15:
; Final matrix anode port write (respects blank request)
	btsc.b	WrFlags,#3	; 3 set = Leds Off
	bclr	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
	btsc	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
	bset	LATC,#8		; column 17
	btss.b	WrFlags,#2	; 2 set = Matrix Off
	mov.b	WREG,LATC	; LED anodes 0...7 set
	swap	w0
	btss.b	WrFlags,#2	; 2 set = Matrix Off
	mov.b	WREG,LATB+1	; LED anodes 8...15 set
	bra	t1exit
; ------------------------------------------------------
disp_w0:		; display service for cathodes 0...7
; Final LED anode port write (respects blank request)
	btsc.b	WrFlags,#3	; 3 set = Leds Off
	bclr	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
disp_w0_noblank:
	btsc	Flag,#12	; #12 temp LATC,#8 (column 17 anode)
	bset	LATC,#8		; column 17
	btss.b	WrFlags,#3	; 3 set = Leds Off
	mov.b	WREG,LATC	; LED anodes 0...7 set
	swap	w0
	btss.b	WrFlags,#3	; 3 set = Leds Off
	mov.b	WREG,LATB+1	; LED anodes 8...15 set
; ------------------------------------------------------
t1exit:
; final exit from interrupt
	pop.s			; /
	bclr	IFS0,#3		; clr int flag
	retfie			; ---------->
; ------------------------------------------------------
aux_int:		; aux_int only turns off LEDs, for dimming
	bclr	Flag,#13	; #13 toggle between int (0) - aux_int (1)
	clr	LATC		; all LEDS off (here's LATC,#8 also)
	clr.b	LATB+1		; all LEDS off

	mov	PR1copy,w0	; w0 =  10,  22,  28,  38,  52...412, 470
	mov	#475,w2
	sub	w2,w0,w0
; mov #10,w0 ;(samo za snimanje 60 fps)
	mov	w0,PR1

	pop.s			; /
	bclr	IFS0,#3		; clr int flag
	retfie			; ---------->

;***********************************************************************************
;****************************       T3 INTERRUPT       *****************************
;****************************       USER'S SYNC        *****************************
;***********************************************************************************
T3Int:				; TIMER 3 int, user's sync
	bset.b	RdFlags,#0	; #0 user's SYNC
	nop			; * * *   MCU BUG FIX  ???   * * *
	bclr	IFS0,#8		; clr int flag (skips this line without NOP)
	retfie

;***********************************************************************************
;***********************              RX1 INTERRUPT           **********************
;***********************************************************************************

; Character is received in U1RXREG
; read char and clear UART RX flag UXRDA
; if SerH:L is free, put it there and inc Received; finished
; if there is no room in FIFO, ignore all and exit (character is lost)
; else, put it at the end of FIFO (@WRPtr), inc WRPtr, calc Received

U1RXInt:			; RX1 interrupt
	push.s		   ; \
	mov	U1RXREG,w1	; <---- get received byte (this resets U1STAL,#URXDA)
	btsc	Flag2,#2	; #2 handshaking flag (for FIFO ---> SerHigh:SerLow)
	bra	move2fifo
; move char to SerH:L
	lsr	w1,#4,w0
	mov.b	WREG,SerHigh
	and	w1,#0x0F,w0
	mov.b	WREG,SerLow
	mov	#1,w0
	mov.b	WREG,Received
	bset	Flag2,#2	; #2 handshaking flag (for FIFO ---> SerHigh:SerLow)
	bclr	IFS0,#11	; clr int flag
	pop.s		   ; /
	retfie
move2fifo:		; move char to fifo
	mov	RXWR,w2
	inc.b	w2,w0		; pre-inc RXWR (in byte mode, to wrap inside RX buffer)
	cp.b	RXRD
	bra	nz,fifo_room	; if there's free place in FIFO, continue, else...
	bset.b	SerCtrl,#3	; ...set error flag...
	bra	nochar		; ...and quit
fifo_room:
	mov.b	w1,[w2]		; put char to FIFO
	inc.b	RXWR		; inc write ptr (in byte mode, to wrap inside RX byffer)
; Calculate Received
	mov	RXRD,WREG
	sub	RXWR,WREG	; RXWR-RXRD (byte mode, to stay in range 0000-00FF)
	ze	w0,w0
	inc	w0,w0		; as SerH:L is surely full
	cp	w0,#15
	ifc
	mov	#15,w0		; clip to 15
	mov.b	WREG,Received
	bset	Flag2,#2	; set handshaking flag (for FIFO ---> SerHigh:SerLow)
nochar:
	bclr	IFS0,#11	; clr int flag
	pop.s		   ; /
	retfie

;***********************************************************************************
;***********************          RX1 ERROR INTERRUPT       **********************
;***********************************************************************************
U1ErrInt:			; U1EIE interrupt
	bclr	U1STA,#1	; clr err flag
	bclr	U1STA,#2	; clr err flag
	bclr	IFS4,#1
	retfie

;***********************************************************************************
;***********************      CURREENTLY UNUSED INTERRUPTS     **********************
;***********************************************************************************
U2RXInt:			; RX2 interrupt
	bclr	IFS1,#14	; clr int flag
	retfie
;		
INT2Int:			; external INT2 interrupt
	bclr	IFS1,#13	; clr int flag
	retfie			; ---------->
;		
U2ErrInt:			; U2EIE interrupt
	bclr	U2STA,#1	; clr err flag
	bclr	U2STA,#2	; clr err flag
	bclr	IFS4,#2
	retfie

;***********************************************************************************
;***********************         EXTERNAL INT 1 SERVICE       **********************
;***********************************************************************************
INT1Int:		; ON/OFF button interrupt (shouldn't come here as INTCON2,#GIE ...
			; ...and IEC1,#4 are never ON at the same time, but just in case)
	bclr	IFS1,#4
	retfie

;ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ
;ZZZZZZZZZZZZZZZZZZZZZZ                S L E E P               ZZZZZZZZZZZZZZZZZZZZZZ
;ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

enter_sleep:		; set ports to lowest current consumption
			; be ready to wake up on EXT INT1 and enter sleep mode
			; After wake up, wait for button safely released and dim up
; peripheral and interrupt control bits
	bclr	INTCON2,#GIE	; global int disable
	bclr	T1CON,#TON	; LED multiplex timer off
	bclr	T2CON,#TON	; T2 timer off
	bclr	T3CON,#TON	; T3 timer off
	bclr	IEC0,#T1IE	; disable Timer 1 interrupt
	bclr	IEC0,#7		; disable Timer 2 interrupt
	bclr	IEC0,#8		; disable Timer 3 interrupt
	bclr	IEC1,#4		; disable external interrupt
; LEDs off, I/O pins hi-z
	clr.b	LATB+1		; all LEDs off
	clr	LATC		; all LEDs off
	setm.b	TRISB+0		; connector ports hi-z
	bset	TRISA,#1	; Tx on SAO also hi-z
; wait for ON-OFF safely released and final wake initialization
	call	test_no_press	; wait for ON-OFF safely released
	bclr	IFS1,#4		; clear interrupt flag to avoid self-triggering
	bset	INTCON2,#INT1EP	; interrupt on negative-going edge
	bset	IEC1,#4		; enable external interrupt
; SLEEP NOW    *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  * 
	pwrsav	#0		; *  *   SLEEP   *  *  *  *  *  *  *  *  *  *  *  *  *
; GOOD MORNING    *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  * 
	mov	#2,w0		; ×10 min autooff at wake-up
	mov.b	WREG,AutoOff
	mov	#37500,w0
	mov	w0,AutoOff_Hi
; restore I/O pins
	mov	#0xF0,w0	; bits 7654 = inputs, bits 3210 = outputs
	mov.b	WREG,TRISB+0	; restore states of connector ports
	bclr	TRISA,#1	; Tx on SAO = output
	btsc.b	WrFlags,#0	; if UART is redirected to I/O connector, then... 
	bclr	TRISB,#7	; ...Tx on I/O connector = output
; restore peripheral and interrupt control bits
	bclr	IEC1,#4		; disable external interrupt
	bset	IEC0,#T1IE	; enable Timer 1 interrupt
	bset	IEC0,#8		; enable Timer 3 interrupt
	bset	T1CON,#TON	; LED multiplex timer on
	bset	T2CON,#TON	; T2 timer on
	bset	T3CON,#TON	; T3 timer on
	bset	INTCON2,#GIE	; global int enable
; initialize key rotors and dim up
	mov	#KeyRotors,w1
	push	RCOUNT		; \
	repeat	#19-1
	clr	[w1++]		; clear key debouncer rotors to ignore pre-pressed keys
	pop	RCOUNT		; /
	bset	Flag,#15	; dim up MAIN REQUEST
	goto	t1exit
; wait for ON-OFF safely released (90 ms debouncer)
test_no_press:
	mov	#900,w0		; w0 × 0.1 ms verify period
1:
	push	RCOUNT		; \
	repeat	#1600-9
	nop
	pop	RCOUNT		; /
	btss	PORTC,#9	; skip if ON-OFF key high
	bra	test_no_press	; it's low, re-initialize the counter from the start
	dec	w0,w0
	bra	nz,1b		; 1 loop = 1600 t = 0.1 ms
	return

; TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT  TIMER TABLES

clk_table:		; CLOCK timing: T2 clock table: PR, Prescaler
	.word	10-1,	0		; 0  max possible
	.word	80-1,	0b00000000	; 1  2 × 100 KHz
	.word	267-1,	0b00000000	; 2  2 ×  30 KHz
	.word	800-1,	0b00000000	; 3  2 ×  10 KHz
	.word	2667-1,	0b00000000	; 4  2 ×   3 KHz
	.word	8000-1, 0b00000000	; 5  2 ×   1 KHz
	.word	16000-1,0b00000000	; 6  2 × 500 Hz
	.word	40000-1,0b00000000	; 7  2 × 200 Hz
	.word	10000-1,0b00010000	; 8  2 × 100 Hz (presc 8)
	.word	20000-1,0b00010000	; 9  2 ×  50 Hz (presc 8)
	.word	6250-1, 0b00100000	; A  2 ×  20 Hz (presc 64)
	.word	12500-1,0b00100000	; B  2 ×  10 Hz (presc 64)
	.word	25000-1,0b00100000	; C  2 ×   5 Hz (presc 64)
	.word	15625-1,0b00110000	; D  2 ×   2 Hz (presc 256)
	.word	31250-1,0b00110000	; E  2 ×   1 Hz (presc 256)
	.word	62500-1,0b00110000	; F  2 × 0.5 Hz (presc 256)

clk_table_high:		; CLOCK timing: T2 for HIGH speed (button FAST pressed)
	.word	10-1,	0		; 0  max possible
	.word	10-1,	0b00000000	; 1  max possible
	.word	10-1,	0b00000000	; 2  max possible
	.word	80-1,	0b00000000	; 3  2 × 100 KHz
	.word	266-1,	0b00000000	; 4  2 ×  30 KHz
	.word	800-1,	0b00000000	; 5  2 ×  10 KHz
	.word	1600-1,	0b00000000	; 6  2 ×   3 KHz
	.word	4000-1,	0b00000000	; 7  2 ×   3 KHz
	.word	8000-1, 0b00000000	; 8  2 ×   1 KHz
	.word	16000-1,0b00000000	; 9  2 × 500 Hz
	.word	40000-1,0b00000000	; A  2 × 200 Hz
	.word	10000-1,0b00010000	; B  2 × 100 Hz (presc 8)
	.word	20000-1,0b00010000	; C  2 ×  50 Hz (presc 8)
	.word	6250-1, 0b00100000	; D  2 ×  20 Hz (presc 64)
	.word	12500-1,0b00100000	; E  2 ×  10 Hz (presc 64)
	.word	25000-1,0b00100000	; F  2 ×   5 Hz (presc 64)

sync_table:		; SYNC timing
	.word	16000-1,0b00000000	; 1 1000 Hz
	.word	26667-1,0b00000000	; 1  600 Hz
	.word	40000-1,0b00000000	; 2  400 Hz
	.word	64000-1,0b00000000	; 3  250 Hz
	.word	13333-1,0b00010000	; 4  150 Hz (presc 8)
	.word	20000-1,0b00010000	; 5  100 Hz (presc 8)
	.word	33333-1,0b00010000	; 6   60 Hz (presc 8)
	.word	50000-1,0b00010000	; 7   40 Hz (presc 8)
	.word	10000-1,0b00100000	; 8   25 Hz (presc 64)
	.word	16667-1,0b00100000	; 9   15 Hz (presc 64)
	.word	25000-1,0b00100000	; A   10 Hz (presc 64)
	.word	41667-1,0b00100000	; B    6 Hz (presc 64)
	.word	62500-1,0b00100000	; C    4 Hz (presc 64)
	.word	25000-1,0b00110000	; D  2.5 Hz (presc 256)
	.word	41667-1,0b00110000	; D  1.5 Hz (presc 256)
	.word	62500-1,0b00110000	; E    1 Hz (presc 256)

sync_table_high:	; SYNC timing (high speed)
	.word	1600-1,0b00000000	; 1 10000 Hz
	.word	2667-1,0b00000000	; 1  6000 Hz
	.word	4000-1,0b00000000	; 2  4000 Hz
	.word	6400-1,0b00000000	; 3  2500 Hz
	.word	1333-1,0b00010000	; 4  1500 Hz (presc 8)
	.word	2000-1,0b00010000	; 5  1000 Hz (presc 8)
	.word	3333-1,0b00010000	; 6   600 Hz (presc 8)
	.word	5000-1,0b00010000	; 7   400 Hz (presc 8)
	.word	1000-1,0b00100000	; 8   250 Hz (presc 64)
	.word	1667-1,0b00100000	; 9   150 Hz (presc 64)
	.word	2500-1,0b00100000	; A   100 Hz (presc 64)
	.word	4167-1,0b00100000	; B    60 Hz (presc 64)
	.word	6250-1,0b00100000	; C    40 Hz (presc 64)
	.word	2500-1,0b00110000	; D    25 Hz (presc 256)
	.word	4167-1,0b00110000	; D    15 Hz (presc 256)
	.word	6250-1,0b00110000	; E    10 Hz (presc 256)


