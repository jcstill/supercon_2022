
pgm_ept:
	btss	Flag2,#12
	call	clr_ram
	bset	Flag2,#12
; initialize or reload PGM variables from PGM Temp
	clr	Stack		; Stack
	clr.b	Page		; Page 0
	mov	PgmTempPMAddr,w9 ; PC
	mov	#0b011101100,w0	; autorpt for Addr-, Addr+, Opcode, OperX, OperY
	mov	w0,AutorptFlag
; PGM initialize
	mov	#3,w0
	mov	w0,Mode		; Mode
	clr	BlinkFlag
	clr	w11
	clr	w12
	clr	w13
	clr	w14
	clr	Vflag
	clr	Zflag
	clr	Cflag
	mov	#3,w0			; 3 = PGM
	mov	w0,Mode
	bset	Flag,#2		; #2 set if Speed > 0 or if Mode <> 2
	clr	w13		; w13 is Accumulator IN, displayed as ACCU IN
	clr	w14		; w14 is Accumulator OUT, displayed as ACCU OUT
; prepare A16 (set RUN and keep Bin/Sel)
	mov	#0b1000000010000000,w0	; keep BIN and SEL only
	and	A16
	mov	#0b0000000100000000,w0	; set RUN
	ior	A16

pgm_farm:
	call	read12		; read new pgm word
pgm_noread:
	btss	Flag,#1		; #1 handshaking flag for buttons
	bra	pgm_noread
	bclr	Flag,#1
	mov	Just,w0
	cp	w0,#5
	ifc
	mov	#5,w0
	bra	w0
; ...
	bra	leave_pgm_ept
	bra	button_addr_set
	bra	button_addr_minus_pgm
	bra	button_addr_plus_pgm
	bra	button_dep
	bra	general_pgm_5
;					ept 0: addr_set (in PGM mode)
button_addr_set:
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	alt_addr_set	; if ALT addr set pressed
	mov	Opcode,w0
	sl	w0,#4,w0
	ior	OperX,WREG
	sl	w0,#4,w0
	ior	OperY,WREG	; data packed in 12 bits
	sl	w0,w0		; ×2
	mov	#Rom,w1
	add	w0,w1,w9	; write new address
	bra	pgm_farm
;					ept 0: addr_set (in PGM mode)
alt_addr_set:
	inc2	w9,w2
	mov	#Rom+0x2000,w1	; end of Program Memory + 2
train2:
	cp	w2,w1		; w9=current program pointer (non-incremented)
	bra	nz,1f
	clr	[--w2]		; last word in program = 0x000
	bra	adr_plus	; train finished
1:
	mov	[w2--],w0	; w0 = contents
	mov	w0,[w2]
	add	#4,w2
	bra	train2
;					ept 1: addr_minus_pgm (in PGM mode)
button_addr_minus_pgm:
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	alt_minus_pgm	; if ALT addr minus pressed
	dec2	w9,w9		; decrement address
	btss	w9,#13
	mov	#Rom+0x1FFE,w9	; wrap to end of program memory
	bra	pgm_farm
;					ept 2: addr_plus_pgm (in PGM mode)
button_addr_plus_pgm:
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	alt_plus_pgm	; if ALT addr plus pressed
adr_plus:
	inc2	w9,w9		; increment address
	btsc	w9,#14
	mov	#Rom,w9		; wrap to start of program memory
	bra	pgm_farm	; test failed, some other key pressed
;					ept 3: dep (in PGM mode)
button_dep:
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	alt_dep		; if ALT DEP pressed
	call	write12		; write new pgm word
	bra	adr_plus
;		
alt_dep:
	mov	[w9],w2		; old value
	call	write12		; *** write new pgm word
	mov	#Rom+0x1FFC,w1	; end of Program Memory - 2
train1:
	cp	w9,w1		; w9=current program pointer (non-incremented)
	bra	nc,1f
	mov	w2,[++w1]
	bra	adr_plus	; train finished
1:
	mov	[w1++],w0	; w0 = contents
	mov	w0,[w1--]
	dec2	w1,w1
	bra	train1
;		
write12:
	mov	Opcode,w0
	sl	w0,#4,w0
	ior	OperX,WREG
	sl	w0,#4,w0
	ior	OperY,WREG	; data packed in 12 bits
	mov	w0,[w9]		; write program word
	return
;		
alt_plus_pgm:
	btss	KeyRotors+4,#0	; it's -ADDR debouncer
	bra	alt_both_pgm	; if alt and both - and + pressed
	call	find_last
	bra	pgm_farm
;		
alt_minus_pgm:
	btss	KeyRotors+6,#0	; it's ADDR+ debouncer
	bra	alt_both_pgm	; if alt and both - and + pressed
	clr	Stack
	mov	#Rom,w9
	bra	pgm_farm
;				; "go addr 0" test	
alt_both_pgm:			; ALT both pressed
	mov	KeyRotors+0,w0
	and	KeyRotors+2,WREG
	mov	#KeyRotors+8,w1
	mov	#14,w2
1:
	and	w0,[w1++],w0
	dec	w2,w2
	bra	nz,1b
	btss	w0,#0
	bra	pgm_farm	; some other key is pressed, ignore command
; erase all data memory
	call	clr_ram
	clr	Stack
	clr.b	Page		; default page 0
	clr.b	Speed		; default Clock max
	clr.b	Sync		; default Sync max
	clr.b	WrFlags		; Leds on, Matrix on, IOPos 0xA-0xB, RxTxPos on SAO
	clr.b	Received	; clear queue
	mov	#15,w0
	mov.b	Wreg,Dimmer	; default Dimmer max
	mov	#0b0011,w0
	mov.b	Wreg,SerCtrl	; default UART
	  call	  rxtx_to_rxtxpos ; RxTx restored back to RxTxpos
	  call	  set_U1BRG	  ; BRG restored to SerCtrl (bits 210)
; erase all program memory
	clr	Stack
	mov	#Rom,w0
	repeat	#0x01000-1
	clr	[w0++]		; clr Rom
	clr.b	Page
	mov	#Rom,w9
	bra	pgm_farm

general_pgm_5:	; -----------------------------------------------------------------
; 13 buttons OPCODE 8... DATA IN
	call	general_key_3O
	bra	pgm_noread
; ------------------------------
read12:
	mov	[w9],w0
	lsr	w0,#8,w1
	mov	w1,Opcode
	lsr	w0,#4,w2
	and	w2,#0x0F,w2
	mov	w2,OperX
	and	w0,#0x0F,w0
	mov	w0,OperY
	return

; -------------------------------------------------------------------------------

leave_pgm_ept:
; save PGM params
	mov	w9,PgmTempPMAddr ; PC
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	alu_ept
	bra	run_ept

