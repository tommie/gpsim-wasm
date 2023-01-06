;
; 
; Copyright (c) 2022 Roy Rankin
;
; This file is part of the gpsim regression tests
; 
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.
; 
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.
; 
; You should have received a copy of the GNU Lesser General Public
; License along with this library; if not, see 
; <http://www.gnu.org/licenses/lgpl-2.1.html>.


	list	p=16f616
        include <p16f616.inc>
        include <coff.inc>

        __CONFIG   _CP_OFF & _WDTE_OFF &  _INTOSCIO &  _MCLRE_OFF

	;; The purpose of this program is to test gpsim's ability to 
	;; simulate a pic 16F616.
	;; Specifically, the comparator and gate function of Tmr1 is tested.

        errorlevel -302 

;; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR

int_cm1       RES  1
int_cm2       RES  1
int_cm4       RES  1
x             RES  1
t1            RES  1
t2            RES  1
avg_lo        RES  1
avg_hi        RES  1
w_temp        RES  1
status_temp   RES  1
adc_cnt       RES  1

  GLOBAL int_cm1, int_cm2, int_cm4
;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        ;movlp  high  start               ; load upper byte of 'start' label
        goto   start                     ; go to beginning of program

	;; 
	;; Interrupt
	;; 
INTERRUPT_VECTOR CODE 0X004

	goto	interrupt

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

   .sim "p16f616.xpos = 72"
   .sim "p16f616.ypos = 72"

   .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
   .sim "module load pullup V1"
   .sim "V1.resistance = 100.0"
   .sim "V1.xpos = 240"
   .sim "V1.ypos = 72"
   .sim "V1.voltage = 4.0"

   .sim "module load pullup V2"
   .sim "V2.resistance = 100.0"
   .sim "V2.xpos = 84"
   .sim "V2.ypos = 24"
   .sim "V2.voltage = 2.5"



   .sim "module load pullup V3"	
   .sim "V3.resistance = 100.0"
   .sim "V3.xpos = 240"
   .sim "V3.ypos = 120"
   .sim "V3.voltage = 4.5"

   .sim "module load pullup V4"	
   .sim "V4.resistance = 100.0"
   .sim "V4.xpos = 240"
   .sim "V4.ypos = 220"
   .sim "V4.voltage = 4.0"

   .sim "node na0"
   .sim "attach na0 V3.pin portc0" ;    C2IN0+
   .sim "node na1"
   .sim "attach na1 V1.pin porta1"	; CxIN0-
   .sim "node na3"
   .sim "attach na3 V2.pin porta0"	; C1IN0+
   .sim "node na4"
   .sim "attach na4 V4.pin portc1"	; CxIN1-	

   .sim "symbol cycleCount=0"

   .sim "node na5"
   .sim "attach na5 porta4 portc5"
    .sim "scope.ch0='portc5'"
    .sim "scope.ch1='portc4'"
    .sim "scope.ch2='portc3'"
    .sim "scope.ch3='portc2'"



                                                                                
    .assert "frequency == 8000000., '*** FAILED p16f616 initial frequency'"
	nop
        BANKSEL OSCTUNE
	movlw	0x0f
	movwf	OSCTUNE
    .assert "frequency == 9000000., '*** FAILED p16f616 max OSCTUNE frequency'"
	nop
	movlw	0x1f
	movwf	OSCTUNE
    .assert "frequency == 7000000., '*** FAILED p16f616 min OSCTUNE frequency'"
	nop
	clrf	OSCTUNE
	BANKSEL PIR1
	movlw	0xff
	movwf   PIR1
    .assert " pir1 == 0x7b, '*** FAILED p16f616 pir1 writeable bits'"
	nop
	clrf	PIR1

	call	setup
	call	test_adc
	call	simp_pwm
	call    full_forward
	call    full_reverse
	call	half_mode
	call	pwm_shutdown
	call	sri
	call    test_compar
	call	test_tot1	; can C2 increment TMR1
  .assert  "'*** PASSED 16F616 PWM, SR, Compare test'"
	nop
	goto	$-1

