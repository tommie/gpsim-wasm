	list	p=16c84

	;; The purpose of this program is to test gpsim's stimulation capability
	;;
	;; Within gpsim, add a stimulus:
	;;   gpsim> stimulus sqw 200 portb 0
	;; this will create a synchronous square wave with a period of 200
	;; cpu cycles (and by default a 50% duty cycle) and will attach it to
	;; bit 0 of port b.
	;; OR
	;;   load the Startup command file 'sync_stim.stc':	
	;;   gpsim> load c sync_stim.stc
	;; This will create a stimulus and load the hex associated with this source.
		
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
begin

	;; Count the rising edges on portb bit 0
	
	movf	portb,w
	xorwf	temp2,w
	xorwf	temp2,f
	andwf	temp2,w
	andlw	1
	
	skpz
	  incf	temp1,f
	
	goto   begin

	end