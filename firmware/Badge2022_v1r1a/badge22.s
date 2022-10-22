
; w11 is DESTINATION (Oper X)
; w12 is SOURCE (Oper Y)
; w13 is Accumulator IN data (bits 0-3). Do not use w13 for any other purpose!
; w14 is Accumulator OUT data (bits 0-3). Do not use w14 for any other purpose!
; HARDWARE MCU PINS:
; decoder     A7,A8,A9,A10
; Kbd in:     A3,A4
; key on-off: C9
; key ALT:    A2
; Out:        B0,B1,B2,B3
; In:         B4,B5,B6,B7
; Rx/Tx:      A0/A1

.list

.equ	autorpt_start, 80	; ×10 ms (200/2 Hz)
.equ	autorpt_next, 15	; ×10 ms (200/2 Hz)

.equ	Ver, 1
.equ	Rev, 1
.equ	Year, 22
.equ	Month, 10
.equ	Day, 30

.include	 "p24FJ256GA704.inc"
.include	 "macros.inc"

; config __FSEC,     0b1111111011110110 ; CONFIG area wr protect, boot segment enabled & wr protect
; config __FBSLIM,   0b1111111111111101 ; boot segment, end address 0x1000 (1 page = 0x800 by)
; config __FOSCSEL,  0b1111111110001001 ; start with FRC & switch, PLLMODE 8-->96 MHz, FRC with PLL
; config __FOSC,     0b1111111111100011 ; clk switch disabled, OSC2=port, primary osc disabled
; config __FWDT,     0b1011011110011111 ; WDT disabled
; config __FPOR,     0b1111111111111100 ; BOR disabled
; config __FICD,     0b1111111111011111 ; JTAG disabled, PGC/PGD 1
; config __FDEVOPT1, 0b1111111111100111 ; SDA-SCL on B5-B6, osc LP mode enabled, TMPRN disabled

		.section __FSEC.sec, code
		.global __FSEC
__FSEC:		.pword 0b1111111011110110 ; CONFIG area wr protect, boot segment enabled & wr protect
		.section __FBSLIM.sec, code
		.global __FBSLIM
__FBSLIM:	.pword 0b1111111111111101 ; boot segment, end address 0x1000 (1 page = 0x800 by)
		.section __FOSCSEL.sec, code
		.global __FOSCSEL
__FOSCSEL:	.pword 0b1111111110001001 ; start with FRC & switch, PLLMODE 8-->96 MHz, FRC with PLL
		.section __FOSC.sec, code
		.global __FOSC
__FOSC:		.pword 0b1111111111100011 ; clk switch disabled, OSC2=port, primary osc disabled
		.section __FWDT.sec, code
		.global __FWDT
__FWDT:		.pword 0b1011011110011111 ; WDT disabled
		.section __FPOR.sec, code
		.global __FPOR
__FPOR:		.pword 0b1111111111111100 ; BOR disabled
		.section __FICD.sec, code
		.global __FICD
__FICD:		.pword 0b1111111111011111 ; JTAG disabled, PGC/PGD 1
		.section __FDEVOPT1.sec, code
		.global __FDEVOPT1
