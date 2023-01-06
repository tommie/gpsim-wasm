	list	p=16c84

	;; The purpose of this program is to test gpsim's stimulation capability
	;; between iopins. In this example, the open collector iopin of porta (bit 4)
	;; is toggled. The response is then examined at portb bit 0.
	;; 
	;; 
	;; 
	;; 
		
include "p16c84.inc"

  cblock  0x20
		
	temp1
	temp2
	temp3
  endc

	org	0
	goto	start
	
	org	4
start
	clrf	temp1		; a counter
	clrf	temp2		; a flag keeping track of the state of port b
	clrf	temp3

	bsf	status,rp0
	bcf	porta,4
	bcf	status,rp0
begin

	movf	temp3,w
	movwf	porta
	;; Count the rising edges on portb bit 0
	
	movf	portb,w
	xorwf	temp2,w
	xorwf	temp2,f
	andwf	temp2,w
	andlw	1
	
	skpz
	  incf	temp1,f

	incf	temp3
	goto	begin

	end