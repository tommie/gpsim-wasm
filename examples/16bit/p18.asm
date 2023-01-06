	;; 18.asm
	;;
	;; Test the 18c instruction set

  processor	18c452	

  cblock  0x0c

	temp,temp1,temp2
  endc

	org 0
start

	addlw	4
	addlw	0xff
	addlw	0x100		; error
	
	addwf	0x11,1
	addwf	0x11,1,0
	addwf	0x11,1,1
	addwf	0x11,1,b
	
	addwfc	0x11,w,0
	addwfc	0x11,f,1
	addwfc	temp,w,b

	andlw	4
	andlw	0xff
	andlw	0x100		; error
	
	andwf	0x11,1,0
	andwf	0x11,1,1
	andwf	0x11,1,b
	
	bc	$+5
	
	bcf	temp1,5,0
	bcf	temp1,7

	bn	$-5
	bnc	$+0x7f
	bnn	$-0x7f

	bnov	start
	bnz	start
	bra	start
	bra	0x3ff

	bsf	temp2,1,b

	btfsc	temp1,4,b
	btfss	temp1,4

	btg	temp,0,b
	bov	start
	bz	$
	
	call	start
	call	$+0x10001,s

	clrf	temp
	clrf	temp1,b
	clrf	temp1,0

	clrwdt

	comf	temp,w,0
	cpfseq	temp1,0
	cpfsgt	temp,1
	cpfslt	temp,1

	daw

	decf	temp,w,0
	decf	temp1,f,1

	decfsz	temp,w,0
	decfsz	temp1,f,1
	
	dcfsnz	temp,w,0
	dcfsnz	temp1,f,1
	
	goto	0xfffff
	
	incf	temp,w,0
	incf	temp1,f,b

	incfsz	temp,w,0
	incfsz	temp1,f,b
	
	infsnz	temp,w,0
	infsnz	temp1,f,b
	
	iorlw	4
	iorlw	0xff
	
	iorwf	0x11,1,0
	iorwf	0x11,1,1

	lfsr	3,0xfab

	movf	temp,w,0
	movf	temp1,f,b

	movff	temp1,temp2

	movlb	0x55
	movlw	0xab

	movwf	temp
	movwf	temp,b

	mullw	0x42
	mulwf	temp,b

	negf	temp,0
	nop

	pop
	push

	rcall	-0x3ff
	rcall	0x7ff

	reset
	retfie	0
	retlw	0xde
	return	1
	
	rlcf	temp,w,0
	rlcf	temp1,f,b

	rlncf	temp,w,0
	rlncf	temp1,f,b

	rrcf	temp,w,0
	rrcf	temp1,f,b

	rrncf	temp,w,0
	rrncf	temp1,f,b

	setf	temp1,b

	sleep

	subfwb	temp,w,0
	subfwb	temp1,f,b

	sublw	0x33
	
	subwf	temp,w,0
	subwf	temp1,f,b

	subwfb	temp,w,0
	subwfb	temp1,f,b

	swapf	temp,w,0
	swapf	temp1,f,b

	sublw	0x33 | 0x0f
	sublw	5 * 8

 	tblrd	*
 	tblrd	*+
 	tblrd	*-
	tblrd	+*

 	tblwt	*
 	tblwt	*+
 	tblwt	*-
	tblwt	+*

	tstfsz	temp1,b

	xorlw	0xff

	xorwf	temp,w,0
	xorwf	temp1,f,b


	end