setup:
	banksel CCP1CON
	clrf   CCP1CON       ;  CCP Module is off
	clrf   TMR2          ;  Clear Timer2
	clrf   TMR0          ;  Clear Timer0
	banksel CCPR1L
	movlw  0x1F          ;
	movwf  CCPR1L        ;  Duty Cycle is 25% of PWM Period
	clrf   INTCON        ;  Disable interrupts and clear T0IF


	BANKSEL ANSEL
	bcf	ANSEL,2
	bcf	ANSEL,3
	BANKSEL TRISA
	bcf	TRISA,2
	bcf    TRISC,5       ;  P1A
	bcf    TRISC,4       ;  P1B
	bcf    TRISC,3       ;  P1C
	bcf    TRISC,2       ;  P1D


        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts
	BANKSEL PIE1
        bsf     PIE1,C1IE       ;CM1 interrupts
        bsf     PIE1,C2IE       ;CM2 interrupts
	return

simp_pwm:
	BANKSEL CCP1CON
	movlw	0xff
	movwf	CCP1CON
	movwf	PWM1CON
	movwf	ECCPAS
	clrf	ECCPAS
	movlw	0x0c	;Simple PWM
	movwf	CCP1CON
	movlw	0x80
	movwf	CCPR1L
	movlw	0x05
	movwf	T2CON
 	bcf	PIR1,TMR2IF
	btfss	PIR1,TMR2IF
	goto	$-1
    .command "cycleCount = cycles"
        nop
	BANKSEL	TRISC
	bcf	TRISC,5
	BANKSEL TMR1H
	clrf	TMR1H
	clrf	TMR1L
	movlw	(1<<TMR1GE) | (1<<TMR1ON) 
	movwf	T1CON
	BANKSEL	PIR1
 	bcf	PIR1,TMR2IF
	btfss	PIR1,TMR2IF
	goto	$-1
    .command "cycleCount = cycles - cycleCount"
	nop
   .assert "(cycleCount >= 1024) && (cycleCount <= 1026), '*** FAILED p16f616 simple pwm period'"
	nop
   .assert "(tmr1h == 2), '*** FAILED p16f616 simple pwm duty cycle'"
	nop
	clrf	T1CON
	clrf	T2CON
	clrf	CCP1CON
	return

full_forward
	banksel CCP1CON
	movlw  0x1F
	movwf  CCPR1L
	movlw  0x6C          ;  PWM full forward mode, 2 LSbs of Duty cycle = 10
	movwf  CCP1CON       ;
	movlw  0x05		 ; Start Timer2 prescaler is 4
	movwf  T2CON
	banksel PR2		 ; Bank 1
	movlw  0x2F          ;
	movwf  PR2           ;
	call   wait_period
  .assert "(portc & 0x3c) == 0x24, '*** FAILED p16f616 epwm full bridge HLLH'"
	nop
	btfsc   PORTC,2
	goto    $-1
  .assert "(portc & 0x3c) == 0x20, '*** FAILED p16f616 epwm full bridge HLLL'"
	nop
	return

full_reverse
	banksel CCP1CON
	movlw  0xEC          ;  PWM full reverse mode, 2 LSbs of Duty cycle = 10
	movwf  CCP1CON       ;
	call   wait_period
  .assert "(portc & 0x3c) == 0x18, '*** FAILED p16f616 epwm half bridge LHHL'"
	nop
	btfsc   PORTC,4
	goto    $-1
  .assert "(portc & 0x3c) == 0x08, '*** FAILED p16f616 epwm full bridge LLHL'"
	nop
	return

