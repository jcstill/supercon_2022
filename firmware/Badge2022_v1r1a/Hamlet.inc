
; r1 ascii lo
; r2 ascii hi
; r3 text ptr lo
; r4 text ptr next
; r5 text prt hi
; r6 char gen ptr lo
; r7 char gen ptr next
; r8 char gen ptr hi
; r9 loop count

display:
	movxn	r0,10	; 0  sync N
	movnnr0	15,2	; 1  ---> Sync SFR
	movxn	r0,4	; 2  page 4
	movnnr0	15,0	; 3  ---> Page SFR
; adr 4
	movxn	r3,0	; 4
	movxn	r4,0	; 5
	movxn	r5,0x5	; 6  text pointer r345, start at 0xc00
; adr 7
; gets new character from text (pointer r345) and calculates column addr
; read char @ r345 to r12
	movxy	pchi,r5	; 7
	movxy	pcm,r4	; 8
	movxy	jsr,r3	; 9  call (read char from ASCII text)  ----------->
	movxy	r1,r0	; A  r1 ascii char low nib
	incy	jsr	; B  call (read char from ASCII text)  ----------->
	movxy	r2,r0	; C  r2 ascii char high nib
; char in r1 (low) r2 (high)
	andxy	r0,r1	; D  just test for ascii text terminator 0xFF
	incy	r0	; E
	movpcnn	0	; F
	skipfm	0b1101	; 10  nz,1
	movxn	pclo,4	; 11  jump to 004 (start)  ----------->


; r1,2-0x20 ---> r1,2
	decy	r2	; 12  readjust ASCII chargen table for -0x20
	decy	r2	; 13  readjust ASCII chargen table for -0x20
; r1,2 × 10 ---> r6,7,8
	addxy	r1,r1	; 14  ×2
	adcxy	r2,r2	; 15  ×2
	movxy	r6,r1	; 16  r6 <--- r1 (r678 text pointer) 
	movxy	r7,r2	; 17  r7 <--- r2 (r678 text pointer) 
	movxn	r8,0	; 18  r8 <--- 0  (r678 text pointer) 
	addxy	r6,r6	; 19  ×4
	adcxy	r7,r7	; 1A  ×4
	adcxy	r8,r8	; 1B  ×4
	addxy	r6,r6	; 1C  ×8
	adcxy	r7,r7	; 1D  ×8
	adcxy	r8,r8	; 1E  ×8
	addxy	r6,r1	; 1F  ×10 (10 nibbles per character)
	adcxy	r7,r2	; 20  ×10 (10 nibbles per character)
	adcxy	r8,r15	; 21  ×10 (addc,0)
; r6,7,8 + 0x100 ---> r6,7,8
	movxn	r0,1	; 22
	addxy	r8,r0	; 23

	movxn	r9,5	; 24  × 5
	movpcnn	3	; 25  subroutine @ 0x036
	movxn	jsr,6	; 26  call  0x036 -----------> (shift 5 columns from char gen)
	dszy	r9	; 27
	jrnn	-4	; 28  loop

	movxn	r6,0	; 29
	movxn	r7,0	; 2A
	movxn	r8,1	; 2B  point to blank (between chars)
	movpcnn	3	; 2C  subroutine @ 0x036
	movxn	jsr,6	; 2D  call  0x036 -----------> (shift 1 column, space between chars)

	incy	r3	; 2E  next char (one char = 2 nibbles)
	incy	r3	; 2F  next char (one char = 2 nibbles)
	skipfm	0b1111	; 30  if nibble overflow, then nz,3
	incy	r4	; 31
	skipfm	0b1101	; 32  if nibble overflow, then nz,1
	incy	r5	; 33

	movpcnn	0	; 34
	movxn	pclo,7	; 35  jump to 007 (loop)  ----------->

; adr 036
; (subroutine) shifts and loads one column (pointer r6r7r8)
; wait sync
	movr0nn	15,4	; 36  RdFlags
	andr0n	0b0001	; 37  Bit #0 = UserSync
	skipfm	0b1101	; 38  nz,1
	jrnn	-4	; 39  loop

	movxy	pchi,r8	; 3A
	movxy	pcm,r7	; 3B
	movxy	jsr,r6	; 3C  call (read from char gen)  ----------->
	movnnr0	2,15	; 3D
	incy	jsr	; 3E  call (read from char gen)  ----------->
	movnnr0	3,15	; 3F

	movxn	r0,2
	addxy	r6,r0
	skipfm	0b1111	; 40  nz,3
	incy	r7	; 41
	skipfm	0b1101	; 42  nz,1
	incy	r8	; 43
