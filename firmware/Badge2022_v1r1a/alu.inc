
alu_ept:	; <-------------------------------------------
; prepare A16 (set ALU and keep Bin/Sel)
	mov	#0b1000000010000000,w0	; keep BIN and SEL only
	and	A16
	mov	#0b0000100000001000,w0	; set ALU and -CLK
	ior	A16
; restore ALU params
	mov	AluTempOpcode,w0
	mov	w0,Opcode	; Opcode
	mov	AluTempOperX,w0
	mov	w0,OperX	; OperX
	mov	AluTempOperY,w0
	mov	w0,OperY	; OperY
	mov	AluTempAtemp,w0
	mov	w0,Atemp	; Accu temp
	mov	AluTempw14,w14	; Accu Out
	mov	AluTempw13,w13	; Accu In
; repair all blinking (non-existing) combinations
	mov	#8,w1
	mov	Opcode,WREG	; affects Z flag
	bra	z,eight_bit_opc	; if eight bit opcode
	cp	w0,#9
	ifc			; if Opcode >= 9...
	mov	w1,Opcode	; ...then fix it to 8
	bra	fixed
eight_bit_opc:
	mov	#2,w1
	mov	OperX,w0
	cp	w0,#2		; 2 = INC RY, legal instruction
	bra	z,fixed
	cp	w0,#3		; 3 = DEC RY, legal instruction
	bra	z,fixed
	cp	w0,#13		; 13 = RRC RY, legal instruction
	ifnz
	mov	w1,OperX	; if unlegal, fix it to 2
fixed:
; restore ALU flags
	mov	AluTempVflag,w0
	mov	w0,Vflag
	mov	AluTempZflag,w0
	mov	w0,Zflag
	mov	AluTempCflag,w0
	mov	w0,Cflag
	bclr	A16,#b_carry
	btsc	Cflag,#2
	bset	A16,#b_carry
; initialize ALU
	clr	Mode			; 0 = ALU
	clr	BlinkFlag
	clr	TBLPAG
	mov	w9,PgmTempPMAddr
	clr.b	Page
	clr	Stack
	clr	w9
	bset	Flag,#2		; #2 set if Speed > 0 or if Mode <> 2
	mov	#1,w0
	cp0.b	Speed
	ifz
	mov.b	WREG,Speed
	mov	#0b011100000,w0	; autorpt for Opcode, OperX, OperY
	mov	w0,AutorptFlag
;------------------------------
alu_farm:			; <------------
	bset	IEC0,#11	; enable RX1 interrupt
; IN register service
	mov	#PORTB,w0
	ze	[w0],w0
	lsr	w0,#4,w0
	mov.b	WREG,Ram+11	; IN register
; read instruction opcode
	mov	Opcode,w0	; read instruction opcode here...
	mov	OperX,w11	; ...or here
	mov	OperY,w12
	cp0	w0		; w0 = Opcode
	bra	z,jpt_8alu	; if it's 8-bit opcode
	mov	#OperX,w7	; default result write address
; w0 = table pos
	bra	w0
;jp table
	bra	alu_cmd	   ; impossible instruction
	bra	alu_A1	   ; ADD  RX,RY
	bra	alu_A2	   ; ADC  RX,RY
	bra	alu_A3	   ; SUB  RX,RY
	bra	alu_A4	   ; SBB  RX,RY
	bra	alu_A5	   ; OR   RX,RY
	bra	alu_A6	   ; AND  RX,RY
	bra	alu_A7	   ; XOR  RX,RY
	bra	alu_A8	   ; MOV  RX,RY
	bra	alu_blink1 ; MOV  RX,RY   (non-existing in ALU mode)
	bra	alu_blink1 ; MOV  RX,N    (non-existing in ALU mode)
	bra	alu_blink1 ; MOV  [XY],R0 (non-existing in ALU mode)
	bra	alu_blink1 ; MOV  R0,[XY] (non-existing in ALU mode)
	bra	alu_blink1 ; MOV  [NN],R0 (non-existing in ALU mode)
	bra	alu_blink1 ; MOV  R0,[NN] (non-existing in ALU mode)
	bra	alu_blink1 ; JR   NN      (non-existing in ALU mode)

jpt_8alu:
	mov	#Dummy,w7	; default result write address (R0 = Accu in)
; w0 = table pos
	bra	w11
;jp table
	bra	alu_blink2   ; CP   R0,N    (uses R0)
	bra	alu_blink2   ; ADD  R0,N    (uses R0)
	bra	alu_G2	     ; INC  RY
	bra	alu_G3	     ; DEC  RY
	bra	alu_blink2   ; DSZ  RY      (non-existing in ALU mode)
	bra	alu_blink2   ; OR   R0,N    (uses R0)
	bra	alu_blink2   ; AND  R0,N    (uses R0)
	bra	alu_blink2   ; XOR  R0,N    (uses R0)
	bra	alu_blink2   ; EXR  N       (non-existing in ALU mode)
	bra	alu_blink2   ; BIT  RG,M    (uses R0)
	bra	alu_blink2   ; BSET RG,M    (uses R0)
	bra	alu_blink2   ; BCLR RG,M    (uses R0)
	bra	alu_blink2   ; BTG  RG,M    (uses R0)
	bra	alu_G13	     ; RRC  RY
	bra	alu_blink2   ; RET  RY,N    (non-existing in ALU mode)
	bra	alu_blink2   ; SKIP F,M     (non-existing in ALU mode)
