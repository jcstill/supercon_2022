
;------------------------------------------------------------------------------------
;  NOTE: FIRMWARE IN THIS PAGE IS IN PROTECTED BOOT SEGMENT, AND CAN NOT BE MODIFIED 
;------------------------------------------------------------------------------------

boot_start:		bra	go_boot_start
initialize:		bra	boot_initialize
set_decoder_to_w1:	bra	boot_set_decoder_to_w1
w0ms2:			bra	boot_w0ms2
eecalc_addr_w0:		bra	boot_eecalc_addr_w0
eesavew0:		bra	boot_eesavew0
eeload:			bra	boot_eeload
eemeasure:		bra	boot_eemeasure
eeburn_128:		bra	boot_eeburn_128
eeerase_1K:		bra	boot_eeerase_1K
burn1k:			bra	boot_burn1k
selfcheck:		bra	boot_selfcheck
bootload:		bra	boot_bootload
key_save:		bra	boot_key_save
key_load:		bra	boot_key_load

;------------------------------------------------------------------------
;--------------------     BOOT  START     -------------------------------
;------------------------------------------------------------------------

go_boot_start:
; **********************************************************************************	
; Note:											
; Ports LATA,#A7-#A10 are a,b,c,d inputs to the 4-to-16 decoder (2×74AHC138). These	
; outputs drive LED matrix, and also buttons. Outputs 0-6 are not used for buttons,	
; output 7 is for rightmost keys, output 8 for Mode and Carry, output 9 for Save and	
; Load, and so on. Input ; PORTA,#3 is for keys Mode, save, Clock... and so every	
; second key. Input Porta,#4 ; is for Carry, Load, Opcode8... and so every second	
; key. Keys On-Off and ALT are not in the keyboard matrix. Please see schematics.	
; **********************************************************************************	
; ALT pressed, initialize small debouncig area
	bclr	INTCON2,#GIE	; global interrupt disable
	setm.b	KeyRotors+0
	setm.b	KeyRotors+4
	setm.b	KeyRotors+6