half_mode
	movlw  0x0A          ; set dead-band
	banksel PWM1CON
	movwf  PWM1CON
	banksel CCP1CON
	movlw  0xAC          ;  PWM mode, 2 LSbs of Duty cycle = 10
	movwf  CCP1CON       ;
	movlw  0x05          ; Start Timer2 prescaler is 4
	movwf  T2CON
	btfss  PORTC,4
	goto   $-1
	btfsc  PORTC,4
	goto   $-1
    .command "cycleCount = cycles"
        nop
	btfss  PORTC,5
	goto   $-1
    .command "cycleCount = cycles - cycleCount"
	nop
    .assert "(cycleCount <= 10) && (cycleCount >=7), '*** FAILED p16f616 Half bridge dead time'"
	nop
	call   wait_period
	return

pwm_shutdown
	BANKSEL CM1CON0
	movlw   (1<<C1ON)|(1<<C1POL)|(1<<C1OE) ; Enable Comparator use C1IN+, C12IN0
	movwf   CM1CON0
	BANKSEL ECCPAS
	; C1 control and tristate mode
	movlw   (1<<ECCPAS0)|(1<<PSSAC1)|(1<<PSSBD1)
	movwf  ECCPAS
	nop
	bcf     PWM1CON,PRSEN       ; Shutdown clears on condition clear
	BANKSEL CM1CON0
	bcf     CM1CON0,C1POL   ; toggle ouput polarity
	
	call   wait_period
	call   wait_period
	return

wait_period
	banksel PIR1
	bcf    PIR1, TMR2IF
	btfss  PIR1, TMR2IF  ; loop until TMR2 sets IF flag
	goto   $-1
	clrf   TMR0
	return

; 
; test set reset inputs
sri:
	; enable comparator and SR Latch outputs
	BANKSEL CM1CON0
	movlw	(1<<C1OE)
	movwf	CM1CON0
	movwf	CM2CON0
	BANKSEL SRCON0
	; enable SR LATCH and Q, NQ outputs
	movlw	 (1<<SR1) | (1<<SR0)
	movwf	SRCON0
	bsf	SRCON0,PULSR
    .assert "((porta & 0x04) == 0x00) && ((portc & 0x10) == 0x10), '*** FAILED p16f616 SR Latch clear'"
	nop
	bsf	SRCON0,PULSS
    .assert "((porta & 0x04) == 0x04) && ((portc & 0x10) == 0x00), '*** FAILED p16f616 SR Latch pulse set'"
	nop
	bsf	SRCON0,PULSR
    .assert "((porta & 0x04) == 0x00) && ((portc & 0x10) == 0x10), '*** FAILED p16f616 SR Latch pulse reset'"
	nop
	; clock Fosc/128
	movlw	(1<<SRCS1) | (1<<SRCS0)
	movwf	SRCON1
    .command "cycleCount = cycles"
        nop
	bsf	SRCON0,SRCLKEN

    .command "cycleCount"
	nop
	BANKSEL PORTA
    ; wait for Q to be set
	btfss	PORTA,2
	goto	$-1
    .command "cycleCount = cycles - cycleCount"
        nop
    .assert "(cycleCount > 32) && (cycleCount < 38), '*** FAILED p16f616 SR Latch pulse SRSCKE'"
	nop 
	BANKSEL SRCON0
	clrf	SRCON0
	clrf	SRCON1
	return
;
; Test Comparator2 gate control of Timer 1
;
test_tot1:
	BANKSEL T1CON		
	movlw	(1<<TMR1GE)|(1<<TMR1ON)		; T1 on with gate
	movwf	T1CON
	clrf	TMR1H
	clrf	TMR1L
	BANKSEL CM1CON0
	clrf	CM1CON0		; turn off C1
	bcf	CM2CON1,T1GSS	; T1 gate source is SYNCC2OUT
	movlw 	(1<<C2ON)|(1<<C2R)	; Enable Comparator use C2VRef, C12IN0
	movwf	CM2CON0
	bsf	CM2CON0,C1POL	; toggle ouput polarity,
	BANKSEL	TMR1L
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, 'FAILED T1 running gate off'"
	nop
	bsf	T1CON,T1GINV	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W != 0, 'FAILED T1 gate invert'"
	nop
	; Test TMR1 IF set (bug 257)
	
	BANKSEL PIR1
	bcf	PIR1,TMR1IF
	BANKSEL TMR1H
	decf	TMR1H,F
	btfsc	TMR1H,7
	goto	$-1	

    .assert "(pir1 & 1) == 1, '*** FAILED P16F882 TMR1IF set'"
	nop
	
	BANKSEL CM2CON0
	bcf	CM2CON0,C1POL	; toggle comparator ouput polarity,
	BANKSEL	TMR1L
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, 'FAILED T1 running gate on'"
	nop
	return

	call	test_t0_SPM_Tog
	call	test_spm