;...
; --------------------------------------------------------------------

alu_blink1:			; if non-existent, instruction column 1 should blink
	bset	BlinkFlag,#0
	bclr	BlinkFlag,#1
	bra	alu_wait_key
alu_blink2:			; if non-existent, instruction column 2 should blink
	bclr	BlinkFlag,#0
	bset	BlinkFlag,#1
	bra	alu_wait_key

;-------------------------------
alu_cmd:			; w11 = Transfer Flag
alu_wait_key:
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bset	Flag2,#8	; #8 set = first button pressed, Ver/Rev deleted
	mov	#PORTB,w0
	ze	[w0],w0
	lsr	w0,#4,w0
	mov.b	WREG,Ram+11	; IN register

	btss	Flag,#1		; #1 handshaking flag for buttons
	bra	alu_wait_key
	bclr	Flag,#1
	btsc	Flag2,#8	; #8 set = first button pressed, Ver/Rev deleted
	bra	not_first
; first keypress, clear Ver/Rev
	bset	Flag2,#8	; #8 set = first button pressed, Ver/Rev deleted
	clr	Ram+0x00
	clr	Ram+0x02
	clr	Ram+0x04
	clr	Ram+0x10
	clr	Ram+0x12
	clr	Ram+0x14
	bra	alu_wait_key
not_first:
	bclr	BlinkFlag,#5	; switch off "SAVE blink" flag
	bclr	BlinkFlag,#7	; switch off "LOAD blink" flag
	mov	Just,w0
	cp	w0,#5
	ifc
	mov	#5,w0
	bra	w0
; jump table FOR BUTTONS
	bra	leave_alu_ept	; -------> next mode (Single Step)
	bra	button_CARRY
	bra	button_SAVE
	bra	button_LOAD
	bra	button_CLOCK
	bra	general_alu_5

; button entry points
; -----------------------------------------------------------------
button_CARRY:
	btg	A16,#b_carry
	bra	alu_farm

; -----------------------------------------------------------------
; TX format:
; Header 00 FF 00 FF A5 C3
; file length (word count, 16-bit length, low byte first)
; message body (words, low byte first)
; checksum (16-bit sum of FILE LENGTH WORD plus MESSAGE BODY words, low byte first)

button_SAVE:
	bclr	Flag2,#6	; #6 EE subroutines adjust to Boot
	bclr	Flag2,#7	; #7 EE load to second halve of Rom area (starting at 0x3000)
	bclr	Flag2,#9	; #9 clr = last command SAVE
	bset	A16,#b_save	; led SAVE on
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	save2tx		; ALT not pressed: save to serial port
; ALT pressed: save to Flash
	mov	FlashAddr,w0
	cp	w0,#15
	bra	nc,save2flash	; save to flash, loc 0...14
	bclr	A16,#b_save	; led SAVE off
	bra	alu_farm
save2tx:		; save to serial port
; save to serial port TX
	  call	  rxtx_to_io	  ; RxTx directed to IO connector
	  call	  def_U1BRG	  ; BRG = 9600
; measure the file length first
	mov	#0x1000,w5	; file length countdown (in words)
	mov	#Rom+0x1FFE,w3	; pointer backwards from the last program memory word
5:
	cp0	[w3--]		; test if word contains data
	bra	nz,6f		; if found meaningful content >0000
	dec	w5,w5
	bra	nz,5b
	bset	A16,#b_save	; LED SAVE on
	bset	BlinkFlag,#5	; LED SAVE blinks = Nothing to save
waitany1:
	btss	Flag,#1		; #1 = "key pressed" flag
	bra	waitany1
	bclr	Flag,#1		; #1 = "key pressed" flag
	bclr	A16,#b_save	; LED SAVE off
	bclr	BlinkFlag,#5	; no blink
	bra	saveload_exit	; nothing to save
6:				; found >0000
; send header 00 FF 00 FF A5 C3 
	mov	#0xFF00,w0
	call	txword		; ----> TX header 00 FF
	mov	#0xFF00,w0
	call	txword		; ----> TX header 00 FF
	mov	#0xC3A5,w0
	call	txword		; ----> TX header A5 C3
; send file length w5 (in words)
	bset	A16,#b_save	; "SAVE" LED on
	mov	w5,w4		; w4 CHS
	lsr	w5,#2,w2	; w2 file length/4, will be used in taskbar calculation
	ifz
	mov	#1,w2		; avoid  divide by zero
	mov	w5,w0
	call	txword		; ----> TX record length in words 16-bit (little endian)
	mov	#Rom,w3		; program memory pointer from the beginning
; send message body (pointer w3, length w5)
5:			  ; \
	mov	[w3++],w0	; read word from program memory
	add	w4,w0,w4	; update CHS
	call	txword		; ----> TX data 16-bit (little endian, hi byte 0000xxxx)
