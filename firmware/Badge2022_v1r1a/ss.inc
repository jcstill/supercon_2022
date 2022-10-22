
ss_ept:
	btss	Flag2,#12
	call	clr_ram
	bset	Flag2,#12
; initialize or reload SS variables from RUN Temp
	mov	SSTempPage,w0
	mov.b	WREG,Page	; Page
	mov	SSTempStack,w0
	mov	w0,Stack	; Stack
	mov	SSTempPMAddr,w9	; PC
	mov	SSTempw14,w14
	mov	SSTempw13,w13
; restore SS flags
	mov	SSTempVflag,w0
	mov	w0,Vflag
	mov	SSTempZflag,w0
	mov	w0,Zflag
	mov	SSTempCflag,w0
	mov	w0,Cflag
	bclr	A16,#b_carry
	btsc	Cflag,#2
	bset	A16,#b_carry
; SS initialize
	mov	#1,w0		; 1 = SS
	mov	w0,Mode		; Mode
	clr	BlinkFlag
	clr	TBLPAG
	bset	Flag,#2		; #2 set if Speed > 0 or if Mode <> 2
	bset	Flag2,#11	; #11 = previous Flag,#2
	mov	SSTempPMAddr,w9 ; const
	mov	#Ram,w10	; const
	mov	#Ram+0x0B,w8	; const
	btsc.b	WrFlags,#1	; I/O port select
	mov	#Ram+0xFB,w8	; const
	mov	#0b011111100,w0	; autorpt allow flag: #0-4=keys1-5, #5=keys6-9,
	mov	w0,AutorptFlag	; #6=keys10-13, #7=keys14-17, #8=key18
	clr	w13		; w13 is Accumulator IN, displayed as ACCU IN
	clr	w14		; w14 is Accumulator OUT, displayed as ACCU OUT
	bclr	Flag2,#5	; #5 Fatal Stack Error
; prepare A16 (set SS and keep Bin/Sel)
	mov	#0b1000000010000000,w0	; keep BIN and SEL only
	and	A16
	mov	#0b0000010000001000,w0	; set SS and -CLK
	ior	A16
;	
	bclr	U1MODE,#15	; disable UART 1 to clear queue and error flags
	bset	U1MODE,#15	; enable UART 1
	bset	U1STAL,#UTXEN	; enable TX 1 (must be after UART enable)
	bset	U1STAL,#URXEN	; enable RX 1 (must be after UART enable)
	bclr	Flag,#9		; possible write to SFR area
	bclr	Flag,#10	; possible read from SFR area
; ------------------------------
ss_farm:
	bclr	Flag,#5		; #5 set: in HISTORY mode
	bclr	BlinkFlag,#3	; #3 set: LED SS blinks
	call	read12
; check if Stack Error occured
	btss	Flag2,#5	; #5 Fatal Stack Error
	bra	no_SS_StackErr
	bset	BlinkFlag,#6	; 6 set = Stack blinks
1:
	  btsc	  Flag,#6	  ; #6 set if ALT pressed (debounced)
	  bra	  2f
	btss	Flag,#1		; #1 handshaking flag for buttons
	bra	1b
2:
	bclr	Flag,#1
	bclr	BlinkFlag,#6	; 6 set = Stack blinks
	clr	Stack
	bclr	Flag2,#5	; #5 Fatal Stack Error
;	call	clr_ram
no_SS_StackErr:
ss_noread:
	mov.b	Ram+0x0A,WREG	; OUT register if WrFlags,#1 = 0
	btsc.b	WrFlags,#1	; I/O port select
	mov.b	Ram+0xFA,WREG	; OUT register if WrFlags,#1 = 1
	mov.b	WREG,LATB	; OUT port

	bclr	Flag,#4		; #4 set if instruction covers CALL and JP (mov, inc, dec)
	mov	OperX,w11	; w11 = OperX
	mov	OperY,w12	; w12 = OperY
	mov	Opcode,w0	; w0 = Opcode
	and	#0x0F,w0
	bra	z,group8runb	; ---------> 8-bit opcode
; w0 = instruction
	mul.uu	w0,#3,w0
	bra	w0
;jp table
	.word	0,0,0		; non-existing instruction
	callSS	ind_4_1		; ADD  RX,RY
	callSS	ind_4_2		; ADC  RX,RY
	callSS	ind_4_3		; SUB  RX,RY
	callSS	ind_4_4		; SBB  RX,RY
	callSS	ind_4_5		; OR   RX,RY
	callSS	ind_4_6		; AND  RX,RY
	callSS	ind_4_7		; XOR  RX,RY
	callSS	ind_4_8		; MOV  RX,RY
	callSS	ind_4_9		; MOV  RX,N (TEST PCL)
	callSS	ind_4_10	; MOV  [XY],R0
	callSS	ind_4_11	; MOV  R0,[XY]
	callSS	ind_4_12	; MOV  [NN],R0
	callSS	ind_4_13	; MOV  R0,[NN]
	callSS	ind_4_14	; MOV  PC,NN
	callSS	ind_4_15	; JR   NN
