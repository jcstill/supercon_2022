
;------------------------------------------------------------------------------------
;  NOTE: FIRMWARE IN THIS PAGE IS IN PROTECTED BOOT SEGMENT, AND CAN NOT BE MODIFIED 
;------------------------------------------------------------------------------------

;  IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII	
;  IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII	
;  IIIIII							  IIIIII	
;  IIIIII		I N I T I A L I Z A T I O N		  IIIIII	
;  IIIIII							  IIIIII	
;  IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII	
;  IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII	

boot_initialize:
	bclr	INTCON2,#GIE		; global int disable

; -------------------------------------	; --- oscilator -----------------------
	mov	#0b0000000100000000,w0	; FRC with PLL, /1, PLL always active
	mov	w0,CLKDIV

; -------------------------------------	; --- REMAPABLE PINS ------------------
	mov	#25,w0			; RP25 PORT_C9...
	mov.b	WREG,RPINR0+1		; ...to Ext_Int_1

	mov	#26,w0			; RP26 PORT  (A0)... (0x3f to disable RX input)
	mov.b	WREG,RPINR18		; ...to U1_RX
;	mov	#6,w0			; RP6  PORT  (B6)...  (0x3f to disable RX input)
;	mov.b	WREG,RPINR18		; ...to U1_RX
; both pins can be Tx (why not?)
	mov	#3,w0			; #3=U1_TX...     (0x00 to return to default OUT port)
	mov.b	WREG,RPOR13+1		; ...to RP27 (A1)
;	mov	#3,w0			; #3=U1_TX...     (0x00 to return to default OUT port)
;	mov.b	WREG,RPOR3+1		; ...to RP7  (B7)