; taskbar ------
	sl	w5,#3,w6	; w6 = w5 × 8
	repeat	#17
	div.u	w6,w2		; w0:w1 (at transmit preogress, w0 decreases from 32 to 0)
	cp	w0,#31
	ifc
	mov	#31,w0		; clip w0 to 0...31
	lsr	w0,w0
	mov	#0xFFFF,w1
	sl	w1,w0,w1
	mov	w1,Taskbar

	dec	w5,w5
	bra	nz,5b	  ; /
; send checksum
	mov	w4,w0
	call	txword		; ----> TX CHS 16-bit (little endian)
	mov	#140,w0
	call	w0ms		; give some time to send the whole queue
; finale
	bclr	A16,#b_save	; "SAVE" LED off
	bra	saveload_exit

save2flash:		; save to flash
	mov	#0x1000,w5	; file length countdown (in words)
	mov	#Rom,w3		; pointer
5:
	cp0	[w3++]		; test if word contains data
	bra	nz,6f		; if found meaningful content >0000
	dec	w5,w5
	bra	nz,5b
	bset	A16,#b_save	; LED SAVE on
	bset	BlinkFlag,#5	; LED SAVE blinks = Nothing to save
waitany2:
	btss	Flag,#1		; #1 = "key pressed" flag
	bra	waitany2
	bclr	Flag,#1		; #1 = "key pressed" flag
	bclr	A16,#b_save	; LED SAVE off
	bclr	BlinkFlag,#5	; no blink
	bra	saveload_exit	; nothing to save
6:			; found meaningful content >0000
	call	com_rom
	mov	FlashAddr,w0
	call	eesavew0	; EEsave
	call	com_rom
	bclr	A16,#b_save
saveload_exit:
	  call	  rxtx_to_rxtxpos ; RxTx restored back to RxTxPos
	  call	  set_U1BRG	  ; BRG restored to SerCtrl (bits 210)
	clr	Taskbar
	bclr	Flag,#1		; reset handshaking flag for buttons (ignore it during save!)
	call	peek_flash	; record flash occupance  (37 ms)
	bra	alu_farm

; ---------------------------------------------------------------------------------------

button_LOAD:
	bclr	Flag2,#6	; #6 EE subroutines adjust to Boot
	bclr	Flag2,#7	; #7 EE load to second halve of Rom area (starting at 0x3000)
	bset	Flag2,#9	; #9 set = last command LOAD
	bset	A16,#b_load	; LED on
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	nc,ALTload	; if ALT pressed: load from Flash
LoadFromRx:		; load from serial port
	bclr	IEC0,#11	; disable RX1 interrupt
	  call	  rxtx_to_io	  ; RxTx directed to IO connector
	  call	  def_U1BRG	  ; BRG = 9600
; synchronize to header A5 C3
6:
	call	rxbyte_20s	; <---------- first RX attempt (timeout 20 sec)
	bra	c,rx_patience_1	; if patience lost or keypress
	mov	#16,w5		; 16 attempts before giving up
5:
	call	rxbyte_4s	; <---------- attempt to RX header 1 (A5) (timeout 4 sec)
	bra	c,rx_patience	; if patience lost or keypress
	sub	#0xA5,w0
	bra	z,found_header_a5
	dec	w5,w5
	bra	nz,5b
	mov	#0,w0
	bra	rx_patience	; giving up after 16 attempts to recognize header
found_header_a5:
	call	rxbyte_500ms	; <---------- RX header 2 (C3)
	bra	c,rx_patience	; if patience lost or keypress
	sub	#0xC3,w0
	bra	nz,6b		; one more chance
; header A5 C3 recognized, now get file length (low byte first)
	call	rxbyte_500ms	; <---------- RX file length low
	bra	c,rx_patience	; patience lost or keypress
	ze	w0,w3		; w3 first low byte (file length low)
	call	rxbyte_500ms	; <---------- RX file length high
	bra	c,rx_patience	; patience lost or keypress
	sl	w0,#8,w0	; high byte go high
	ior	w0,w3,w3	; w3 file length (in words)
	mov	w3,w5		; w5 = CHS
	lsr	w3,#2,w2	; w2 file length/4, will be used in taskbar calculation
	ifz
	mov	#1,w2		; avoid  divide by zero
	bra	z,only_timeout	; zero length - file too short (LOAD blinks)
	mov	#0x1001,w1	; max length+1 (number of words!)
	cp	w3,w1
	bra	c,only_timeout	; file too long (LOAD blinks)
; receive message body
	mov	#Rom,w6		; write pointer
3:			; <-------- RX loop
	call	rxbyte_500ms	; <---------- RX message body low
	bra	c,rx_patience	; if patience lost or keypress
	ze	w0,w4
	call	rxbyte_500ms	; <---------- RX message body high
	bra	c,rx_patience	; if patience lost or keypress
	sl	w0,#8,w0	; high byte go high
	ior	w0,w4,w0
	mov	w0,[w6++]	; WRITE RECEIVED WORD
	add	w0,w5,w5	; update CHS

