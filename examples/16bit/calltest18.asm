	;; calltest18.asm
	;;
	;; The purpose of this program is to test how well gpsim can simulate
	;; a 16bit-core pic (like the 18cxxx family not the 17c family).
	;; Nothing useful is performed - this program is only used to
	;; debug gpsim.
	;; calltest.asm tests the stack. See it18.asm
	;; for a program that tests the instructions

	
include "p18c242.inc"

fast	equ	1
	
  cblock  0

	temp,temp1,temp2
  endc
	
	org 0

	clrf	temp
	
	rcall	ret		;Relative  call
	
	call	ret		;Long call

	movlw	0x10
	movwf	temp
	rcall	recursive
	
	bra	ct1

ret:	return


recursive:
	dcfsnz	temp,f
	 return
	rcall	recursive
	return
	

ct1:
	bra	ct2

ct2:	

	bra	$

	end