; shift screen down
	exrn	0	; 44  exchange registers (save)

	movxn	r6,14	; 45  rd ptr
	movxn	r7,15	; 46  wr ptr
	movxn	r1,4	; 47  hi nib ptr page 4
	movxn	r2,5	; 48  hi nib ptr page 5
	movxn	r8,15	; 49  loop counter

	movr0xy	r1,6	; 4A  rd
	movxyr0	r1,7	; 4B  wr
	movr0xy	r2,6	; 4C  rd
	movxyr0	r2,7	; 4D  wr

	decy	r6	; 4E  decrement rd ptr
	decy	r7	; 4F  decrement wr ptr
	dszy	r8	; 50  decrement loop counter
	jrnn	-8	; 51  if counter>0, loop

	exrn	0	; 52  exchange registers (restore)

	movr0nn	2,15	; 53  get new 4 pixels...
	movnnr0	4,0	; 54  ...and put them on visible page
	movr0nn	3,15	; 55  get new 4 pixels...
	movnnr0	5,0	; 56  ...and put them on visible page

	retr0n	0	; 57  (.87)	; ----------->

	.word	0x8000		; terminator


chargen:	; 480 bytes (960 nibbles)        @ 0x100
	retr0n2	0b00000000	; sp
	retr0n2	0b00000000
	retr0n2	0b00000000
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b00000000	; !
	retr0n2	0b00000000
	retr0n2	0b11111010
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b00000000	; "
	retr0n2	0b11000000
	retr0n2	0b00000000
	retr0n2	0b11000000
	retr0n2	0b00000000

	retr0n2	0b00101000	; #
	retr0n2	0b11111110
	retr0n2	0b00101000
	retr0n2	0b11111110
	retr0n2	0b00101000

	retr0n2	0b00100100	; $
	retr0n2	0b01010100
	retr0n2	0b11111110
	retr0n2	0b01010100
	retr0n2	0b01001000

	retr0n2	0b01000110	; %
	retr0n2	0b00100110
	retr0n2	0b00010000
	retr0n2	0b11001000
	retr0n2	0b11000100

	retr0n2	0b01101100	; &
	retr0n2	0b10010010
	retr0n2	0b01101010
	retr0n2	0b00001100
	retr0n2	0b00010010

	retr0n2	0b00000000	; '
	retr0n2	0b00010000
	retr0n2	0b01100000
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b00000000	; (
	retr0n2	0b00111000
	retr0n2	0b01000100
	retr0n2	0b10000010
	retr0n2	0b00000000

	retr0n2	0b00000000	; )
	retr0n2	0b10000010
	retr0n2	0b01000100
	retr0n2	0b00111000
	retr0n2	0b00000000

	retr0n2	0b01000100	; *
	retr0n2	0b00101000
	retr0n2	0b11111110
	retr0n2	0b00101000
	retr0n2	0b01000100

	retr0n2	0b00010000	; +
	retr0n2	0b00010000
	retr0n2	0b01111100
	retr0n2	0b00010000
	retr0n2	0b00010000

	retr0n2	0b00000000	; ,
	retr0n2	0b00000001
	retr0n2	0b00000110
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b00010000	; -
	retr0n2	0b00010000
	retr0n2	0b00010000
	retr0n2	0b00010000
	retr0n2	0b00010000

	retr0n2	0b00000000	; .
	retr0n2	0b00000000
	retr0n2	0b00000110
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b01000000	; /
	retr0n2	0b00100000
	retr0n2	0b00010000
	retr0n2	0b00001000
	retr0n2	0b00000100

	retr0n2	0b01111100	; 0
	retr0n2	0b10001010
	retr0n2	0b10010010
	retr0n2	0b10100010
	retr0n2	0b01111100

	retr0n2	0b00000000	; 1
	retr0n2	0b01000010
	retr0n2	0b11111110
	retr0n2	0b00000010
	retr0n2	0b00000000

	retr0n2	0b01000010	; 2
	retr0n2	0b10000110
	retr0n2	0b10001010
	retr0n2	0b10010010
	retr0n2	0b01100010

	retr0n2	0b01000100	; 3
	retr0n2	0b10000010
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b01101100

	retr0n2	0b00111000	; 4
	retr0n2	0b01001000
	retr0n2	0b10001000
	retr0n2	0b00011110
	retr0n2	0b00001000

	retr0n2	0b11110010	; 5
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10001100

	retr0n2	0b00111100	; 6
	retr0n2	0b01010010
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b00001100

	retr0n2	0b10000110	; 7
	retr0n2	0b10001000
	retr0n2	0b10010000
	retr0n2	0b10100000
	retr0n2	0b11000000

	retr0n2	0b01101100	; 8
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b01101100

	retr0n2	0b01100000	; 9
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10010100
	retr0n2	0b01111000

	retr0n2	0b00000000	; :
	retr0n2	0b00000000
	retr0n2	0b01100110
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b00000000	; ;
	retr0n2	0b00000001
	retr0n2	0b01100110
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b00010000	; <
	retr0n2	0b00101000
	retr0n2	0b01000100
	retr0n2	0b10000010
	retr0n2	0b00000000

	retr0n2	0b00101000	; =
	retr0n2	0b00101000
	retr0n2	0b00101000
	retr0n2	0b00101000
	retr0n2	0b00101000

	retr0n2	0b00000000	; >
	retr0n2	0b10000010
	retr0n2	0b01000100
	retr0n2	0b00101000
	retr0n2	0b00010000

	retr0n2	0b01000000	; ?
	retr0n2	0b10000000
	retr0n2	0b10001010
	retr0n2	0b10010000
	retr0n2	0b01100000