__FDEVOPT1:	.pword 0b1111111111100111 ; SDA-SCL on B5-B6, osc LP mode enabled, TMPRN disabled
;------------------------------------------------------------------------------
.bss
Ram:		.space 240	; RAM contents (bits 3210) (first 15 pages)
;-------------------------------------------------------------------------------------- 
; SFR - READ/WRITE:	  ;                                                             
Page:		.space 1  ;  0  3210=Ram Display Page                              0000 
Speed:		.space 1  ;  1  3210=Speed Index                                   0000 
Sync:		.space 1  ;  2  3210=Sync Index                                    0000 
WrFlags:	.space 1  ;  3  3=LedsOff 2=MatrixOff 1=IOPos 0=RxTxPos            0000 
RdFlags:	.space 1  ;  4  1=Vflag 0=UserSync(CI)                             0000 
SerCtrl:	.space 1  ;  5  3=Error(CI), 210=BaudRate                          0011 
SerLow:		.space 1  ;  6  3210=Ser Buf Low (Wr:Tx, Rd:Rx)                    0000 
SerHigh:	.space 1  ;  7  3210=Ser Buf High (Wr:Tx, Rd:Rx)                   0000 
Received:	.space 1  ;  8  3210=RX Fifo ptr (15 means >=15)                   0000 
AutoOff:	.space 1  ;  9  3210=AutoOff count down (×10 minutes)              0010 
Out2:		.space 1  ; 10  Alternate Out register                             0000 
In2:		.space 1  ; 11  Alternate In register                              0000 
KeyStatus:	.space 1  ; 12  3=AltPress 2=AnyPress 1=LastPress 0=JustPress(CI)  0000
KeyReg:		.space 1  ; 13  3210=Key                                           0000 
Dimmer:		.space 1  ; 14  3210=Dim PWM level                                 1111 
Random:		.space 1  ; 15  3210=Random                                        0000 
;-------------------------------------------------------------------------------------- 
; HISTORY storage
History_Visible:.space 2	; address for history visible RAM page
				; (address for history RAM page 0 is History_Visible+16)
History_w9:	.space 2	; history temporary
History_w11:	.space 2	; history temporary
History_w12:	.space 2	; history temporary
History_w13:	.space 2	; history temporary
History_w14:	.space 2	; history temporary
History_A16:	.space 2	; history temporary
History_Atemp:	.space 2	; history temporary
History_Page:	.space 2	; history temporary
History_Sync:	.space 2	; history temporary
History_Speed:	.space 2	; history temporary
History_Vflag:	.space 2	; history temporary
History_Zflag:	.space 2	; history temporary
History_Cflag:	.space 2	; history temporary
History_Stack:	.space 2	; history temporary
History_Ptr:	.space 2	; pointer for history review, 0...127
Insert_Count_0:	.space 2	; counter of 24by packets written to History buffer from reset
; ------------------------------
Opcode:		.space 2	; Opcode (3210)
OperX:		.space 2	; Oper X (3210)
OperY:		.space 2	; Oper Y (3210)
Atemp:		.space 2	; Accu TEMP (Accu IN is w13, Accu OUT is w14)
Stack:		.space 2	; Stack pointer (bits 210)
Cflag:		.space 2	; C flag, bit #2 izlaz - #1 temp - #0 ulaz
Zflag:		.space 2	; Z flag, bit #2 izlaz - #1 temp - #0 ulaz
Vflag:		.space 2	; V flag, bit #2 izlaz - #1 temp - #0 ulaz
; ALU temp storage
AluTempOpcode:	.space 2	; Opcode (3210) temporary from ALU
AluTempOperX:	.space 2	; Oper X (3210) temporary from ALU
AluTempOperY:	.space 2	; Oper Y (3210) temporary from ALU
AluTempAtemp:	.space 2	; Accu TEMP temporary from ALU
AluTempw13:	.space 2	; Accu TEMP w13 (Accu In) temporary from ALU
AluTempw14:	.space 2	; Accu TEMP w14 (Accu Out) temporary from ALU
AluTempVflag:	.space 2	; Temp V for ALU
AluTempZflag:	.space 2	; Temp Z for ALU
AluTempCflag:	.space 2	; Temp C for ALU
; SS temp storage
SSTempPMAddr:	.space 2	; Program Memory Address (BA9876543210) temporary from SS
SSTempPage:	.space 2	; TEMP Page temporary from SS
SSTempStack:	.space 2	; TEMP Stack temporary from SS
SSTempVflag:	.space 2	; Temp V for SS
SSTempZflag:	.space 2	; Temp Z for SS
SSTempCflag:	.space 2	; Temp C for SS
SSTempw13:	.space 2	; Accu TEMP w13 (Accu In) temporary from SS
SSTempw14:	.space 2	; Accu TEMP w14 (Accu Out) temporary from SS
; RUN temp storage
RunTempPMAddr:	.space 2	; TEMP w9 in RUN mode
RunTempStack:	.space 2	; TEMP Stack in RUN mode
RunTempPage:	.space 2	; TEMP Page in RUN mode
RunTempVflag:	.space 2	; Temp V for Run
RunTempZflag:	.space 2	; Temp Z for Run
RunTempCflag:	.space 2	; Temp C for Run
RunTempw13:	.space 2	; Accu TEMP w13 (Accu In) temporary from Run
RunTempw14:	.space 2	; Accu TEMP w14 (Accu Out) temporary from Run
; PGM temp storage
PgmTempPMAddr:	.space 2	; Program Memory Address (BA9876543210) temporary from PGM
; general
Files:		.space 16	; flash record lengths for 16 files (1 bit = 0x200 words)
Mode:		.space 2	; 0=ALU, 1=SS, 2=RUN, 3=PGM
RowScan:	.space 2	; Row Scan counter (3210)
DataIn:		.space 2	; 0=BIN, 1=STEP
BlinkCount:	.space 2	; free running counter in T1 interrupt
KeyRotors:	.space 40	; key debouncers (18 buttons + 19th On/Off + 20th ALT)
Just:		.space 2	; just pressed key
Anypress:	.space 2	; bit 0 will be reset if any key still depressed
AutorptCount:	.space 2	; autorepeat counter
AutorptFlag:	.space 2	; autorpt allow flag: #0-4=keys1-5, #5=keys6-9, #6=keys10-13,
				; #7=keys14-17, #8=key18