;	call	test_cm1_Tog
	return

; test Toggle with Single Pulse Mode with gate from T0
test_t0_SPM_Tog:
  	banksel OPTION_REG
	bcf 	OPTION_REG,T0CS	; tmr0 om fosc/4
;	banksel T1GCON
;	; gate single pulse mode from t0 overflow
;	movlw	(1<<TMR1GE)|(1<<T1GSS0)|(1<<T1GSPM)|(1<<T1GGO)|(1<<T1GTM)
;	movwf	T1GCON
;	bsf 	T1CON,T1CKPS0	; T1 prescale 1:2
;	clrf	TMR1L
;	movf	TMR1L,W		; is Timer 1 running ?
;	btfss	T1GCON,T1GVAL	; wait for t1gval going high
;	GOTO	$-1
;	nop
;	btfsc	T1GCON,T1GVAL	; wait for t1gval going high
;	GOTO	$-1
   .assert "tmr1l == 0x80, '*** FAILED 16f616 Tmr1 T0 gate toggle'"
	nop
	; wait for next tmr0 overflow, tmr1 should not restart
	bcf 	INTCON,TMR0IF
	btfss	INTCON,TMR0IF
	goto	$-1
   .assert "tmr1l == 0x80, '*** FAILED 16f616 Tmr1 T0 gate toggle has stopped'"
	nop
  	banksel OPTION_REG
	bsf 	OPTION_REG,T0CS	; turn off tmr0 
	return

	; Test Single Pulse Mode using gate from CM1
test_spm
	.assert "'rewrite test'"
	nop
;	banksel T1GCON
;	; single pulse mode from CM1
;	movlw	(1<<TMR1GE)|(1<<T1GSS1)|(1<<T1GSPM)
;	movwf	T1GCON
;	bsf 	T1GCON,T1GPOL	; invert gate control
;	bsf 	T1GCON,T1GGO	; arm single pulse mode
;	movf	TMR1L,W		; is Timer 1 running ?
;	nop
;	nop
;	SUBWF	TMR1L,W
;  .assert "W == 0, '*** FAILED 16f616 T1 not running armed SPM'"
;	nop
;  .assert "(t1gcon & 0x04) == 0, '*** FAILED 16f616 T1GVAL clear armed SPM'"
;	nop
;	bcf 	T1GCON,T1GPOL	; invert gate control
;	movf	TMR1L,W		; is Timer 1 running ?
;	nop
;	nop
;	SUBWF	TMR1L,W
;  .assert "W != 0, '*** FAILED 16f616 T1 running SPM'"
;	nop
;  .assert "(t1gcon & 0x04) != 0, '*** FAILED 16f616 T1GVAL set running SPM'"
;	nop
;	bsf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, '*** FAILED 16f616 T1 stopped after SPM'"
	nop
;  .assert "(t1gcon & 0x0c) == 0, '*** FAILED 16f616 T1GVAL,T1GGO clear after running SPM'"
;	nop
;	bcf 	T1GCON,T1GPOL	; invert gate control
;	movf	TMR1L,W		; is Timer 1 running ?
;	nop
;	nop
;	SUBWF	TMR1L,W
;  .assert "W == 0, '*** FAILED 16f616 T1 not running 2nd pulse SPM'"
;	nop
;  .assert "(t1gcon & 0x04) == 0, '*** FAILED 16f616 T1GVAL clear 2nd pulse SPM'"
;	nop
;	bsf 	T1GCON,T1GPOL	; t1g_in to 0
	return

