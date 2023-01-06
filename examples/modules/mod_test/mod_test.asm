	list	p=16c84

	;; 
	;; mod_test.asm - a simple program to test gpsim's ability to interface
	;; to modules
	;; 
		
include "p16c84.inc"


  cblock 0x0c

    pb_shadow
    temp

  endc

	org	0

begin

	;; Make all of portb I/O pins inputs
	movlw	0xff

	bsf	STATUS,RP0
	movwf	TRISB^0x80
	bcf	STATUS,RP0


	movf	PORTB,w
	movwf	pb_shadow


	bsf	STATUS,RP0
	clrf	TRISB^0x80
	bcf	STATUS,RP0


	bsf	temp,4
l1:	incf	PORTB,f
	decfsz	temp,f
	 goto	l1

	clrf	PORTB

	goto	begin

	end