A16:		.space 2	; extra anode column
Dummy:		.space 2	; dummy write (if result is not needed)
PcmPch:		.space 2	; Temporary PCM and PCH for INC/DEC 12-bit extension
BackIndex:	.space 2	; Runtime Back Index 0,1,2, to jp back where it was stopped
AutoOff_Hi:	.space 2	; High counter 62.5 Hz for AutoOff (countdown 37500 ---> 0)
Taskbar:	.space 2	; Task progress bar for Load/Save from/to record 15 (UART)
Temp_Dimmer:	.space 2	; Dimmer during sleep
RXWR:		.space 2	; RX Write pointer, points inside 256-byte RX FIFO Buffer
RXRD:		.space 2	; RX Read pointer, points inside 256-byte RX FIFO Buffer
FlashAddr:	.space 2	; 0...15, addr of Flash Load/Save
PR1copy:	.space 2	; first phase of dimmer timing, used to calculate 2nd phase
Rndlo:		.space 2	; Seed lo
Rndhi:		.space 2	; Seed hi
Column1:	.space 2	; Opcode column raw (used in self-check mode)
Column2:	.space 2	; Oper X column raw (used in self-check mode)
Column3:	.space 2	; Oper Y column raw (used in self-check mode)
Row3:		.space 2	; row 3 (Source - Address) (used in self-check mode)
Row4:		.space 2	; row 4 (V - AdderSum - AdderCarry - Dest) (used in self-check mode)
Row5:		.space 2	; row 5 (Z - Or - And - Xor) (used in self-check mode)
Row6:		.space 2	; row 6 (C - AccuOut - AccuTmp - AccuIn) (used in self-check mode)
Row7:		.space 2	; row 7 (Page - Opcode - OperX - OperY) (used in self-check mode)
CHS1:		.space 2	; Checksum 1 (Boot sector), swapped bytes
CHS2:		.space 2	; Checksum 2 (General sector), swapped bytes

Flag:		.space 2	; #0 Set if program runs in run mode
				; #1 handshaking flag for buttons, set when any key pressed
				; #2 set if Speed > 0 or if Mode <> 2
				; #3 self-check mode
				; #4 set if instruction covers CALL and JP (mov, inc, dec)
				; #5 set in HISTORY mode
				; #6 set if ALT pressed (debounced)
				; #7 set if there is PCM/PCH extension
				; #8 set if FAST mode
				; #9 set if instruction is writing to SFR area
				; #10 set if instruction is reading from SFR area
				; #11 flag "any Key Pressed" (will be moved to KeyStatus,#2)
				; #12 temp LATC,#8 (column 17 anode)
				; #13 toggle between int (0) - aux_int (1)
				; #14 set if dim down in progress
				; #15 set if dim up in progress

