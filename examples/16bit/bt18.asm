	;; bt18.asm
	;;
	;; The purpose of this program is to test how well gpsim can simulate
	;; a 16bit-core pic (like the 18cxxx family not the 17c family).
	;; Nothing useful is performed - this program is only used to
	;; debug gpsim.
	;; bt18.asm tests most (all?) of the branching modes. See it18.asm
	;; for a program that tests the instructions

	
include "p18c242.inc"

fast	equ	1
	
  cblock  0

	temp,temp1,temp2
  endc
	
	org 0

	clrf	temp

	;; short branch (relative branch)
	
	bra	l1
	nop
	nop

	;; long branch
	
l1:	goto	l2
	nop
	nop

	;; old-fashioned call
	
l2:	call	l3

	bra	l4

l3:	return

	;; new call; test the fast stack push and pop
l4:
	movlw	0xaa
	clrf	bsr,0
	clrf	status,0
	clrz
	
	call	l5,fast

	skpc
	 skpnz
	  bra	$

	skpndc
	 bra	$

	skpov
	 skpnn
	  bra	$

	xorlw	0xaa
	skpz
	 bra	$
	tstfsz	bsr,0
	 bra	$

	goto	l6
	
l5:	clrw
	movlb	0x02
	setz
	setc
	setn
	setov
	setdc

	return	fast

l6:

	;;
	;; memory tests
	;;

	clrf	bsr,0		;Point to bank 0
	movlw	0xaa
	movwf	temp,1		;Write 0xaa to register temp
	subwf	temp,w,0	;Access temp without using the bsr
	skpz
	 bra	$

	

	movlb	1		;Point to bank 1
	movlw	0x55
	movwf	temp,1		;Write 0x55 to register temp, but
				;in bank 1
	movwf	temp1,0		;Write 0x55 to register temp1 in
				; bank 0
	subwf	temp,w,0	;Access temp in bank 0. This is 0xaa
				;and is different than the 0x55 just
				;written to temp in bank 1
	skpnz
	 bra	$
	
	rrncf	temp,w,0	;Shift temp in bank 0 right and
				;store the result in w.
	xorlw	0x55
	skpz
	 bra	$

	movlb	0		;Point to bank 0
	rlncf	temp,w,1	;Shift temp in bank 0 left and
				;store the result in w.
	xorlw	0x55
	skpz
	 bra	$


	bra	$

