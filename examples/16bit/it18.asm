	;; it.asm
	;;
	;; The purpose of this program is to test how well gpsim can simulate
	;; a 16bit-core pic (like the 18cxxx family not the 17c family.
	;; Nothing useful is performed - this program is only used to
	;; debug gpsim.

	
include "p18c242.inc"

  cblock  0

	temp,temp1,temp2
  endc
	
	org 0


	clrf	temp1		;Assume clrf works...
				;

start:	
	;; Perform some basic tests on some important instructions

	setc			;set the Carry flag
	skpc			;and verify that it happened
	 bra	$

	clrc			;Now try clearing the carry
	skpnc
	 bra	$

	setz			;set the Zero flag
	skpz			;and verify that it happened
	 bra	$

	clrz			;Now try clearing it
	skpnz
	 bra	$
	
	setdc			;set the Digit Carry flag
	skpdc			;and verify that it happened
	 bra	$

	clrdc			;Now try clearing it
	skpndc
	 bra	$

	setov			;set the Over Flow flag
	skpov			;and verify that it happened
	 bra	$

	clrov			;Now try clearing it
	skpnov
	 bra	$

	setn			;set the negative flag
	skpn			;and verify that it happened
	 bra	$

	clrn			;Now try clearing it
	skpnn
	 bra	$


	movlw	1
	movwf	temp
	movf	temp,f
	skpnz
	 bra	$

	movlw	0
	movwf	temp
	movf	temp,f
	skpz
	 bra   $
	
	clrf	temp		;Clear a register
	skpz			;and verify that the Z bit in the
	 bra	$		;status register was set.

	movf	temp,w		;Read the register that was just
	skpz			;cleared, and again verify that Z
	 bra	$		;was set

	incf	temp,w		;Now this should cause the Z bit to
	skpnz			;get cleared.
	 bra	$

	movlw	0xff
	movwf	temp
	incf	temp

	skpz
	 bra	$

	;;
	;; incfsz
	;;

	movlw	0xff
	movwf	temp
	movf	temp,f		;Should clear Z
	incfsz	temp,w
	 bra	$

	skpnz			;incfsz shouldn't affect Z
	 bra	$

	incfsz	temp,f		;temp should now be zero
	 bra	$

	incfsz	temp,w		;the following inst. shouldn't be skipped
	 skpnz			;Z should be clear from above.
	  bra	$
		
	
	;; 
	;; addwf test:
	;;
	;; The purpose of this test is to a) verify that the addwf instruction
	;; adds correctly and b) that the appropriate status register bits are
	;; set correctly.

	clrw
	clrf	temp
	
	addwf	temp,w		; 0+0
	skpnc
	 bra	$
	skpz
	 bra	$
	skpndc
	 bra	$

	movlw	1		; 0+1
	addwf	temp,w
	
	skpc
	 skpnz
	  bra	$
	skpndc
	 bra	$
	skpn
	 skpnov
	  bra	$

	movlw	8		; 8+8 , test dc
	movwf	temp
	addwf	temp,w
	
	skpc
	 skpnz
	  bra	$
	skpdc
	 bra	$
	skpn
	 skpnov
	  bra	$

	movlw	0x88		; 0x88+0x88
	movwf	temp
	addwf	temp,w
	
	skpnc
	 skpnz
	  bra	$
	skpdc
	 bra	$
	skpn
	 skpov
	  bra	$

	movlw	0x80		; 0x80+0x80
	movwf	temp
	addwf	temp,w
	
	skpnc
	 skpz
	  bra	$
	skpndc
	 bra	$
	skpn
	 skpov
	  bra	$

	clrw
	addwf	temp,w		; 0x80+0
	
	skpc
	 skpnz
	  bra	$
	skpndc
	 bra	$
	skpnn
	 skpov
	  bra	$


	clrf	temp
	addwf	temp,w		; 0+0x80
	
	skpc
	 skpnz
	  bra	$
	skpndc
	 bra	$
	skpnn
	 skpnov
	  bra	$

	movlw	1
	movwf	temp
	movlw	0xff
	addwf	temp,w

	skpc
	 bra	$
	skpz
	 bra	$
	skpdc
	 bra	$


	clrc
	clrdc
		
	;;
	;; andwf test:
	;;
	clrf	temp
	movlw	1
	andwf	temp,f

	skpz
	 bra	$

	skpc
	 skpndc
	  bra  $

	andwf	temp,w

	skpz
	 bra	$

	skpc
	 skpndc
	  bra  $

	movlw	1
	movwf	temp
	andwf	temp,f

	skpnz
	 bra	$

	skpc
	 skpndc
	  bra  $

	movlw	0xff
	andwf	temp,f		;1 & 0xff should = 1

	skpnz
	 bra	$

	skpc
	 skpndc
	  bra  $

	movlw	0xff		;Now add 0xff to see if temp
	addwf	temp,f		;really is 1. (should cause a carry)

	skpnz
	 skpc
	  bra	$

	;;
	;; iorwf
	;;

	movlw	0
	movwf	temp
	iorwf	temp,f
	skpz
	 bra	$

	movlw	1
	iorwf	temp,f
	skpnz
	 bra	$

	movlw	0xff
	addwf	temp,f
	skpnz
	 skpc
	  bra	$


	;;
	;; rlcf
	;;

	clrdc
	clrf	temp
	clrc
	rlcf	temp,w
	
	skpdc
	 skpnc
	  bra	$

	skpz
	 bra	$

	setc
	rlcf	temp,f

	skpdc
	 skpnc
	  bra	$

	skpnz
	 bra	$

	movlw	0xff
	addwf	temp,f
	skpnz
	 skpc
	  bra	$

	rlcf	temp,f
	skpnc
	 bra	$
	
	movlw	0x80
	movwf	temp
	rlcf	temp,w

	skpc
	 bra	$
	movwf	temp
	addwf	temp,w
	skpz
	 bra	$

	;;
	;; rrcf
	;;

	clrdc
	clrf	temp
	clrc
	rrcf	temp,w
	
	skpdc
	 skpnc
	  bra	$

	skpz
	 bra	$

	setc
	rrcf	temp,f

	skpdc
	 skpnc
	  bra	$

	skpnz
	 bra	$

	movlw	0x80
	addwf	temp,f
	skpnz
	 skpc
	  bra	$

	rrcf	temp,f
	skpnc
	 bra	$
	
	movlw	1
	movwf	temp
	rrcf	temp,w

	skpc
	 bra	$
	movwf	temp
	addwf	temp,w
	skpz
	 bra	$


	;; 
	;; subwf test:
	;;

	clrf	wreg
	clrf	temp
	
	subwf	temp,w		; 0-0
	skpc
	 bra	$
	skpz
	 bra	$
	skpdc
	 bra	$

	movlw	1		; 0-1
	subwf	temp,w
	
	skpnc
	 bra	$
	skpnz
	 bra	$
	skpndc
	 bra	$

	movlw	0x10		; 0x10-0x10
	movwf	temp
	subwf	temp,w
	
	skpc
	 bra	$
	skpz
	 bra	$
	skpdc
	 bra	$

	movlw	0x88		; 0x88-0x88
	movwf	temp
	subwf	temp,w
	
	skpc
	 bra	$
	skpz
	 bra	$
	skpdc
	 bra	$

	clrf	temp
	movlw	0x80		; 0 - 0x80
	subwf	temp,f
	
	skpnc
	 bra	$
	skpnz
	 bra	$
	skpdc
	 bra	$

	movlw	0		; 0x80 - 0
	subwf	temp,w

	skpc
	 bra	$
	skpnz
	 bra	$
	skpdc
	 bra	$

	movlw	1		; 0x80 - 1
	subwf	temp,w

	skpc
	 bra	$
	skpnz
	 bra	$
	skpndc
	 bra	$
	

	clrc
	clrdc

	;; 
	;; xorwf
	;;

	clrf	temp
	movlw	0xff
	xorwf	temp,f

	skpc
	 skpndc
	  bra	$

	skpnz
	 bra	$

	xorwf	temp,f

	skpz
	 bra	$

	xorwf	temp,f
	incfsz	temp,f
	 bra	$

	;;
	;; swapf
	;;

	movlw	0x0f
	movwf	temp
	swapf	temp,f
	xorwf	temp,f
	incfsz	temp,f
	 bra	$

	decf	temp1,f
	incfsz	temp1,w
	 goto	done

	;; repeat the test with in the next bank
	
	bsf	status,rp0
	goto	start
	
done:
	bcf	status,rp0
	
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	
	bra	$
	
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
		

	;;
	;; Test indirect branching
	;;
test_pcl:
	
	clrf	temp
	clrf	temp1

tt1:
	movf	temp,w
	call	table_test

	xorwf	temp,w
	skpz
	 bsf	temp1,0

	incf	temp,f
	btfss	temp,4
	 goto	tt1

	goto	test_pcl2
table_test:
	addwf	pcl,f
table:
	retlw	0
	retlw	1
	retlw	2
	retlw	3
	retlw	4
	retlw	5
	retlw	6
	retlw	7
	retlw	8
	retlw	9
	retlw	10
	retlw	11
	retlw	12
	retlw	13
	retlw	14
	retlw	15

test_pcl2:
	
	btfsc	temp2,0
	 goto	test_pcl3
        movlw   0x20
        movwf   fsr0l
	clrf	fsr0h
	
next    clrf    indf0
        incf    fsr0l,1
        btfss   fsr0l,4
        goto    next

	bsf	temp2,0
        clrf    pcl

test_pcl3:

	movlw	ly
	movwf	pcl		; == goto ly
lx
	goto	test_pcl4
ly
	movlw	lx
	movwf	pcl

test_pcl4:
	
	goto	$
	end