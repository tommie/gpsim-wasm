	;; mul.asm
	;;
	;; Test the multiply instructions

	nolist
include "p18c242.inc"
	list
	
  cblock  0
	temp
	suml,sumh
  endc


	org 0
start


	movlb	0		;Point to bank 0

	clrw
	clrz			;
	mullw	0		;0*0 should equal 0

	bz	error		;but mul shouldn't affect z

	movf	prodh,f,0
	iorwf	prodl,w,0

	bnz	error

	mullw	1		;1*0 should equal 0

	movf	prodh,f,0
	iorwf	prodl,w,0
	bnz	error

	clrf	temp
	clrz

	mulwf	temp,1
	
	movf	prodh,f,0
	iorwf	prodl,w,0
	bnz	error

	movlw	2

	mulwf	wreg,0		;prodh:prodl = 00:04
	mulwf	prodl,0		;prodh:prodl = 00:08
	mulwf	prodl,0		;prodh:prodl = 00:10
	mulwf	prodl,0		;prodh:prodl = 00:20
	mulwf	prodl,0		;prodh:prodl = 00:40
	mulwf	prodl,0		;prodh:prodl = 00:80
	mulwf	prodl,0		;prodh:prodl = 01:00

	dcfsnz	prodh,f,0	;prodh should be 1
	 tstfsz	prodl,0		;prodl should be 0
	  bra	error

	movlw	0xff		;0xff*0xff = 0xfe01
	mullw	0xff
	
	movf	prodl,w,0	;      1
	addwf	prodl,w,0	; +    1
	addwf	prodh,w,0	; + 0xfe
				;-------
	bnz	error		;      0

;;; Check multiplication:
;;; sum of the first N integers is = N*(N+1)/2
	movlw	1
	movwf	temp
	clrf	suml
	clrf	sumh

int_sum:
	movf	temp,w
	
	addwf	suml,f		;Add the next integer to the sum
	skpnc
	 incf	sumh,f

	addlw	1
	mulwf	temp		;The next integer times the previous
	clrc
	rrcf	prodh,f,0	;Divided by two
	rrcf	prodl,w,0

	subwf	suml,w
	bnz	error
	movf	sumh,w
	subwfb	prodh,w,0
	bnz	error
	
	incfsz	temp,f
	 bra	int_sum

	bra	$
error
	bra	$
	
	end
