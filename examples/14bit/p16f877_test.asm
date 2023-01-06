
	list	p=16f877
  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16f877.
	;; (this was derived from the similar program for testing the c64).
	
include "p16f877.inc"
		
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
	swapf	status,w
	movwf	status_temp
	
	btfss	intcon,peie
	 goto	exit_int

	bcf	status,rp1
	bcf	status,rp0

	btfss	(pir2 & 0x7f),eeif
	 goto	exit_int

;;; eeprom has interrupted
	bcf	(pir2 & 0x7f),eeif

exit_int:
	swapf	status_temp,w
	movwf	status
	swapf	w_temp,f
	swapf	w_temp,w
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
	
	clrf	intcon

	call	obliterate_data_eeprom
	call	bank_access_test
	call	read_program_flash
	
;;	call	tmr1_test
;; 	call	tmr2_test
;; 	call	tmr1_test2
;; 	call	tmr1_test3
;; 	call	pwm_test

	goto	$

	;; =========================================================
	;; read_program_flash
	;;
	;; Copy a string from program memory to data memory
	
read_program_flash

;;;
;;;

	clrf	status		; Bank 0
	movlw	pma_string	; Pointer to where we'll copy the string
	movwf	fsr
	
	movlw	pgm_table_end - pgm_table ; Length of the string
	movwf	adr_cnt			  ; (actually 1/2 string length)
	
	bsf	status,rp1	;Point to bank 2
	bcf	status,rp0	;

	movlw	LOW(pgm_table)	; Start of the string
	movwf	eeadr		; in program memory
	movlw	HIGH(pgm_table)
	movwf	eeadrh

pgm_flash_read_loop1:	
	bsf	status,rp0	; bank 3
	
	bsf	eecon1,eepgd	; program memory (instead of data flash)
	bsf	eecon1,rd	; start the read.
	bcf	status,rp0	; two cycle delay to read memory
	nop
	
	rlf	eedata,w	; Copy the 14bit value that was read
	rlf	eedath,w	; from program memory. It's broken into
	movwf	indf		; two 7-bit values that are copied to
	incf	fsr,f		; data memory
	movf	eedata,w
	andlw	0x7f
	movwf	indf
	incf	fsr,f

	incf	eeadrh,f	; Next program memory location
	incfsz	eeadr,f
	 decf	eeadrh,f
	decfsz	adr_cnt,f
	 goto	pgm_flash_read_loop1

	return

pgm_table

	DATA14	't','e'
	DATA14	's','t'
pgm_table_end

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

	return
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

	return

tmr1_test2:	
	bsf	status,rp0
 	bsf	portc,2		;CCP bit is an input
	bcf	status,rp0

	movlw	7
	movwf	ccp1con

	;; Start the timer

	bsf	t1con,tmr1on
	clrf	y
tt2:	
	movf	portc,w
	decfsz	x,f
	 goto	$-1
	decfsz	y,f
	  goto	tt2


	return


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
	clrf	status
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

	return	
die:

	goto	$
	;; ======================================================
	;; preset_ram
	;;
	;; copy the contents of w to all of ram

preset_ram
	clrf	status

	movwf	0x20		;save the constant
	movlw	0x20		;start of ram
	movwf	fsr		;use indirect addressing to preset
	movf	0x20,w		;get the constant
cb0:	
	movwf	indf		;Preset...
	incf	fsr,f		;next register
	btfss	fsr,7		;loop until fsr == 0x80
	 goto	cb0
	bsf	fsr,5		;fsr = 0xa0
cb1:	
	movwf	indf
	incfsz	fsr,f		; loop until fsr == 0
	 goto	cb1
	bsf	fsr,4		;fsr =0x10
	bsf	status,7	;irp=1 ==> fsr =0x110
cb2:	
	movwf	indf
	incf	fsr,f
	btfss	fsr,7		;loop until fsr == 0x80
	 goto	cb2
	bsf	fsr,4		;fsr == 0x90 (but irp is still set)
cb3:	
	movwf	indf
	incfsz	fsr,f
	 goto	cb3

	clrf	status		;clear irp,rp0,rp1

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
	
	bcf	status,rp0
	movlw	0x70
	movwf	fsr

	clrf	0x70
	incf	indf,f
	btfss	0x70,0
	 goto	$-1

	bsf	status,rp0	;bank 1
	incf	indf,f
	btfsc	0x70,0
	 goto	$-1

	bsf	status,rp1	;bank 3
	incf	indf,f
	btfss	0x70,0
	 goto	$-1

	bcf	status,rp0	;bank 2
	incf	indf,f
	btfsc	0x70,0
	 goto	$-1

	bcf	status,rp1	;bank 0

	return
	
obliterate_data_eeprom
	
	clrf	adr_cnt
	clrf	data_cnt
	incf    data_cnt,f
	bsf	intcon,eeie

l1:	


	bsf	status,rp1	;Point to bank 2
	bcf	status,rp0	;

	movf	adr_cnt,w
	movwf	eeadr
	movf	data_cnt,W
	movwf	eedata

	bsf	status,rp0
	bcf	intcon,gie	;Disable interrupts while enabling write

	bcf	(eecon1 & 0x7f),eepgd	;Point to data and not program mem.
	bsf	(eecon1 & 0x7f),wren	;Enable eeprom writes

	movlw	0x55		;Magic sequence to enable eeprom write
	movwf	(eecon2 & 0x7f)
	movlw	0xaa
	movwf	(eecon2 & 0x7f)

	bsf	(eecon1 & 0x7f),wr	;Begin eeprom write

	bsf	intcon,gie	;Re-enable interrupts
	
	btfsc	(eecon1 & 0x7f),wr	;Wait for the write to complete 
	 goto	$-1

;; 	bcf	status,rp0
	
	incfsz	adr_cnt,f
	 goto	l1

	incfsz	data_cnt,F
	 goto	l1

	return

	
		
	org	0x2100

	de	"Linux is cool!",0
	de	0xaa,0x55,0xf0,0x0f
	de	'g','p','s','i','m'
	de	"p16f877_test.asm - a program to test "
	de	"eveything an 'f877 can possibly do."
		
	end