	list	p=16c84

	;; 
	;; led_mod.asm - a simple program to test gpsim's ability to interface
	;; to a 7-segment led module
	;; 
		
include "p16c84.inc"

  cblock  0x0d
	digit

  endc
	
	org	0

	;; Make all of portb I/O pins outputs
	clrw
	tris	PORTB	
	tris	PORTA
	
	;; Loop continuously and increment portb.
	;; It's assumed that a 7-segemnt is attached to the simulated pic.
	;; Port A bit0 controls the common cathode, while port B 0-6 control
	;; anodes of the 7 segments.

	clrf	PORTA
begin

	incf	digit,w
	andlw	0x0f
	movwf	digit
	
	call	decode_segments
	movwf	PORTB
	

	goto	begin

decode_segments
	addwf	PCL,f
	retlw	0x3f		; 0
	retlw	0x06		; 1
	retlw	0x5b		; 2
	retlw	0x4F		; 3
	retlw	0x66		; 4
	retlw	0x6d		; 5
	retlw	0x7c		; 6
	retlw	0x07		; 7
	retlw	0x7f		; 8
	retlw	0x67		; 9
	retlw	0x77		; A
	retlw	0x7c		; B
	retlw	0x58		; C
	retlw	0x5e		; D
	retlw	0x79		; E
	retlw	0x71		; F
	
	end
