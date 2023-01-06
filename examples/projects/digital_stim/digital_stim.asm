	list	p=16f84

	;; The purpose of this program is to test gpsim's stimulation capability
	;; A stimulus is attached to PORTB pin 0 and this program will count
	;; the rising edges that are seen on that pin.
	;;
	;;   Start gpsim:
	;;   $ gpsim
	;;   then load the Startup command file 'digital_stim.stc':	
	;;   gpsim> load c digital_stim.stc
	;;
	;;    OR
	;;
	;;   invoke gpsim with the command file
	;;   $ gpsim -c digital_stim.stc
	;;
	;;    OR
	;;
	;;   invoke the simulation from the Makefile:
	;;   make sim

	;; In all cases, the stimulus file will load the simulation
	;; file and create the stimuli. In the Makefile case, the
	;; simulation file will be (re)created if the .asm has been
	;; been changed.
	;;
	;;


		
include "p16f84.inc"

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
	
	movf	PORTB,w
	xorwf	temp2,w
	xorwf	temp2,f
	andwf	temp2,w
	andlw	1
	
	skpz
	  incf	temp1,f
	
	goto   begin

	end
