
	list	p=16c64

  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to simulate
	;; the pwm's in the mid-range pic core (e.g. p16c64).

include "p16c64.inc"
		
  cblock  0x20
	status_temp,w_temp
	interrupt_temp

	temp1,temp2
	t1,t2,t3
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

check_tmr2:
	btfsc	pir1,tmr2if
	 btfss	interrupt_temp,tmr2ie
	  goto	exit_int

    ;; tmr2 has rolled over
	
	bcf	pir1,tmr2if	; Clear the pending interrupt
	bsf	temp1,0		; Set a flag to indicate rollover

exit_int:		

	swapf	status_temp,w
	movwf	status
	swapf	w_temp,f
	swapf	w_temp,w
	retfie

	;;
	;;
	;; 
main:

	clrf	temp1
	clrf	temp2

	;; disable (primarily) global and peripheral interrupts
	
	clrf	intcon
	clrf	pir1

pwm_test1:

	bsf	pir1,tmr2if
	bsf	intcon,peie
	bsf	intcon,gie
	
	bsf	status,rp0

 	;error bsf	portc,2		;CCP bit is an input
 	bcf	trisc,2		;CCP bit is an output
 	
	;; Set the pwm frequency to (fosc/4)/(PR2 + 1)
	;; (note, TMR2 prescale is 1 on reset)
	;; (note, there's only one period register for
	;; both PWM modules. Hence, they have the same
	;; frquency [and phase].) fr=39062 Hz pr2=127 9bit resolution
	
	movlw	0x7f ;period = 128
	movwf	pr2

	;; Enable the tmr2 interrupt
	bsf	pie1,tmr2ie

	bcf	status,rp0

	; error movlw	0x80 
	movlw   0x20  ;128 25% DC 0b100000-00
	movwf	ccpr1l

	movlw	TMR2ON    ;tmr2 on
	movwf	t2con

	;;
	;; Initialize the CCP module for PWM:
	;;
	
	movlw	0xc 		;0b1100 pwm on
	movwf	ccp1con

	;;
	;; Count the number of pwm cycles
	;; 
	btfss	temp1,0		; Interrupt routine will set this
	 goto	$-1
	bcf	temp1,0
	incfsz	temp2,f
	 goto	$-4
	
	goto	$

	
	end
