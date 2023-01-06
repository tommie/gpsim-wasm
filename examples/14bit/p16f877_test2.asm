
	list	p=16f877
  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to 
	;; simulate a pic 16f877.
	;; Specifically, the a/d converter is tested.
	;; This file was copied from the similar file that
	;; tests the 16c74's a/d.
	
include "p16f877.inc"
		
  cblock  0x20

	x
	t1,t2,t3
	avg_lo,avg_hi
	w_temp,status_temp,interrupt_temp
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

check_a2d:
	btfsc	pir1,adif
	 btfss	interrupt_temp,adie
	  goto	exit_int

	bcf	status,rp0	;adcon0 is in bank 0


;;	An A/D interrupt has occurred
	bsf	t1,0		;Set a flag to indicate we got the int.
	bcf	pir1,adif	;Clear the a/d interrupt

check:
exit_int:
	swapf	status_temp,w
	movwf	status
	swapf	w_temp,f
	swapf	w_temp,w
	retfie


main:

	;; Let's use the ADC's interrupt
	
	clrf	intcon
	clrf	pir1
	
#define FAST_CONVERSION		((1<<adcs1) | (1<<adcs0))
	movlw	FAST_CONVERSION | (1<<adon)
	movwf	adcon0

#define ENABLE_INTS		((1<<gie) | (1<<peie))
	movlw	ENABLE_INTS
	movwf	intcon
	bsf	status,rp0
	bsf	pie1 ^ 0x80, adie
	bcf	status,rp0

	;; Upon power up, porta is configured for analog inputs.
	;; (how many times have I been burnt by this???).	
	;; So let's test this out to see if it's true:

	call	do_some_conversions


	;; The next test consists of misusing the A/D converter.
	;; TRISA is configured such that the I/O pins are digital outputs.
	;; Normally you want them to be configued as inputs. According to
	;; the data sheet, the A/D converter will measure the voltage produced
	;; by the digital I/O output:	 either 0 volts or 5 volts (or Vdd).
	;; [I wonder if this would be a useful way of measuring the power supply
	;; level in the event that there's an external reference connected to
	;; an3?]
	

	movlw   0
	bsf	status,rp0
	movwf	porta		;Make the I/O's digital outputs
	movwf	adcon1		;Configure porta to be completely analog
				;(note that this is unnecessary since that's
				;the condition they're in at power up.)
	bcf	status,rp0

	movwf	porta		;Drive the digital I/O's low

	;;
	;; First do a few conversion with porta configured as a digital output
	;; that is driving low
	;;
	
	call	do_some_conversions

	;;
	;; Now do some with the digital output high
	;;

	movlw	0xff
	movwf	porta
	
	call	do_some_conversions

	;;
	;; Now make the inputs analog (like they normally would be)
	;;

	bsf	status,rp0
	movlw	0xff
	movwf	porta
	bcf	status,rp0

	call	do_some_conversions

	;;
	;; Now let's use the external analog signal connected to AN3
	;; as the voltage reference

	bsf	status,rp0
	movlw	1<<pcfg0
	movwf	adcon1 ^ 0x80
	bcf	status,rp0

	call	do_some_conversions
	

loop_and_convert
	decfsz	t2,f
	 goto	loop_and_convert
	call	do_some_conversions
	decfsz	t3,f
	 goto	loop_and_convert
	clrf	status
	bsf	status,rp0	; bank 1
	movlw	(1<<ADFM)
	xorwf	adcon1,f	; toggle ADFM (result format) for a/d
	clrf	status
	goto	loop_and_convert
	

	;;
	;; Now use the capture compare module to initiate the conversions
	;;

	bsf	status,rp0
	movlw	0		;Use the internal Vref
	movwf	adcon1 ^ 0x80
	bcf	status,rp0

a2d_via_ccp:

	clrf	t1con
	clrf	tmr1l
	clrf	tmr1h

	bsf	status,rp0
 	bcf	portc,2		;CCP bit is an output
	bcf	status,rp0

	;; Compare mode 11 is the 'special trigger event'
	;; it will: 1) set the CCP2IF bit, 2) reset TMR1, and 3) start 
	;; an A/D conversion

	movlw	0x0b
	movwf	ccp2con

	;;
	clrf	pir1
	clrf	pir2
	
	bsf	status,rp0
	bcf	pie2,ccp2ie	;Disable the ccp2 interrupt.
	bcf	status,rp0

	;; Initialize the 16-bit compare register:

	movlw	0x34
	movwf	ccpr1l
	movlw	0x12
	movwf	ccpr1h

	clrf	x
	
tt3:
	;; Stop and clear tmr1
	clrf	t1con
	clrf	tmr1l
	clrf	tmr1h

	;; Now start it
	bsf	t1con,tmr1on

	;; Wait for the interrupt routine to set the flag:
	btfss	t1,0
	 incf	x,f		;Count the conversions

	bcf	t1,0
	goto	$-3



do_some_conversions
	clrf	avg_lo
	clrf	avg_hi

	movlw	4
	movwf	t2		;Used to count the number of A/D conversions

l1:

	clrf	t1		;flag set by the interrupt routine
	
	bsf	adcon0,go	;Start the A/D conversion

	btfss	t1,0		;Wait for the interrupt to set the flag
	 goto	$-1

	movf	adresl,w	;Read the result
	addwf	avg_lo,f	;and accumulate it here
	movf	adresh,w
	skpnc
	 incfsz	adresh,w
	  addwf	avg_hi,f

	decfsz	t2,f
	 goto	l1

	rrf	avg_hi,f
	rrf	avg_lo,f
	rrf	avg_hi,f
	rrf	avg_lo,f
	
	return
	end