	list	p=16c84

	;; 
	;; led_mod.asm - a simple program to test gpsim's ability to interface
	;; to a 7-segment led module
	;; 
		
include "p16c84.inc"


	org	0

	;; Make all of portb I/O pins outputs
	clrw
	tris	PORTB	

	
	;; Loop continuously and increment portb.
	;; bits 0 & 1 of port b are connected to a 2-input AND gate
	;; The output is connected to Port A bit0

begin

	incf	PORTB,F

	goto	begin

	end
