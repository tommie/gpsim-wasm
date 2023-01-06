
	list	p=16c71
  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16c71.
	;; Specifically, the a/d converter is tested.

include "p16c71.inc"
		
  cblock  0x0c

	x
	t1,t2
	avg_lo,avg_hi
	w_temp,status_temp
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

	bcf	status,rp0	;adcon0 is in bank 0

	btfsc	intcon,adie
	 btfss	adcon0,adif
	  goto	check

;;	An A/D interrupt has occurred
	bsf	t1,0		;Set a flag to indicate we got the int.
	bcf	adcon0,adif	;Clear the a/d interrupt

check:
	swapf	status_temp,w
	movwf	status
	swapf	w_temp,f
	swapf	w_temp,w
	retfie


main:

	;; Let's use the ADC's interrupt
	
	clrf	intcon
	
#define FAST_CONVERSION		((1<<adcs1) | (1<<adcs0))
	movlw	FAST_CONVERSION | (1<<adon)
	movwf	adcon0

#define ENABLE_INTS		((1<<gie) | (1<<adie))
	movlw	ENABLE_INTS
	movwf	intcon

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
	
	goto	$-1



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

	movf	adres,w		;Read the result
	addwf	avg_lo,f	;and accumulate it here
	skpnc
	 incf	avg_hi,f

	decfsz	t2,f
	 goto	l1

	rrf	avg_hi,f
	rrf	avg_lo,f
	rrf	avg_hi,f
	rrf	avg_lo,f
	
	return
	end