; -------------------------------------	; --- TIMERS --------------------------
	mov	#0b0010000000010000,w0	; stops in sleep mode, prescaler = 1:8
	mov	w0,T1CON		; TIMER 1 = 3300 Hz (display matrix rfsh)
	mov	#514-40-1,w0		; heartbeat def period 16M/8/514=3888Hz, /16=243Hz
	mov	w0,PR1

	mov	#0b0010000000000000,w0	; prescaler bits 5,4 (CLOCK period)
	mov	w0,T2CON		; TIMER 2 = instruction clock (no interrupt, IFS0,#7 only!)
	mov	#10-1,w0		; instruction def period
	mov	w0,PR2

	mov	#0b0010000000000000,w0	; prescaler bits 5,4 (SYNC period)
	mov	w0,T3CON		; TIMER 3 = user's sync (interrupt only sets RdFlags,#0)
	mov	#10-1,w0		; sync def period
	mov	w0,PR3

; -------------------------------------	; --- UART -----------------------------
	mov	#0b0010100000001000,w0	; BRGH=1, UART disabled, RTS/CTS disabled, BRGH set
	mov	w0,U1MODE		; UART1 je za infrared
	mov	#0b0000000000000000,w0	; TX idle mode = high
	mov	w0,U1STAL
	mov	#417-1,w0		; 16 M / 4 / 417 = 9600 Baud
	mov	w0,U1BRG
	bset	U1MODE,#15		; enable UART 1
	bset	U1STAL,#URXEN		; enable RX 1 (must be after UART enable)
	bset	U1STAL,#UTXEN		; enable TX 1 (must be after UART enable)

; ------------------------------------- ; --- INTERRUPTS -----------------------
	bclr	RCON,#SWDTEN		; disable WDT
	bset	INTCON1,#NSTDIS		; disable interrupt nesting

; -------------------------------------	; --- PORTS ----------------------------
	clr	ANSA
	clr	ANSB
	clr	ANSC

	mov	#0b0000000000011100,w0	; pull-ups
	mov	w0,IOCPUA
	mov	#0b0000001000000000,w0	; pull-ups
	mov	w0,IOCPUC

	mov	#0b0000000000000110,w0
	mov	w0,LATA
	mov	#0b0000000011110000,w0
	mov	w0,LATB
	mov	#0b0000000000000000,w0
	mov	w0,LATC
	mov	#0b0000000000011101,w0
	mov	w0,TRISA
	mov	#0b0000000011110000,w0
	mov	w0,TRISB
	mov	#0b0000001000000000,w0
	mov	w0,TRISC
	return

;------------------------------------------------------------------------------------
;-----------------------------  S U B S  --------------------------------------------
;------------------------------------------------------------------------------------

boot_set_decoder_to_w1:
	mov	#0b1111100001111111,w0
	and	LATA		; clear LATA bits 7 8 9 10
	sl	w1,#7,w0
	ior	LATA		; decoder state is at LATA,#7...#10 control
	inc	w1,w1
	return

rotate_ram:
	mov	#0x0F0F,w0
	mov	#Ram+0,w1
	mov	#16,w2
2:
	lsr	[w1],[w1]	; (skips this line without NOP)
	btsc	[w1],#0
	bset	[w1],#3
	btsc	[w1],#8
	bset	[w1],#11
	and	w0,[w1],[w1++]
	nop			; * * *   MCU BUG FIX  ???   * * *
	dec	w2,w2
	bra	nz,2b
	return

;										
;-------
boot_rxtx_to_io2:
; B7 output now (Tx)
	bclr	U1MODE,#15		; disable UART 1
	bclr	TRISB,#7
; Peripheral Pin Select <---- Rx and Tx pins to I/O connector
	mov	#6,w0		; RP6  PORT  (B6)...  (0x3f to disable RX input)
	mov.b	WREG,RPINR18	; ...to U1_RX
	mov	#3,w0		; #3=U1_TX...     (0x00 to return to default OUT port)
	mov.b	WREG,RPOR3+1	; ...to RP7  (B7)
; disable old pin for Tx
;	clr.b	RPOR13+1	; ...to RP27 (A1) (0x00 to return to default OUT port)
	bset	Flag2,#3	; #3 Previous RxTxPos
	bset	U1MODE,#15		; enable UART 1
	clr.b	Received
	mov	#RX_buf,w0
	mov	w0,RXWR
	mov	w0,RXRD
	return
;------------------------------------------------------------------------
boot_w0ms2:
	repeat	#16000-1
	nop
	dec	w0,w0
	bra	nz,w0ms2
	return

;-----------------------------------------------------------------------------
;----------------------                              -------------------------
;----------------------        EE SUBROUTINES        -------------------------
;----------------------                              -------------------------
;-----------------------------------------------------------------------------

boot_eecalc_addr_w0:	; USER PROGRAM SAVE/LOAD (8K=0x2000 by blocks, 0x1A000...)
			; w0 × 0x1000 + 0x1A000 ----> w6:w7 (end must be < 2AFFE)
			; one record 0x1000 bytes, start 0x1A000, end 0x2A000
	btsc	Flag2,#6	; #6 EE subroutines adjust to Boot
	bra	eecalc_page
	mov	#0x1000,w1
	mul.uu	w0,w1,w6	; w6:w7 = FlashAddr × 0x1000
	mov	#0xA000,w0
	mov	#0x0001,w1	; w0:w1 = 0x0001:A000 start of 1st flash save block
	add	w6,w0,w6
	addc	w7,w1,w7
	return
;-----------------------
eecalc_page:		; BOOTLOAD (4K=0x800 by blocks, 0x1000,0x1800...)
			; w0 × 0x800 + 0x1000 ---> w6:w7 (one record 0x800 = 2K by)
			; w0 = page, 0...23 (for Firmware Backup, w0 = 24...47)
	mov	#0x800,w1
	mul.uu	w0,w1,w6	; w6:w7 = FlashAddr × 0x800
	mov	#0x1000,w0
	mov	#0x0000,w1	; w0:w1 = 0x0000:1000 start of 1st flash save block
	add	w6,w0,w6
	addc	w7,w1,w7
	return
;----------------------------------------------------------------------------------
boot_eesavew0:		; data (@Rom) ----> flash w0
	call	eecalc_addr_w0
	bclr	INTCON2,#GIE	; global int disable
	clr.b	LATC		; all LEDS off
	clr.b	LATB+1		; all LEDS off
	bclr	LATC,#8		; anode 17 off
	mov	#0b1111100001111111,w0
	and	LATA		; clear LATA bits 7 8 9 10
	mov	#0b0000011010000000,w0	; 13 = LED SAVE
	btsc	Flag2,#9	; #9 set = last command LOAD
	mov	#0b0000011100000000,w0	; 14 = LED LOAD
	ior	LATA		; decoder state = 13 or 14 (there is led SAVE or LOAD)
	push.d	w6	   ; \    w6, w7
	call	eeerase_1K	; erase block of 1K instructions (2K user pgm bytes + 1K high)
	btss	Flag2,#6	; only one page for BOOT !
	call	eeerase_1K	; erase block of 1K instructions (2K user pgm bytes + 1K high)
	pop.d	w6	   ; /    w6, w7
	mov	#Rom,w2
	btsc	Flag2,#7	; #7 EE save from 2nd halve of Rom area (starting at 0x3000)
	mov	#Rom+0x1000,w2
	mov	#16,w8		; 16× for User's Program Save (4K chunks)
	btsc	Flag2,#6
	mov	#8,w8		; only 8× for BOOT ! (2K chunks)
8:                          ; <----------
	call	eeburn_128	; burn
	btss	Flag2,#6	; #6 EE Boot
	btsc	Flag2,#10	; #8 set = 1st byte in Bootloader received
	bset	LATC,#8		; anode 17 on
	repeat	#8000-1		; led SAVE ON time	
	nop
	bclr	LATC,#8		; anode 17 off
	mov	#8,w0		; led SAVE OFF time	
	call	w0ms2
	dec.b	w8,w8
	bra	nz,8b       ; ---------->
	clr	TBLPAG
	btss	Flag2,#6	; #6 EE Boot mode
	bset	INTCON2,#GIE	; global int enable
	mov	#0x1000,w0
	mov	w0,NVMCON
	return

;---------------------------------------------------------------------------
boot_eeload:		; flash (w0×0x1000+0x1A000 or w0×0x800+0x1000) --> data (Rom or Rom+0x1000)
	call	eecalc_addr_w0
	mov	w7,TBLPAG
	mov	#Rom,w2
	btsc	Flag2,#7	; #7 EE load to second halve of Rom area (start at 0x3000)
	mov	#Rom+0x1000,w2
	btsc	Flag2,#6
	bra	case_read_bootload
	mov	#0x800,w3	; 2048×24 bits for User's Program Load
3:			;  <---- RD WR ----------------------------------
	tblrdh.b [w6],w1	; 8    w1 = high 2n xxxx xxxx BA98 ba98	
	tblrdl.b [w6++],w0	; 8    w0 = low by  xxxx xxxx 7654 3210	
	swap	w0		;      w0 = low by  7654 3210 xxxx xxxx	
	mov.b	w1,w0		;      w0 = low by  7654 3210 BA98 ba98	
	ior.b	#0xF0,w0	;      w0 = low by  7654 3210 FFFF ba98 (complemented data)
	swap	w0		;      w0 = low by  FFFF ba98 7654 3210	
	mov	w0,[w2++]	;   12	 write 1st program word		

	tblrdl.b [w6++],w0	; 8    w0 = low by  xxxx xxxx 7654 3210	
	swap	w0		;      w0 = low by  7654 3210 xxxx xxxx	
	swap.b	w1		;      w1 = high 2n xxxx xxxx ba98 BA98	
	mov.b	w1,w0		;      w0 = low by  7654 3210 ba98 BA98	
	ior.b	#0xF0,w0	;      w0 = low by  7654 3210 FFFF BA98 (complemented data)
	swap	w0		;      w0 = low by  FFFF BA98 7654 3210	
	mov	w0,[w2++]	;   12	 write 2nd program word		

	dec	w3,w3
	bra	nz,3b	; ---------------------------------------------->
	clr	TBLPAG
	return
;			
case_read_bootload:
	mov	#0x400,w3	; only 1024×24 bits for Boot !
3:			; \
	tblrdh.b [w6],w1	; read bits 16-23
	nop
	tblrdl.b [w6++],[w2++]	; bits 0-7
	nop
	tblrdl.b [w6++],[w2++]	; bits 8-15
	nop
	mov.b	w1,[w2++]	; write bits 16-23
	nop
	clr.b	[w2++]		; skip 4th dummy byte
	nop
	dec	w3,w3
	bra	nz,3b	; /
	clr	TBLPAG
	return

;-------------------------------
boot_eemeasure:		; occupance of w0 (0...15), output w2 bits #7...#0  (2.3 ms)
	bclr	Flag2,#6	; #6 EE subroutines adjust to Boot
	call	eecalc_addr_w0	; w6:w7
	mov	w7,TBLPAG
	mov	#0x8,w3		; w3 loop counter, 8 steps, 0x200 words = 1 bit ea
	mov	#0,w2		; w2 measured occupance (8 bits)
3:			   ; \
	mov	#0x100,w4	; w4 inner loop counter
	setm	w1
4:			   ; \\
	tblrdh.b [w6],w0	; w1 = high 2n xxxx xxxx BA98 ba98
	and.b	w1,w0,w1
	tblrdl [w6++],w0	; w0 = low by  7654 3210 7654 3210
	and	w1,w0,w1
	dec	w4,w4
	bra	nz,4b	   ; //
	sl	w2,w2
	inc	w1,w1
	ifnz
	bset	w2,#0
	dec	w3,w3
	bra	nz,3b	   ; /
	clr	TBLPAG
	return

; ------------------------------ PAGE ERASE -----------------------------
boot_eeerase_1K:	; erase block of 1K instructions (2K user pgm bytes + 1K high)
	mov	#0x5003,w0
	mov	w0,NVMCON
; Load the address of the page to be erased into the NVMADR register pair
	mov	w6,NVMADR
	mov	w7,NVMADRU
	mov	#0x800,w0
	add	w6,w0,w6
	addc	w7,#0,w7
; Set the WR bit
	bra	eeiniseq_128

; ------------------------------- ROW BURN ------------------------------
boot_eeburn_128:	; burn row of 128 instructions, source ptr w2, dest w6:w7
; Set NVMCON for 128 instruction words (256 user pgm words), rd ptr w2, wr ptr w6:w7
	mov	#0x5002,w0
	mov	w0,NVMCON
; Set the NVMADRU/NVMADR reg pair to starting address
	mov	w6,NVMADR
	mov	w7,NVMADRU
	add	#256,w6		; prepare for the next row programming
	addc	w7,#0,w7
; Initialize the TBLPAG register for writing to the latches
	mov	#0xfa,w0	; pgm latches are mapped at 0xFA0000-0xFA00FE
	mov	w0,TBLPAG
	clr	w1		; low address
; Set up write data address pointer, W2 point to data in RAM
	mov	#128,w3		; loop counter
	btsc	Flag2,#6	; #6 EE Boot
	bra	case_boot
3:			; read 8+4+8+4 data bits, write 8+8+8 bits to latches
			;	 rd wr				
	tblwtl.b [w2++],[w1++]	; 8 8  [w1] = low by  7654 3210
	ze	[w2++],w4	; 4    w4 = 0000 0000 xxxx ba98
	and	#0x0F,w4	;      w4 = 0000 0000 0000 ba98
	tblwtl.b [w2++],[w1--]	; 8 8  [w1] = mid by  7654 3210
	ze	[w2++],w5	; 4    w5 = 0000 0000 xxxx BA98
	sl	w5,#4,w5	;      w4 = 0000 xxxx BA98 0000
	ior	w4,w5,w4	;      w4 = 0000 xxxx BA98 ba98 (low word=low nib)
	tblwth.b w4,[w1]	;   8  [w1] = high 2n BA98 ba98
	inc2	w1,w1
	dec	w3,w3
	bra	nz,3b
; Set the WR bit
eeiniseq_128:
	disi	#5		; DI for next 5 instructions
	mov.b	#0x55,w0
	mov	w0,NVMKEY	; Write the 1st key
	mov.b	#0xAA,w0
	mov	w0,NVMKEY	; Write the 2nd key
	bset	NVMCON,#WR	; Start the programming sequence
	nop			; Required delay
	nop			; Required delay
	nop			; Required delay
	btsc	NVMCON,#15	; and wait for it to be completed
	bra	$-2
	clr	TBLPAG
	return
;	
case_boot:
4:			; read 8+8+8 data bits + skip, write 8+8+8 bits to latches
	tblwtl.b [w2++],[w1++]	; bits 0-7
	nop
	tblwtl.b [w2++],[w1--]	; bits 8-15
	nop
	tblwth.b [w2++],[w1]	; bits 16-23
	nop
	inc2	w1,w1		; write ptr +2
	inc	w2,w2		; skip 4th dummy byte
	dec	w3,w3
	bra	nz,4b
; Set the WR bit
	bra	eeiniseq_128
;------------------------------------------------------------------------
boot_burn1k:		; burn 1K instructions, 3K bytes + 1K dummy 0's
	mov	#Rom,w0
	mov	#Rom+0x1000,w1
	repeat	#0x800-1	; 2048 ×
	mov	[w0++],[w1++]	; move 4K bytes from Rom 1st halve to 2nd halve
	mov	#Rom,w0
	repeat	#0x800-1	; 2048 ×
	setm	[w0++]		; clear	1st halve
	mov	w11,w0
	call	eesavew0	; burn 1024 instructions (×24 bits) from Rom 2nd halve
	inc	w11,w11
	return

