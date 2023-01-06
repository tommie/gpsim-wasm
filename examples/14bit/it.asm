	;; it.asm
	;;
	;; The purpose of this program is to test how well gpsim can simulate
	;; a pic. Nothing useful is performed - this program is only used to
	;; debug gpsim.

	list	p=16c84
	
include "p16c84.inc"

  cblock  0x0c

	temp,temp1,temp2
  endc
	
	org 0


	clrf	temp1		;Assume clrf works...
				;
	;;
	;; map tmr0 to the internal clock 
	;; 
	clrw
	option			;initialize tmr0

start:	
	;; Perform some basic tests on some important instructions

	setc			;set the Carry flag
	skpc			;and verify that it happened
	 goto	$

	clrc			;Now try clearing the carry
	skpnc
	 goto	$

	setz			;set the Zero flag
	skpz			;and verify that it happened
	 goto	$

	clrz			;Now try clearing it
	skpnz
	 goto	$
	
	setdc			;set the Digit Carry flag
	skpdc			;and verify that it happened
	 goto	$

	clrdc			;Now try clearing it
	skpndc
	 goto	$

	movlw	1
	movwf	temp
	movf	temp,f
	skpnz
	 goto   $

	movlw	0
	movwf	temp
	movf	temp,f
	skpz
	 goto   $
	
	clrf	temp		;Clear a register
	skpz			;and verify that the Z bit in the
	 goto	$		;status register was set.

	movf	temp,w		;Read the register that was just
	skpz			;cleared, and again verify that Z
	 goto	$		;was set

	incf	temp,w		;Now this should cause the Z bit to
	skpnz			;get cleared.
	 goto	$

	movlw	0xff
	movwf	temp
	incf	temp

	skpz
	 goto	$

	;;
	;; incfsz
	;;

	movlw	0xff
	movwf	temp
	movf	temp,f		;Should clear Z
	incfsz	temp,w
	 goto	$

	skpnz			;incfsz shouldn't affect Z
	 goto	$

	incfsz	temp,f		;temp should now be zero
	 goto	$

	incfsz	temp,w		;the following inst. shouldn't be skipped
	 skpnz			;Z should be clear from above.
	  goto	$
		
	
	;; 	addlw	0xaa

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
	 goto	$
	skpz
	 goto	$
	skpndc
	 goto	$

	movlw	1		; 0+1
	addwf	temp,w
	
	skpnc
	 goto	$
	skpnz
	 goto	$
	skpndc
	 goto	$

	movlw	8		; 8+8
	movwf	temp
	addwf	temp,w
	
	skpnc
	 goto	$
	skpnz
	 goto	$
	skpdc
	 goto	$

	movlw	0x88		; 0x88+0x88
	movwf	temp
	addwf	temp,w
	
	skpc
	 goto	$
	skpnz
	 goto	$
	skpdc
	 goto	$

	movlw	0x80		; 0x80+0x80
	movwf	temp
	addwf	temp,w
	
	skpc
	 goto	$
	skpz
	 goto	$
	skpndc
	 goto	$

	movlw	1
	movwf	temp
	movlw	0xff
	addwf	temp,w

	skpc
	 goto	$
	skpz
	 goto	$
	skpdc
	 goto	$


	clrc
	clrdc
		
	;;
	;; andwf test:
	;;
	clrf	temp
	movlw	1
	andwf	temp,f

	skpz
	 goto	$

	skpc
	 skpndc
	  goto  $

	andwf	temp,w

	skpz
	 goto	$

	skpc
	 skpndc
	  goto  $

	movlw	1
	movwf	temp
	andwf	temp,f

	skpnz
	 goto	$

	skpc
	 skpndc
	  goto  $

	movlw	0xff
	andwf	temp,f		;1 & 0xff should = 1

	skpnz
	 goto	$

	skpc
	 skpndc
	  goto  $

	movlw	0xff		;Now add 0xff to see if temp
	addwf	temp,f		;really is 1. (should cause a carry)

	skpnz
	 skpc
	  goto	$

	;;
	;; iorwf
	;;

	movlw	0
	movwf	temp
	iorwf	temp,f
	skpz
	 goto	$

	movlw	1
	iorwf	temp,f
	skpnz
	 goto	$

	movlw	0xff
	addwf	temp,f
	skpnz
	 skpc
	  goto	$


	;;
	;; rlf
	;;

	clrdc
	clrf	temp
	clrc
	rlf	temp,w
	
	skpdc
	 skpnc
	  goto	$

	skpz
	 goto	$

	setc
	rlf	temp,f

	skpdc
	 skpnc
	  goto	$

	skpz
	 goto	$

	movlw	0xff
	addwf	temp,f
	skpnz
	 skpc
	  goto	$

	rlf	temp,f
	skpnc
	 goto	$
	
	movlw	0x80
	movwf	temp
	rlf	temp,w

	skpc
	 goto	$
	movwf	temp
	addwf	temp,w
	skpz
	 goto	$

	;;
	;; rrf
	;;

	clrdc
	clrf	temp
	clrc
	rrf	temp,w
	
	skpdc
	 skpnc
	  goto	$

	skpz
	 goto	$

	setc
	rrf	temp,f

	skpdc
	 skpnc
	  goto	$

	skpz
	 goto	$

	movlw	0x80
	addwf	temp,f
	skpnz
	 skpc
	  goto	$

	rrf	temp,f
	skpnc
	 goto	$
	
	movlw	1
	movwf	temp
	rrf	temp,w

	skpc
	 goto	$
	movwf	temp
	addwf	temp,w
	skpz
	 goto	$


	;; 
	;; subwf test:
	;;

	clrw
	clrf	temp
	
	subwf	temp,w		; 0-0
	skpc
	 goto	$
	skpz
	 goto	$
	skpdc
	 goto	$

	movlw	1		; 0-1
	subwf	temp,w
	
	skpnc
	 goto	$
	skpnz
	 goto	$
	skpndc
	 goto	$

	movlw	0x10		; 0x10-0x10
	movwf	temp
	subwf	temp,w
	
	skpc
	 goto	$
	skpz
	 goto	$
	skpdc
	 goto	$

	movlw	0x88		; 0x88-0x88
	movwf	temp
	subwf	temp,w
	
	skpc
	 goto	$
	skpz
	 goto	$
	skpdc
	 goto	$

	clrf	temp
	movlw	0x80		; 0 - 0x80
	subwf	temp,f
	
	skpnc
	 goto	$
	skpnz
	 goto	$
	skpdc
	 goto	$

	movlw	0		; 0x80 - 0
	subwf	temp,w

	skpc
	 goto	$
	skpnz
	 goto	$
	skpdc
	 goto	$

	movlw	1		; 0x80 - 1
	subwf	temp,w

	skpc
	 goto	$
	skpnz
	 goto	$
	skpndc
	 goto	$
	

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
	  goto	$

	skpnz
	 goto	$

	xorwf	temp,f

	skpz
	 goto	$

	xorwf	temp,f
	incfsz	temp,f
	 goto	$

	;;
	;; swapf
	;;

	movlw	0x0f
	movwf	temp
	swapf	temp,f
	xorwf	temp,f
	incfsz	temp,f
	 goto	$

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
	
	goto	$
	
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
		
	andlw	0xaa
	andwf	temp,w
	bcf	temp,5
	bsf	temp,5
	btfsc	temp,5
	btfss	temp,5
	call	routine
	clrf	temp
	clrw
	clrwdt
	comf	temp,w
	decf	temp,w
	decfsz	temp,w

	goto	l1
	nop
	nop
	nop
l1:	
	incf	temp,w
	incfsz	temp,w
	iorlw	0xaa
	iorwf	temp,w
	movlw	0xaa
	movf	temp,w
	movwf	temp
	nop
	option


	goto	r1
		
	retfie
	retlw   0xaa
routine
	return
r1:

	rlf	temp,w
	rrf	temp,w
	sleep
	sublw	0xaa
	subwf	temp,w
	swapf	temp,w
	tris	5
	xorlw	0xaa
	xorwf	temp,w
	

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
        movwf   fsr
next    clrf    indf
        incf    fsr,1
        btfss   fsr,4
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