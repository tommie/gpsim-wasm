        list    p=18f452,t=ON,c=132,n=80
        radix   dec

	__CONFIG    _CONFIG2H, _WDT_OFF_2H ; & _WDTPS_128_3

;	__config	CONFIG2H,0	;Disable WDT
;	__config	DEVID1,0xab
;	__config	DEVID2,0xcd

	;; The purpose of this program is to test gpsim's ability to simulate
	;; TMR0 in the 18cxxx core.

	
include "p18f452.inc"
		
  cblock  0

	temp1
	temp2
	temp3
	temp4
	temp5
	adr_cnt
	data_cnt

	w_temp
	status_temp

	TenthsSeconds_HI
	TenthsSeconds_LO
	MilliSeconds
	MicroSeconds_HI
	MicroSeconds_LO

  endc
	org 0

	goto	start

	org	8
	;; Interrupt
	;; 
;	movwf	w_temp
;	swapf	status,w,0
;	movwf	status_temp

check_t0:
	btfsc	INTCON,T0IF,0
	 btfss	INTCON,T0IE,0
	  bra	exit_int

    ;; tmr0 has rolled over
	
	bcf	INTCON,T0IF,0	; Clear the pending interrupt
	bsf	temp1,0		; Set a flag to indicate rollover
		
exit_int:		

	movlw	0x55 ;test
;	swapf	status_temp,w,0
;	movwf	STATUS,0
;	swapf	w_temp,f
;	swapf	w_temp,w

	retfie  1		;Restore from `fast stack'

start
	;;
	;; tmr0 test
	;;
	;; The following block of code tests tmr0 together with interrupts.
	;; Each prescale value (0-7) is loaded into the timer. The software
	;; waits until the interrupt due to tmr0 rollover occurs before
	;; loading the new prescale value.
	
	bsf	INTCON,T0IE,0	;Enable TMR0 overflow interrupts
	
	bsf	INTCON,GIE,0	;Global interrupts

  bra	tmr0_done

	clrf	temp1		;Software flag used to monitor when the
				;interrupt has been serviced.
	movlw	(1<<TMR0ON)
	movwf	temp2		;Keeps track of 8 vs 16 bit mode for tmr0

tmr0_toggle_size
	btg	T0CON,T08BIT

test_tmr0:
	iorwf	temp2,W
	movwf	T0CON,0		;Assign new prescale value

tmr0_wait_for_int
	clrwdt

	btfss   temp1,0		;Wait for the interrupt to occur and
	 bra    tmr0_wait_for_int ;be serviced

	
	clrf	temp1		;Clear flag for the next time
	
	incf	T0CON,W,0
	
	andlw	0x7		;Check the prescaler
	skpz
	 bra	test_tmr0

	btfsc	T0CON,T08BIT
	 bra	test_tmr0-1
	
	bcf	INTCON,T0IE,0	;Disable tmr0 interrupts

	bra	tmr0_done	;Don't perform check with external clock

	;; Now check tmr0 with an external clock source
	;;
	;; It assumes that port b bit 0 is the stimulus.
	;; This requires that the following gpsim commands be invoked:
	;;  gpsim> node new_test_node
	;;  gpsim> attach new_test_node porta4 portb0

	bcf	TRISB,0,0	;portb bit 0 is an output

	;; assign the prescaler to the wdt so that tmr0 counts every edge
	;; select the clock source to be tocki
	;; and capture low to high transitions (tose = 0)
	
 	movlw	(1<<T0CS) | (1<<PSA)
	movwf	T0CON,0
	
	movlw	0xff
	movwf	temp2
	movwf	temp3
	movwf	temp4
	
	bcf	INTCON,T0IF,0	;not necessary..., but clear pending int.
	bsf	INTCON,T0IE,0	;Re-enable tmr0 interrupts
	
tmr0_l1:
	bcf	temp1,0		;Interrupt flag

	clrz	
	bcf	PORTB,0,0
	btfsc	temp1,0
	 decf	temp2,f		;Falling edge caused the interrupt

	bcf	temp1,0
	bsf	PORTB,0,0
	btfsc	temp1,0
	 decf	temp3,f		;Rising edge caused the interrupt

	;; if either temp2 or temp3 decremented to zero, then z will be set
	bnz	tmr0_l1

	incfsz	temp4,f
	 bra	tmr0_done
	
	;; Now let's test external clocking with the falling edge
	
	movlw	(1<<T0CS) | (1<<PSA) | (1<<T0SE)
	movwf	T0CON,0
	
	bra	tmr0_l1

	;; tmr0 test is done.

tmr0_done:
	; Now set up timer 0 to repeatedly generate a 0.1 sec interrupt

	;Assume fosc = 10.0 MHz. With a prescale of 1, TMR0 increments 
	;once every 4 fosc tics which comes out once per microsecond.
	;If we set the prescale to 4, then we'll get an int every 1.024
	;milliseconds. 

	movlw	(1<<TMR0ON) | (1<<T0PS0) | (1<<T08BIT)
	movwf	T0CON,0

tloop
	clrwdt

	btfss   temp1,0		;Wait for the interrupt to occur and
	 bra    tloop		 ;be serviced
	bcf	temp1,0

	incf	MilliSeconds,f
	movlw	6
	addwf	MicroSeconds_HI,f
	bnc	check_tenths
	addwf	MicroSeconds_HI,f
	
	incf	MilliSeconds,f

check_tenths
	movlw	100
	subwf	MilliSeconds,w
	
	bnc	no_roll

	clrf	MilliSeconds
	infsnz	TenthsSeconds_LO,f
	 incf	TenthsSeconds_HI,f
no_roll
	bra	tloop
	
	end
