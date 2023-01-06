	list	p=16f628
  __CONFIG _WDT_OFF

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
	;;   invoke the simualtion from the Makefile:
	;;   make sim

	;; In all cases, the stimulus file will load the simulation
	;; file and create the stimuli. In the Makefile case, the
	;; simulation file will be (re)created if the .asm has been
	;; been changed.
	;;
	;;



	;; The purpose of this program is to test gpsim's ability to simulate a pic 16f627/8.
	;; (this was derived from the similar program for testing the c64).
	
include "p16f628.inc"
		
  cblock  0x20

	x,y
	t1,t2,t3
	pma_string		; starting here's where a string will be copied from flash
  endc

	;; Take advantage of the upper 16 bytes all banks being aliased
  cblock 0x70

	adr_cnt
	data_cnt
	w_temp
	status_temp

  endc

DATA14  macro  _a, _b
	data (_a<<7) | (_b)
  endm
	
  org	0
	goto	main


  org	4
	;; Interrupt
	;; 
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp
	
	btfss	INTCON,PEIE
	 goto	exit_int

	bcf	STATUS,RP1
	bcf	STATUS,RP0

	btfss	PIR1,EEIF
	 goto	exit_int

;;; eeprom has interrupted
	bcf	PIR1,EEIF

exit_int:
	swapf	status_temp,W
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie



main:
	
	;; clear ram
	movlw	0
	call	preset_ram
	movlw	0x55
	call	preset_ram
	movlw	0
	call	preset_ram


	;; disable (primarily) global and peripheral interrupts
	
	clrf	INTCON

	call	obliterate_data_eeprom
	call	bank_access_test
	
	call	tmr1_test
 	call	tmr2_test
 	call	tmr1_test2
 	call	tmr1_test3
 	call	pwm_test1

	goto	$


	;;
	;; TMR1 test
	;;
tmr1_test:	
	;; Clear all of the bits of the TMR1 control register:
	;; this will:
	;;	Turn the tmr off
	;;	Select Fosc/4 as the clock source
	;;	Disable the External oscillator feedback circuit
	;;	Select a 1:1 prescale
	
	clrf	T1CON		;
	bcf	PIR1,TMR1IF	; Clear the interrupt/roll over flag

	;; Zero the TMR1

	clrf	TMR1L
	clrf	TMR1H

	;; Test rollover
	;;  the following block of code will test tmr1's rollover for
	;; each of the 4 prescale values.
	
	clrf	x

	;; Start the timer

	bsf	T1CON,TMR1ON

tmr1_test1:


	;; Loop until the timer rolls over:

	btfss	PIR1,TMR1IF
	 goto	$-1

	bcf	PIR1,TMR1IF
	
	incf	x,F
	btfss	x,2
	 goto   tmr1_test1

	clrf	x

	movf	T1CON,W
	addlw   (1<<T1CKPS0)
	movwf   T1CON
	andlw	0x40
	skpnz
	 goto   tmr1_test1
	
	clrf	T1CON

	return


	;;
	;; TMR2 test
	;;
	;; For this test, we load the PR2 register (timer2 period register)
	;; with 0x40, 0x80, 0xc0, and 0x00 and wait for TMR2 to count up
	;; to the period.
	;;

tmr2_test:

	clrf	T2CON
	movlw	0x40
	movwf	t1
	clrf	x

tmr2_test1:

	movf	t1,W		;Get the next period setting
	bsf	STATUS,RP0
	movwf	PR2		;write the new period.
	bcf	STATUS,RP0

	btfss	T2CON,TMR2ON	;If tmr2 is not already on
	 bsf	T2CON,TMR2ON	;then turn it on
	
	bcf	PIR1,TMR2IF	;tmr2if is set when TMR2 finishes count
	
	btfss	PIR1,TMR2IF	;wait for the roll over
	 goto	$-1

	movlw	0x40		;Increase the period by 0x40 counts
	addwf	t1,F

	incf	x,F
	btfss	x,2
	 goto	tmr2_test1

	return

tmr1_test2:	
	bsf	STATUS,RP0
; 	bsf	PORTC,2		;CCP bit is an input
	bcf	STATUS,RP0

	movlw	7
	movwf	CCP1CON

	;; Start the timer

	bsf	T1CON,TMR1ON
	clrf	y
tt2:	
;	movf	PORTC,w
	decfsz	x,F
	 goto	$-1
	decfsz	y,F
	  goto	tt2


	return


tmr1_test3:
	bsf	STATUS,RP0 
;	bcf	PORTC,2		;CCP bit is an output
	bcf	STATUS,RP0

	movlw	0xb
	movwf	CCP1CON

	;; Initialize the 16-bit compare register:

	movlw	0x34
	movwf	CCPR1L
	movlw	0x12
	movwf	CCPR1H

tt3:
	;;
	bcf	PIR1,CCP1IF
	
	;; Try the next capture mode