;	
	retr0n2	0b01111100	; @
	retr0n2	0b10010010
	retr0n2	0b10101010
	retr0n2	0b10101010
	retr0n2	0b01110010

	retr0n2	0b00111110	; A
	retr0n2	0b01001000
	retr0n2	0b10001000
	retr0n2	0b01001000
	retr0n2	0b00111110

	retr0n2	0b11111110	; B
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b01101100

	retr0n2	0b01111100	; C
	retr0n2	0b10000010
	retr0n2	0b10000010
	retr0n2	0b10000010
	retr0n2	0b01000100

	retr0n2	0b11111110	; D
	retr0n2	0b10000010
	retr0n2	0b10000010
	retr0n2	0b01000100
	retr0n2	0b00111000

	retr0n2	0b11111110	; E
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10000010

	retr0n2	0b11111111	; F
	retr0n2	0b10010000
	retr0n2	0b10010000
	retr0n2	0b10010000
	retr0n2	0b10000000

	retr0n2	0b01111100	; G
	retr0n2	0b10000010
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10011110

	retr0n2	0b11111110	; H
	retr0n2	0b00010000
	retr0n2	0b00010000
	retr0n2	0b00010000
	retr0n2	0b11111110

	retr0n2	0b00000000	; I
	retr0n2	0b10000010
	retr0n2	0b11111110
	retr0n2	0b10000010
	retr0n2	0b00000000

	retr0n2	0b10000100	; J
	retr0n2	0b10000010
	retr0n2	0b10000010
	retr0n2	0b10000010
	retr0n2	0b11111100

	retr0n2	0b11111110	; K
	retr0n2	0b00010000
	retr0n2	0b00101000
	retr0n2	0b01000100
	retr0n2	0b10000010

	retr0n2	0b11111110	; L
	retr0n2	0b00000010
	retr0n2	0b00000010
	retr0n2	0b00000010
	retr0n2	0b00000010

	retr0n2	0b11111110	; M
	retr0n2	0b01000000
	retr0n2	0b00100000
	retr0n2	0b01000000
	retr0n2	0b11111110

	retr0n2	0b11111110	; N
	retr0n2	0b00010000
	retr0n2	0b00001000
	retr0n2	0b00000100
	retr0n2	0b11111110

	retr0n2	0b01111100	; O
	retr0n2	0b10000010
	retr0n2	0b10000010
	retr0n2	0b10000010
	retr0n2	0b01111100

	retr0n2	0b11111110	; P
	retr0n2	0b10010000
	retr0n2	0b10010000
	retr0n2	0b10010000
	retr0n2	0b01100000

	retr0n2	0b01111100	; Q
	retr0n2	0b10000010
	retr0n2	0b10001010
	retr0n2	0b10000100
	retr0n2	0b01111010

	retr0n2	0b11111110	; R
	retr0n2	0b10010000
	retr0n2	0b10011000
	retr0n2	0b10010100
	retr0n2	0b01100010

	retr0n2	0b01100100	; S
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b10010010
	retr0n2	0b01001100

	retr0n2	0b10000000	; T
	retr0n2	0b10000000
	retr0n2	0b11111110
	retr0n2	0b10000000
	retr0n2	0b10000000

	retr0n2	0b11111100	; U
	retr0n2	0b00000010
	retr0n2	0b00000010
	retr0n2	0b00000010
	retr0n2	0b11111100

	retr0n2	0b11111000	; V
	retr0n2	0b00000100
	retr0n2	0b00000010
	retr0n2	0b00000100
	retr0n2	0b11111000

	retr0n2	0b11111110	; W
	retr0n2	0b00000100
	retr0n2	0b00001000
	retr0n2	0b00000100
	retr0n2	0b11111110

	retr0n2	0b11000110	; X
	retr0n2	0b00101000
	retr0n2	0b00010000
	retr0n2	0b00101000
	retr0n2	0b11000110

	retr0n2	0b11100000	; Y
	retr0n2	0b00010000
	retr0n2	0b00001110
	retr0n2	0b00010000
	retr0n2	0b11100000

	retr0n2	0b10000110	; Z
	retr0n2	0b10001010
	retr0n2	0b10010010
	retr0n2	0b10100010
	retr0n2	0b11000010

	retr0n2	0b00000000	; [
	retr0n2	0b10000010
	retr0n2	0b11111110
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b01000000	; \
	retr0n2	0b00100000
	retr0n2	0b00010000
	retr0n2	0b00001000
	retr0n2	0b00000100

	retr0n2	0b00000000	; ]
	retr0n2	0b00000000
	retr0n2	0b11111110
	retr0n2	0b10000010
	retr0n2	0b00000000

	retr0n2	0b00100000	; ^
	retr0n2	0b01000000
	retr0n2	0b10000000
	retr0n2	0b01000000
	retr0n2	0b00100000

	retr0n2	0b00000001	; _
	retr0n2	0b00000001
	retr0n2	0b00000001
	retr0n2	0b00000001
	retr0n2	0b00000001
