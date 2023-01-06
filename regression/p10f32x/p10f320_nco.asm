
   ;;
   ;; This regression test exercises the 10f320.  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  Numerically controlled oscillator (NCO)
   ;;     


	list    p=10f320                 ; list directive to define processor
	include <p10f320.inc>            ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

.command macro x
  .direct "C", x
  endm

CONFIG_WORD	EQU	_CP_OFF 
        __CONFIG  CONFIG_WORD & _FOSC_INTOSC

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
failures        RES     1
w_temp		RES	1
status_temp	RES	1
nco_int		RES	1
loop_count	RES	1


  GLOBAL loop_count, nco_int, status_temp, w_temp

  GLOBAL done

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

INT_VECTOR   CODE    0x004               ; interrupt vector location
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp
	goto	interrupts

  .sim "p10f320.xpos = 72"
  .sim "p10f320.ypos = 84"

  .sim "module library libgpsim_modules"
  ; Use a pullup resistor as a voltage source
  .sim "module load pullup V1"
  .sim "V1.resistance = 10000.0"
  .sim "V1.capacitance = 20e-12"
  .sim "V1.voltage=4.0"
  .sim "V1.xpos = 72"
  .sim "V1.ypos = 24"


  .sim "node n3"
  .sim "attach n3 ra0 ra1"
   ;    Node Test

   .sim "scope.ch0 = 'ra1'"
   .sim "scope.ch1 = 'ra2'"
   .sim "symbol cycleCounter=0"

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

	btfss	OSCCON,HFIOFS
	goto	$-1
	CLRF	failures	;Assume success

	bsf	PIE1,NCO1IE
	bsf	INTCON,GIE
	bsf	INTCON,PEIE
        bcf	TRISA,0
        bsf	TRISA,1
        bcf	TRISA,2
	clrf	ANSELA
	clrf	NCO1INCL
	movlw	0x21
	movwf	NCO1CLK
	clrf	WDTCON		; WDT to 1 ms

	movlw	0x05
	movwf	loop_count
        movlw	(1<<N1EN)|(1<<N1OE) |(1<<N1PFM)
	movwf	NCO1CON
	; set inch for about 64 instruction cycles per interrupt
	movlw	0x10;
	movwf	NCO1INCH
	movlw	0x00
	movwf	NCO1INCL
	sleep
	nop
    .command "cycleCounter = cycles"
	nop

	sleep
	nop
	decfsz	loop_count,F
	goto    $-3
    .assert "((cycles - cycleCounter) >= 320) && ((cycles - cycleCounter) <= 324), '**** FAILED p10f320 NCO FOSC loop'"

	nop

	clrf	NCO1CLK		; use NCO1CLK input pin
	bcf	NCO1CON,N1PFM	; output toggle mode
	movlw	0x80;
	movwf	NCO1INCH
	movlw	0x00
	movwf	NCO1INCL
	clrf	NCO1ACCU
	clrf	NCO1ACCH
	clrf	NCO1ACCL	; zero acc
	bcf	PIE1,NCO1IE	; disable interrupts
inc_loop:
	clrf	loop_count
	bsf	LATA,0
	bcf	LATA,0
	incf	loop_count,F
	btfss	PIR1,NCO1IE
	goto	$-4
	nop
   .assert "loop_count == 0x22, '*** FAILED p10f320 NCO1CLK pin increment'"
	nop
	bcf	PIR1,NCO1IE
	
	MOVLW	0xff
	movwf   PIR1
  .assert "pir1 == 0x5a, '*** FAILED P10F320 PIR1 bits'"
	nop
	clrf	PIR1
	BSF	STATUS,RP0
	BCF 	STATUS,RP0
	goto	done


wait_tmr2:
	bcf	PIR1, TMR2IF
        btfss   PIR1,TMR2IF
	goto	$-1
	return

pwm_test:
;        movlw   0x4
;        movwf   ANSELA
;	movlw	0x0f
;        movwf   TRISA
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
	movlw	(1<<PWM1EN)
	MOVWF	PWM1CON
	return
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
;	clrf	T2CON
	return



FAILED:
  .assert  "'*** FAILED 10f320 test'"
	INCF	failures,F
done:
  .assert  "'*** PASSED p10f320 NOC test'"
	nop
	GOTO	$

interrupts:
	nop
	btfsc	PIR1,NCO1IF
	goto	nco_interrupt
	goto	int_ret

nco_interrupt:
	bcf     PIR1,NCO1IF
	incf	nco_int,F

int_ret
	swapf	status_temp,W
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie
  end
