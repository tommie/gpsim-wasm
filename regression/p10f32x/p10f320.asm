
   ;;
   ;; This regression test exercises the 10f320.  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  Program flash read/write
   ;;  A2D and PWM
   ;;     


	list    p=10f320                 ; list directive to define processor
	include <p10f320.inc>            ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

CONFIG_WORD	EQU	_CP_OFF 
        __CONFIG  CONFIG_WORD & _FOSC_INTOSC

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
failures        RES     1


  GLOBAL done

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

INT_VECTOR   CODE    0x004               ; interrupt vector location
	nop


  .sim "module library libgpsim_modules"
  ; Use a pullup resistor as a voltage source
  .sim "module load pullup V1"
  .sim "V1.resistance = 10000.0"
  .sim "V1.capacitance = 20e-12"
  .sim "V1.voltage=1.0"
  .sim "V1.xpos = 72"
  .sim "V1.ypos = 24"

  .sim "p10f320.xpos = 72"
  .sim "p10f320.ypos = 84"


  .sim "node n3"
  .sim "attach n3 ra2 V1.pin"
   ;    Node Test

   .sim "scope.ch0 = 'ra0'"
   .sim "scope.ch1 = 'ra1'"

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

	btfss	OSCCON,HFIOFS
	goto	$-1
	CLRF	failures	;Assume success

	MOVLW	0xff
	movwf   PIR1
  .assert "pir1 == 0x5a, '*** FAILED P10F320 PIR1 bits'"
	nop
	clrf	PIR1

	CLRF	TRISA	;Port A is an output
	BSF	STATUS,RP0

	BCF 	STATUS,RP0

	call	flash_write
	call	flash_read
	call	pwm_test
        call	a2dtest

	goto	done

flash_write:
        call  read_prog
	movlw (1<<LWLO)|(1<<WREN)
	movwf PMCON1
        movlw 0x00
        movwf PMADRH
        movlw 0xf0
        movwf PMADRL
        movlw 0x40
        movwf FSR
LOOP    movf  INDF,W      ;Load first data byte into lower
        movwf PMDATL      ;
        incf  FSR,F       ;Next byte
        movf  INDF,W      ;Load second data byte into upper
        movwf PMDATH
        incf  FSR,F
        bcf   INTCON,GIE  ;Disable interrupts
        btfsc INTCON,GIE  ;See AN576
        goto $-2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Required Sequence

        movlw 55h         ;Start of required write sequence:
        movwf PMCON2      ;Write 55h
        movlw 0AAh        ;
        movwf PMCON2      ;Write 0AAh
        bsf   PMCON1,WR   ;Set WR bit to begin write
        nop               ;Required to transfer data to the buffer
        nop               ;registers

     
        bsf   INTCON,GIE ;Enable interrupts
        movf  PMADRL, W
        incf  PMADRL,F     ;Increment address
        andlw 0x0f        ;Indicates when sixteen words have been programmed
        sublw 0x0e        
        btfsc STATUS,Z    ;Clear LWLO for last write
	bcf   PMCON1,LWLO
	btfsc STATUS,C    ;Exit on a match,
        goto  LOOP         ;Continue if more data needs to be written
	btfsc PMCON1,WR
	goto  $-1
	call	write_verify
        return

write_verify:
        movlw 0x00
        movwf PMADRH
        movlw 0xf0
        movwf PMADRL
        movlw 0x40
        movwf FSR
	clrf  PMCON1
wv_loop
	bsf   PMCON1,RD
	nop
	nop
        movf  INDF,W 
   .assert "W == pmdatl, '*** FAILED P10F320 flash write verify low'"
        nop
	incf  FSR,F
        movf  INDF,W 
   .assert "W == pmdath, '*** FAILED P10F320 flash write verify high'"
        nop
	incf  FSR,F
        incf  PMADRL,F     ;Increment address
	movf  PMADRL,W
        andlw 0x0f        ;Indicates when sixteen words looked at
	btfss STATUS,Z    ;Exit on a match,
        goto  wv_loop
	return
	
read_prog:
        movlw       low flash_write
        movwf       PMADRL
        movlw       high flash_write
        movwf       PMADRH
        movlw	0x40
        movwf	FSR
        clrf	PMCON1
rp_loop
        bsf         PMCON1,RD
        nop
        nop
        movf	PMDATL,W
        movwf	INDF
        incf	FSR,F
        movf	PMDATH,W
        movwf	INDF
        incf	FSR,F
        incf	PMADRL,F
	movf	FSR,W
	sublw	0x60
        btfss   STATUS,Z
        goto	rp_loop
        return
    
    

flash_read:
        clrf	PMADRH
        movlw	6	; Read Device ID
        movwf	PMADRL
        movlw  (1<<RD)|(1<<CFGS)
        movwf	PMCON1
        nop
        nop
  .assert "pmdatl == 0xa0, '*** FAILED P10F320 ID low'"
        nop
  .assert "pmdath == 0x29, '*** FAILED P10F320 ID high'"
        nop
        movlw	7	; Read Config Word
        movwf	PMADRL
        bsf	PMCON1,RD	; start read
        nop
        nop
  .assert "pmdatl == 0xfe, '*** FAILED P10F320 config low'"
        nop
  .assert "pmdath == 0x3f, '*** FAILED P10F320 config high'"
        nop
        return

