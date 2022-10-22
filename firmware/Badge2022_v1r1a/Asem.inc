
; r1 ascii lo
; r2 ascii hi
; r6 char gen ptr lo
; r7 char gen ptr next
; r8 char gen ptr hi
; r9 loop count

serial_disp:
	movxn	r0,3	; 0  3 = 9600 baud
	movnnr0	15,5	; 1  baud rate
	movxn	r0,10	; 2  sync N
	movnnr0	15,2	; 3  ---> Sync SFR
	movxn	r0,4	; 4  page 4
	movnnr0	15,0	; 5  ---> Page SFR
	movxn	r0,1	; 6  BIT 0 SET = RxTx to IO con
	movnnr0	15,3	; 7  to WrFlags
	cpr0n	0	; 8
	cpr0n	0	; 9
	cpr0n	0	; A
	cpr0n	0	; B

; takes new character FROM SERIAL PORT, IF NOT ARRIVED THEN CHAR = SPACE
; adr 0xC
	movxn	r1,0	; C  default char 0x20 = space
	movxn	r2,2	; D  default char 0x20 = space
	movr0nn	15,8	; E  Received ---> r0
	cpr0n	0	; F  cp r0 - 0
	skipfm	0b1101	; 10 nz,1
	jrnn	5	; 11 NO NEW CHAR, USE SPACE
	movr0nn	15,7	; 12 SerHigh ---> r0
	andr0n	0b0111	; 13 trim to ascii range 0-127 ;;;;;
	movxy	r2,r0	; 14 SerHigh ---> r2
	movr0nn	15,6	; 15 SerLow ---> r0
	movxy	r1,r0	; 16 SerLow ---> r1
; r1,2-0x20 ---> r1,2
	movxy	r0,r2	; 17
	cpr0n	2	; 18
	skipfm	0b0110	; 19  NC,2
	decy	r2	; 1A  readjust ASCII chargen table for -0x20
	decy	r2	; 1B  readjust ASCII chargen table for -0x20
	cpr0n	0	; 1C
; r1,2 × 10 ---> r6,7,8
	addxy	r1,r1	; 1D  ×2
	adcxy	r2,r2	; 1E  ×2
	movxy	r6,r1	; 1F  r6 <--- r1 (r678 char gen pointer) 
	movxy	r7,r2	; 20  r7 <--- r2 (r678 char gen pointer) 
	movxn	r8,0	; 21  r8 <--- 0  (r678 char gen pointer) 
	addxy	r6,r6	; 22  ×4
	adcxy	r7,r7	; 23  ×4
	adcxy	r8,r8	; 24  ×4
	addxy	r6,r6	; 25  ×8
	adcxy	r7,r7	; 26  ×8
	adcxy	r8,r8	; 27  ×8
	addxy	r6,r1	; 28  ×10 (10 nibbles per character)
	adcxy	r7,r2	; 29  ×10 (10 nibbles per character)
	adcxy	r8,r15	; 2A  ×10 (addc,0)
; r6,7,8 + 0x100 ----------> r6,7,8
	incy	r8	; 2B

	movxn	r9,5	; 2C  × 5
	movpcnn	3	; 2D  subroutine @ 0x038
	movxn	jsr,8	; 2E  call  0x038 -----------> (shift 5 columns from char gen)
	dszy	r9	; 2F
	jrnn	-4	; 30  loop

	movxn	r6,0	; 31
	movxn	r7,0	; 32
	movxn	r8,1	; 33  point to blank (between chars)
	movpcnn	3	; 34  subroutine @ 0x038
	movxn	jsr,8	; 35  call  0x038 -----------> (shift 1 column, space between chars)

	movpcnn	0	; 36
	movxn	pclo,12	; 37  jump to 00C (loop)  ----------->

;	cpr0n	0
;	cpr0n	0
;	cpr0n	0
;	cpr0n	0

; adr 038 (subroutine) gets pixels from char gen (pointer r6r7r8)
; (subroutine) shifts and loads one column
; wait for sync first
	movr0nn	15,4	; 40  RdFlags
	andr0n	0b0001	; 41  Bit #0 = UserSync
	skipfm	0b1101	; 42  nz,1
	jrnn	-4	; 43  loop

	movxy	pchi,r8	; 44
	movxy	pcm,r7	; 45
	movxy	jsr,r6	; 46  call (read text low nib)  ----------->
	movnnr0	2,15	; 47
	incy	jsr	; 48  call (read text high nib)  ----------->
	movnnr0	3,15	; 49

	movxn	r0,2	; 4A
	addxy	r6,r0	; 4B
	skipfm	0b1111	; 4C  nz,3
	incy	r7	; 4D
	skipfm	0b1101	; 4E  nz,1
	incy	r8	; 4F
; shift screen down
	exrn	10	; 50  exchange registers (save)

	movxn	r6,14	; 51  rd ptr
	movxn	r7,15	; 52  wr ptr
	movxn	r1,4	; 53  hi nib ptr page 4
	movxn	r2,5	; 54  hi nib ptr page 5
	movxn	r8,15	; 55  loop counter
; jrnn here
	movr0xy	r1,6	; 56  rd
	movxyr0	r1,7	; 57  wr
	movr0xy	r2,6	; 58  rd
	movxyr0	r2,7	; 59  wr

	decy	r6	; 5A  decrement rd ptr
	decy	r7	; 5B  decrement wr ptr
	dszy	r8	; 5C  decrement loop counter
	jrnn	-8	; 5D  if counter>0, loop

	exrn	10	; 5E  exchange registers (restore)

	movr0nn	2,15	; 5F  get new 4 pixels...
	movnnr0	4,0	; 60  ...and put them on visible page
	movr0nn	3,15	; 61  get new 4 pixels...
	movnnr0	5,0	; 62  ...and put them on visible page

	retr0n	0	; 63	; ----------->

	.word	0x8000		; terminator
