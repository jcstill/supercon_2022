
start:
	datahi
	clockhi
	datalo
	clocklo
	return
;				
stop:
	datalo
	clockhi
	datahi
	clocklo
	return
;				
tx.w0:
	rlc.b	w0,w0
	bra	c,nack
ack:			; ack (bit=0 transmit)
	datalo
	clockhi
	clocklo
	return
nack:			; nack (bit=1 transmit)
	datahi
	clockhi
	clocklo
	return
;				
i2c_tx_ack:			; transmit w0, then SEND ack
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	bra	ack
;				
i2c_tx_no_ack:			; transmit w0, then get ack
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
	call	tx.w0
get.ack:			; read 1 bit ---> carry
	datahi
	clockhi
	clrc			; acknowledged (bit 0)
	ifdatahi		; test data pin
	setc			; not acknowledged (bit 1)
	clocklo
	return
;				
i2c_rx:			; read 8 bits ---> w0
	mov	#8,w2		; loop bit count
loopab2:				; \
	call	get.ack		; get bit ---> carry
	rlc.b	w0,w0

	dec	w2,w2
	bra	nz,loopab2	; /
	return
;				
t38:				; I2C period (16 MIPS)
	repeat	#156
	nop
	return

