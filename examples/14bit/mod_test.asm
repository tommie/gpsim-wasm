	list	p=16c84

	;; 
	;; mod_test.asm - a simple program to test gpsim's ability to interface
	;; to modules
	;; 
		
include "p16c84.inc"


	org	0

	;; Make all of portb I/O pins outputs
	clrw
	tris	portb	

	;; Loop continuously and increment portb. If a module is
	;; is attached to this port, then it will get the changing
	;; count.
begin

	incf	portb,f

	goto	begin

	end