
   ;;
   ;; This regression test exercises the 10f320.  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  Complementary Waveform generate(CWG) with shutdown
   ;;  - 
   ;;     


	list    p=10f320                 ; list directive to define processor
	include <p10f320.inc>            ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

.command macro x
  .direct "C", x
  endm

CONFIG_WORD	EQU	_CP_OFF 
        __CONFIG  CONFIG_WORD & _FOSC_INTOSC & _WDTE_OFF

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
  .sim "attach n3 V1.pin ra2"
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

        bcf	TRISA,0
        bcf	TRISA,1
	clrf	ANSELA

	call	pwm_test

;	movlw   (1<<G1EN)|(1<<G1OEA)
;	movwf	CWG1CON0
loop:
	movlw	0x3f
	movwf	CWG1DBR
	movwf	CWG1DBF
	bsf	CWG1CON0,G1EN
	bsf	CWG1CON0,G1OEA
	bsf	CWG1CON0,G1OEB
	call	wait_tmr2
	call	wait_tmr2
	movlw	0x50		; shutdown A,B tristate
	movwf	CWG1CON1
	movlw	(1<<G1ARSEN) | (1<<G1ASE)
	movwf	CWG1CON2
	btfsc	CWG1CON2,G1ASE
	goto	$-1
	call	wait_tmr2
	movlw	0xe0		; shutdown A=0 B=1
	movwf   CWG1CON1
	bsf	CWG1CON2,G1ASDSFLT
    .command "V1.voltage=0.5"
	nop
	call	wait_tmr2
    .command "V1.voltage=4.5"
	nop
	call	wait_tmr2

	MOVLW	0xff
	movwf   PIR1
  .assert "pir1 == 0x5a, '*** FAILED P10F320 PIR1 bits'"
	nop
	clrf	PIR1
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
  .assert  "'*** PASSED 10f320 CWG test'"
	GOTO	$


  end