;	
	retr0n2	0b00000000	; `
	retr0n2	0b10000000
	retr0n2	0b01000000
	retr0n2	0b00100000
	retr0n2	0b00000000

	retr0n2	0b00000100	; a
	retr0n2	0b00101010
	retr0n2	0b00101010
	retr0n2	0b00101010
	retr0n2	0b00011110

	retr0n2	0b11111110	; b
	retr0n2	0b00010010
	retr0n2	0b00100010
	retr0n2	0b00100010
	retr0n2	0b00011100

	retr0n2	0b00011100	; c
	retr0n2	0b00100010
	retr0n2	0b00100010
	retr0n2	0b00100010
	retr0n2	0b00100010

	retr0n2	0b00011100	; d
	retr0n2	0b00100010
	retr0n2	0b00100010
	retr0n2	0b00010010
	retr0n2	0b11111110

	retr0n2	0b00011100	; e
	retr0n2	0b00101010
	retr0n2	0b00101010
	retr0n2	0b00101010
	retr0n2	0b00011000

	retr0n2	0b00000000	; f
	retr0n2	0b00001000
	retr0n2	0b01111111
	retr0n2	0b10001000
	retr0n2	0b01000000

	retr0n2	0b00011000	; g
	retr0n2	0b00100101
	retr0n2	0b00100101
	retr0n2	0b00100101
	retr0n2	0b00011110

	retr0n2	0b11111110	; h
	retr0n2	0b00010000
	retr0n2	0b00100000
	retr0n2	0b00100000
	retr0n2	0b00011110

	retr0n2	0b00000000	; i
	retr0n2	0b00100010
	retr0n2	0b10111110
	retr0n2	0b00000010
	retr0n2	0b00000000

	retr0n2	0b00000000	; j
	retr0n2	0b00000001
	retr0n2	0b00000001
	retr0n2	0b10111110
	retr0n2	0b00000000

	retr0n2	0b00000000	; k
	retr0n2	0b11111110
	retr0n2	0b00001000
	retr0n2	0b00010100
	retr0n2	0b00100010

	retr0n2	0b00000000	; l
	retr0n2	0b10000010
	retr0n2	0b11111110
	retr0n2	0b00000010
	retr0n2	0b00000000

	retr0n2	0b00111110	; m
	retr0n2	0b00100000
	retr0n2	0b00011110
	retr0n2	0b00100000
	retr0n2	0b00011110

	retr0n2	0b00111110	; n
	retr0n2	0b00010000
	retr0n2	0b00100000
	retr0n2	0b00100000
	retr0n2	0b00011110

	retr0n2	0b00011100	; o
	retr0n2	0b00100010
	retr0n2	0b00100010
	retr0n2	0b00100010
	retr0n2	0b00011100

	retr0n2	0b00111111	; p
	retr0n2	0b00100100
	retr0n2	0b00100100
	retr0n2	0b00100100
	retr0n2	0b00011000

	retr0n2	0b00011000	; q
	retr0n2	0b00100100
	retr0n2	0b00100100
	retr0n2	0b00100100
	retr0n2	0b00111111

	retr0n2	0b00111110	; r
	retr0n2	0b00010000
	retr0n2	0b00100000
	retr0n2	0b00100000
	retr0n2	0b00010000

	retr0n2	0b00010010	; s
	retr0n2	0b00101010
	retr0n2	0b00101010
	retr0n2	0b00101010
	retr0n2	0b00100100

	retr0n2	0b00100000	; t
	retr0n2	0b11111100
	retr0n2	0b00100010
	retr0n2	0b00000010
	retr0n2	0b00000100

	retr0n2	0b00111100	; u
	retr0n2	0b00000010
	retr0n2	0b00000010
	retr0n2	0b00000100
	retr0n2	0b00111110

	retr0n2	0b00111000	; v
	retr0n2	0b00000100
	retr0n2	0b00000010
	retr0n2	0b00000100
	retr0n2	0b00111000

	retr0n2	0b00111110	; w
	retr0n2	0b00000100
	retr0n2	0b00001000
	retr0n2	0b00000100
	retr0n2	0b00111110

	retr0n2	0b00100010	; x
	retr0n2	0b00010100
	retr0n2	0b00001000
	retr0n2	0b00010100
	retr0n2	0b00100010

	retr0n2	0b00110001	; y
	retr0n2	0b00001001
	retr0n2	0b00000110
	retr0n2	0b00000100
	retr0n2	0b00111000

	retr0n2	0b00100010	; z
	retr0n2	0b00100110
	retr0n2	0b00101010
	retr0n2	0b00110010
	retr0n2	0b00100010

	retr0n2	0b00000000	; {
	retr0n2	0b00010000
	retr0n2	0b01101100
	retr0n2	0b10000010
	retr0n2	0b00000000

	retr0n2	0b00000000	; |
	retr0n2	0b00000000
	retr0n2	0b11111110
	retr0n2	0b00000000
	retr0n2	0b00000000

	retr0n2	0b00000000	; }
	retr0n2	0b10000010
	retr0n2	0b01101100
	retr0n2	0b00010000
	retr0n2	0b00000000

	retr0n2	0b01000000	; ~
	retr0n2	0b10000000
	retr0n2	0b01000000
	retr0n2	0b00100000
	retr0n2	0b01000000

	retr0n2	0b00000000	; sp
	retr0n2	0b00000000
	retr0n2	0b00000000
	retr0n2	0b00000000
	retr0n2	0b00000000

	.word	0x8000		; terminator


disptext:	; 394 bytes (788 nibbles)        @ 0x500
;	.ascii	"I am HAL 9000 computer. I became operational at the HAL plant in Urbana,"
;	.ascii	" Illinois, on January 12th, 1991. My first instructor was Mr. Arkany. He"
;	.ascii	" taught me to sing a song... it goes like this... Daisy, Daisy, give me "
;	.ascii	"your answer do. I'm half crazy all for the love of you. It won't be a st"
;	.ascii	"ylish marriage. I can't afford a carriage. But you'll look sweet Upon th"
;	.ascii	"e seat of a bicycle built for two.  "

 .ascii "Though yet of Hamlet our dear brother's death The memory be green, and that it us befitted  To bear our hearts in grief and our whole kingdom To be contracted in one brow of woe, Yet so far hath discretion fought with nature That we with wisest sorrow think on him, Together with remembrance of ourselves. Therefore our sometime sister, now our queen, The imperial jointress to this warlike state, Have we, as 'twere with a defeated joy, With an auspicious and a dropping eye, With mirth in funeral and with dirge in marriage, In equal scale weighing delight and dole, Taken to wife: nor have we herein barr'd Your better wisdoms, which have freely gone With this affair along. For all, our thanks. Now follows, that you know, young Fortinbras, Holding a weak supposal of our worth, Or thinking by our late dear brother's death Our state to be disjoint and out of frame, Colleagued with the dream of his advantage, He hath not fail'd to pester us with message, Importing the surrender of those lands Lost by his father, with all bonds of law, To our most valiant brother. So much for him. Now for ourself and for this time of meeting: Thus much the business is: we have here writ To Norway, uncle of young Fortinbras >> "

	.word	0xFFFF		; terminator
pgm_end:			; this is supposed to be the last INC. file