; Check if some other key excl. Save and Load pressed (if so, don't allow spec mode)
	mov	#7,w1		; 7 = here are keys OperY1 and DataIn
	  call	  set_decoder_to_w1
	btss	PORTA,#3
	bra	selfcheck_back	; if OperY 1 pressed
	btss	PORTA,#4
	bra	selfcheck_back	; if DataIn pressed
	  call	  set_decoder_to_w1
	btss	PORTA,#3
	bra	selfcheck_back	; if Mode pressed
	btss	PORTA,#4
	bra	selfcheck_back	; if Carry pressed
	  call	  set_decoder_to_w1
	btss	PORTA,#3
	bra	key_save	; if Save pressed -----------> SAVE
	btss	PORTA,#4
	bra	key_load	; if Load pressed -----------> LOAD

	mov	#6,w2		; w2 loop count
6:			; \
	  call	  set_decoder_to_w1
	btss	PORTA,#3
	bra	selfcheck_back	; if Clock, Opcode4, etc... pressed
	btss	PORTA,#4
	bra	selfcheck_back	; if Opcode8, Opcode2, etc... pressed
	dec	w2,w2
	bra	nz,6b	; /
	bra	selfcheck	; if ALT only pressed

; ----------------------------------------------------------------------
; -----------------------  S E L F   C H E C K  ------------------------
; ----------------------------------------------------------------------
boot_selfcheck:
	mov	#5,w0
	mov	w0,Mode
	bset	Flag,#3		; #3 self-check mode

	clr	TMR1
	bset	T1CON,#TON	; LED multiplex timer on
	bset	IEC0,#T1IE	; enable Timer 1 interrupt
	bset	INTCON2,#GIE	; global int enable
; initialize pixels for self check
	mov	#0b0100100100100100,w0
	mov	w0,Column1
	mov	#0b0010010010010010,w0
	mov	w0,Column2
	mov	#0b1001001001001001,w0
	mov	w0,Column3
	mov	#0b0010010010010010,w0
	mov	w0,Row3
	mov	#0b0100100100100100,w0
	mov	w0,Row4
	mov	#0b0010010010010010,w0
	mov	w0,Row5
	mov	#0b0100100100100100,w0
	mov	w0,Row6
	mov	#0b1001001001001001,w0
	mov	w0,Row7
	mov	#0b1001001001001001,w0
	mov	w0,A16

	mov	#0b010000001001,w0
	mov	w0,Ram+0
	mov	w0,Ram+6
	mov	w0,Ram+12
	mov	w0,Ram+18
	mov	w0,Ram+24
	mov	w0,Ram+30
	mov	#0b100100000010,w0
	mov	w0,Ram+2
	mov	w0,Ram+8
	mov	w0,Ram+14
	mov	w0,Ram+20
	mov	w0,Ram+26
	mov	#0b001000000100,w0
	mov	w0,Ram+4
	mov	w0,Ram+10
	mov	w0,Ram+16
	mov	w0,Ram+22
	mov	w0,Ram+28
self_check_loop:
; STEP 1: WAIT ---------
	mov	BlinkCount,w0
	and	#0x03FF,w0
	bra	nz,self_check_loop

; STEP 2: SHIFT --------
.macro	rotate	variab,if,then
	lsr	\variab
	btsc	\variab,#\if
	bset	\variab,#\then
.endm
	rotate	Column1,0,15
	rotate	Column2,0,15
	rotate	Column3,0,15
	rotate	Row3,0,15
	rotate	Row4,0,15
	rotate	Row5,0,15
	rotate	Row6,0,15
	rotate	Row7,0,15
	rotate	A16,6,15
	bclr	A16,#2
	btsc	A16,#12
	bset	A16,#2
	call	rotate_ram

; any key pressed?
	btss	Flag,#1		; #1 handshaking flag for buttons
	bra	self_check_loop	; no, loop ------------------------->

	setm	Column1
	setm	Column2
	setm	Column3
	setm	Row3
	setm	Row4
	setm	Row5
	setm	Row6
	mov	#0b1111000000000000,w0
	mov	w0,Row7
	mov	#0b0000000001111011,w0
	mov	w0,A16
wait_still:
; test if ASCII "BL"
	mov	Row7,w0
	mov	#0x0FFF,w1
	and	w0,w1,w0
	mov	#0x024C,w1	; ASCII BL = 0x424C
	cp	w0,w1
	bra	nz,not_BL
	btsc	A16,#b_carry	; carry   = bit 7 (should be 0)
	bra	not_BL
	btss	A16,#b_save	; save    = bit 6 (should be 1)
	bra	not_BL
	btsc	A16,#b_load	; load    = bit 5 (should be 0)
	bra	not_BL
	btsc	A16,#b_clk_key	; clk_key = bit 4 (should be 0)
	bra	not_BL
	btsc	A16,#b_alu	; Mode field LEDs off?
	bra	not_BL
	btsc	A16,#b_bin	; Data_In field LEDs off?
	bra	not_BL

	clr	Column1
	clr	Column2
	clr	Column3
	clr	Row3
	clr	Row4
	clr	Row5
	clr	Row6
	clr	Row7
	mov	#Ram,w0		; clr_ram
	repeat	#120-1		; clr_ram (clear all exept SFR)
	clr	[w0++]		; clr_ram
	mov	#0b0110000000000000,w0	; only SAVE and LOAD are ON
	mov	w0,A16
	bra	bootload	; BOOTLOAD ------------------->
not_BL:
1:
	btss	Flag,#1		; #1 handshaking flag for buttons
	bra	1b		; no, loop ------------------------->
	bclr	Flag,#1		; #1 handshaking flag for buttons
	call	rotate_ram
.macro		ct1	valct1
		mov	#0b\valct1,w0
		bra	cont_test1
.endm
.macro		ct2	valct2
		mov	#0b\valct2,w0
		bra	cont_test2
.endm
	mov	Just,w0
	sl	w0,w0		; ×2
	bra	w0
; ...
	ct1	0000111100000000	 ; mode
	ct1	0001000000000000	 ; carry
	ct1	0010000000000000	 ; save
	ct1	0100000000000000	 ; load
	ct1	0000000000000100	 ; clock
	ct2	0000100000000000	 ; opcode 8
	ct2	0000010000000000	 ; opcode 4
	ct2	0000001000000000	 ; opcode 2
	ct2	0000000100000000	 ; opcode 1
	ct2	0000000010000000	 ; oper X 8
	ct2	0000000001000000	 ; oper X 4
	ct2	0000000000100000	 ; oper X 2
	ct2	0000000000010000	 ; oper X 1
	ct2	0000000000001000	 ; oper Y 8
	ct2	0000000000000100	 ; oper Y 4
	ct2	0000000000000010	 ; oper Y 2
	ct2	0000000000000001	 ; oper Y 1
	ct1	1000000010000000	 ; DataIn 
cont_test1:
	xor	A16
	bra	ctx
cont_test2:
	xor	Row7
ctx:
	mov	Row7,w0
	mov	#0x0FFF,w1
	and	w0,w1,w0
	cp	w0,w1
	bra	nz,wait_still	; not yet all keys tested

	mov	A16,w0
	mov	#0b1111111110000100,w1
	and	w0,w1,w0
	cp	w0,w1
	bra	nz,wait_still	; not yet all keys tested

	mov	#-2000,w0	; count 2000 = 500 ms
	mov	w0,BlinkCount
wait_Xs:
	cp0	BlinkCount
	bra	nz,wait_Xs
	reset

;-----------------------------------------------------------------------------
;------------------------- BOOTLOAD ------------------------------------------
;-----------------------------------------------------------------------------

boot_bootload:
	bclr	INTCON2,#GIE	; global interrupt disable
	setm.b	KeyRotors+0
	setm.b	KeyRotors+4
	setm.b	KeyRotors+6
1:
	clr.b	LATC		; all LEDS off
	clr.b	LATB+1		; all LEDS off
	bclr	LATC,#8		; 17th anode driver off
; led SAVE on
	mov	#0b1111100001111111,w0
	and	LATA		; clear LATA bits 7 8 9 10 (decoder 138)
	mov	#0b0000011010000000,w0	; 13 = LED SAVE
	ior	LATA		; decoder state = 13 (there is led SAVE)
	bset	LATC,#8		; anode 17 on
	repeat	#8000		; ON time 0.5 ms
	nop
; led LOAD on
	mov	#0b1111100001111111,w0
	and	LATA		; clear LATA bits 7 8 9 10 (decoder 138)
	mov	#0b0000011100000000,w0	; 14 = LED LOAD
	ior	LATA		; decoder state = 14 (there is led LOAD)
	repeat	#8000		; ON time 0.5 ms
	nop
	bclr	LATC,#8		; anode 17 off
; set decoder output 0 and wait 2 ms
	mov	#0b1111100001111111,w0
	and	LATA		; clear LATA bits 7 8 9 10 (decoder 138)
	mov	#0b0000010000000000,w0	; 8 = buttons MODE and CARRY
	ior	LATA		; decoder state = 8 (there is key MODE)
	repeat	#16000		; 1 ms
	nop
	repeat	#16000		; 1 ms
	nop
; debounce and test key MODE
	sl.b	KeyRotors+0
	btsc	PORTA,#3
	bset.b	KeyRotors+0,#0
	ifz
	reset			; if MODE pressed then suicide
; set decoder output 1 and wait 2 ms
	mov	#0b1111100001111111,w0
	and	LATA		; clear LATA bits 7 8 9 10 (decoder 138)
	mov	#0b0000010010000000,w0	; 9 = buttons SAVE and LOAD
	ior	LATA		; decoder state = 9 (there are keys SAVE and LOAD)
	repeat	#16000		; 1 ms
	nop
	repeat	#16000		; 1 ms
	nop
; debounce and test key SAVE
	sl.b	KeyRotors+4
	btsc	PORTA,#3
	bset.b	KeyRotors+4,#0
;	bra	z,key_save	; if SAVE pressed  (* THIS FUNCTION CANCELED *)
; debounce and test key LOAD
	sl.b	KeyRotors+6
	btsc	PORTA,#4
	bset.b	KeyRotors+6,#0
	bra	z,key_load	; if LOAD pressed
; wait 2 ms and loop 
	repeat	#16000		; 1 ms
	nop
	repeat	#16000		; 1 ms
	nop
	bra	1b

;--------------------------------------------------------------
boot_key_save:
	bset	Flag2,#6	; #6 EE pointers adjust to Boot
	bclr	Flag2,#7	; #7 EE load/save to/from the first halve of Rom area
	bclr	Flag2,#9	; #9 clr = last command SAVE
	clr	w10		; w10 Flash Page, step 0x800
9:
	mov	w10,w0
	call	eeload		; firmware 1 ---> Rom 1 (3K bytes + 1K dummy 0's)
	add	w10,#24,w0
	bset	Flag2,#7	; #7 EE load to second halve of Rom area
	call	eeload		; firmware 2 ---> Rom 2 (3K bytes + 1K dummy 0's)

	mov	w10,w0
	call	eesavew0	; Rom 2 ---> firmware 1 (3K bytes + 1K dummy 0's)
	add	w10,#24,w0
	bclr	Flag2,#7	; #7 EE save from first halve of Rom area
	call	eesavew0	; Rom 1 ---> firmware 2 (3K bytes + 1K dummy 0's)

	inc	w10,w10
	cp	w10,#24
	bra	nc,9b
	bra	bootload

;--------------------------------------------------------------
boot_key_load:
	bset	Flag2,#6	; #6 EE subroutines adjust to Boot
	bset	Flag2,#7	; #7 EE burn from 2nd halve of Rom area (start @ 0x3000)
	bset	Flag2,#9	; #9 set = last command LOAD

; ------------------------------- DMA ------------------------------
; initialize DMA 0
	mov	#0b0000000000000000,w0	; Disabled, Fixed Priority
	mov	w0,DMACON		; Bit 15 = DMAEN (DMA Enable)
	mov	#0b0000001000010110,w0	; reload, SRC fixed, DST++, one-shot, byte mode
	mov	w0,DMACH0		; Bit 0: CHEN (Channel Enable)
	mov	#0b0100010100000000,w0	; Source is UART1 RX Interrupt
	mov	w0,DMAINT0
	mov	#Rom,w0
	mov	w0,DMAL		; low limit
	mov	#Rom+0x2000,w0
	mov	w0,DMAH		; high limit
	mov	#U1RXREG,w0
	mov	w0,DMASRC0	; Source address
	mov	#Rom,w0
	mov	w0,DMADST0	; Destination start address
	mov	#4096,w0
	mov	w0,DMACNT0	; RX block format (1024 instructions)
	bset	DMACON,#15	; Bit 15 = DMAEN (DMA Enable)
	bset	DMACH0,#0	; Bit 0: CHEN (Channel Enable)
; initialize peripheral hardware and variables
	bclr	IEC0,#11	; disable RX1 interrupt
	call	boot_rxtx_to_io2; RxTx directed to IO connector
	mov	#0b1111100001111111,w0
	and	LATA		; clear LATA bits 7 8 9 10
	mov	#0b0000011100000000,w0	; 14 = LED LOAD
	ior	LATA		; decoder state = 14 (there is led LOAD)
	clr	w8		; w8 = blink count (#4,#1) about 18 Hz (1 Hz, 8 Hz)
	mov	DMADST0,w14	; w14 = previous DMADST0
	mov	#50000,w13	; w13 = "DMADST changed" counter (0 when no RX data)
	mov	#Rom,w0
	repeat	#0x800-1
	setm	[w0++]		; clear	1st halve of Rom to 0xFFFF
	mov	#0,w11		; Page burn pointer (×0x800)
; main BOOT loop
	mov	#23,w12		; max number of 4K packets (+1 non-complete at end)
BootLoop:		 ; <------------------------------ bootload loop ~20t
	nop
; test if byte received (in that case set Flag2,#10 and initialize w13 counter)
	mov	DMADST0,w0	; Destination address
	cp	w0,w14
	bra	z,equal_dst	; if byte not received in this cycle
; byte received, set Flag2,#10 and initialize w13 counter (patience meter)
	bset	Flag2,#10	; #8 set = 1st byte in Bootloader received
	mov	w0,w14		; refresh previous DMADST0
	mov	#50000,w13
equal_dst:
	btss	Flag2,#10	; #8 set = 1st byte in Bootloader received
	bra	noRXdata	; if no bytes received yet
	dec	w13,w13		; patience counter
	bra	z,bl_lost_patience ; if end of reception (pause detected)
noRXdata:
	inc	w9,w9		; fast counter
	ifz
	inc	w8,w8		; slow counter
; fast or slow blinking
	mov	#0b0000000011110000,w0
	and	w0,w9,w0
	bra	nz,9f		; 15 of 16 cycles
	btsc	Flag2,#10	; #8 set = 1st byte in Bootloader received
	bra	2f		; if not 1st cycle (1st byte was received)
	btss	w8,#3		; bit #4 (slow blinking)
	bra	8f
	bra	7f
2:
	btss	w8,#0		; bit #1 (fast blinking)
	bra	9f
8:
	bset	LATC,#8		; LED LOAD on (1/16 duty cycle)
	bra	7f
9:
	bclr	LATC,#8		; LED LOAD off (15/16 duty cycle)
7:
	btss	DMAINT0,#DONEIF	; 1 = DMA Operation Completed Interrupt Flag
	bra	BootLoop ; ------------------------------> bootload loop
; Operation Completed, 1st halve of Rom received 4K bytes via UART
	bclr	DMAINT0,#DONEIF	; 1 = DMA Operation Completed Interrupt Flag
	dec	w12,w12
	bra	z,bl_lost_patience ; number of received bytes overflow
	call	burn1k		; burn 1 K block received
	bra	BootLoop ; --------> bootload loop after one 1K instr segment written
;------------------------
bl_lost_patience:
	bclr	DMAINT0,#DONEIF	; 1 = DMA Operation Completed Interrupt Flag
	bclr	DMACON,#15	; Bit 15 = DMAEN (DMA Enable)
	bclr	DMACH0,#0	; Bit 0: CHEN (Channel Enable)
	call	burn1k		; burn last block received, even incomplete
	bra	bootload	; end of reception (pause detected)

;------------------------------------------------------------------------