; taskbar ------
	sl	w3,#3,w1	; w1 = w5 × 8
	repeat	#17
	div.u	w1,w2		; w0:w1 (at transmit preogress, w0 decreases from 32 to 0)
	cp	w0,#31
	ifc
	mov	#31,w0		; clip w0 to 0...31
	lsr	w0,w0
	mov	#0xFFFF,w1
	lsr	w1,w0,w1	; taskbar progress  UP ---> DOWN
	mov	w1,Taskbar
	dec	w3,w3
	bra	nz,3b		; loop
; receive checksum
	call	rxbyte_500ms	; <---------- RX CHS low
	bra	c,rx_patience	; if patience lost or keypress
	ze	w0,w4
	call	rxbyte_500ms	; <---------- RX CHS high
	bra	c,rx_patience	; if patience lost or keypress
	swap	w0
	ior	w0,w4,w0
	cp	w0,w5		; received CHS - calculated CHS
	bra	nz,rx_patience	; CHS does not match!
; RX finished, fill 00s to the end (pointer w6)
fill00:
	btsc	w6,#14		; #14 will be set when w6 reaches 0x4000
	bra	filled
	clr.b	[w6++]
	bra	fill00
filled:
	bclr	A16,#b_load
	bra	saveload_exit	; -----> RX LOAD FINISHED

;								
rx_patience_1:		; jumps here if timeout or any key pressed during waiting for RX
	cp0	w0
	bra	z,only_timeout	; if no key pressed or gave up after 16 attempts to recognize header
	bclr	A16,#b_load	; LED LOAD off
	bclr	BlinkFlag,#7	; no blinking
	bra	saveload_exit	; key pressed, just exit

;			
ALTload:		; ALT pressed, load from Flash
	bset	A16,#b_load
	call	com_rom
	mov	#15,w0
	cp.b	FlashAddr
	ifnz
	call	eesavew0	; just in case, saved on record 15
	call	com_rom

	mov	FlashAddr,w0
	call	eeload
	call	com_rom
	bclr	A16,#b_load
	bra	saveload_exit
;			
rx_patience:		; jumps here if timeout or any key pressed during waiting for RX
	cp0	w0		; w0=0 if no key pressed
	ifnz			; if key pressed, then LED LOAD blinks = TIMEOUT
only_timeout:
	bset	BlinkFlag,#7	; LED LOAD blinks = TIMEOUT
	bra	saveload_exit	; regular exit

;											
rxbyte_20s:		; RX byte ---> w0 (NC), patience 20s (C set if pat lost)
	mov	#20000,w1	; patience 20 s
	bra	1f
rxbyte_4s:		; RX byte ---> w0 (NC), patience 4s (C set if pat lost)
	mov	#4000,w1	; patience 4 s
	bra	1f
rxbyte_500ms:		; RX byte ---> w0 (NC), patience 500 ms (C set if pat lost)
	mov	#500,w1		; patience 500 ms
1:
	mov	#2240,w0	; inner loop 1 ms
2:
	btsc	Flag,#1		; #1 handshaking flag for buttons, set when any key pressed
	bra	button_press_rx	; if button pressed
	btsc	U1STAL,#URXDA	; bit set if RX data available
	bra	rx_arrived_2
	dec	w0,w0
	bra	nz,2b		; loop 7t × 2240 = 15680t = 1 ms (INT time estimated)
	dec	w1,w1
	bra	nz,1b		; loop 1 ms × 500 = 500 ms
;patience_lost
	mov	#0,w0		; flag no keys pressed (only timeout)
	setc			; Carry set = NOT OK
	return
