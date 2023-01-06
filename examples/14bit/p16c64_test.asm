
	;; gpasm bug --- c64 is not recognized...
	
	list	p=16c64
  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16c64.

include "p16c64.inc"
		
  cblock  0x20

	x
	t1,t2,t3
  endc


	
  org	0
	goto	main


  org	4
	retfie

main:

	;; disable (primarily) global and peripheral interrupts
	
	clrf	intcon

	goto	pwm_test1

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
	
	clrf	x

	;; Start the timer

	bsf	t1con,tmr1on

tmr1_test1:


	;; Loop until the timer rolls over:

	btfss	pir1,tmr1if
	 goto	$-1

	bcf	pir1,tmr1if
	
	incf	x,f
	btfss	x,2
	 goto   tmr1_test1

	clrf	x

	movf	t1con,w
	addlw   (1<<t1ckps0)
	movwf   t1con
	andlw	0x40
	skpnz
	 goto   tmr1_test1
	
	clrf	t1con

	;;
	;; TMR2 test
	;;

tmr2_test:

	clrf	t2con
	movlw	0x40
	movwf	t1
	clrf	x

tmr2_test1:

	movf	t1,w
	bsf	status,rp0
	movwf	pr2
	bcf	status,rp0

	btfss	t2con,tmr2on
	 bsf	t2con,tmr2on
	
	bcf	pir1,tmr2if
	
	btfss	pir1,tmr2if
	 goto	$-1

	movlw	0x40
	addwf	t1,f

	incf	x,f
	btfss	x,2
	 goto	tmr2_test1

	goto	$

tmr1_test2:	
	bsf	status,rp0
 	bsf	portc,2		;CCP bit is an input
	bcf	status,rp0

	movlw	7
	movwf	ccp1con

	;; Start the timer

	bsf	t1con,tmr1on

tt2:	
	movf	portc,w
	decfsz	x,f
	 goto	$-1
	
	goto	tt2	
	goto	main

tmr1_test3:
	bsf	status,rp0
 	bcf	portc,2		;CCP bit is an output
	bcf	status,rp0

	movlw	0xb
	movwf	ccp1con

	;; Initialize the 16-bit compare register:

	movlw	0x34
	movwf	ccpr1l
	movlw	0x12
	movwf	ccpr1h

tt3:
	;;
	bcf	pir1,ccp1if
	
	;; Try the next capture mode
;; 	incf	ccp1con,f	

	;; If bit 2 is set then we're through with capture modes
	;; (and are starting pwm modes)

	btfsc	ccp1con,2
	 goto	die

	;; Start the timer

	bsf	t1con,tmr1on


	btfss	pir1,ccp1if
	 goto	$-1
	goto	tt3

pwm_test1:
	bsf	status,rp0

 	bsf	portc,2		;CCP bit is an input

	movlw	0x7f
	movwf	pr2

	bcf	status,rp0

	movlw	0x40
	movwf	ccpr1l
	clrf	ccpr1h
	movlw	4
	movwf	t2con
	movlw	0xc
	movwf	ccp1con
	
die:

	goto	$
	end