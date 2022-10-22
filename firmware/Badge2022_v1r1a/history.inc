
; FROM	      SIZE BITS	   PACKED TO BYTE(S)   BITS USED	UNPACKED TO		
;											
; page 0,+1	  32×4		0...15		all		History_Page_0 (16)	
; current page,+1 32×4		16...31		all		History_Visible (16)	
;											
; A16		  16		32,33		all		History_A16		
; w9		  12		34,35		11-0		History_w9		
;											
; w11		  4		36,37		15-12		History_w11		
; w12		  4		36,37		11-8		History_w12		
; w13		  4		36,37		7-4		History_w13		
; w14		  4		36,37		3-0		History_w14		
;											
; Atemp		  4		38,39		15-12		History_Atemp		
; Page		  4		38,39		11-8		History_Page		
; Sync		  4		38,39		7-4		History_Sync		
; Speed		  4		38,39		3-0		History_Speed		
;											
; Vflag		  3		40,41		11-9		History_Vflag		
; Zflag		  3		40,41		8-6		History_Zflag		
; Cflag		  3		40,41		5-3		History_Cflag		
; Stack		  3		40,41		2-0		History_Stack		
;											
; History_Ptr		; pointer for history review, 0...127				
; Insert_Count_0	; counter of inserts written to History buffer from big bang	
; History_Page_0	; page 0 (unpacked, 32 by) from History buffer			
; History_Visible	; visible page (unpacked, 32by) from History buffer		
;											
; .equ	History,0x0B00	; FIFO buffer, space: 128 × 42 by = 5376 by = 0x1500 by		
;											

ss_history:
	cp0	Insert_Count_0		; if no history inserts...
	bra	z,ss_farm		; ...then ignore and return
	bset	BlinkFlag,#3		; #3 set: LED SS blinks
	bset	Flag,#5			; #5 set: in HISTORY mode
history_pos_1:
	mov	#1,w0
	mov	w0,History_Ptr
history_farm:
	cp0	History_Ptr
	bra	z,history_exit
	call	unpack_ptr		; unpack from History buffer, pos History_Ptr
wait_key_hist:				; <---------------
	btss	Flag,#1
	bra	wait_key_hist
	bclr	Flag,#1

	mov	Just,w4
	cp	w4,#1
	bra	z,history_pos_1		; key History again: go pos 1  ---------------->
	cp	w4,#2
	bra	nz,not_hist_minus
;  - ADDR 				
	  inc	  History_Ptr		  ; ++
	  mov	  History_Ptr,w0
	  mov	  Insert_Count_0,w2	  ; count of 24by packets inserted in History buffer from reset
	  inc	  w2,w2

	  cp	  w0,w2
	  bra	  nc,1f			  ; if [History_Ptr] >= [Insert_Count_0] then...
	  dec	  History_Ptr		  ; ...clip [history ptr] to [Insert_Count_0] max
	  bset	  History_A16,#b_load	  ; ...and also LED ADDR+ ON
2:
	  btss	  KeyRotors+6,#0
	  bra	  2b			  ; loop and wait for key released
1:
	  call	  unpack_ptr		  ; UNPACK
	  bra	  history_farm		  ; ---------------->
not_hist_minus:
	cp	w4,#3
	bra	nz,not_hist_plus
;  ADDR	+			
	  dec	  History_Ptr		  ; --
	  bra	  nz,1f			  ; if [History_Ptr] = 0 then...
	  inc	  History_Ptr		  ; ...restore to 1, as history ptr can't reach 0
	  bset	  History_A16,#b_save	  ; ...and also LED ADDR- ON
2:
	  btss	  KeyRotors+4,#0
	  bra	  2b			  ; loop and wait for key released
1:
	  call	  unpack_ptr		  ; UNPACK
	  bra	  history_farm		  ; ---------------->
not_hist_plus:
	cp	w4,#4
	bra	nz,wait_key_hist	; no valid key hit, loop  ---------------->
history_exit:
	clr	History_Ptr
	bra	ss_farm

; -----------------------------------------------------------------------------------
; -----------------------------------------------------------------------------------

; --------------------------------------------------  PACK
; shift buffer up, pack and insert new 21 by (DO NOT USE w5...w15)
insert:			; insert Ram current, Ram page 0 and variables to start of History
; shift FIFO up, to make room for one packet (21 by)
	mov	#History+0x14FE-42,w1
	mov	#History+0x14FE,w2
	repeat	#2667-1		; 127 × 42 = 5334 bytes = 2667 words to move
	mov	[w1--],[w2--]	; shift history buffer up
	inc	Insert_Count_0
	btsc	Insert_Count_0,#7
	dec	Insert_Count_0
; pack proc variables and visible Ram to start of history buffer 22 bytes
pack:			; pack Ram current, Ram page 0 and variables to start of History
	mov	#History,w3	; w3 = history buffer physical address
; --------------------------------------- 0...15 ----- STEP 1: page 0
	mov	#Ram+1,w2
	add	w2,#16,w1
	mov	#8,w4		; loop counter