;				
rx_arrived_2:
	mov.b	U1RXREG,WREG	; <---- get received byte (this resets U1STAL,#URXDA)
	ze	w0,w0
	clrc
	return
;				
button_press_rx:
	bclr	Flag,#1		; #1 handshaking flag for buttons, set when any key pressed
	mov	#1,w0		; flag key pressed (not timeout)
	setc			; Carry set = NOT OK
	return
; -----------------------------------------------------------------
button_CLOCK:
	mov	#3,w0
	and	BlinkFlag,WREG
	bra	nz,alu_farm	; no execution if the instruction is illegal

	bset	A16,#b_clk_key	; set CLK key
	call	transfer1	; uses w11 bits to decide flag transfer
1:
	inc	KeyRotors+8,WREG
	bra	nz,1b		; if not yet 0x11111111
	bclr	A16,#b_clk_key
	call	transfer2	; uses w11 bits to decide flag transfer

	mov.b	w13,[w7]	; write result to destination
	bra	alu_farm

; **********************************************************************************************
com_rom:		; complement Rom 0x1800 bytes
	mov	#Rom,w2
	mov	#0x1000,w1
1:
	com	[w2],[w2++]
	dec	w1,w1
	bra	nz,1b
	return

; --------------------------------------------------------------------------------------
; buttons Opcode - OperX - OperY - DataIn						
; --------------------------------------------------------------------------------------

general_alu_5:	; -----------------------------------------------------------------
; 13 buttons OPCODE 8... DATA IN
	call	alu_key_3
	bra	alu_farm
; ------------------------------
alu_key_3:		; Opcode-OperX-OperY --- Dim-Dummy-FlashAddr
	mov	#Opcode,w3
	mov	#OperX,w4
	mov	#OperY,w5
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	general		; if ALT not pressed
	mov	#Dimmer,w3
	mov	#SerCtrl,w4
	bclr.b	[w4],#3		; spec case: Buad Rate can't be greater than 7
	mov	#FlashAddr,w5
	call	general
	bclr.b	SerCtrl,#3	; spec case: Buad Rate can't be greater than 7
	mov.b	SerCtrl,WREG
	and	#7,w0
	call	set_U1BRG_from_w0	; set new Baud Rate
	return
limited_key_3:		; can't affect pgm data, only Dim Clock Page
	mov	#Dummy,w3
	mov	#Dummy,w4
	mov	#Dummy,w5
	bra	uni_key
general_key_3O:
	mov	#Opcode,w3
	mov	#OperX,w4
	mov	#OperY,w5
uni_key:
	mov	Mode,w0		; *** if PGM mode (disable 12 keys)
	cp.b	w0,#3		; *** if PGM mode (disable 12 keys)
	bra	z,general	; *** if PGM mode (disable 12 keys)

	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	general		; if ALT not pressed
	mov	#Sync,w3
	mov	#Speed,w4
	mov	#Page,w5
general:
; is it key 5...8?
	mov	Just,w0
	sub	#5,w0
	cp	w0,#4
	bra	nc,key0_3
; is it key 9...12?
	mov	w4,w3
	sub	#4,w0
	cp	w0,#4
	bra	nc,key0_3
; is it key 13...16?
	mov	w5,w3
	sub	#4,w0
	cp	w0,#4
	bra	nc,key0_3
; then it must be key 17 = Data In
	btg	A16,#b_bin
	bclr	A16,#b_sel
	btss	A16,#b_bin
	bset	A16,#b_sel
	return
; -------- 
key0_3:
	btss	A16,#b_bin
	bra	its_sel
; it's binary mode here
	cp0	w0
	ifz
	btg.b	[w3],#3		; key 8 in binary mode
	cp	w0,#1
	ifz
	btg.b	[w3],#2		; key 4 in binary mode
	cp	w0,#2
	ifz
	btg.b	[w3],#1		; key 2 in binary mode
	cp	w0,#3
	ifz
	btg.b	[w3],#0		; key 1 in binary mode
	return
its_sel:
	ze	[w3],w4
	cp0	w0
	ifz
	sub	#4,w4		; key ---- in select mode
	cp	w0,#1
	ifz
	dec	w4,w4		; key - in select mode
	cp	w0,#2
	ifz
	inc	w4,w4		; key + in select mode
	cp	w0,#3
	ifz
	add	#4,w4		; key ++++ in select mode
; clip w3 to 0...15
	btsc	w4,#15		; if w4<0...
	clr	w4		; ...then w4=0
	btsc	w4,#4		; if w4>15...
	mov	#15,w4		; ...then w4=15
	mov.b	w4,[w3]
	return

; --------------------------------------------------------------------------------------
; Transfers through Accu and Flags  In - Temp - Out					
; --------------------------------------------------------------------------------------
transfer1:		; in the first clock semiphase or when STEP is depressed
	bclr	A16,#b_noclk_sch
	bset	A16,#b_clk_sch	; set CLK schematic
	bra	w6
;...
	return			; no transfers if w6=0000
	bra	n0001
	bra	n0010
	bra	n0011
	bra	n0100
	bra	n0101
	bra	n0110
	bra	n0111
	bra	n1000
	bra	n1001
	bra	n1010
	bra	n1011
	bra	n1100
	bra	n1101
	bra	n1110
	bra	n1111
; -------------------
n0001:
	mov	w13,Atemp	; 0 transfer ACCU in to ACCU temp
	return
n0010:
	bclr	Cflag,#1	; 1 transfer CARRY in to CARRY temp
	btsc	Cflag,#0
	bset	Cflag,#1
	return
n0011:
	bclr	Cflag,#1	; 1 transfer CARRY in to CARRY temp
	btsc	Cflag,#0
	bset	Cflag,#1
	mov	w13,Atemp	; 0 transfer ACCU in to ACCU temp
	return
n0100:
	bclr	Zflag,#1	; 2 transfer ZERO in to ZERO temp
	btsc	Zflag,#0
	bset	Zflag,#1
	return
n0101:
	bclr	Zflag,#1	; 2 transfer ZERO in to ZERO temp
	btsc	Zflag,#0
	bset	Zflag,#1
	mov	w13,Atemp	; 0 transfer ACCU in to ACCU temp
	return
n0110:
	bclr	Zflag,#1	; 2 transfer ZERO in to ZERO temp
	btsc	Zflag,#0
	bset	Zflag,#1
	bclr	Cflag,#1	; 1 transfer CARRY in to CARRY temp
	btsc	Cflag,#0
	bset	Cflag,#1
	return
n0111:
	bclr	Zflag,#1	; 2 transfer ZERO in to ZERO temp
	btsc	Zflag,#0
	bset	Zflag,#1
	bclr	Cflag,#1	; 1 transfer CARRY in to CARRY temp
	btsc	Cflag,#0
	bset	Cflag,#1
	mov	w13,Atemp	; 0 transfer ACCU in to ACCU temp
	return
n1000:
	bclr	Vflag,#1	; 3 transfer OVF in to OVF temp
	btsc	Vflag,#0
	bset	Vflag,#1
	return
n1001:
	bclr	Vflag,#1	; 3 transfer OVF in to OVF temp
	btsc	Vflag,#0
	bset	Vflag,#1
	mov	w13,Atemp	; 0 transfer ACCU in to ACCU temp
	return
n1010:
	bclr	Vflag,#1	; 3 transfer OVF in to OVF temp
	btsc	Vflag,#0
	bset	Vflag,#1
	bclr	Cflag,#1	; 1 transfer CARRY in to CARRY temp
	btsc	Cflag,#0
	bset	Cflag,#1
	return
n1011:
	bclr	Vflag,#1	; 3 transfer OVF in to OVF temp
	btsc	Vflag,#0
	bset	Vflag,#1
	bclr	Cflag,#1	; 1 transfer CARRY in to CARRY temp
	btsc	Cflag,#0
	bset	Cflag,#1
	mov	w13,Atemp	; 0 transfer ACCU in to ACCU temp
	return

n1100:
	bclr	Vflag,#1	; 3 transfer OVF in to OVF temp
	btsc	Vflag,#0
	bset	Vflag,#1
	bclr	Zflag,#1	; 2 transfer ZERO in to ZERO temp
	btsc	Zflag,#0
	bset	Zflag,#1
	return
n1101:
	bclr	Vflag,#1	; 3 transfer OVF in to OVF temp
	btsc	Vflag,#0
	bset	Vflag,#1
	bclr	Zflag,#1	; 2 transfer ZERO in to ZERO temp
	btsc	Zflag,#0
	bset	Zflag,#1
	mov	w13,Atemp	; 0 transfer ACCU in to ACCU temp
	return
n1110:
	bclr	Vflag,#1	; 3 transfer OVF in to OVF temp
	btsc	Vflag,#0
	bset	Vflag,#1
	bclr	Zflag,#1	; 2 transfer ZERO in to ZERO temp
	btsc	Zflag,#0
	bset	Zflag,#1
	bclr	Cflag,#1	; 1 transfer CARRY in to CARRY temp
	btsc	Cflag,#0
	bset	Cflag,#1
	return
n1111:
	bclr	Vflag,#1	; 3 transfer OVF in to OVF temp
	btsc	Vflag,#0
	bset	Vflag,#1
	bclr	Zflag,#1	; 2 transfer ZERO in to ZERO temp
	btsc	Zflag,#0
	bset	Zflag,#1
	bclr	Cflag,#1	; 1 transfer CARRY in to CARRY temp
	btsc	Cflag,#0
	bset	Cflag,#1
	mov	w13,Atemp	; 0 transfer ACCU in to ACCU temp
	return
;-----------------------------------------------------------------------------------

transfer2:		; in the second clack semiphase or when STEP is released
	bclr	A16,#b_clk_sch
	bset	A16,#b_noclk_sch ; set NOCLK schematic
	bra	w6
;...
	return
	bra	z0001
	bra	z0010
	bra	z0011
	bra	z0100
	bra	z0101
	bra	z0110
	bra	z0111
	bra	z1000
	bra	z1001
	bra	z1010
	bra	z1011
	bra	z1100
	bra	z1101
	bra	z1110
	bra	z1111
; -------------------
z0001:
	mov	Atemp,w14	; 0 transfer ACCU data to ouptput
	return
z0010:
	bclr	Cflag,#2	; 1 transfer carry temp to Carry out...
	btsc	Cflag,#1
	bset	Cflag,#2
	bclr	A16,#b_carry	; 1 ...and to Carry button LED
	btsc	Cflag,#2	; 1 ...and to Carry button LED
	bset	A16,#b_carry	; 1 ...and to Carry button LED
	return
z0011:
	bclr	Cflag,#2	; 1 transfer carry temp to Carry out...
	btsc	Cflag,#1
	bset	Cflag,#2
	bclr	A16,#b_carry	; 1 ...and to Carry button LED
	btsc	Cflag,#2	; 1 ...and to Carry button LED
	bset	A16,#b_carry	; 1 ...and to Carry button LED
	mov	Atemp,w14	; 0 transfer ACCU data to ouptput
	return
z0100:
	bclr	Zflag,#2	; 2 transfer zero temp to zero out...
	btsc	Zflag,#1
	bset	Zflag,#2
	return
z0101:
	bclr	Zflag,#2	; 2 transfer zero temp to zero out...
	btsc	Zflag,#1
	bset	Zflag,#2
	mov	Atemp,w14	; 0 transfer ACCU data to ouptput
	return
z0110:
	bclr	Zflag,#2	; 2 transfer zero temp to zero out...
	btsc	Zflag,#1
	bset	Zflag,#2
	bclr	Cflag,#2	; 1 transfer carry temp to Carry out...
	btsc	Cflag,#1
	bset	Cflag,#2
	bclr	A16,#b_carry	; 1 ...and to Carry button LED
	btsc	Cflag,#2	; 1 ...and to Carry button LED
	bset	A16,#b_carry	; 1 ...and to Carry button LED
	return
z0111:
	bclr	Zflag,#2	; 2 transfer zero temp to zero out...
	btsc	Zflag,#1
	bset	Zflag,#2
	bclr	Cflag,#2	; 1 transfer carry temp to Carry out...
	btsc	Cflag,#1
	bset	Cflag,#2
	bclr	A16,#b_carry	; 1 ...and to Carry button LED
	btsc	Cflag,#2	; 1 ...and to Carry button LED
	bset	A16,#b_carry	; 1 ...and to Carry button LED
	mov	Atemp,w14	; 0 transfer ACCU data to ouptput
	return
z1000:
	bclr	Vflag,#2	; 3 transfer overflow temp to overflow out...
	btsc	Vflag,#1
	bset	Vflag,#2
	bclr.b	RdFlags,#1	; 3 ...and to readable V flag pos
	btsc	Vflag,#1	; 3 ...and to readable V flag pos
	bset.b	RdFlags,#1	; 3 ...and to readable V flag pos
	return
z1001:
	bclr	Vflag,#2	; 3 transfer overflow temp to overflow out...
	btsc	Vflag,#1
	bset	Vflag,#2
	bclr.b	RdFlags,#1	; 3 ...and to readable V flag pos
	btsc	Vflag,#1	; 3 ...and to readable V flag pos
	bset.b	RdFlags,#1	; 3 ...and to readable V flag pos
	mov	Atemp,w14	; 0 transfer ACCU data to ouptput
	return
z1010:
	bclr	Cflag,#2	; 1 transfer carry temp to Carry out...
	btsc	Cflag,#1
	bset	Cflag,#2
	bclr	A16,#b_carry	; 1 ...and to Carry button LED
	btsc	Cflag,#2	; 1 ...and to Carry button LED
	bset	A16,#b_carry	; 1 ...and to Carry button LED
	return
z1011:
	bclr	Vflag,#2	; 3 transfer overflow temp to overflow out...
	btsc	Vflag,#1
	bset	Vflag,#2
	bclr.b	RdFlags,#1	; 3 ...and to readable V flag pos
	btsc	Vflag,#1	; 3 ...and to readable V flag pos
	bset.b	RdFlags,#1	; 3 ...and to readable V flag pos
	bclr	Cflag,#2	; 1 transfer carry temp to Carry out...
	btsc	Cflag,#1
	bset	Cflag,#2
	bclr	A16,#b_carry	; 1 ...and to Carry button LED
	btsc	Cflag,#2	; 1 ...and to Carry button LED
	bset	A16,#b_carry	; 1 ...and to Carry button LED
	mov	Atemp,w14	; 0 transfer ACCU data to ouptput
	return
z1100:
	bclr	Vflag,#2	; 3 transfer overflow temp to overflow out...
	btsc	Vflag,#1
	bset	Vflag,#2
	bclr.b	RdFlags,#1	; 3 ...and to readable V flag pos
	btsc	Vflag,#1	; 3 ...and to readable V flag pos
	bset.b	RdFlags,#1	; 3 ...and to readable V flag pos
	bclr	Zflag,#2	; 2 transfer zero temp to zero out...
	btsc	Zflag,#1
	bset	Zflag,#2
	return
z1101:
	bclr	Vflag,#2	; 3 transfer overflow temp to overflow out...
	btsc	Vflag,#1
	bset	Vflag,#2
	bclr.b	RdFlags,#1	; 3 ...and to readable V flag pos
	btsc	Vflag,#1	; 3 ...and to readable V flag pos
	bset.b	RdFlags,#1	; 3 ...and to readable V flag pos
	bclr	Zflag,#2	; 2 transfer zero temp to zero out...
	btsc	Zflag,#1
	bset	Zflag,#2
	mov	Atemp,w14	; 0 transfer ACCU data to ouptput
	return
z1110:
	bclr	Vflag,#2	; 3 transfer overflow temp to overflow out...
	btsc	Vflag,#1
	bset	Vflag,#2
	bclr.b	RdFlags,#1	; 3 ...and to readable V flag pos
	btsc	Vflag,#1	; 3 ...and to readable V flag pos
	bset.b	RdFlags,#1	; 3 ...and to readable V flag pos
	bclr	Zflag,#2	; 2 transfer zero temp to zero out...
	btsc	Zflag,#1
	bset	Zflag,#2
	bclr	Cflag,#2	; 1 transfer carry temp to Carry out...
	btsc	Cflag,#1
	bset	Cflag,#2
	bclr	A16,#b_carry	; 1 ...and to Carry button LED
	btsc	Cflag,#2	; 1 ...and to Carry button LED
	bset	A16,#b_carry	; 1 ...and to Carry button LED
	return
z1111:
	bclr	Vflag,#2	; 3 transfer overflow temp to overflow out...
	btsc	Vflag,#1
	bset	Vflag,#2
	bclr.b	RdFlags,#1	; 3 ...and to readable V flag pos
	btsc	Vflag,#1	; 3 ...and to readable V flag pos
	bset.b	RdFlags,#1	; 3 ...and to readable V flag pos
	bclr	Zflag,#2	; 2 transfer zero temp to zero out...
	btsc	Zflag,#1
	bset	Zflag,#2
	bclr	Cflag,#2	; 1 transfer carry temp to Carry out...
	btsc	Cflag,#1
	bset	Cflag,#2
	bclr	A16,#b_carry	; 1 ...and to Carry button LED
	btsc	Cflag,#2	; 1 ...and to Carry button LED
	bset	A16,#b_carry	; 1 ...and to Carry button LED
	mov	Atemp,w14	; 0 transfer ACCU data to ouptput
	return

; ----------------------------------------------------------   INSTRUCTIONS  -----

alu_A1:			; ADD  RX,RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	call	dir_4_1
	bra	alu_cmd
alu_A2:			; ADC  RX,RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	call	dir_4_2
	bra	alu_cmd
alu_A3:			; SUB  RX,RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	call	dir_4_3
	bra	alu_cmd
alu_A4:			; SBB  RX,RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	call	dir_4_4
	bra	alu_cmd
alu_A5:			; OR   RX,RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	call	dir_4_5
	bra	alu_cmd
alu_A6:			; AND  RX,RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	call	dir_4_6
	bra	alu_cmd
alu_A7:			; XOR  RX,RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	call	dir_4_7
	bra	alu_cmd
alu_A8:			; MOV  RX,RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	call	dir_4_8
	bra	alu_cmd
;-----------------------
;alu_G0:			; CP    R0,N
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;	mov	w14,w11
;	call	dir_8_0
;	bra	alu_cmd
;alu_G1:			; ADD   R0,N
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;	mov	w14,w11
;	call	dir_8_1
;	bra	alu_cmd
alu_G2:			; INC   RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	mov	#OperY,w7	; result write address
	call	dir_8_2
	bra	alu_cmd
alu_G3:			; DEC   RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	mov	#OperY,w7	; result write address
	call	dir_8_3
	bra	alu_cmd
;alu_G5:			; OR    R0,N
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;	mov	w14,w11
;	call	dir_8_5
;	bra	alu_cmd
;alu_G6:			; AND   R0,N
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;	mov	w14,w11
;	call	dir_8_6
;	bra	alu_cmd
;alu_G7:			; XOR   R0,N
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;	mov	w14,w11
;	call	dir_8_7
;	bra	alu_cmd
;alu_G9:			; BIT   RG,M
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;; adjust register address
;	mov	#WREG14,w7	; result write address = Accu
;	and	w12,#0x0C,w0	; bits 3,2 = register
;	cp	w0,#0x0C
;	ifz
;	mov	#Ram+11,w7	; register read address = IN port
;	call	dir_8_9
;	bra	alu_cmd
;alu_G10:		; BSET  RG,M
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;; adjust register address
;	mov	#WREG14,w7	; result write address = Accu
;	and	w12,#0x0C,w0	; bits 3,2 = register
;	cp	w0,#0x0C
;	ifz
;	mov	#Ram+10,w7	; register read/write address = OUT port
;	call	dir_8_10
;	bra	alu_cmd
;alu_G11:		; BCLR  RG,M
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;; adjust register address
;	mov	#WREG14,w7	; result write address = Accu
;	and	w12,#0x0C,w0	; bits 3,2 = register
;	cp	w0,#0x0C
;	ifz
;	mov	#Ram+10,w7	; register read/write address = OUT port
;	call	dir_8_11
;	bra	alu_cmd
;alu_G12:		; BTG   RG,M
;	bclr	BlinkFlag,#0
;	bclr	BlinkFlag,#1
;; adjust register address
;	mov	#WREG14,w7	; result write address = Accu
;	and	w12,#0x0C,w0	; bits 3,2 = register
;	cp	w0,#0x0C
;	ifz
;	mov	#Ram+10,w7	; register read/write address = OUT port
;	call	dir_8_12
;	bra	alu_cmd
alu_G13:		; RRC   RY
	bclr	BlinkFlag,#0
	bclr	BlinkFlag,#1
	mov	#OperY,w7	; result write address
	call	dir_8_13
	bra	alu_cmd

; --------------------------------------------------------------------

peek_flash:		; measure all 16 Flash occupancies, write to Files (16×1 by) 37 ms
	mov	#Files,w5	; wr ptr
	mov	#0,w8		; loop count and record count
8:
	mov	w8,w0
	call	eemeasure	; occupance of w0 (0...15), output w2 bits #7...#0  (2.3 ms)
	mov.b	w2,[w5++]
	inc	w8,w8
	btss	w8,#4
	bra	8b
	return

; --------------------------------------------------------------------
; --------------------------------------------------------------------
; --------------------------------------------------------------------

leave_alu_ept:
; save ALU variables to ALU Temp
	mov	Opcode,w0
	mov	w0,AluTempOpcode ; Opcode
	mov	OperX,w0
	mov	w0,AluTempOperX	; OperX
	mov	OperY,w0
	mov	w0,AluTempOperY	; OperY
	mov	Atemp,w0
	mov	w0,AluTempAtemp	; Accu temp
	mov	w14,AluTempw14	; Accu out
	mov	w13,AluTempw13	; Accu in
; save Alu flags
	mov	Vflag,w0
	mov	w0,AluTempVflag
	mov	Zflag,w0
	mov	w0,AluTempZflag
	mov	Cflag,w0
	mov	w0,AluTempCflag
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	ss_ept
	bra	pgm_ept