test_compar:
   .command "V1.voltage = 2.0"
	nop
	BANKSEL CM1CON0
	clrf	CM1CON0
	clrf	CM2CON0
	movlw	0xff
	movwf   CM2CON1		; test writable bits
  .assert "cm2con1 == 0x1f, '*** FAILED 16f616 cm2con1 writable bits'"
	nop
        clrf	CM2CON1
	bsf 	CM2CON0,0	; CM2 use portc1 for in-

	bcf 	CM1CON0,C1POL	; wake up CM1, bug ?
  .assert "(cm2con1 & 0xc0) == 0, '*** FAILED 16f616 cm1out, cm2out mirrors clear'"
	nop
	bsf 	CM1CON0,C1POL	; toggle ouput polarity, not ON
  .assert "cm1con0 == 0x50, '*** FAILED 16f616 cm1con0 off invert'"
	nop
  .assert "(cm2con1 & 0xc0) == 0x80, '*** FAILED 16f616 cm2con1 mirror C1OUT off invert'"
	nop

	movlw 	(1<<C1ON)	; Enable Comparator use C1IN+, C12IN0
	movwf	CM1CON0
  .assert "cm1con0  == 0xC0, '*** FAILED 16f616 cm1con0 ON=1 C1OUT=1'"
	nop
	bsf 	CM1CON0,C1POL	; toggle ouput polarity
  .assert "cm1con0  == 0x90, '*** FAILED 16f616 cm1con0 ON=1 POL=1 C1OUT=0'"
	nop
	bsf 	CM2CON0,C2POL	; toggle ouput polarity, not ON
  .assert "cm2con0 == 0x51, '*** FAILED 16f616 cm2con0 ON=0 POL=1'"
	nop
  .assert "(cm2con1 & 0xc0) == 0x40, '*** FAILED 16f616 cmout  C2OUT ON=0 POL=1'"
	nop
	bsf 	CM1CON0,C1OE	; C1OUT to RA2
   .assert "(porta & 0x04) == 0x00, '*** FAILED 16f616 compare C1OUT(RA2)  low POL=1'"
	nop 
	bcf 	CM1CON0,C1POL	; toggle ouput polarity
   .assert "(porta & 0x04) == 0x04, '*** FAILED 16f616 compare C1OUT(RA2) high POL=0'"
	nop
	bsf 	CM2CON0,C2OE	; C2OUT to RC4
   .assert "(portc & 0x10) == 0x10, '*** FAILED 16f616 compare C2OUT(RC4) ON=0 POL=1'"
	nop 
	bsf	CM2CON0,C2ON
   .assert "(portc & 0x10) == 0x00, '*** FAILED 16f616 compare C2OUT(RC4) ON=1 POL=1'"
	nop 
	nop
	clrf   int_cm2
	; Test change in voltage detected C2IN+ 4.5 -> 2.0
   .command "V3.voltage = 2.0"
	nop
	nop
   .assert "int_cm2 == 1, '*** FAILED 16f616 cm2 +edge interrupt'"
	nop
	clrf	int_cm1
	bsf 	CM1CON0,C1POL   ; change state out 1 -> 0
   .assert "int_cm1 == 0, '*** FAILED 16f616 cm1 -edge unexpected interrupt'"
	nop
	clrf	int_cm1		
	bcf 	CM1CON0,C1POL
   .assert "int_cm1 == 1, '*** FAILED 16f616 cm1 +edge interrupt'"
	nop
	clrf	int_cm2
	bcf 	CM2CON0,C2POL   ; change state
   .assert "int_cm2 == 0, '*** FAILED 16f616 cm2 -edge unexpected interrupt'"
	nop
	movlw	(1<<FVREN) | (1<<C1VREN)  ;CV1REF = Vdd/4 = 1.25 Volts
	movwf	VRCON	
	; wait for Vref to settle
	nop
	nop
	bsf	CM1CON0,C1R
	bsf	CM2CON0,C2R
    .assert "(cm2con1 & 0xc0) == 0, '*** FAILED p16f161 vref=0.6V'"
	nop
	bsf	CM2CON1,C2HYS
	movlw	0x9f
	movwf	VRCON
    .assert "(cm2con1 & 0xc0) == 0x80, '*** FAILED p16f161 C1vref= 3.59, C2vref=0.6'"
	nop
	return



