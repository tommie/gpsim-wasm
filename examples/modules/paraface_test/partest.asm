	list	p=16c84
	__config _WDT_OFF & _RC_OSC

include "p16c84.inc"

	org	0
	goto	start
	
	org	4
start
	bsf     STATUS,RP0
	clrf PORTB
	bcf     STATUS,RP0

	
   	; Every write to PORTB updates the parallel port data

	; When the parallel port STATUS lines change, the
	; change will be seen on porta

begin
	incf PORTB,f	
	goto begin

	end

