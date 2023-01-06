
	;; gpasm bug --- c64 is not recognized...
	
	list	p=16c64
  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16c64's
	;; tmr1 peripheral. Check out p16c64_ccp.asm to see additional tmr1 examples.

include "p16c64.inc"
		
  cblock  0x20
	status_temp,w_temp
	interrupt_temp

	temp1,temp2
	t1,t2,t3
	x
  endc

  cblock 0xa0
	status_temp_alias,w_temp_alias
  endc

	
  org	0
	goto	main


  org	4
	;; 
	;; Interrupt
	;;

	movwf	w_temp
	swapf	status,w
	movwf	status_temp

	;; Are peripheral interrupts enabled?
	btfss	intcon,peie
	 goto	exit_int

	bsf	status,rp0
	movf	pie1,w
	bcf	status,rp0
	movwf	interrupt_temp

check_tmr1:
	btfsc	pir1,tmr1if
	 btfss	interrupt_temp,tmr1ie
	  goto	exit_int

    ;; tmr1 has rolled over
	
	bcf	pir1,tmr1if	; Clear the pending interrupt
	bsf	temp1,0		; Set a flag to indicate rollover

exit_int:		

	swapf	status_temp,w
	movwf	status
	swapf	w_temp,f
	swapf	w_temp,w

	retfie

main:

	;; disable (primarily) global and peripheral interrupts
	
	clrf	intcon
	clrf	pir1

	;; Now enable only the tmr1 interrupt

	bsf	intcon,peie
	bsf	intcon,gie

	bsf	status,rp0
	bsf	pie1,tmr1ie
	bcf	status,rp0



	;;
	;; TMR1 test
	;;

	;; Clear all of the bits of the TMR1 control register:
	;; this will:
	;;	Turn the tmr off
	;;	Select Fosc/4 as the clock source
	;;	Disable the External oscillator feedback circuit
	;;	Select a 1:1 prescale
	
	clrf	t1con		;
	bcf	pir1,tmr1if	; Clear the interrupt/roll over flag

	;; Zero the TMR1

	clrf	tmr1l
	clrf	tmr1h

	;; Test rollover
	;;  the following block of code will test tmr1's rollover for
	;; each of the 4 prescale values.
	;;
	;; The way it works is by starting the timer and waiting for it
	;; to rollover. When the timer rolls over, it will generate an
	;; interrupt. The interrupt routine will see the least significant
	;; bit in 'temp1'. So we just monitor this bit until it goes hign.
	;;
	;; After tmr1 rolls over 4 times, we increment the prescaler and
	;; wait for more rollovers. We repeat this for each one of the
	;; prescale values.
	
	clrf	x

	;; Start the timer

	bsf	t1con,tmr1on

tmr1_test1:


	;; Loop until the timer rolls over:

	btfss	temp1,0
	 goto	$-1

	bcf	temp1,0
	
	incf	x,f
	btfss	x,2		; check for x mod 4 == 0
	 goto   tmr1_test1

	clrf	x

	movf	t1con,w		; Increment the prescaler.
	addlw   (1<<t1ckps0)
	movwf   t1con
	andlw	0x40
	skpnz
	 goto   tmr1_test1

	
	clrf	t1con		;Stop the timer.


	goto	$
	end