interrupt:

;	movlb	0		;select bank 0

	btfsc   PIR1,C1IF
	  goto  int_com1
	btfsc   PIR1,C2IF
	  goto  int_com2

	btfsc   PIR1,ADIF
            goto adc_int

   .assert "'*** FAILED 16F616 unexpected interrupt'"
	nop
back_interrupt:
	retfie

;;	An A/D interrupt has occurred
adc_int
        incf    adc_cnt,F
        bcf     PIR1,ADIF
        goto    back_interrupt


;;	Compatator 1 interrupt
int_com1:
	bsf 	int_cm1,0
	bcf 	PIR1,C1IF
	goto	back_interrupt


;;	Compatator 2 interrupt
int_com2:
	bsf 	int_cm2,0
	bcf 	PIR1,C2IF
	goto	back_interrupt



test_adc:

  ; ADCS = 110 Fosc/64
  ; ANS = 1 AN0 is only analog port
	BANKSEL ANSEL
	movlw	(1 << ANS0)
	movwf	ANSEL
	movlw	(1 << ADCS2) | (1 << ADCS1) 
	movwf	ADCON1
	movlw	0x3f
	movwf	TRISA		; All pins input
	bsf     PIE1,ADIE
	bcf	WPUA,5		; turn off input pullup on porta5
	bcf	WPUA,4		; turn off input pullup on porta4
	BANKSEL ADCON0
	bsf	ADCON0,ADON	; enable ADC

        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts
        call    Convert
   .assert "W == 0x7f, '*** FAILED 16f616 ADC test - AN0 = 2.5 Vref = Vdd ADFM=0'"
	nop

	BANKSEL VRCON
	movlw	(1<<C1VREN)
	movwf	VRCON
	movlw	(1<<ADON) | (0xc<<2)	; Input Vref
	movwf	ADCON0
	call	Convert
   .assert "W == 0x40, '*** FAILED 16f616 ADC test - CVref = 1.25v Vref = Vdd(5v) ADFM=0'"
	nop
	movlw	(1<<ADON) | (0xe<<2)	; Input 1.2V fixed voltage reference
	movwf	ADCON0
	call	Convert
   .assert "W == 0x3d, '*** FAILED 16f616 ADC test - IN  = 1.2v Vref = Vdd(5v) ADFM=0'"
	nop

	BANKSEL ANSEL
	bsf	ANSEL,ANS1	; AN1 is analog
	BANKSEL ADCON0
	movlw	(1<<ADON) | (0<<2)	; Input chan 0 (AN0)
	movwf	ADCON0
	bsf	ADCON0,VCFG	; AN1 as hi Vref 
        call    Convert
   .assert "W == 0x9f, '*** FAILED 16f616 ADC test - AN0 = 2.5V Vref = AN1 = 4.V ADFM=0'"
	nop
	bsf	ADCON0,ADFM	; AN1 as hi Vref , ADFM=1
        call    Convert
   .assert "W == 0x02, '*** FAILED 16f616 ADC test - AN0 = 2 Vref = AN1 = 4 ADFM=0'"
	nop
    .command "echo expect channel 2  not a configured input"
	nop
	bsf	ADCON0,CHS1	; select shannel 2 which is not ADC analog pin
	call	Convert

	BANKSEL ANSEL
	clrf	ANSEL		; return ports to digital I/O
	BANKSEL ADCON0

	return

Convert:

        clrf    adc_cnt              ;flag set by the interrupt routine

        bsf     ADCON0,GO       ;Start the A/D conversion

        btfss   adc_cnt,0            ;Wait for the interrupt to set the flag
         goto   $-1

        movf    ADRESH,W                ;Read the high 8-bits of the result

        return



	end
