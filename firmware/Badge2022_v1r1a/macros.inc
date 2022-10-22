
;_______________________________ system flags

.macro	ifdc
	btsc	SR,#DC
.endm
.macro	ifndc
	btss	SR,#DC
.endm
.macro	ifz
	btsc	SR,#Z
.endm
.macro	ifnz
	btss	SR,#Z
.endm
.macro	ifc
	btsc	SR,#C
.endm
.macro	ifnc
	btss	SR,#C
.endm
.macro	manji
	btss	SR,#C
.endm
.macro	istiveci
	btsc	SR,#C
.endm
.macro	setc
	bset	SR,#C
.endm
.macro	clrc
	bclr	SR,#C
.endm
.macro	ccf
	btg	SR,#C
.endm
; ------------------------ table member for SS
.macro	callSS	call1
	call	\call1
	bra	ss_cont
.endm
; ------------------------ table member for RUN
.macro	callRUN	call2
	call	\call2
	btsc	Flag2,#11	; #11 = previous Flag,#2
	call	run_cont
	bra	run_farm
.endm
; ------------------------ clear LEDs Cena, DataInv and Cin
.macro	clr3leds
	mov	#0b1111111110001111,w0
	and	A16
.endm
; ------------------------ increment PC (which is w9), keep in RAM range
.macro	incpc
	inc2	w9,w9		; INC PC
	btsc	w9,#14		; INC PC
	mov	#Rom,w9		; INC PC  wrap to start of program memory
.endm

; ----------------------------------------------------------

;				I2C	in		
.macro	ifdatahi
	btsc	PORTA,#0
.endm
.macro	ifdatalo
	btss	PORTA,#0
.endm
.macro	ifclockhi
	btsc	PORTA,#1
.endm
.macro	ifclocklo
	btss	PORTA,#1
.endm
;				I2C	out		
.macro	datahi
	bset	LATA,#0
	call	t38
.endm
.macro	datalo
	bclr	LATA,#0
	call	t38
.endm
.macro	clockhi
	bset	LATA,#1
8:
	nop
	btss	PORTA,#1
	bra	8b		; clock stretching
	call	t38
.endm
.macro	clocklo
	bclr	LATA,#1
	call	t38
.endm

;AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
;AAAAAAAA                  Q U A S I   -   A S S E M B L E R                    AAAAAAAAAA
;AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

.equ	r0,0x0
.equ	r1,0x1
.equ	r2,0x2
.equ	r3,0x3
.equ	r4,0x4
.equ	r5,0x5
.equ	r6,0x6
.equ	r7,0x7
.equ	r8,0x8
.equ	r9,0x9
.equ	ra,0xa
.equ	rb,0xb
.equ	rc,0xc
.equ	rd,0xd
.equ	re,0xe
.equ	rf,0xf
;-----------------------------------------
.equ	r10,0xa
.equ	r11,0xb
.equ	r12,0xc
.equ	r13,0xd
.equ	r14,0xe
.equ	r15,0xf
;-----------------------------------------
.equ	hi,0x9
.equ	out,0xa
.equ	in,0xb
.equ	jsr,0xc
.equ	pclo,0xd
.equ	pcm,0xe
.equ	pchi,0xf
;-----------------------------------------
.macro	no	param1,param2
	.word	0x000+(\param1<<4)+\param2
.endm
.macro	addxy	param1,param2
	.word	0x100+(\param1<<4)+\param2
.endm
.macro	adcxy	param1,param2
	.word	0x200+(\param1<<4)+\param2
.endm
.macro	subxy	param1,param2
	.word	0x300+(\param1<<4)+\param2
.endm
.macro	sbbxy	param1,param2
	.word	0x400+(\param1<<4)+\param2
.endm
.macro	orxy	param1,param2
	.word	0x500+(\param1<<4)+\param2
.endm
.macro	andxy	param1,param2
	.word	0x600+(\param1<<4)+\param2
.endm
.macro	xorxy	param1,param2
	.word	0x700+(\param1<<4)+\param2
.endm
.macro	movxy	param1,param2
	.word	0x800+(\param1<<4)+\param2
.endm
.macro	movxn	param1,param2
	.word	0x900+(\param1<<4)+\param2
.endm
.macro	movxyr0	param1,param2
	.word	0xa00+(\param1<<4)+\param2
.endm
.macro	movr0xy	param1,param2
	.word	0xb00+(\param1<<4)+\param2
.endm
.macro	movnnr0	param1,param2
	.word	0xc00+(\param1<<4)+\param2
.endm
.macro	movr0nn	param1,param2
	.word	0xd00+(\param1<<4)+\param2
.endm
.macro	movpcnn	param
	.word	0xe00+(\param&0xFF)
.endm
.macro	jrnn	param
	.word	0xf00+(\param&0xFF)
.endm
;-----------------------------------------
.macro	cpr0n	param
	.word	0x000+(\param&0xF)
.endm
.macro	addr0n	param
	.word	0x010+(\param&0xF)
.endm
.macro	incy	param
	.word	0x020+(\param&0xF)
.endm
.macro	decy	param
	.word	0x030+(\param&0xF)
.endm
.macro	dszy	param
	.word	0x040+(\param&0xF)
.endm
.macro	orr0n	param
	.word	0x050+(\param&0xF)
.endm
.macro	andr0n	param
	.word	0x060+(\param&0xF)
.endm
.macro	xorr0n	param
	.word	0x070+(\param&0xF)
.endm
.macro	exrn	param
	.word	0x080+(\param&0xF)
.endm
.macro	bitrgm	param
	.word	0x090+(\param&0xF)
.endm
.macro	bsetrgm	param
	.word	0x0a0+(\param&0xF)
.endm
.macro	bclrrgm	param
	.word	0x0b0+(\param&0xF)
.endm
.macro	btgrgm	param
	.word	0x0c0+(\param&0xF)
.endm
.macro	rrcy	param
	.word	0x0d0+(\param&0xF)
.endm
.macro	retr0n	param
	.word	0x0e0+(\param&0xF)
.endm
.macro	skipfm	param
	.word	0x0f0+(\param&0xF)
.endm

.macro	retr0n2	param8bits
	.word	0x0e0+(\param8bits&&0xF),0x0e0+((\param8bits>>4)&&0xF)
.endm