;...
group8runb:
; w0 = instruction
	mul.uu	w11,#3,w0
	bra	w0
;jp table
	callSS	ind_8_0		; CP   R0,N
	callSS	ind_8_1		; ADD  R0,N
	callSS	ind_8_2		; INC  RY (TEST PCL)
	callSS	ind_8_3		; DEC  RY (TEST PCL)
	callSS	ind_8_4		; DSZ  RY
	callSS	ind_8_5		; OR   R0,N
	callSS	ind_8_6		; AND  R0,N
	callSS	ind_8_7		; XOR  R0,N
	callSS	ind_8_8		; EXR  N
	callSS	ind_8_9		; BIT  RG,M
	callSS	ind_8_10	; BSET RG,M
	callSS	ind_8_11	; BCLR RG,M
	callSS	ind_8_12	; BTG  RG,M
	callSS	ind_8_13	; RRC  RY
	callSS	ind_8_14	; RET  R0,N
	callSS	ind_8_15	; SKIP F,M
;..............................................
ss_cont:
1:
	btss	Flag,#1		; #1 handshaking flag for buttons
	bra	1b
	bclr	Flag,#1
	mov	Just,w0
; test if Rnd has to be invoked
	cp.b	w0,#17		; Data In
	bra	nz,9f		; if not key Data In
	mov	Page,w0
	cp.b	w0,#0x0F
	ifz			; if Page=15
	call	new_rnd
	mov	Just,w0
9:
; continue
	cp	w0,#5
	ifc
	mov	#5,w0
	bra	w0
; ...
	bra	leave_ss_ept		; mode ++
	bra	button_ss_history	; ss_history
	bra	button_ss_addr_minus	; ss_addr_minus
	bra	button_ss_addr_plus	; ss_addr_plus
	bra	button_ss_step		; ss_step
	bra	button_ss_key5plus
;				
button_ss_history:
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	ss_history		; ss_history ------>
	btg	A16,#b_carry
	call	read12
	bra	ss_noread
;				
button_ss_key5plus:
; 13 buttons OPCODE 8... DATA IN
	call	general_key_3O
	bra	ss_noread
; --------------------------------------------------
button_ss_addr_minus:
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	adr000_ss	; if alt-ADR minus pressed
	dec2	w9,w9		; decrement address
	btss	w9,#13
	mov	#Rom+0x1FFE,w9	; wrap to end of program memory
	bra	ss_farm
; -----------
button_ss_addr_plus:
	btsc	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	adrFFF_ss	; if alt-ADR plus pressed
ss_adr_plus:
	inc2	w9,w9		; increment address
	btsc	w9,#14
	mov	#Rom,w9		; wrap to start of program memory
	bra	ss_farm
;		
adr000_ss:
	clr	Stack
	clr.b	Page
	mov	#Rom,w9
	call	clr_ram
	call	read12
	bra	ss_noread
;		
adrFFF_ss:
	call	find_last
	call	read12
	bra	ss_noread
;		
find_last:
	mov	#Rom+0x2000,w9	; pointer on top of pgm mem
	mov	#0x1000,w1	; loop count
1:
	cp0	[--w9]
	ifnz
	return
	dec	w1,w1
	bra	nz,1b
	return

; ------------------------------------------------
button_ss_step:
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	final_ss_step	; if not ALT, then simple ss_step
; address set from Opc/OperX/OperY
	mov	Opcode,w0
	sl	w0,#4,w0
	ior	OperX,WREG
	sl	w0,#4,w0
	ior	OperY,WREG	; data packed in 12 bits
	sl	w0,w0		; ×2
	mov	#Rom,w1
	add	w0,w1,w9	; write new address
	bra	ss_farm
;		
final_ss_step:
	call	insert		; shift buffer up, pack and insert new 32 by (used 28, spare 4)
; w6 = flags to affect
	bset	A16,#b_clk_key	; set CLK key
	call	transfer1	; uses w6 bits to decide flag transfer
2:				; wait for key STEP released
	inc	KeyRotors+8,WREG
	bra	nz,2b		; if not yet 0x1111111111111111
	bclr	A16,#b_clk_key
	call	transfer2	; uses w6 bits to decide flag transfer
	call	ins_part_2	; INSTRUCTION PART 2
	bra	ss_farm		; LOOP -------------------------------------->

; ---------------------------------------------------------------------------------
; ---------------------------------------------------------------------------------
; ---------------------------------------------------------------------------------

leave_ss_ept:
	clr	BlinkFlag
	clr	TBLPAG
; save SS variables to SS Temp
	mov.b	Page,WREG
	mov.b	WREG,SSTempPage	; Page
	mov	Stack,w0
	mov	w0,SSTempStack	; Stack
	mov	w9,SSTempPMAddr	; PC
	mov	w14,SSTempw14
	mov	w13,SSTempw13
; save SS flags
	mov	Vflag,w0
	mov	w0,SSTempVflag
	mov	Zflag,w0
	mov	w0,SSTempZflag
	mov	Cflag,w0
	mov	w0,SSTempCflag
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	run_ept
	bra	alu_ept