pack_loop_1:
; get and stack RAM nibbles
	ze	[w1--],w0	; 0000 0000 0000 AAAA
	sl	w0,#4,w0	; 0000 0000 AAAA 0000
	ior.b	w0,[w2--],w0	; 0000 0000 AAAA CCCC
	sl	w0,#4,w0	; 0000 AAAA CCCC 0000
	ior.b	w0,[w1++],w0	; 0000 AAAA CCCC BBBB
	sl	w0,#4,w0	; AAAA CCCC BBBB 0000
	ior.b	w0,[w2++],w0	; AAAA CCCC BBBB DDDD

	mov	w0,[w3++]	; store
	inc2	w1,w1
	inc2	w2,w2
	dec.b	w4,w4
	bra	nz,pack_loop_1	; loop
; -------------------------------------- 16...31 ----- STEP 2: current page
	mov	#Ram+1,w2
	mov.b	Page,WREG
	ze	w0,w0
	sl	w0,#4,w0	; ×16
	add	w2,w0,w2
	add	w2,#16,w1
	mov	#8,w4		; loop counter
; if Page>15 then wrap to 0
	mov.b	Page,WREG
	ze	w0,w0
	cp	w0,#15
	ifz
	sub	#256,w1		; if >=Ram+256 then wrap to start of Ram
pack_loop_2:
; get and stack RAM nibbles
	ze	[w1--],w0	; 0000 0000 0000 AAAA
	sl	w0,#4,w0	; 0000 0000 AAAA 0000
	ior.b	w0,[w2--],w0	; 0000 0000 AAAA CCCC
	sl	w0,#4,w0	; 0000 AAAA CCCC 0000
	ior.b	w0,[w1++],w0	; 0000 AAAA CCCC BBBB
	sl	w0,#4,w0	; AAAA CCCC BBBB 0000
	ior.b	w0,[w2++],w0	; AAAA CCCC BBBB DDDD

	mov	w0,[w3++]	; store
	inc2	w1,w1
	inc2	w2,w2
	dec.b	w4,w4
	bra	nz,pack_loop_2	; loop
; -------------------------------------- 32...35 ----- STEP 3: A16 and w9 (no pack)
	mov	A16,w0
	mov	w0,[w3++]	; store
	mov	w9,[w3++]	; store
; ---------------------------------------- 36,37 ----- STEP 4: w11, w12, w13, w14
	mov	w11,w0
	sl	w0,#4,w0
	ior.b	w0,w12,w0
	sl	w0,#4,w0
	ior.b	w0,w13,w0
	sl	w0,#4,w0
	ior.b	w0,w14,w0
	mov	w0,[w3++]	; store
; ---------------------------------------- 38,39 ----- STEP 5: Atemp,Page,Sync,Speed
	mov	Atemp,w0
	sl	w0,#4,w0
	ior.b	Page,WREG
	sl	w0,#4,w0
	ior.b	Sync,WREG
	sl	w0,#4,w0
	ior.b	Speed,WREG
	mov	w0,[w3++]	; store
; ---------------------------------------- 40,41 ----- STEP 6: V/Z/Cflag, Stack
	mov	Vflag,w0
	sl	w0,#3,w0
	ior.b	Zflag,WREG
	sl	w0,#3,w0
	ior.b	Cflag,WREG
	sl	w0,#3,w0
	ior.b	Stack,WREG
	mov	w0,[w3++]	; store
; ----------------------------------------------------
	return
; ----------------------------------------------------  UNPACK
; unpack from History_Ptr
unpack_ptr:		; unpack using History_Ptr as input
	dec	History_Ptr,WREG
	mov	#42,w1
	mul.uu	w0,w1,w0	; w0 × 42 ---> w0:w1
	mov	#History,w1
	add	w0,w1,w2	; w2 = #History + 42 × (History_Ptr-1)
; unpack from history buffer [w2] 42 bytes to proc variables (Ram ---> History_Visible)
unpack:			; unpack using w2 as pointer
	mov	w2,History_Visible
	add	#32,w2		; skip History Ram, only pointer was needed
	mov	[w2++],w0	; Packet start + 32
	mov	w0,History_A16
	mov	[w2++],w0	; Packet start + 34
	mov	w0,History_w9
	mov	[w2++],w0	; Packet start + 36
	and	w0,#0x0F,w1
	mov	w1,History_w14
	lsr	w0,#4,w1
	and	w1,#0x0F,w1
	mov	w1,History_w13
	lsr	w0,#8,w1
	and	w1,#0x0F,w1
	mov	w1,History_w12
	lsr	w0,#12,w1
	and	w1,#0x0F,w1
	mov	w1,History_w11
	mov	[w2++],w0	; Packet start + 38
	and	w0,#0x0F,w1
	mov	w1,History_Speed
	lsr	w0,#4,w1
	and	w1,#0x0F,w1
	mov	w1,History_Sync
	lsr	w0,#8,w1
	and	w1,#0x0F,w1
	mov	w1,History_Page
	lsr	w0,#12,w1
	and	w1,#0x0F,w1
	mov	w1,History_Atemp
	mov	[w2++],w0	; Packet start + 40
	and	w0,#0x07,w1
	mov	w1,History_Stack
	lsr	w0,#3,w1
	and	w1,#0x07,w1
	mov	w1,History_Cflag
	lsr	w0,#6,w1
	and	w1,#0x07,w1
	mov	w1,History_Zflag
	lsr	w0,#9,w1
	and	w1,#0x07,w1
	mov	w1,History_Vflag
	return