wait_tmr2:
	bcf	PIR1, TMR2IF
        btfss   PIR1,TMR2IF
	goto	$-1
	return

pwm_test:
        movlw   0x4
        movwf   ANSELA
	movlw	0x0f
        movwf   TRISA
	clrf	PWM1CON
	clrf	PWM2CON
	movlw	0x3f
	movwf	PR2
	movlw	0x1f
	movwf	PWM1DCH      ; Duty cycle 50%
	movlw	0x0f
	movwf	PWM2DCH      ; Duty cycle 25%
	movlw	0xc0
	movwf	PWM1DCL
	movlw	0xc0
	movwf	PWM2DCL
	movlw  	0x06          ; Start Timer2 with prescaler as 16
	movwf  	T2CON
	clrf	TMR0
	call	wait_tmr2
	bcf	TRISA,0
	bcf	TRISA,1
        movlw   0x83          ; Tmr0 internal clock prescaler 16
        movwf   OPTION_REG
	movlw	(1<<PWM1EN)|(1<<PWM1OE)
	MOVWF	PWM1CON
	MOVWF	PWM2CON
	call	wait_tmr2
	clrf    TMR0
	btfsc	PWM2CON,PWM2OUT
	goto	$-1
	.assert "tmr0 == 0x0f, '***FAIL p10f322 CCP2 duty cycle'"
	nop
	btfsc	PWM1CON,PWM1OUT
	goto	$-1
	.assert "tmr0 == 0x1f, '***FAIL p10f322 CCP1 duty cycle'"
	nop
	call	wait_tmr2
	.assert "tmr0 == 0x3f, '***FAIL p10f322 TMR2 == PR2'"
	nop
	call	wait_tmr2
	call	wait_tmr2
	movlw	0x40		; Duty cycle = 100%
	movwf	PWM1DCH
	call	wait_tmr2
	call	wait_tmr2
	clrf	PWM1DCH		; Duty cycle == 0%
	call	wait_tmr2
	call	wait_tmr2
	clrf	T2CON
	return


a2dtest:
	movlw	0x4
	movwf	TRISA
	movwf	ANSELA
	bcf	WPUA,2
	movlw	(1<<CHS1) | (1<<ADON)	; select an2 and turn on ADC
	movwf	ADCON
	nop
	bsf	ADCON,GO_NOT_DONE
	btfsc	ADCON,GO_NOT_DONE
	goto	$-1
   .assert "adres==0x33, '*** FAILED 10f320 ADC an2'"
	nop
        movlw   (1<<TSEN)|(1<<TSRNG)	; Turn on Temp high range
				; vt = 0.659 - (Tcpu + 40) * 0.00132
				; vt = 0.5666 for Tcpu=30
				; Vout= 5 - 4* vt = 2.7336
	movwf	FVRCON
	bsf	ADCON,CHS2	; select Temperature ADC channel
	nop
	bsf	ADCON,GO_NOT_DONE
	btfsc	ADCON,GO_NOT_DONE
	goto	$-1
   .assert "adres== 0x8b, '*** FAILED 10f320 ADC Temp=30'"
	nop
	movlw	(1<<FVREN)|(1<<ADFVR0) ; Fixed voltage reference 1.024V
        movwf   FVRCON
	movlw	0xfd		; select FVR chan, FRC
	movwf	ADCON
        nop
        bsf     ADCON,GO_NOT_DONE
        btfsc   ADCON,GO_NOT_DONE
        goto    $-1
   .assert "adres== 0x34, '*** FAILED 10f320 ADC FVR=1.024'"
        nop

	return
        

a_to_b_loop:
	MOVWF	PORTA		;Port A and Port B are externally

	BSF	PORTA,4		;Port A bit 4 is an open collector.
  .assert  "'*** FAILED 10f320 test -RA4 stuck high'"
	 GOTO	FAILED


   ; Now let's write from PORTB to PORTA.
   ; With the configuration bit setting we have, all of PORTA I/O lines
   ; should be able to serve as inputs

	BSF	STATUS,RP0

	COMF	TRISA^0x80,F	;Port A is now an input port

	BCF 	STATUS,RP0

	CLRW


b_to_a_loop:
	XORWF	PORTA,W		;connected. So we should see the
	ANDLW	0x1f
	SKPZ			;same thing on each port.
	 GOTO	FAILED

	 goto	b_to_a_loop

   ;
   ; Now test PORTB's internal pullups
   ;


	MOVLW	0xff
	BSF	STATUS,RP0

	MOVWF	TRISA^0x80	;Port A is an input


	BCF 	STATUS,RP0

	ANDLW	0x0F
	SKPZ
	 GOTO	FAILED

	BSF	STATUS,RP0
	BCF 	STATUS,RP0

	XORLW	0xFF
	SKPZ
	 GOTO	FAILED

	GOTO	done

FAILED:
  .assert  "'*** FAILED 10f320 test'"
	INCF	failures,F
done:
  .assert  "'*** PASSED 10f320 A2D, PWM test'"
	GOTO	$


  end
