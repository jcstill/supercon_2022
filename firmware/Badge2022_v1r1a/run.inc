;         INPUT variables for indAn and indGn subroutines:
; w7   Result write address
; w8   IN register, #Ram+0x0B or #Ram+0xFF
; w9   #Program Memory Address + #Rom (++ after reading)
; w10  #Ram (used for R0...R15 read/write)
; w11  OperX contents
; w12  OperY contents
; w13  Accumulator IN
; w14  Accumulator OUT
; 
;         OUTPUT variables for indAn and indGn subroutines:
; w5   Address/Displacement for JR/RET/SKIP/DSZ/EXR_N/MOV_PC,NN
; w6   Transfer flags (bits 3210)
; w7   Dest address or flag for JR/RET/SKIP/DSZ/EXR_N/MOV_PC,NN
; w8   must be unchanged (#Ram+0x0B or #Ram+0xFF)
; w9   incremented or modified by instruction (if pgm branches)
; w10  must be unchanged (#Ram)
; w11  don't care (displayed value)
; w12  don't care (displayed value)
; w13  Accumulator IN
; w14  Accumulator OUT
;
;            w6 (Transfer Flag) on exit:
; #0 set: Propagate Accumulator after this instruction
; #1 set: Transfer Carry flag after this instruction
; #2 set: Transfer Zero flag after this instruction
; #3 set: Transfer Overflow flag after this instruction

run_ept:
	btss	Flag2,#12
	call	clr_ram
	bset	Flag2,#12
; initialize or reload RUN variables from RUN Temp
	mov	RunTempPMAddr,w9 ; PC
	mov	RunTempStack,w0
	mov	w0,Stack
	mov	RunTempPage,w0
	mov.b	WREG,Page	; restore Page
	mov	RunTempw14,w14
	mov	RunTempw13,w13
; restore RUN flags
	mov	RunTempVflag,w0
	mov	w0,Vflag
	mov	RunTempZflag,w0
	mov	w0,Zflag
	mov	RunTempCflag,w0
	mov	w0,Cflag
	bclr	A16,#b_carry
	btsc	Cflag,#2
	bset	A16,#b_carry
; RUN initialize
	mov	#2,w0		; 2 = RUN
	mov	w0,Mode
	mov	#0,w0		; default speed 0 (max)
	call	set_T2_from_w0
	mov	#0,w0		; default sync 0 (fastest)
	call	set_T3_from_w0
	mov	#Ram,w10	; const
	mov	#Ram+0x0B,w8	; const
	btsc.b	WrFlags,#1	; I/O port select
	mov	#Ram+0xFB,w8	; const
	mov	#0b001110000,w0	; autorpt Opcode, OperX, OperY
	mov	w0,AutorptFlag
	bclr	Flag,#9		; #9 set if instruction is writing to SFR area
	bclr	Flag,#5		; #5 set in HISTORY mode
	clr	w13		; w13 is Accumulator IN, displayed as ACCU IN
	clr	w14		; w14 is Accumulator OUT, displayed as ACCU OUT
; prepare A16 (set RUN and keep Bin/Sel)
	mov	#0b1000000010000000,w0	; keep BIN and SEL only
	and	A16
	mov	#0b0000001000001000,w0	; set RUN and -CLK
	ior	A16

	bclr	Flag,#9
	bclr	Flag,#10
run_wait_key:		; outside pgm execution
	bclr	Flag2,#1	; #1 program Runs 
	bclr	A16,#b_clk_key	; LED RUN off
1:
	bclr	Flag,#3		; #3 set = no RAM matrix display
	btss	Flag,#1		; #1 handshaking flag for buttons
	bra	1b
	bclr	Flag,#1
	mov	Just,w0
	cp	w0,#5
	ifc
	mov	#5,w0
	bra	w0
; ...
	bra	leave_run_ept	; mode ++
	bra	run_wait_key	; run_fast
	bra	run_wait_key	; run_pause
	bra	run_wait_key	; run_break
	bra	run_run
	bra	run_keys_5
;				
run_keys_5:
; 13 buttons OPCODE 8... DATA IN
	call	limited_key_3	; can't affect pgm data, only Sync Clock Page
	call	set_T2_from_Speed
	call	set_T3_from_Sync
	bra	run_wait_key

; ------------------------------------------------
run_run:
	bclr	U1MODE,#15	; disable UART 1 to clear queue and error flags
	bset	U1MODE,#15	; enable UART 1
	bset	U1STAL,#UTXEN	; enable TX 1 (must be after UART enable)
	bset	U1STAL,#URXEN	; enable RX 1 (must be after UART enable)

	bclr	BlinkFlag,#2	; 2 set = LED Run blinks
	mov	#RX_buf,w0
	mov	w0,RXWR
	mov	w0,RXRD
	bset	Flag2,#1	; #1 program Runs 
	mov	#0b000000000,w0	; no autorpt in runtime
	mov	w0,AutorptFlag
	bset	A16,#b_clk_key	 ; LED RUN on
	bclr	A16,#b_carry
	bclr	Flag2,#5	; #5 Fatal Stack Error
	mov	#Rom,w9
	call	read12
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
	call	read12		; READ 12
run_farm:
; OUT port service
	mov.b	Ram+0x0A,WREG	; OUT register if WrFlags,#1 = 0
	btsc.b	WrFlags,#1	; I/O port select
	mov.b	Ram+0xFA,WREG	; OUT register if WrFlags,#1 = 1
	mov.b	WREG,LATB	; OUT port
; check if Stack Error occured
	btsc	Flag2,#5	; #5 Fatal Stack Error
	bra	Run_StackErr
; check if runtime key pressed
	bclr	Flag,#4		; #4 set if instruction allows CALL and JP (mov, inc, dec)
	btsc	Flag,#1		; #1 handshaking flag for buttons
	bra	runtime_key_0	; key pressed during pgm execution
runtime_back_0:
	bclr	Flag2,#11	; #11 = previous Flag,#2
	btsc	Flag,#2		; #2 set if Speed > 0 or if Mode <> 2
	bset	Flag2,#11	; #11 = previous Flag,#2
; [PC] ---> w12,OperY, w11,OperX, w0,Opcode
	mov	[w9],w0		; read instruction
	and	w0,#0x0F,w12
	mov	w12,OperY	; w12 = OperY
	lsr	w0,#4,w11
	and	w11,#0x0F,w11
	mov	w11,OperX	; w11 = OperX
	lsr	w0,#8,w0
	mov	w0,Opcode	; w0 = Opcode
	bra	z,group8run	; ---------> 8-bit opcode
; table jump, w0 = instruction
	mul.uu	w0,#6,w0
	bra	w0		; execution step 1 (4-bit opcode)
;jp table
	.word	0,0,0,0,0,0	; non-existing instruction
	callRUN	ind_4_1		; ADD  RX,RY
	callRUN	ind_4_2		; ADC  RX,RY
	callRUN	ind_4_3		; SUB  RX,RY
	callRUN	ind_4_4		; SBB  RX,RY
	callRUN	ind_4_5		; OR   RX,RY
	callRUN	ind_4_6		; AND  RX,RY
	callRUN	ind_4_7		; XOR  RX,RY
	callRUN	ind_4_8		; MOV  RX,RY (TEST PCL)
	callRUN	ind_4_9		; MOV  RX,N (TEST PCL)
	callRUN	ind_4_10	; MOV  [XY],R0
	callRUN	ind_4_11	; MOV  R0,[XY]
	callRUN	ind_4_12	; MOV  [NN],R0
	callRUN	ind_4_13	; MOV  R0,[NN]
	callRUN	ind_4_14	; MOV  PC,NN
	callRUN	ind_4_15	; JR   NN
;...
group8run:
; table jump, w0 = instruction
	mul.uu	w11,#6,w0
	bra	w0		; execution step 1 (8-bit opcode)
;jp table
	callRUN	ind_8_0		; CP   R0,N
	callRUN	ind_8_1		; ADD  R0,N
	callRUN	ind_8_2		; INC  RY (TEST PCL)
	callRUN	ind_8_3		; DEC  RY (TEST PCL)
	callRUN	ind_8_4		; DSZ  RY
	callRUN	ind_8_5		; OR   R0,N
	callRUN	ind_8_6		; AND  R0,N
	callRUN	ind_8_7		; XOR  R0,N
	callRUN	ind_8_8		; EXR  N
	callRUN	ind_8_9		; BIT  RG,M
	callRUN	ind_8_10	; BSET RG,M
	callRUN	ind_8_11	; BCLR RG,M
	callRUN	ind_8_12	; BTG  RG,M
	callRUN	ind_8_13	; RRC  RY
	callRUN	ind_8_14	; RET  R0,N
	callRUN	ind_8_15	; SKIP F,M
;...
; #######################################################################################

runtime_key_0:
	mov	#0,w0
runtime_key:			; key pressed during pgm execution (don't trash w5-w15!)
	mov	w0,BackIndex
	bclr	Flag,#1
	mov	Just,w0
	cp	w0,#5
	ifc
	mov	#5,w0
	bra	w0
; ...
	bra	runtime_back_X	; runtime mode (just ignore)
	bra	runtime_fast 	; runtime fast 
	bra	runtime_pause	; runtime pause
	bra	runtime_break	; runtime break
	bra	runtime_back_X	; runtime run (just ignore)
	bra	runtime_keys_5
;...............................
runtime_fast:			; runtime FAST
	btg	Flag2,#0	; #0 Run Fast toggler (1=fast)
	btg	BlinkFlag,#2	; 2 set = LED Run blinks
	call	set_T2_from_Speed
	call	set_T3_from_Sync
;...
runtime_back_X:		; uses BackIndex to return exactly where it came from
	mov	BackIndex,w0
	cp0	w0
	bra	z,runtime_back_0 ; if [BackIndex] = 0
	dec	w0,w0
	bra	z,runtime_back_1 ; if [BackIndex] = 1
	bra	runtime_back_2	 ; if [BackIndex] = 2
;				
runtime_keys_5:
; 13 buttons OPCODE 8... DATA IN
	push	w5	; \
	call	limited_key_3	; can't affect pgm data, only Dim Clock Page
	call	set_T2_from_Speed
	call	set_T3_from_Sync
	pop	w5	; /
	bra	runtime_back_X
;-----------------------------------------------
runtime_pause:			; runtime PAUSE
	bclr	Flag2,#1	; #1 program Runs 
	bset	A16,#b_noclk_sch
	bclr	A16,#b_clk_sch
	bset	A16,#b_save
	bset	BlinkFlag,#5	; 5 set = LED Save (Pause) blinks
	bclr	A16,#b_clk_sch
	bset	A16,#b_noclk_sch
1:
	btss	Flag,#1		; #1 handshaking flag for buttons
	bra	1b
	bclr	Flag,#1
	mov	Just,w0
	cp	w0,#5
	ifc
	mov	#5,w0
	bra	w0
; ...
	bra	runtime_pause	; pause mode (just ignore)
	bra	pause_exit 	; pause fast (exit from pause)
	bra	pause_exit	; pause pause (exit from pause)
	bra	runtime_break	; pause break (break pgm execution)
	bra	pause_exit	; pause run (exit from pause)
	bra	general_pause_5	; keys 6...18
;................................
general_pause_5:
; 13 buttons OPCODE 8... DATA IN
	push	w5	; \
	call	limited_key_3	; can't affect pgm data, only Dim Clock Page
	call	set_T2_from_Speed
	call	set_T3_from_Sync
	pop	w5	; /
	bra	runtime_pause
; ------------------------------
pause_exit:
	bset	Flag2,#1	; #1 program Runs 
	bclr	A16,#b_save
	bclr	BlinkFlag,#5	; 5 set = LED Save (Pause) blinks
	bra	runtime_back_X	; continue execution
;------------------------------------------------
Run_StackErr:		; Stack Error occured
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
runtime_break:			; runtime BREAK
	bclr	Flag2,#5	; #5 Fatal Stack Error
	bclr	A16,#b_clk_sch
	bset	A16,#b_noclk_sch
	bclr	A16,#b_clk_key
	bset	A16,#b_noclk_sch
	bclr	A16,#b_clk_sch
	bclr	A16,#b_save
	bclr	BlinkFlag,#5	; 5 set = LED Save (Pause) blinks
	mov	#0b001110000,w0	; autorpt Opcode, OperX, OperY
	mov	w0,AutorptFlag
; clear SFR and spec registers
	bclr	A16,#b_carry
	mov	#Rom,w9
	call	read12
;	call	clr_ram
;	clr	Stack
;	clr.b	Page		; default page 0
;	clr.b	Speed		; default Clock max
;	clr.b	Sync		; default Sync max
	clr.b	WrFlags		; Leds on, Matrix on, IOPos 0xA-0xB, RxTxPos on SAO
	clr.b	Received	; clear queue
	mov	#15,w0
	mov.b	Wreg,Dimmer	; default Dimmer max
	mov	#0b0011,w0
	mov.b	Wreg,SerCtrl	; default UART
	  call	  rxtx_to_rxtxpos ; RxTx restored back to RxTxpos
	  call	  set_U1BRG	  ; BRG restored to SerCtrl (bits 210)
; clear Fast parameters
	  bclr	  BlinkFlag,#2		; 2 set = LED Run blinks
	  btss	  Flag2,#0		; #0 Run Fast toggler (1=fast)
	  bra	  not_running_fast
	  bclr	  Flag2,#0		; #0 Run Fast toggler (1=fast)
	  call	  set_T2_from_Speed
	  call	  set_T3_from_Sync
not_running_fast:
	mov	#0,w0		; default speed 0 (max)
	call	set_T2_from_w0
	mov	#0,w0		; default sync 0 (fastest)
	call	set_T3_from_w0
	bra	run_wait_key

; ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;

run_cont:			; phases (ONLY IF SPEED>0)
wait_phase_1:
	  mov	  #1,w0
	  btsc	  Flag,#1	; #1 handshaking flag for buttons
	  bra	  runtime_key	; if key pressed in runtime
runtime_back_1:
	  btss	  IFS0,#7	; test int flag
	  bra	  wait_phase_1
	  bclr	  IFS0,#7	; clr int flag
;	
	  call	  transfer1	; uses w6 bits to decide flag transfer
wait_phase_2:
	  mov	  #2,w0
	  btsc	  Flag,#1	; #1 handshaking flag for buttons
	  bra	  runtime_key	; if key pressed in runtime
runtime_back_2:
	  btss	  IFS0,#7	; test int flag
	  bra	  wait_phase_2
	  bclr	  IFS0,#7	; clr int flag
	  call	  transfer2	; uses w6 bits to decide flag transfer
; ------
ins_part_2:			; called from SS mode
	incpc			; increment address     * * * * * * * * * * * * * * * * *
; check if w7 < 8, if so perform special instruction
	cp	w7,#8		; flag for special instructions (if w7 < 8)
	bra	nc,spec_ins
	mov.b	w13,[w7]	;  W R I T E   R E S U L T   T O   D E S T I N A T I O N
; check if destination was JSR or PCL, if so, then perform CALL or JUMP
	btss	Flag,#4		; #4 set if instruction allows CALL and JP (mov, inc, dec)
	bra	chk_sfr
	bclr	Flag,#4		; #4 set if instruction allows CALL and JP (mov, inc, dec)
; check and perform if there is PCM, PCH 12-bit extension
	  mov	  PcmPch,w0
	  btsc	  Flag,#7	; #7 set if there is PCM/PCH extension
	  mov	  w0,Ram+14	; PcmPch ---> PCM, PCH (extension restored)
	  bclr	  Flag,#7	; #7 set if there is PCM/PCH extension
	mov	#Ram+12,w0	; Ram+12=JSR
	sub	w7,w0,w1	; w7= destination address
	bra	z,run_call	; - - - - - - - - - - - - - - - - -  RUN CALL  ---->
	dec	w1,w1		; Ram+13=JUMP
	bra	z,run_jump	; - - - - - - - - - - - - - - - - -  RUN JUMP  ---->
; check if source or dest was in SFR area, if so perform perpheral service
chk_sfr:
	  btsc	  Flag,#9		; #9 set if instruction is writing to SFR area
	  call	  perform_SFR_write
	  btsc	  Flag,#10		; #10 set if instruction is reading from SFR area
	  call	  perform_SFR_read
	return
;------------------------
spec_ins:			; <------ executed both from SS and RUN
	bra	w7
;...
	bra	spec0	; dummy
	bra	spec1	; JR   N
	bra	spec2	; DSZ  RY
	bra	spec3	; EXR  N
	bra	spec4	; RET R0,N
	bra	spec5	; LPC  NN
	bra	spec6	; SKIP F,M
	bra	spec7	; reserved
; --------------------
spec0:			; flag 00 = dummy instruction
	return
;		
spec1:			; flag 01 = special instruction: JR NN
	btsc	w5,#8		; displacement is signed 9-bit number
	sub	#0x200,w5	; if displacement is negative
	add	w9,w5,w9	; new PC value
w9range:
	mov	#0x2000,w0
	btsc	w9,#14
	sub	w9,w0,w9	; fit w9 < end of program memory
	btss	w9,#13
	add	w9,w0,w9	; fit w9 > start-1 of program memory
	return
;		
spec2:			; flag 02 = special instruction: DSZ RY
	mov.b	w13,[w5]	; w13 = RY after decrement, w5 = RY address
	cp0	w13
	ifnz
	return
	inc2	w9,w9		; skip (inrement PC)
	btsc	w9,#14
	mov	#Rom,w9		; wrap to start of program memory
	return
;		
spec3:			; flag 03 = special instruction: EXR N
	btg	A16,#b_exr
	goto	swapper
;		
spec4:			; flag 04 = special instruction: RET R0,N
	cp0	Stack
	bra	z,stackerr7	; stack underflow
	mov.b	w12,[w10]	; store #N to R0
	mov	w5,w9		; store ret address to PC
	dec	Stack		; new Stack
	return
;		
spec5:			; flag 05 = special instruction: MOV PC,NN
	mov.b	w5,[w10+15]	; write hi nibble to PCH (irregular, but possible use of w5)
	mov.b	w8,[w10+14]	; write low nibble to PCL (irregular, but possible use of w8)
	mov	#Ram+0x0B,w8	; restore w8
	btsc.b	WrFlags,#1	; I/O port select
	mov	#Ram+0xFB,w8	; restore w8
	return
;		
spec6:			; flag 06 = special instruction: SKIP F,M
	mov	w5,w9		; store new address to PC
	return
;		
spec7:			; flag 07 is reserved
	return
;------------------------------------------
stackerr6:			; Fatal Error, Stack overflow
	mov	#6,w0		; Stack=6: Overflow (5+1)
	bra	skrerr
stackerr7:			; Fatal Error, Stack underflow
	mov	#7,w0		; Stack=7: Underflow (0-1)
skrerr:
	mov	w0,Stack
	bset	Flag2,#5	; #5 Fatal Stack Error
	return

;#################################################################################
run_call:			; - - - - - - - - - - - - - - - - -  RUN CALL   <----
	mov	Stack,w0
	cp	w0,#5
	bra	c,stackerr6	; stack overflow
	mul.uu	w0,#3,w0	; w0:w1
	add	#16,w0		; stack starts at Ram+16
	add	w0,w10,w1	; w1 = current stack
; push ret addr on stack
	mov	#Rom,w0
	sub	w9,w0,w9
	lsr	w9,w9
	and	w9,#0x0F,w0
	mov.b	w0,[w1++]	; push low ret addr on stack
	lsr	w9,#4,w0
	and	w0,#0x0F,w0
	mov.b	w0,[w1++]	; push med ret addr on stack
	lsr	w9,#8,w0
	and	w0,#0x0F,w0
	mov.b	w0,[w1++]	; push hi ret addr on stack
	inc	Stack
; load new ProgMemAddr from registers RC RE RF
	mov.b	Ram+12,WREG
	bra	skr_run
run_jump:			; - - - - - - - - - - - - - - - - -  RUN JUMP   <----
	mov.b	Ram+13,WREG
skr_run:
	ze	w0,w1
	mov.b	Ram+15,WREG
	sl	w0,#4,w0
	ior.b	Ram+14,WREG
	ze	w0,w0
	sl	w0,#4,w0
	ior.b	w0,w1,w0
	mov	w0,w9		; correction, as SS farm increments Prog Mem Addr
	sl	w9,w9		; ×2
	mov	#Rom,w0
	add	w9,w0,w9
	return

; ----------------------------------------------------------------------------------- 
; ------------------------     I N S T R U C T I O N S     -------------------------- 
; ----------------------------------------------------------------------------------- 
; Label Name Rules:								      
; IND subroutines: with INDirectly addressed operands (SS and RUN modes)              
; DIR subroutines: with DIRectly addressed operands (ALU mode)                        
; First digit: _4 means 4-bit operand						      
; First digit: _8 means 8-bit operand						      
; Second digit _0..._15 is Opcode (or Opcode extension) digit			      

ind_4_1:   ; ------------------------------------------------------   1   ADD RX,RY (49T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	ze	[w7],w11	; X value
	mov.b	[w10+w12],w12	; Y value
dir_4_1:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds		; uses w0
; exec Adder Sum & Carry
	sl	w12,#4,w0	; w0 = source × 16
	ior	w11,w0,w0	; w0 = source × 16 + dest
	sl	w0,#2,w0	; w0 = 4 × (source × 16 + dest)
	mov	#tbloffset(table_adder),w2
	add	w0,w2,w0	; w0 = #table_adder + 4 × (source × 16 + dest)
	tblrdl	[w0],w0		; 0000 0VZC AAAA CCCC
	lsr	w0,#4,w0	; 0000 0000 0VZC AAAA
	and	w0,#0x0F,w13	; write result to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow3flags1111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	clr	Vflag
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	btsc	w0,#6
	mov	w2,Vflag	; if V flag set
	btsc	w0,#5
	mov	w2,Zflag	; if Z flag set

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	w0,#4
	return
	mov	w2,Cflag	; if C flag set
	bset	A16,#b_carry	; Carry button LED
	return

slow3flags1111:
	mov	#0b1111,w6	; move VZCA
slow3flags_keepw6:
	bclr	Vflag,#0
	bclr	Zflag,#0
	bclr	Cflag,#0
	btsc	w0,#6
	bset	Vflag,#0
	btsc	w0,#5
	bset	Zflag,#0
	btsc	w0,#4
	bset	Cflag,#0
	return

ind_4_2:   ; ------------------------------------------------------   2   ADC RX,RY (52T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	ze	[w7],w11	; X value
	mov.b	[w10+w12],w12	; Y value
dir_4_2:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	bclr	A16,#b_data_inv
	bset	A16,#b_cena
	mov	#A16,w0
	btst.c	[w0],#b_carry	; A16,#b_carry ---> C
	mov	#b_cin,w3
	bsw.c	[w0],w3		; C ---> A16,#b_cin
; exec Adder Sum & Carry
	sl	w12,#4,w0	; w0 = source × 16
	ior	w11,w0,w0	; w0 = source × 16 + dest
	sl	w0,#2,w0	; w0 = 4 × (source × 16 + dest)
	mov	#tbloffset(table_adder),w2
	add	w0,w2,w0	; w0 = #table_adder + 4 × (source × 16 + dest)
	btsc	A16,#b_cin	; Extra bit 16: C IN (schematic) 
	inc2	w0,w0		; if Cin set
	tblrdl	[w0],w0		; 0000 0VZC AAAA CCCC
	lsr	w0,#4,w0	; 0000 0000 0VZC AAAA
	and	w0,#0x0F,w13	; write result to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow3flags1111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	clr	Vflag
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	btsc	w0,#6
	mov	w2,Vflag	; if V flag set
	btsc	w0,#5
	mov	w2,Zflag	; if Z flag set

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	w0,#4
	return
	mov	w2,Cflag	; if C flag set
	bset	A16,#b_carry	; Carry button LED
	return

ind_4_3:   ; ------------------------------------------------------   3   SUB RX,RY (50T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	ze	[w7],w11	; X value
	mov.b	[w10+w12],w12	; Y value
dir_4_3:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
	xor	w12,#0x0F,w12	; inv Y value (will be displayed)
; maintain logic diagram
	bset	A16,#b_data_inv
	bclr	A16,#b_cena
	bset	A16,#b_cin	; subtract = inv (add) with inv (C)
; exec Adder Sum & Carry
	sl	w12,#4,w0	; w0 = source × 16
	ior	w11,w0,w0	; w0 = source × 16 + dest
	sl	w0,#2,w0	; w0 = 4 × (source × 16 + dest)
	mov	#tbloffset(table_adder)+2,w2
	add	w0,w2,w0	; w0 = #table_adder + 4 × (source × 16 + dest)
	tblrdl	[w0],w0		; 0000 0VZC AAAA CCCC
	lsr	w0,#4,w0	; 0000 0000 0VZC AAAA
	and	w0,#0x0F,w13	; write result to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow3flags1111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	clr	Vflag
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	btsc	w0,#6
	mov	w2,Vflag	; if V flag set
	btsc	w0,#5
	mov	w2,Zflag	; if Z flag set

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	w0,#4
	return
	mov	w2,Cflag	; if C flag set
	bset	A16,#b_carry	; Carry button LED
	return

ind_4_4:   ; ------------------------------------------------------   4   SBB RX,RY (54T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	ze	[w7],w11	; X value
	mov.b	[w10+w12],w12	; Y value
dir_4_4:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
	xor	w12,#0x0F,w12	; inv Y value (will be displayed)
; maintain logic diagram
	bset	A16,#b_data_inv
	bset	A16,#b_cena
	mov	#A16,w0
	btst	[w0],#b_carry	; A16,#b_carry ---> C
	ccf
	mov	#b_cin,w3
	bsw	[w0],w3		; C ---> A16,#b_cin
	ccf
; exec Adder Sum & Carry
	sl	w12,#4,w0	; w0 = source × 16
	ior	w11,w0,w0	; w0 = source × 16 + dest
	sl	w0,#2,w0	; w0 = 4 × (source × 16 + dest)
	mov	#tbloffset(table_adder),w2
	add	w0,w2,w0	; w0 = #table_adder + 4 × (source × 16 + dest)
	btsc	A16,#b_cin	; Extra bit 16: C IN (schematic) 
	inc2	w0,w0		; if Cin set
	tblrdl	[w0],w0		; 0000 0VZC AAAA CCCC
	lsr	w0,#4,w0	; 0000 0000 0VZC AAAA
	and	w0,#0x0F,w13	; write result to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow3flags1111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	clr	Vflag
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	btsc	w0,#6
	mov	w2,Vflag	; if V flag set
	btsc	w0,#5
	mov	w2,Zflag	; if Z flag set

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	w0,#4
	return
	mov	w2,Cflag	; if C flag set
	bset	A16,#b_carry	; Carry button LED
	return

ind_4_5:   ; ------------------------------------------------------   5   OR RX,RY (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	mov.b	[w10+w11],w11	; X value
	mov.b	[w10+w12],w12	; Y value
dir_4_5:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds
; exec
	ior	w11,w12,w13	; exec
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowZflag0101
				; fast simplified Z flag (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14
	mov	#7,w2
	clr	Zflag
	ifz
	mov	w2,Zflag

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return
slowZflag0101:
	mov	#0b0101,w6	; move VZCA
	bclr	Zflag,#0
	ifz
	bset	Zflag,#0
	return

ind_4_6:   ; ------------------------------------------------------   6   AND RX,RY (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	mov.b	[w10+w11],w11	; X value
	mov.b	[w10+w12],w12	; Y value
dir_4_6:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds
; exec
	and	w11,w12,w13	; exec
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowZflag0101
				; fast simplified Z flag (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14
	mov	#7,w2
	clr	Zflag
	ifz
	mov	w2,Zflag

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

ind_4_7:   ; ------------------------------------------------------   7   XOR RX,RY (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	mov.b	[w10+w11],w11	; X value
	mov.b	[w10+w12],w12	; Y value
dir_4_7:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds
; exec
	xor	w11,w12,w13	; exec
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowZflag0101
				; fast simplified Z flag (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14
	mov	#7,w2
	clr	Zflag
	ifz
	mov	w2,Zflag

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

ind_4_8:   ; ------------------------------------------------------   8   MOV RX,RY (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	ze	[w7],w11	; X value
	mov.b	[w10+w12],w12	; Y value
dir_4_8:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
	bset	Flag,#4		; #4 set if instruction covers CALL and JP (mov, inc, dec)
; maintain logic diagram
	clr3leds
; exec
	mov	w12,w13		; exec
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowAccu0001
				; fast simplified exit (no flags)
	mov	w13,Atemp
	mov	w13,w14
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

; check if destination was JSR or PCL, if so perform CALL or JUMP
	mov	#Ram+12,w0	; Ram+12=JSR
	sub	w7,w0,w1	; w7= destination address
	bra	z,run_call	; - - - - - - - - - - - - - - - - -  RUN CALL  ---->
	dec	w1,w1		; Ram+13=JUMP
	bra	z,run_jump	; - - - - - - - - - - - - - - - - -  RUN JUMP  ---->
	return
slowAccu0001:
	mov	#0b0001,w6	; move VZCA
	return

ind_4_9:   ; ------------------------------------------------------   9   MOV RX,N (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect values from registers
	add	w10,w11,w7	; result write address
	mov.b	[w10+w11],w11	; X value
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
	bset	Flag,#4		; #4 set if instruction covers CALL and JP (mov, inc, dec)
; maintain logic diagram
	clr3leds
; exec
	mov	w12,w13		; exec
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowAccu0001
				; fast simplified exit (no flags)
	mov	w13,Atemp
	mov	w13,w14
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

; check if destination was JSR or PCL, if so perform CALL or JUMP
	mov	#Ram+12,w0	; Ram+12=JSR
	sub	w7,w0,w1	; w7= destination address
	bra	z,run_call	; - - - - - - - - - - - - - - - - -  RUN CALL  ---->
	dec	w1,w1		; Ram+13=JUMP
	bra	z,run_jump	; - - - - - - - - - - - - - - - - -  RUN JUMP  ---->
	return

ind_4_10:   ; -----------------------------------------------------   10  MOV [XY],R0 (T)
; get indirect values from registers
	mov.b	[w10+w11],w11	; X value
	mov.b	[w10+w12],w12	; Y value
	sl	w11,#4,w7	; 16 × X
	add	w7,w12,w7	; 16 × X + Y
	add	w7,w10,w7	; 16 × X + Y + Ram (result write address)

	  cp	  w11,#15
	  ifz
	  bset	  Flag,#9	; #9 set if instruction is writing to SFR area
	  mov	  w12,w5	; SFR service will need the low nibble of SFR address

	clr	w11		; clear 1st operand displayed
	ze	[w7],w14	; pre-load Accu out
	mov	w14,Atemp	; pre-load Accu temp
	ze	[w10],w12	; source (2nd) operand displayed
; maintain logic diagram
	clr3leds
; exec
	mov	w12,w13
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowAccu0001
				; fast simplified exit (no flags)
	mov	w13,Atemp
	mov	w13,w14
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	Flag,#9		; #9 set if instruction is writing to SFR area
	return
	bra	perform_SFR_write

ind_4_11:   ; -----------------------------------------------------   11  MOV R0,[XY] (T)
; get indirect values from registers
	mov.b	[w10+w11],w11	; X value
	mov.b	[w10+w12],w12	; Y value
	mov	w10,w7		; dst addr to w7 (just for convention)
	ze	[w7],w14	; pre-load Accu out
	mov	w14,Atemp	; pre-load Accu temp

	sl	w11,#4,w1	; 16 × X
	add	w1,w12,w1	; 16 × X + Y
	add	w1,w10,w1	; 16 × X + Y + Ram (source address)

	  cp	  w11,#15
	  bra	  nz,nosfr1
	  bset	  Flag,#10	; #10 set if instruction is reading from SFR area
	  mov	  w12,w5	; SFR service will need the low nibble of SFR address
nosfr1:
	clr	w11		; clear 1st operand displayed
	ze	[w1],w12	; source (2nd) operand displayed
; maintain logic diagram
	clr3leds
; exec
	mov	w12,w13
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowAccu0001
				; fast simplified exit (no flags)
	mov	w13,Atemp
	mov	w13,w14
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	Flag,#10	; #10 set if instruction is reading from SFR area
	return
	bra	perform_SFR_read

ind_4_12:   ; -----------------------------------------------------   12  MOV [NN],R0 (T)
	sl	w11,#4,w7	; 16 × X
	add	w7,w12,w7	; 16 × X + Y
	add	w7,w10,w7	; 16 × X + Y + Ram (result write address)

	  cp	  w11,#15
	  ifz
	  bset	  Flag,#9	; #9 set if instruction is writing to SFR area
	  mov	  w12,w5	; SFR service will need the low nibble of SFR address

	clr	w11		; clear 1st operand displayed
	ze	[w7],w14	; pre-load Accu out
	mov	w14,Atemp	; pre-load Accu temp
	ze	[w10],w12	; source (2nd) operand displayed
; maintain logic diagram
	clr3leds
; exec
	mov	w12,w13
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowAccu0001
				; fast simplified exit (no flags)
	mov	w13,Atemp
	mov	w13,w14
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	Flag,#9		; #9 set if instruction is writing to SFR area
	return
	bra	perform_SFR_write


ind_4_13:   ; -----------------------------------------------------   13  MOV R0,[NN] (T)
	mov	w10,w7		; dst addr to w7 (just for convention)
	ze	[w7],w14	; pre-load Accu out
	mov	w14,Atemp	; pre-load Accu temp

	sl	w11,#4,w1	; 16 × X
	add	w1,w12,w1	; 16 × X + Y
	add	w1,w10,w1	; 16 × X + Y + Ram (source address)

	  cp	  w11,#15
	  bra	  nz,nosfr2
	  bset	  Flag,#10	; #10 set if instruction is reading from SFR area
	  mov	  w12,w5	; SFR service will need the low nibble of SFR address
nosfr2:
	clr	w11		; clear 1st operand displayed
	ze	[w1],w12	; source (2nd) operand displayed
; maintain logic diagram
	clr3leds
; exec
	mov	w12,w13
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowAccu0001
				; fast simplified exit (no flags)
	mov	w13,Atemp
	mov	w13,w14
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	Flag,#10	; #10 set if instruction is reading from SFR area
	return
	bra	perform_SFR_read

ind_4_14:   ; -----------------------------------------------------   14  MOV PC,NN (T)
; maintain logic diagram
	clr3leds

	mov	w11,w5
	mov	w12,w8
	clr	w11		; clear 1st operand displayed
	clr	w12		; clear 2nd operand displayed

	mov	#0x05,w7	; flag "MOV PC,NN" instead of "MOV W13,[W7]"
	mov	w14,w13
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	phases_0

	incpc			; w9++

	bra	spec5
phases_0:
	mov	#0b0000,w6	; move VZCA
	return

ind_4_15:   ; -----------------------------------------------------   15  JR NN (T)
; maintain logic diagram
	clr3leds
; exec
	sl	w11,#4,w5	; 16 × X
	add	w5,w12,w5	; 16 × X + Y (8-bit displacement addr)
	sl	w5,w5		; ×2, now in 9-bit range 0...0x1FE (signed -0x100...0xFF)
	clr	w11		; clear 1st operand displayed
	clr	w12		; clear 2nd operand displayed

	mov	#0x01,w7	; flag "JR NN" instead of "MOV W13,[W7]"
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	phases_0
				; fast simplified exit (no flags)
	incpc			; w9++

	bra	spec1

; ###################################################################################

ind_8_0:   ; -----------------------------------------------------    00  CP R0,N (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect value from register R0
	ze	[w10],w11	; X value (R0 is on Ram+0 addr)
dir_8_0:
	xor	w12,#0x0F,w12	; inv Y value (will be displayed)
; maintain logic diagram
	bset	A16,#b_data_inv
	bclr	A16,#b_cena
	bset	A16,#b_cin	; subtract = inv (add) with inv (C)
; exec Adder Sum & Carry
	sl	w12,#4,w0	; w0 = source × 16
	ior	w11,w0,w0	; w0 = source × 16 + dest
	sl	w0,#2,w0	; w0 = 4 × (source × 16 + dest)
	mov	#tbloffset(table_adder)+2,w2
	add	w0,w2,w0	; w0 = #table_adder + 4 × (source × 16 + dest)
	tblrdl	[w0],w0		; 0000 0VZC AAAA CCCC
	lsr	w0,#4,w0	; 0000 0000 0VZC AAAA
	and	w0,#0x0F,w13	; result to Accu IN
	mov	#Dummy,w7
; get flags
	mov	#0b1110,w6	; move VZCA
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow3flags_keepw6
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	clr	Vflag
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	btsc	w0,#6
	mov	w2,Vflag	; if V flag set
	btsc	w0,#5
	mov	w2,Zflag	; if Z flag set

	incpc			; w9++

	btss	w0,#4
	return
	mov	w2,Cflag	; if C flag set
	bset	A16,#b_carry	; Carry button LED
	return

ind_8_1:   ; -----------------------------------------------------    01  ADD R0,N (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect value from register R0
	ze	[w10],w11	; X value (R0 is on Ram+0 addr)
	mov	w10,w7		; result write address
dir_8_1:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds
; exec Adder Sum & Carry
	sl	w12,#4,w0	; w0 = source × 16
	ior	w11,w0,w0	; w0 = source × 16 + dest
	sl	w0,#2,w0	; w0 = 4 × (source × 16 + dest)
	mov	#tbloffset(table_adder),w2
	add	w0,w2,w0	; w0 = #table_adder + 4 × (source × 16 + dest)
	tblrdl	[w0],w0		; 0000 0VZC AAAA CCCC
	lsr	w0,#4,w0	; 0000 0000 0VZC AAAA
	and	w0,#0x0F,w13	; write result to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow3flags1111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	clr	Vflag
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	btsc	w0,#6
	mov	w2,Vflag	; if V flag set
	btsc	w0,#5
	mov	w2,Zflag	; if Z flag set

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	btss	w0,#4
	return
	mov	w2,Cflag	; if C flag set
	bset	A16,#b_carry	; Carry button LED
	return

ind_8_2:   ; -----------------------------------------------------    02  INC RY (T)
; get indirect values from OperY
	add	w10,w12,w7	; result write address
	mov.b	[w10+w12],w12	; Y value
dir_8_2:
	mov	w12,Atemp	; pre-load Accu temp
	mov	w12,w14		; pre-load Accu out
	mov	#1,w11		; "add 1" = "inc" (will be displayed)
	bset	Flag,#4		; #4 set if instruction covers CALL and JP (mov, inc, dec)
; maintain logic diagram
	clr3leds
; exec 
	inc	w12,w0		; exec
	and	w0,#0x0F,w13	; write result to Accu in
	sub	w7,w10,w3
	sub	#12,w3
	and	#0x0FE,w3	; will be 0 only for registers PCL or JSR (12 or 13)
	bra	nz,4f		; if not registers PCL or JSR
	cp0	Mode
	bra	z,4f		; no extension in ALU mode
; extension to PCM and PCH
	cp0	w13
	bra	nz,4f		; if no overflow from nibble 1
	  bset	  Flag,#7	; #7 set if there is PCM/PCH extension
	  mov	  Ram+14,w0
	  mov	  w0,PcmPch	; PCM, PCH ---> PcmPch
	  inc.b	  PcmPch+0	; inc PCM
	  btss.b  PcmPch+0,#4	; PCM overflow?
	  bra	  4f		; if no overflow
	  bclr.b  PcmPch+0,#4	; keep PCM in 4-bit range
	  cp0.b	  PcmPch+0	; is there overflow from PCM nibble?
	  ifz			; if overflow from PCM nibble...
	  inc.b	  PcmPch+1	; ...then inc PCH...
	  bclr.b  PcmPch+1,#4	; ...and keep it in 4-bit range
4:
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow0111

	incpc			; w9++

; fast simplified ZC flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	mov.b	w13,[w7]	; write result to destination
	  mov	  PcmPch+14,w0
	  btsc	  Flag,#7	; #7 set if there is PCM/PCH extension
	  mov	  w0,Ram+14	; PcmPch ---> PCM, PCH (extension restored)
	  bclr	  Flag,#7	; #7 set if there is PCM/PCH extension
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	cp0	w13
	bra	nz,no_write_02
	mov	w2,Zflag	; if result zero, Z flag set
	mov	w2,Cflag	; if overflow, C flag set
	bset	A16,#b_carry	; Carry button LED
no_write_02:
; check if destination was JSR or PCL, if so perform CALL or JUMP
	mov	#Ram+12,w0	; Ram+12=JSR
	sub	w7,w0,w1	; w7= destination address
	bra	z,run_call	; - - - - - - - - - - - - - - - - -  RUN CALL  ---->
	dec	w1,w1		; Ram+13=JUMP
	bra	z,run_jump	; - - - - - - - - - - - - - - - - -  RUN JUMP  ---->
	return
slow0111:
	mov	#0b0111,w6	; move VZCA
	bclr	Zflag,#0
	bclr	Cflag,#0
	cp0	w13
	bra	nz,goback
	bset	Zflag,#0	; if result zero, Z flag set
	bset	Cflag,#0	; if overflow, C flag set
goback:
	return

ind_8_3:   ; -----------------------------------------------------    03  DEC RY (T)
; get indirect values from OperY
	add	w10,w12,w7	; result write address
	mov.b	[w10+w12],w12	; Y value
dir_8_3:
	mov	w12,Atemp	; pre-load Accu temp
	mov	w12,w14		; pre-load Accu out
	mov	#0x0F,w11	; "add -1" = "dec" (will be displayed)
	bset	Flag,#4		; #4 set if instruction covers CALL and JP (mov, inc, dec)
; maintain logic diagram
	clr3leds
; exec 
	dec	w12,w0		; exec
	and	w0,#0x0F,w13	; write result to Accu in
	sub	w7,w10,w3
	sub	#12,w3
	and	#0x0FE,w3	; will be 0 only for registers PCL or JSR (12 or 12)
	bra	nz,4f		; if not registers PCL or JSR
	cp0	Mode
	bra	z,4f		; no extension in ALU mode
; extension to PCM and PCH
	cp0	w12
	bra	nz,4f		; if PCL or JSR was not 0 before dec
	  bset	  Flag,#7	; #7 set if there is PCM/PCH extension
	  mov	  Ram+14,w0
	  mov	  w0,PcmPch	; PCM, PCH ---> PcmPch
	  dec.b	  PcmPch+0	; dec PCM
	  btss.b  PcmPch+0,#7	; PCM underflow?
	  bra	  4f		; if no underflow
	mov	#0x0F,w0
	and.b	PcmPch+0	; mask low nibble bits of PCM
	dec.b	PcmPch+1	; dec PCH
	and.b	PcmPch+1	; mask low nibble bits of PCH
4:
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow03

	incpc			; w9++

; fast simplified ZC flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	mov.b	w13,[w7]	; write result to destination
	  mov	  PcmPch+14,w0
	  btsc	  Flag,#7	; #7 set if there is PCM/PCH extension
	  mov	  w0,Ram+14	; PcmPch ---> PCM, PCH (extension restored)
	  bclr	  Flag,#7	; #7 set if there is PCM/PCH extension
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	cp0	w13
	ifz
	mov	w2,Zflag	; if zero, Z flag set
	cp	w13,#0x0F
	ifnz
	return
	mov	w2,Cflag	; if "dec" did not cause result 0x1111
	bset	A16,#b_carry	; Carry button LED
	return
slow03:
	mov	#0b0111,w6	; move VZCA
	bclr	Zflag,#0
	bclr	Cflag,#0
	cp0	w13
	ifz
	bset	Zflag,#0	; if zero, Z flag set
	cp	w13,#0x0F
	ifnz
	bset	Cflag,#0	; if "dec" did not cause result 0x1111
	return

ind_8_4:   ; -----------------------------------------------------    04  DSZ RY (T)
; get indirect values from OperY
	add	w10,w12,w7	; result write address
	ze	[w7],w12	; Y value
dir_8_4:
	mov	w12,Atemp	; pre-load Accu temp
	mov	w12,w14		; pre-load Accu out
	mov	#0x0F,w11	; "add -1" = "dec" (will be displayed)
; maintain logic diagram
	clr3leds
; exec 
	dec	w12,w2		; exec
	and	w2,#0x0F,w13	; w13 = new Accu value
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow04

	incpc			; w9++
				; fast simplified execution (no flags)
	mov.b	w13,[w7]	; write result to destination
	mov	w13,w14
	mov	w13,Atemp
	cp0	w13
	ifnz
	return

	incpc			; w9++

	return
slow04:
	mov	w12,Atemp	; pre-load Accu temp
	mov	w12,w14		; pre-load Accu out
	mov	w7,w5		; new destination address (not w7)!
	mov	#0x02,w7	; flag "DSZ RY" instead of "MOV W13,[W7]"
	mov	#0b0000,w6	; move VZCA
	return

ind_8_5:   ; -----------------------------------------------------    05  OR R0,N (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect value from register R0
	ze	[w10],w11	; X value (R0 is on Ram+0 addr)
	mov	w10,w7		; result write address
dir_8_5:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds
; exec 
	ior	w11,w12,w0
	and	w0,#0x0F,w13	; write result to Accu in
; get flags
	setc			; SET C unconditionally
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowZCAflag0111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	mov	#7,w2		; constant to write to in/temp/out flag
	clr	Zflag
	cp0	w13
	ifz
	mov	w2,Zflag	; if Z flag set
	mov	w2,Cflag	; SET C unconditionally
	bset	A16,#b_carry	; Carry button LED unconditionally
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

slowZCAflag0111:
	mov	#0b0111,w6	; move VZCA
	bclr	Zflag,#0
	ifz
	bset	Zflag,#0
	bclr	Cflag,#0
	ifc
	bset	Cflag,#0
	return

ind_8_6:   ; -----------------------------------------------------    06  AND R0,N (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect value from register R0
	ze	[w10],w11	; X value (R0 is on Ram+0 addr)
	mov	w10,w7		; result write address
dir_8_6:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds
; exec 
	and	w11,w12,w0
	and	w0,#0x0F,w13	; write result to Accu in
; get flags
	clrc			; CLR C unconditionally
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowZCAflag0111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	mov	#7,w2		; constant to write to in/temp/out flag
	clr	Zflag
	cp0	w13
	ifz
	mov	w2,Zflag	; if Z flag set
	clr	Cflag		; clr C unconditionally
	bclr	A16,#b_carry	; Carry button LED unconditionally
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

ind_8_7:   ; -----------------------------------------------------    07  XOR R0,N (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect value from register R0
	ze	[w10],w11	; X value (R0 is on Ram+0 addr)
	mov	w10,w7		; result write address
dir_8_7:
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds
; exec 
	clrc
	btss	A16,#b_carry
	setc			; CPL C unconditionally
	xor	w11,w12,w0
	and	w0,#0x0F,w13	; write result to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowZCAflag0111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	mov	#7,w2		; constant to write to in/temp/out flag
	clr	Zflag
	bclr	A16,#b_carry	; Carry button LED
	cp0	w13
	ifz
	mov	w2,Zflag	; Z flag

	btsc	Cflag,#2
	clr	w2		; w2,#2,1,0 = complement Cflag,#2
	mov	w2,Cflag	; cpl C unconditionally (bits 2,1,0)
	
	btsc	w2,#0
	bset	A16,#b_carry	; Carry button LED

	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

ind_8_8:   ; -----------------------------------------------------    08  EXR N (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF

	clr	w11		; 1st operand = 0 (what else?)
; maintain logic diagram
	clr3leds
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slow08

	incpc			; w9++
; exec 
swapper:
	mov	w12,w2
	cp0	w2
	ifz
	mov	#16,w2		; 0 members ---> 16 members
	mov	#Ram,w3
	mov	#Ram+(14*16),w4	; page 14
2:
	ze	[w4],w0		; [page 14] ---> w0
	mov.b	[w3],[w4++]	;  [page 0] ---> [page 14]
	mov.b	w0,[w3++]	;        w0 ---> [page 0]
	dec	w2,w2
	bra	nz,2b		; swap loop -------->
	return
slow08:
	mov	#0x03,w7	; flag "EXR N" instead of "MOV W13,[W7]"
	mov	#0b0000,w6	; move VZCA
	return

ind_8_9:   ; -----------------------------------------------------    09  BIT RG,M (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect value from RG (bits 3,2 of OperY)
	and	w12,#0x0C,w0	; bits 3,2 = register
	lsr	w0,#2,w0
	cp	w0,#3
	bra	nz,1f		; if reading from reg 0, 1, 2
; if reading from reg 3 (IN port)
	mov	#0x0B,w0	; R0A (IN A)
	btsc.b	WrFlags,#1	; WRFlags,  InOutPos
	mov	#0xFB,w0	; RFA (IN B)
1:
	add	w10,w0,w7	; result write address
dir_8_9:
	ze	[w7],w11	; value of addressed nibble
	ze	[w7],w13	; this cancels later writing to [w7] (in routine "phases")
	and	w12,#3,w2	; w2 = M isolated
	mov	#1,w0
	sl	w0,w2,w12	; now w12 is with addressed bit set
; exec 
	and	w12,w11,w0	; only flag Z is the result
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowZflag0100
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	#7,w2		; constant to write to in/temp/out flag
	clr	Zflag
	ifz
	mov	w2,Zflag	; if Z flag set

	incpc			; w9++

	return
slowZflag0100:
	mov	#0b0100,w6	; move VZCA
	bclr	Zflag,#0
	ifz
	bset	Zflag,#0
	return

ind_8_10:   ; -----------------------------------------------------    0A  BSET RG,M (T)
; get indirect value from RG (bits 3,2 of OperY)
	and	w12,#0x0C,w0	; bits 3,2 = register
	lsr	w0,#2,w0
	cp	w0,#3
	bra	nz,1f		; if writing to reg 0, 1, 2
; if writing to reg 3 (OUT port)
	mov	#0x0A,w0	; R0A (OUT A)
	btsc.b	WrFlags,#1	; WRFlags,  InOutPos
	mov	#0xFA,w0	; RFA (OUT B)
1:
	add	w10,w0,w7	; result write address
dir_8_10:
	ze	[w7],w11	; value of addressed nibble
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
	and	w12,#3,w2	; w2 = M isolated
	mov	#1,w0
	sl	w0,w2,w12	; now w12 is with addressed bit set
; maintain logic diagram
	clr3leds
; exec 
	ior	w12,w11,w13	; w11 with bit set to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	phases_0
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

ind_8_11:   ; -----------------------------------------------------    0B  BCLR RG,M (T)
; get indirect value from RG (bits 3,2 of OperY)
	and	w12,#0x0C,w0	; bits 3,2 = register
	lsr	w0,#2,w0
	cp	w0,#3
	bra	nz,1f		; if writing to reg 0, 1, 2
; if writing to reg 3 (OUT port)
	mov	#0x0A,w0	; R0A (OUT A)
	btsc.b	WrFlags,#1	; WRFlags,  InOutPos
	mov	#0xFA,w0	; RFA (OUT B)
1:
	add	w10,w0,w7	; result write address
dir_8_11:
	ze	[w7],w11	; value of addressed nibble
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
	and	w12,#3,w2	; w2 = M isolated
	mov	#1,w0
	sl	w0,w2,w12	; now w12 is with addressed bit set
; maintain logic diagram
	clr3leds
; exec 
	xor	w12,#0x0F,w0
	and	w0,w11,w13	; w11 with bit reset to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	phases_0
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

ind_8_12:   ; -----------------------------------------------------    0C  BTG RG,M (T)
; get indirect value from RG (bits 3,2 of OperY)
	and	w12,#0x0C,w0	; bits 3,2 = register
	lsr	w0,#2,w0
	cp	w0,#3
	bra	nz,1f		; if writing to reg 0, 1, 2
; if writing to reg 3 (OUT port)
	mov	#0x0A,w0	; R0A (OUT A)
	btsc.b	WrFlags,#1	; WRFlags,  InOutPos
	mov	#0xFA,w0	; RFA (OUT B)
1:
	add	w10,w0,w7	; result write address
dir_8_12:
	ze	[w7],w11	; value of addressed nibble
	mov	w11,Atemp	; pre-load Accu temp
	mov	w11,w14		; pre-load Accu out
	and	w12,#3,w2	; w2 = M isolated
	mov	#1,w0
	sl	w0,w2,w12	; now w12 is with addressed bit set
; maintain logic diagram
	clr3leds
; exec 
	xor	w12,w11,w13	; w11 with bit flipped to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	phases_0
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

ind_8_13:   ; -----------------------------------------------------    0D  RRC RY (T)
; IN register service
	mov	PORTB,w0
	lsr	w0,#4,w0
	and	#0x0F,w0
	mov.b	w0,[w8]		; #Ram+0x0B or #Ram+0xFF
; get indirect value from OperY
	add	w10,w12,w7	; result write address
	mov.b	[w10+w12],w12	; Y value
dir_8_13:
	clr	w11		; clr oper 1 (will be displayed)
	mov	w12,Atemp	; pre-load Accu temp
	mov	w12,w14		; pre-load Accu out
; maintain logic diagram
	clr3leds
; exec 
	btsc	A16,#b_carry
	bset	w12,#4		; Carry flag for 4-bit operand
	lsr	w12,w13		; write result straight to Accu in
; get flags
	btsc	Flag,#2		; #2 set if Speed>0
	bra	slowZCAflag0111
				; fast simplified VZCA flags (no bit #1 and #2 shifts)
	mov	w13,Atemp
	mov	w13,w14		; propagate result to all Accumulator steps
	clr	Zflag
	clr	Cflag
	bclr	A16,#b_carry	; Carry button LED
	mov	#7,w2		; constant to write to in/temp/out flag
	ifz
	mov	w2,Zflag	; if Z flag set
	ifc
	mov	w2,Cflag	; if C flag set
	ifc
	bset	A16,#b_carry	; Carry button LED
	mov.b	w13,[w7]	; write result to destination

	incpc			; w9++

	return

ind_8_14:   ; -----------------------------------------------------    0E  RET R0,N (T)
	clr	w11		; will be displayed
	mov.b	w12,w13		; preset #N to Acc in
	mov.b	[w10],w14	; preset previous R0 to Acc out
	mov	w14,Atemp	; preset previous R0 to Acc temp
	mov	#0x04,w7	; flag "RET R0,N" instead of "MOV W13,[W7]"
; maintain logic diagram
	clr3leds
; exec
	mov	Stack,w0
	mul.uu	w0,#3,w0	; w0:w1
	add	#16-1,w0	; stack starts at Ram+16
	add	w0,w10,w1	; stack starts at Ram+16
	ze	[w1--],w0	; high nibble of ret addr
	sl	w0,#4,w0
	ior.b	w0,[w1--],w0	; medium nibble of ret addr
	sl	w0,#4,w0
	ior.b	w0,[w1],w0	; low nibble of ret addr
	sl	w0,w0		; ×2
	mov	#Rom,w5
	add	w0,w5,w5	; ret address
	btss	Flag,#2		; #2 set if Speed>0
	bra	spec4
	bra	phases_0

ind_8_15:   ; -----------------------------------------------------    0F  SKIP F,M (T)
	clr	w11		; will be displayed
	mov	#0x06,w7	; flag "SKIP F,M" instead of "MOV W13,[W7]"
; maintain logic diagram
	clr3leds
; exec
	and	w12,#3,w0	; w0 skip format
	ifz
	mov	#4,w0		; skip 0 = skip 4
	sl	w0,w0		; ×2
	lsr	w12,#2,w1	; w1 = condition
	and	#3,w1
	bra	w1
;.........................
	bra	skip_C
	bra	skip_NC
	bra	skip_Z
	bra	skip_NZ
skip_C:
	mov	w9,w5
	btsc	A16,#b_carry	; cc0: C
	add	w5,w0,w5	; skip
	bra	skr_skip
skip_NC:
	mov	w9,w5
	btss	A16,#b_carry	; cc1: NC
	add	w5,w0,w5	; skip
	bra	skr_skip
skip_Z:
	mov	w9,w5
	btsc	Zflag,#2	; cc2: Z
	add	w5,w0,w5	; skip
	bra	skr_skip
skip_NZ:
	mov	w9,w5
	btss	Zflag,#2	; cc3: NZ
	add	w5,w0,w5	; skip
skr_skip:
	inc2	w5,w5		; because INC2 W5 will not be executed like INC2 W9
	btsc	w5,#14
	mov	#Rom,w5		; wrap to start of program memory

	btsc	Flag,#2		; #2 set if Speed>0
	bra	phases_0

	mov	w5,w9
	bra	w9range

; ---------------------------------------------------------------------------
set_T2_from_Speed:
	mov.b	Speed,WREG
set_T2_from_w0:
	and	#0x0F,w0
	mov.b	WREG,Speed	; Speed index, bit 3210
	clr	TBLPAG
	mov	#tbloffset(clk_table),w1
	btsc	Flag2,#0	; #0 set if FAST mode
	mov	#tbloffset(clk_table_high),w1
	bclr	T2CON,#TON	; stop Timer 2
	clr	TMR2		; clear timing counter
	  bset	  Flag,#2	; #2 set if Speed > 0 or if Mode <> 2
	  cp0.b	  w0
	  ifz
	  bclr	  Flag,#2	; #2 set if Speed > 0 or if Mode <> 2
	bclr	IFS0,#7		; clr int flag
	sl	w0,#2,w0	; × 4
	add	w1,w0,w0	; table + Speed × 4
	tblrdl	[w0++],w1
	mov	w1,PR2		; update T2 divider value
	tblrdl	[w0++],w0
	mov.b	WREG,T2CON	; update prescaler value (low byte only)
	  dec2	  Mode,WREG
	  ifnz
	  bset	  Flag,#2	; #2 set if Speed > 0 or if Mode <> 2
	bset	T2CON,#TON	; start Timer 2
	return

; ---------------------------------------------------------------------------
set_T3_from_Sync:
	mov.b	Sync,WREG
set_T3_from_w0:
	and	#0x0F,w0
	mov.b	WREG,Sync	; Sync index, bit 3210
	clr	TBLPAG
	mov	#tbloffset(sync_table),w1
	btsc	Flag2,#0	; #0 set if FAST mode
	mov	#tbloffset(sync_table_high),w1
	bclr	T3CON,#TON	; stop Timer 3
	clr	TMR3		; clear timing counter
	bclr	IFS0,#8		; clr int flag
	bclr.b	RdFlags,#0	; clr Sync flag
	sl	w0,#2,w0	; × 4
	add	w1,w0,w0	; table + Sync × 4
	tblrdl	[w0++],w1
	mov	w1,PR3		; update T3 divider value
	tblrdl	[w0++],w0
	mov.b	WREG,T3CON	; update prescaler value (low byte only)
	bset	T3CON,#TON	; start Timer 3
	return
; ------------------------------------------------------------------------------

rnd:			     ; RND to w0 bits 3-0
    ; rnd seed ---> w4:w5
	mov	Rndlo,w4     ; existing lo
	mov	Rndhi,w5     ; existing hi
    ; w4:w5 × 0x41c64e6d + 0x6073 ---> w4:w5
	mov	#0x41c6,w3   ; hi
	mul.uu	w4,w3,w2     ; lo * hi ---> w2:w3
	mov	#0x4e6d,w3   ; lo
	mul.uu	w5,w3,w0     ; hi * lo ---> w0:w1
	add	w2,w0,w0
	mul.uu	w4,w3,w4     ; lo * lo ---> w4:w5
	add	w5,w2,w5     ; hi + hi ---> hi
	add	w5,w0,w5     ; hi + hi ---> hi
	mov	#0x6073,w3
	add	w4,w3,w4     ; lo + 0x6073 --> lo
	addc	w5,#0,w5     ; hi + Carry ---> hi
    ; w4:w5 ---> rnd seed
	mov	w4,Rndlo     ; new lo
	mov	w5,Rndhi     ; new hi
extract4bits:
	xor	w4,w5,w0
	add.b	WREG0+1,WREG
	asr	w0,#4,w2
	xor	w0,w2,w0
	and	w0,#0x0F,w0  ; RND in w0 bits 3-0
	return

getrnd:
	mov	Rndlo,w4     ; existing lo
	mov	Rndhi,w5     ; existing hi
	bra	extract4bits

; ------------------------------------------------------------------------------
; ------------------------------------------------------------------------------

perform_SFR_write:
	bclr	Flag,#9		; #9 set if instruction is writing to SFR area
	and	#0x0f,w5
	bra	w5
;...
	return
	bra	SFR_write_1
	bra	SFR_write_2
	bra	SFR_write_3
	return
	bra	SFR_write_5
	bra	SFR_write_6
	return
	bra	SFR_write_8
	bra	SFR_write_9
	return
	return
	return
	return
	return
	bra	SFR_write_15
;...
SFR_write_1:		; Speed
	call	set_T2_from_Speed
	return
;	
SFR_write_2:		; Sync
	call	set_T3_from_Sync
	return
;	
SFR_write_3:		; WrFlags (PinPos); in: SerCtrl,#3=0...1,  out: RPINR, RPOR
	mov.b	SerCtrl,WREG
sel_pinpos:
	btss.b	WrFlags,#0	; #0 RxTxPos
	bra	RxTx0
; RxTxPos=1 (Rx and Tx are on I/O connector)
	btsc	Flag2,#3	; #3 Previous RxTxPos
	bra	WrFlags_next	; if nothing new on RxTxPos
	  call	  rxtx_to_io	; SWITCH RxTx to IO
	bra	WrFlags_next
RxTx0:
	btss	Flag2,#3	; #3 Previous RxTxPos
	bra	WrFlags_next	; if nothing new on RxTxPos
	  call	  rxtx_to_sao	; SWITCH RxTx to SAO
WrFlags_next:
	return
;	
SFR_write_5:		; SerCtrl (SERVICE BaudRate)
	and	w13,#0x07,w0
	bra	set_U1BRG_from_w0
;	
SFR_write_6:		; SerLow (TX BYTE)
	mov.b	SerHigh,WREG
	sl	w0,#4,w0
	ior.b	SerLow,WREG
waittx_2:
	btsc	U1STA,#UTXBF
	bra	waittx_2
	mov.b	WREG,U1TXREG	; ----> transmit w0
	return
;	
SFR_write_8:		; Received
	cp0.b	Received
	bra	nz,go_cont_8_2
	mov	#RX_buf,w0
	mov	w0,RXWR
	mov	w0,RXRD
go_cont_8_2:
	ifnz
	return
;	
SFR_write_9:		; AutoOff
; as the destination is AutoOff, AutoOff_Hi must be preloaded
	mov	#37500,w0
	mov	w0,AutoOff_Hi
	return
;	
SFR_write_15:		; RND
	cp.b	w13,#15
	bra	z,seed15
	sl	w13,#4,w0
	ior	w13,w0,w0	; 4 ---> 8 bits
	sl	w0,#8,w13
	ior	w13,w0,w0	; 8 ---> 16 bits
	mov	w0,Rndhi
	mov	w0,Rndlo
	and	#0x0F,w0
	mov.b	WREG,Ram+0xFF	; write to RND SFR
	return
seed15:			; if seed=15, then total RND
	mov	TMR2,w0
	sl	w0,#4,w0
	ior.b	RowScan,WREG
	sl	w0,#4,w0
	ior.b	TMR1,WREG
	mov	w0,Rndhi
	mov	AutoOff_Hi,w1
	xor	w0,w1,w0
	mov	w0,Rndlo
	and	#0x0F,w0
	mov.b	WREG,Ram+0xFF	; write to RND SFR
	return

; ------------------------------------------------------------------------------

perform_SFR_read:
	bclr	Flag,#10	; #10 set if instruction is reading from SFR area
	and	#0x0f,w5
	bra	w5
;...
	return
	return
	return
	return
	bra	SFR_read_4
	bra	SFR_read_5
	bra	SFR_read_6
	return
	return
	return
	bra	SFR_read_10
	return
	return
	return
	return
	bra	SFR_read_15
;...
SFR_read_4:		; RdFlags
	bclr	IEC0,#8		; disable Timer 3 interrupt
	mov.b	RdFlags,WREG
	and.b	Ram+0,WREG
	btss	w0,#0
	bra	1f
	mov.b	RdFlags,WREG
	mov.b	WREG,Ram+0	; write to R0 again, timing is critical!
	and	#0b1110,w0	; kill flag UserSync (#0)
	mov.b	WREG,RdFlags
1:
	bset	IEC0,#8		; enable Timer 3 interrupt
	return
;	
SFR_read_5:
	bclr	SerCtrl,#3
	return
;	
SFR_read_6:		; SerLow
	cp0.b	Received
	ifz
	return			; if zero received
	bclr	IEC0,#11   ; \    disable RX1 interrupt
; move char from SerLow to r0
	mov.b	SerLow,WREG
	mov.b	w0,[w7]
	bclr	Flag2,#2	; clr handshaking flag (for FIFO ---> SerHigh:SerLow)
; test if there is anything in FIFO...
	mov	RXRD,w0
	cp.b	RXWR
	bra	z,calc_rcvd	; if FIFO empty
; ...checked, FIFO > 0, move 1st char from FIFO to ser buf Ser L:H and inc RDPtr
	ze	[w0],w1		; w1 1st byte from FIFO
	and	w1,#0x0F,w0	; low nibble
	mov.b	WREG,SerLow
	lsr	w1,#4,w0	; hi nibble
	mov.b	WREG,SerHigh
	bset	Flag2,#2	; set handshaking flag (for FIFO ---> SerHigh:SerLow)
	inc.b	RXRD		; inc RD ptr (8-bit inc, to stay inside FIFO)
	bset	Flag2,#2	; set handshaking flag (for FIFO ---> SerHigh:SerLow)
calc_rcvd:
; calculate Received
	clr	w0		; to clear high byte
	mov.b	RXWR,WREG
	sub.b	RXRD,WREG	; RXRD-RXWR (byte mode, to stay in range 0000-00FF)
	btsc	Flag2,#2	; handshaking flag (for FIFO ---> SerHigh:SerLow)
	inc	w0,w0		; plus char in Ser H:L (if any)
	cp	w0,#15
	ifc
	mov	#15,w0		; clip to 15
	mov.b	WREG,Received
	bset	IEC0,#11   ; /    enable RX1 interrupt
	return
;	
SFR_read_10:		; KeyStatus
	disi	#4
	mov.b	KeyStatus,WREG
	mov.b	WREG,Ram+0	; write to R0
	bclr	KeyStatus,#0
	return
;	
SFR_read_15:		; RND
	call	getrnd
	mov.b	WREG,Ram+0	; write to R0
new_rnd:
	call	rnd
	mov.b	WREG,Ram+0xFF	; write to RND SFR
	return

; ---------------------------------------------------------------------------
def_U1BRG:		; set Baud Rate to 9600
	mov	#3,w0
	bra	set_U1BRG_from_w0
set_U1BRG:		; restore the original Baud Rate
	mov.b	SerCtrl,WREG
	and	#7,w0
set_U1BRG_from_w0:	; in: w0,  out: U1BRG
	bclr	U1MODE,#15		; disable UART 1
	clr	TBLPAG
	sl	w0,w0		; ×2
	mov	#tbloffset(baud_tab),w1
	add	w0,w1,w1
	tblrdl	[w1],w0
	mov	w0,U1BRG
	bset	U1MODE,#15		; enable UART 1
	nop
	bset	U1STAL,#UTXEN	; enable TX 1 (must be after UART enable)
	bset	U1STAL,#URXEN	; enable RX 1 (must be after UART enable)
	clr.b	Received
	mov	#RX_buf,w0
	mov	w0,RXWR
	mov	w0,RXRD
	return
baud_tab:
	.word	3333-1	;  0.  1200
	.word	1667-1	;  1.  2400
	.word	833-1	;  2.  4800
	.word	417-1	;  3.  9600
	.word	208-1	;  4.  19200
	.word	104-1	;  5.  38400
	.word	69-1	;  6.  57600
	.word	35-1	;  7.  115200
; ---------------------------------------------------
rxtx_to_rxtxpos:
	btsc.b	WrFlags,#0	; WrFlags,#0: RxTxPos
	bra	rxtx_to_io	; if RxTxPos=1
rxtx_to_sao:
; B7 input now (GP input)
	bclr	U1MODE,#15		; disable UART 1
	bset	TRISB,#7
; Peripheral Pin Select <---- Rx and Tx pins to SAO connector
	mov	#26,w0		; RP26 PORT  (A0)... (0x3f to disable RX input)
	mov.b	WREG,RPINR18	; ...to U1_RX
	mov	#3,w0		; #3=U1_TX...     (0x00 to return to default OUT port)
	mov.b	RPOR13+1	; ...to RP27 (A1)
; disable old pin for Tx
	clr.b	RPOR3+1		; ...to RP7  (B7) (0x00 to return to default OUT port)
	bclr	Flag2,#3	; #3 Previous RxTxPos
	bra	skr_rxtx
;----------------------------------------------------
rxtx_to_io:
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
skr_rxtx:
	bset	U1MODE,#15		; enable UART 1
	clr.b	Received
	mov	#RX_buf,w0
	mov	w0,RXWR
	mov	w0,RXRD
	return

; ---------------------------------------------------------------------------------
; ---------------------------------------------------------------------------------
; ---------------------------------------------------------------------------------

leave_run_ept:
; save RUN variables to RUN Temp
	mov	Stack,w0
	mov	w0,RunTempStack	; Stack
	mov	w9,RunTempPMAddr; PC
	mov.b	Page,WREG
	mov.b	WREG,RunTempPage ; Page
; save Run flags
	mov	Vflag,w0
	mov	w0,RunTempVflag
	mov	Zflag,w0
	mov	w0,RunTempZflag
	mov	Cflag,w0
	mov	w0,RunTempCflag
	mov	w14,RunTempw14
	mov	w13,RunTempw13
	btss	Flag,#6		; #6 set if ALT pressed (debounced)
	bra	pgm_ept
	bra	ss_ept