;; 	incf	CCP1CON,F	

	;; If bit 2 is set then we're through with capture modes
	;; (and are starting pwm modes)

	btfsc	CCP1CON,2
	 goto	die

	;; Start the timer

	bsf	T1CON,TMR1ON


	btfss	PIR1,CCP1IF
	 goto	$-1
	goto	tt3

pwm_test1:
	clrf	STATUS
	bsf	STATUS,RP0

; 	bsf	PORTC,2		;CCP bit is an input

	movlw	0x7f
	movwf	PR2

	bcf	STATUS,RP0

	movlw	0x40
	movwf	CCPR1L
	clrf	CCPR1H
	movlw	4
	movwf	T2CON
	movlw	0xc
	movwf	CCP1CON

	return	
die:

	goto	$
	;; ======================================================
	;; preset_ram
	;;
	;; copy the contents of w to all of ram

preset_ram
	clrf	STATUS

	movwf	0x20		;save the constant
	movlw	0x20		;start of ram
	movwf	FSR		;use indirect addressing to preset
	movf	0x20,W		;get the constant

	;; write to address 0x20 - 0x7f
cb0:	
	movwf	INDF		;Preset...
	incf	FSR,F		;next register
	btfss	FSR,7		;loop until fsr == 0x80
	 goto	cb0

	bsf	FSR,5		;fsr = 0xa0

	;; write to address 0xa0 - 0xff
cb1:	
	movwf	INDF
	incfsz	FSR,f		; loop until fsr == 0
	 goto	cb1

	bsf	FSR,5		;fsr =0x20
	bsf	STATUS,7	;irp=1 ==> fsr =0x120

	;; write to address 0x120 - 0x14f
cb2:	
	movwf	INDF
	incf	FSR,F
	
	btfsc	FSR,6		;loop until fsr == 0x50
	 btfss  FSR,4		;i.e. check for the bit pattern x1x1xxxx
	  goto	cb2

	bsf	FSR,7		;fsr == 0xd0
	bsf	FSR,5		;fsr == 0xf0

	;; write to address 0x1f0 - 0x1ff

cb3:	
	movwf	INDF
	incfsz	FSR,F
	 goto	cb3

	clrf	STATUS		;clear irp,rp0,rp1

	return


	;; =========================================================
	;; bank_access_test:
	;; 
	;; use the indirect register to increment register 0x70.
	;; This register is aliased in all four banks.
	;; Then use direct addressing to verify that the increment
	;; occurred. Note that bank switching is necessary to access
	;; 0x70 directly in the other banks (that is to access 0xf0,
	;; 0x170 and 0x1f0 directly, you need to switch to bank 1,2
	;; or 3 respectively).
bank_access_test:	
	
	bcf	STATUS,RP0
	movlw	0x70
	movwf	FSR

	clrf	0x70
	incf	INDF,F
	btfss	0x70,0
	 goto	$-1

	bsf	STATUS,RP0	;bank 1
	incf	INDF,F
	btfsc	0x70,0
	 goto	$-1

	bsf	STATUS,RP1	;bank 3
	incf	INDF,F
	btfss	0x70,0
	 goto	$-1

	bcf	STATUS,RP0	;bank 2
	incf	INDF,F
	btfsc	0x70,0
	 goto	$-1

	bcf	STATUS,RP1	;bank 0

	return


;-------------------------------------------------------------
; obliterate_data_eeprom
;
; This routine will loop 255 times and write the loop iteration
;number to the entire contents of the EEPROM. I.e. on the first
;pass all 1's are written, on the second pass all 2's are written
;and so on.

obliterate_data_eeprom
	
	clrf	adr_cnt
	clrf	data_cnt
	incf    data_cnt,f

	bcf	STATUS,RP1	;Point to bank 1
	bsf	STATUS,RP0	;(That's where the EEPROM registers are)
	bsf	PIE1,EEIE	;Enable eeprom interrupts
	bsf	INTCON,PEIE	;The peripheral interrupt bit mus also be set
				;to allow ints.

l1:	
	movf	adr_cnt,W
	movwf	EEADR
	movf	data_cnt,W
	movwf	EEDATA

	bcf	INTCON,GIE	;Disable interrupts while enabling write

	bsf	EECON1,WREN	;Enable eeprom writes

	movlw	0x55		;Magic sequence to enable eeprom write
	movwf	EECON2
	movlw	0xaa
	movwf	EECON2

	bsf	EECON1,WR	;Begin eeprom write

	bsf	INTCON,GIE	;Re-enable interrupts
	
	btfsc	EECON1,WR	;Wait for the write to complete 
	 goto	$-1

	incf	adr_cnt,f

	btfss	adr_cnt,7	;Does the address == 0x80?
	 goto	l1

	clrf	adr_cnt

	incfsz	data_cnt,F
	 goto	l1

	return

	
		
	org	0x2100

	de	"Linux is cool!",0
	de	0xaa,0x55,0xf0,0x0f
	de	'g','p','s','i','m'
	de	"f628.asm - a program to test "
	de	"eveything an 'f877 can possibly do."
		
	end