Flag2:		.space 2	; #0 Run Fast toggler (1=fast)
				; #1 program Runs 
				; #2 handshaking flag for moving from FIFO to SerHigh:SerLow
				; #3 Previous RxTxPos
				; #4 set when initialized
				; #5 Fatal Stack Error
				; #6 EE subroutines adjust to Boot
				; #7 EE load to second halve of Rom area (starting at 0x3000)
				; #8 set = first button pressed, Ver/Rev deleted
				; #9 set = last command LOAD (clr = last command SAVE)
				; #10 set = 1st byte in Bootloader received
				; #11 = previous Flag,#2
				; #12 = RAM cleared, not in VER/REV mode (2nd test)

BlinkFlag:	.space 2	; #0 set = column Opcode blinks
				; #1 set = column Oper X blinks
				; #2 set = LED Run blinks
				; #3 set = LED SS blinks
				; #4 set = LED Carry blinks
				; #5 set = LED Save (Pause) blinks
				; #6 set = Stack blinks
				; #7 set = LED Load blinks

.equ	History,0x0B00		; FIFO buffer, 128 × 42 by = 5376 by = 0x1500 by
.equ	Rom,0x2000		; 2×4K Program Memory 0x2000-0x3FFF

.equ	RX_buf,0x4600		; RX FIFO buffer, 256 by
.equ	STACK_START,0x4700	; SP, 0x4700-0x47FF
.equ	STACK_END,0x47fe	; end of data memory
;--------------------------------------------------------------------------------------------------
.equ	b_clk_sch,0		; A16 bits
.equ	b_exr,1
.equ	b_clk_key,2
.equ	b_noclk_sch,3
.equ	b_data_inv,4
.equ	b_cin,5
.equ	b_cena,6
.equ	b_sel,7
.equ	b_pgm,8
.equ	b_run,9
.equ	b_ss,10
.equ	b_alu,11
.equ	b_carry,12
.equ	b_save,13
.equ	b_load,14
.equ	b_bin,15
;											
;				0 x 0 2 0 0    (physical) 				
;_______________________________________________________________________________________
.text
	.org	0x0042, 0xFF	;   * * *   0x0042+0x01BE=0x0200 (fill 0xFF's)
; boot segment
	.ascii	"BOOT"		; boot segment start
.global __reset
__reset:
	mov	#STACK_START,w15
	mov	#STACK_END,w0
	mov	w0,SPLIM 		; Stack Pointer Limit
;------------------------------------------
	call	initialize
;------------------------------------------ clear RAM
	mov	#0x800,w0
	repeat	#0x400-1
	xor	w2,[w0++],w2
	repeat	#0x1C00-1
	mov	w2,w3
	xor	w3,[w0++],w3
	mov	#0x800,w1
	repeat 	#0x2000-1	;total 0x4000=16K bytes
	clr	[w1++]
	mov	w2,Rndlo	; initialize Rnd Seed
	and	w2,#0x0F,w0
	mov.b	Wreg,Ram+0xFF	; Write Rnd value
	mov	w3,Rndhi	; initialize Rnd Seed
; ini AutoOff
	mov	#2,w0		; 2×10 min autooff at hard reset
	mov.b	WREG,AutoOff
	mov	#37500,w0
	mov	w0,AutoOff_Hi
; ini dimmer
	mov	#15,w0
	mov.b	WREG,Dimmer
; check if button ALT depressed (self check button)
	btsc	PORTA,#2	; PORTA,#2 = ALT key
	bra	selfcheck_back	; if ALT not pressed
	bra	boot_start

	.include	"Boot.inc"
	.include	"flash.inc"

;											
;				0 x 1 0 0 0    (physical)				
;_______________________________________________________________________________________
	.org	0x1000-0x1BE, 0xFF	;   * * *   0x1000 (fill 0xFF's)
; general segment
	.ascii	"GENS"		; general segment start
; Ver/Rev/Year/Month/Day
ver_data:
	.word	Ver
	.word	Rev
	.word	Year
	.word	Month
	.word	Day
; IVT 
.global __T1Interrupt		; must be @ 0x1000
__T1Interrupt:
	goto	T1Int
.global __T3Interrupt		; must be @ 0x1004
__T3Interrupt:
	goto	T3Int
.global	_INT1Interrupt		; must be @ 0x1008
_INT1Interrupt:
	goto	INT1Int
.global	_INT2Interrupt		; must be @ 0x100C
_INT2Interrupt:
	goto	INT2Int
.global __U1RXInterrupt		; must be @ 0x1010
__U1RXInterrupt:
	goto	U1RXInt
.global __U1ErrInterrupt	; must be @ 0x1014
__U1ErrInterrupt:
	goto	U1ErrInt
.global __U2RXInterrupt		; must be @ 0x1018
__U2RXInterrupt:
	goto	U2RXInt
.global __U2ErrInterrupt	; must be @ 0x101C
__U2ErrInterrupt:
	goto	U2ErrInt

; **********************************************************************

selfcheck_back:
; ini Mode specific params
	mov	#Ram+11,w8
	mov	#Rom,w9
	mov	w9,SSTempPMAddr
	mov	w9,RunTempPMAddr
	mov	#Ram,w10
	clr	w11
	clr	w12
	clr	w13
	clr	w14
	mov	w9,History_w9
; ini Rx FIFO
	mov	#RX_buf,w0
	mov	w0,RXWR
	mov	w0,RXRD
; ini function LEDs on Cathode 17
	mov	#0b0000100010001000,w0	; ALU def (Set ALU, SEL, -CLK)
	mov	w0,A16
; ini SFR
	mov	#3,w0
	mov.b	WREG,SerCtrl
	call	def_U1BRG
	call	rxtx_to_rxtxpos
	call	show_vrymd
; show CHS in the middle of matrix
	mov	#0,w1
	mov	#0x1000/2,w2
	mov	#CHS1,w4	; point w4 to CHS1
	call	get_chs
; show CHS in the bottom of matrix
	mov	#0x1000,w1
	mov	#tbloffset(pgm_end)-0x1000,w2
	lsr	w2,w2		; /2
	mov	#CHS2,w4	; point w4 to CHS2
	call	get_chs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Transfer program HAMLET
	clr	TBLPAG
	mov	#tbloffset(display),w0
; code @ 0x000
	mov	#Rom,w2
0:
	tblrdl	[w0++],w1
	btsc	w1,#15
	bra	1f
	mov	w1,[w2++]
	bra	0b
1:
; char gen @ 0x100
	mov	#tbloffset(chargen),w0
	mov	#Rom+2*0x100,w2
2:
	tblrdl	[w0++],w1
	btsc	w1,#15
	bra	1f
	mov	w1,[w2++]
	bra	2b
1:
; text @ 0x500
	mov	#tbloffset(disptext),w0
	mov	#Rom+2*0x500,w2		; adr 0x500
	mov	#0xE0,w4		; RET code
2:
	tblrdl.b [w0++],w1
	tblrdl.b [w0],w5
	and.b	w5,w1,w5
	inc.b	w5,w5
	bra	z,1f

	and	w1,#0x0F,w3
	ior	w3,w4,w3
	mov	w3,[w2++]
	lsr	w1,#4,w3
	and	w3,#0x0F,w3
	ior	w3,w4,w3
	mov	w3,[w2++]
	bra	2b
1:
	mov	#0x00EF,w5		; RET F
	mov	w5,[w2++]		; text terminator
	mov	w5,[w2++]		; text terminator
; save it to flash 14
	call	com_rom
	mov	#14,w0
	bclr	Flag2,#6	; #6 EE subroutines adjust to Boot
	bclr	Flag2,#7	; #7 EE load to second halve of Rom area (starting at 0x3000)
	call	eesavew0	; EEsave on record 14
	call	com_rom
	mov	#Rom,w0
	repeat	#0x01000-1
	clr	[w0++]		; clr Rom
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Transfer program UART DISP
	mov	#tbloffset(serial_disp),w0
; code @ 0x000
	mov	#Rom,w2
0:
	tblrdl	[w0++],w1
	btsc	w1,#15
	bra	1f
	mov	w1,[w2++]
	bra	0b
1:
; char gen @ 0x100
	mov	#tbloffset(chargen),w0
	mov	#Rom+2*0x100,w2
2:
	tblrdl	[w0++],w1
	btsc	w1,#15
	bra	1f
	mov	w1,[w2++]
	bra	2b
1:
; save it to flash 13
	call	com_rom
	mov	#13,w0
	bclr	Flag2,#6	; #6 EE subroutines adjust to Boot
	bclr	Flag2,#7	; #7 EE load to second halve of Rom area (starting at 0x3000)
	call	eesavew0	; EEsave on seg 13
	call	com_rom
	mov	#Rom,w0
	repeat	#0x01000-1
	clr	[w0++]		; clr Rom
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	bset	Flag2,#4	; #4 set when initialized
	call	peek_flash	; record flash occupance  (37 ms)
; -----------------------------
; ini perif
	clr	TMR1
	clr	TMR2
	clr	TMR3
	bset	T1CON,#TON	; LED multiplex timer on
	bset	T2CON,#TON
	bset	T3CON,#TON
	bset	IEC0,#T1IE	; enable Timer 1 interrupt
	bset	IEC0,#8		; enable Timer 3 interrupt
	bset	IEC0,#11	; enable RX1 interrupt
	bset	IEC4,#1		; enable RX1 Error Interrupt
	bset	INTCON2,#GIE	; global interrupt enable

;------------------------------------------------------------------------
;------------------------------------------------------------------------
;---------------------------   F A R M   --------------------------------
;------------------------------------------------------------------------
;------------------------------------------------------------------------

main_farm:
	bra	alu_ept

;											
txword:				; TX w0_low, w0_high
	call	txbyte
	swap	w0
txbyte:				; TX w0
	btsc	U1STA,#UTXBF
	bra	txbyte
	mov.b	WREG,U1TXREG	; ----> transmit w0
	return

;											
get_chs:		; pointer w1, length (in words) w2, CHS in w3 on exit, wr ptr w4
	clr	w3
chs_loop:
	tblrdh.b [w1],w0
	ze	w0,w0
	add	w0,w3,w3
	tblrdl	[w1++],w0
	add	w0,w3,w3
	dec	w2,w2
	bra	nz,chs_loop
; CHS is in w3
	swap	w3	; because display routine shows in little endian, should be big
	mov	w3,[w4]
	return

;-----------------------
show_vrymd:	; show Ver/Rev/Year/Month/Day
	mov	#Ram+0,w1
	mov	#tbloffset(ver_data),w3
	call	vrymd
	call	vrymd
	call	vrymd
	call	vrymd
vrymd:			; Ver / Rev / Year / Month / Day
	tblrdl	[w3++],w2
	and	w2,#0xF,w0
	mov.b	w0,[w1]
	lsr	w2,#4,w0
	mov.b	w0,[w1+16]
	inc	w1,w1
	return
;-----------------------
clr_ram:
	mov	#Ram,w0
	repeat	#120-1		; clear all exept SFR
	clr	[w0++]
	return
;------------------------------------------------------------------------
w0ms:
	repeat	#16000-1
	nop
	dec	w0,w0
	bra	nz,w0ms
	return

;											

	.include	"alu.inc"
	.include	"ss.inc"
	.include	"run.inc"
	.include	"pgm.inc"
	.include	"int.inc"
	.include	"history.inc"
	.include	"table2.inc"
	.include	"asem.inc"
	.include	"Hamlet.inc"

	.end
