;
; 
; Copyright (c) 2015 Roy Rankin
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


	list	p=16f1823
        include <p16f1823.inc>
        include <coff.inc>

        __CONFIG  _CONFIG1, _CP_OFF & _WDTE_OFF &  _FOSC_INTOSC &  _LVP_OFF &  _MCLRE_OFF
        __CONFIG _CONFIG2, _PLLEN_OFF

	;; The purpose of this program is to test gpsim's ability to 
	;; simulate a pic 16F1823.
	;; Specifically, the comparator and gate function of Tmr1 is tested.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR

int_cm1 RES 1
int_cm2 RES 1
x  RES  1
t1 RES  1
t2 RES  1
avg_lo RES  1
avg_hi RES  1
w_temp RES  1
status_temp RES  1

  GLOBAL int_cm1, int_cm2
;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlp  high  start               ; load upper byte of 'start' label
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

   .sim "p16f1823.xpos = 72"
   .sim "p16f1823.ypos = 72"

   .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
   .sim "module load pullup V1"
   .sim "V1.resistance = 100.0"
   .sim "V1.xpos = 240"
   .sim "V1.ypos = 72"
   .sim "V1.voltage = 2.0"

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

   .sim "node na0"
   .sim "attach na0 V3.pin porta0 portc0" ; C1IN+, C2IN+
   .sim "node na1"
   .sim "attach na1 V1.pin portc1"	; C12IN1-
   .sim "node na3"
   .sim "attach na3 V2.pin porta1"	; C12IN0-


                                                                                

        movlw   0x03
	BANKSEL ANSELA
	clrf	ANSELA
	clrf	ANSELC
	movwf	ANSELA		; select AN0, AN1
	movwf	ANSELC  	; select AN4, AN5
	BANKSEL TRISA
        movwf   TRISA
 ;	movlw	0x02
	movwf	TRISC		; portc0 (AN4) portc1(AN5) input, portc4 output

        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts
	BANKSEL PIE2
        bsf     PIE2,C1IE       ;CM1 interrupts
        bsf     PIE2,C2IE       ;CM2 interrupts
	call    test_compar
	call	test_tot1	; can C2 increment TMR1
	call	test_sr		; can CM control SR latch
  .assert  "'*** PASSED 16F1823 comparator test'"
	nop
	goto	$-1

;
; Test Comparator driving SR latch
; Assumes called with C1OUT = C2OUT = 0 and
;    C1POL = 0, C2POL = 1
;
test_sr:

	BANKSEL	SRCON0
	; turn on Q output pin
	movlw	(1<<SRLEN)|(1<<SRQEN)
	movwf	SRCON0
	; CM1 set, CM2 reset
	movlw	(1<<SRSC1E)|(1<<SRRC2E)
	movwf	SRCON1
	BANKSEL	CM1CON0
	bsf	CM1CON0,C1ON	; turn on CM1
	; toggle CM1
	bsf 	CM1CON0,C1POL
    .assert "(porta&0x04) == 0x04, '*** FAILED 16f1823 CM1 set SR latch'"
	nop
	bcf 	CM1CON0,C1POL
	; toggle CM2
	bcf 	CM2CON0,C2POL
    .assert "(porta&0x04) == 0x00, '*** FAILED 16f1823 CM2 reset SR latch'"
	nop
	bsf 	CM2CON0,C2POL
	return
;
; Test Comparator2 gate control of Timer 1
;
test_tot1:
	BANKSEL FVRCON
	movlw	0x8c
	movwf	FVRCON
	BANKSEL T1CON		
;	bsf 	T1GCON,TMR1GE
	movlw	(1<<TMR1ON)		; T1 on 
	movwf	T1CON
	clrf	TMR1H
	clrf	TMR1L
	movlw	(1<<TMR1GE)|(1<<T1GSS0)|(1<<T1GSS1) ; T1 gate source is SYNCC2OUT
	movwf	T1GCON
	BANKSEL CM1CON0
	clrf	CM1CON0		; turn off C1
	movlw	(1<<C2PCH1)	; use FVR, C12IN0
	movwf	CM2CON1 
	bsf 	CM2CON0,C2ON	; Enable Comparator 2
	bcf 	CM2CON0,C2POL	; toggle ouput polarity,
	BANKSEL	TMR1L
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, 'FAILED 16f1823 T1 running CM2 gate off'"
	nop
	bsf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W != 0, 'FAILED 16f1823 T1 CM2 gate invert'"
	nop
	BANKSEL CM2CON0
	bsf 	CM2CON0,C2POL	; toggle comparator ouput polarity,
	BANKSEL	TMR1L
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, 'FAILED 16f1823 T1 running gate on'"
	nop
	call	test_t0_SPM_Tog
	call	test_spm
	call	test_cm1_Tog
	return

; Test Toggle gate mode with CM1
test_cm1_Tog:
	banksel	T1GCON
	movlw	(1<<TMR1GE)|(1<<T1GSS1)|(1<<T1GTM)
	movwf	T1GCON
	bsf 	T1GCON,T1GPOL	; set gate to 0
    .assert "(pir1 & 0x80) == 0x80, '*** FAILED 16f1823 TMR1GIF set'"
	nop
	bcf 	PIR1,TMR1GIF
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, 'FAILED 16f1823 T1 gate toggle set, t1 stopped'"
	nop
	bcf 	T1GCON,T1GPOL	; low-high gate transition
        nop
  .assert "(t1gcon & 0x04) != 0, 'FAILED 16f1823 T1 gate toggle set, T1GVAL set'"
	nop
	bsf 	T1GCON,T1GPOL	; high low gate transition
	; T1 should still be running
  .assert "(t1gcon & 0x04) != 0, 'FAILED 16f1823 T1 gate toggle set, T1GVAL set - edge'"
	nop
	bcf 	T1GCON,T1GPOL	; low-high gate transition
	; T1 should be stopped
  .assert "(t1gcon & 0x04) == 0, 'FAILED 16f1823 T1 gate toggle set, T1GVAL clear 2nd +edge'"
	nop
	bsf 	T1GCON,T1GPOL	; high low gate transition
	bcf 	T1GCON,T1GPOL	; low-high gate transition
  .assert "(t1gcon & 0x04) != 0, 'FAILED 16f1823 T1 gate toggle set, T1GVAL set 3rd + edge'"
	nop

	return


; test Toggle with Single Pulse Mode with gate from T0
test_t0_SPM_Tog:
  	banksel OPTION_REG
	bcf 	OPTION_REG,TMR0CS	; tmr0 om fosc/4
	banksel T1GCON
	; gate single pulse mode from t0 overflow
	movlw	(1<<TMR1GE)|(1<<T1GSS0)|(1<<T1GSPM)|(1<<T1GGO)|(1<<T1GTM)
	movwf	T1GCON
	bsf 	T1CON,T1CKPS0	; T1 prescale 1:2
	clrf	TMR1L
	movf	TMR1L,W		; is Timer 1 running ?
	btfss	T1GCON,T1GVAL	; wait for t1gval going high
	GOTO	$-1
	nop
	btfsc	T1GCON,T1GVAL	; wait for t1gval going high
	GOTO	$-1
   .assert "tmr1l == 0x80, '*** FAILED 16f1823 Tmr1 T0 gate toggle'"
	nop
	; wait for next tmr0 overflow, tmr1 should not restart
	bcf 	INTCON,TMR0IF
	btfss	INTCON,TMR0IF
	goto	$-1
   .assert "tmr1l == 0x80, '*** FAILED 16f1823 Tmr1 T0 gate toggle has stopped'"
	nop
  	banksel OPTION_REG
	bsf 	OPTION_REG,TMR0CS	; turn off tmr0 
	return

	; Test Single Pulse Mode using gate from CM1
test_spm
	banksel T1GCON
	; single pulse mode from CM1
	movlw	(1<<TMR1GE)|(1<<T1GSS1)|(1<<T1GSPM)
	movwf	T1GCON
	bsf 	T1GCON,T1GPOL	; invert gate control
	bsf 	T1GCON,T1GGO	; arm single pulse mode
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, 'FAILED 16f1823 T1 not running armed SPM'"
	nop
  .assert "(t1gcon & 0x04) == 0, 'FAILED 16f1823 T1GVAL clear armed SPM'"
	nop
	bcf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W != 0, 'FAILED 16f1823 T1 running SPM'"
	nop
  .assert "(t1gcon & 0x04) != 0, 'FAILED 16f1823 T1GVAL set running SPM'"
	nop
	bsf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, 'FAILED 16f1823 T1 stopped after SPM'"
	nop
  .assert "(t1gcon & 0x0c) == 0, 'FAILED 16f1823 T1GVAL,T1GGO clear after running SPM'"
	nop
	bcf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, 'FAILED 16f1823 T1 not running 2nd pulse SPM'"
	nop
  .assert "(t1gcon & 0x04) == 0, 'FAILED 16f1823 T1GVAL clear 2nd pulse SPM'"
	nop
	bsf 	T1GCON,T1GPOL	; t1g_in to 0
	return

test_compar:
	BANKSEL CM1CON0
  .assert "cm1con0 == 0x04, 'FAILED 16f1823 cm1con0 unexpected init value'"
	nop
  .assert "cm2con0 == 0x04, 'FAILED 16f1823 cm2con0 unexpected init value'"
	nop
  .assert "cm1con1 == 0x00, 'FAILED 16f1823 cm1con1 unexpected init value'"
	nop
  .assert "cm2con1 == 0x00, 'FAILED 16f1823 cm2con1 unexpected init value'"
	nop
	movlw	0xff
	movwf   CM2CON1		; test writable bits
  .assert "cm2con1 == 0xf3, 'FAILED 16f1823 cm2con1 writable bits'"
	nop
	movlw	(1<<C2NCH0)	; CM2 use CN12N1- (portc1), C2N+, no intterupts
	movwf	CM2CON1
  .assert "cmout == 0, 'FAILED 16f1823 cmout default 0'"
	nop
        call 	off_pol
	movlw 	(1<<C1ON)	; Enable Comparator use C1IN+, C12IN0
	movwf	CM1CON0
  .assert "cm1con0  == 0xC0, 'FAILED 16f1823 cm1con0 ON=1 C1OUT=1'"
	nop
	bsf 	CM1CON0,C1POL	; toggle ouput polarity
  .assert "cm1con0  == 0x90, 'FAILED 16f1823 cm1con0 ON=1 POL=1 C1OUT=0'"
	nop
	bsf 	CM1CON0,C1OE	; C1OUT to RA2
   .assert "(porta & 0x04) == 0x00, 'FAILED 16f1823 CM1 C1OUT not low'"
	nop 
	bcf 	CM1CON0,C1POL	; toggle ouput polarity
   .assert "(porta & 0x04) == 0x04, 'FAILED 16f1823 CM1 C1OUT(RA2) not high'"
	nop
	; turn on CM2
	movlw 	(1<<C2ON)|(1<<C2OE)	; Enable Comparator use C2IN+, C12IN0
	movwf	CM2CON0
   .assert "(portc & 0x10) == 0x10, 'FAILED 16f1823 CM1 C2OUT(RC4) high'"
	nop
	bsf	CM2CON0,C2POL
   .assert "(portc & 0x10) == 0x00, 'FAILED 16f1823 CM1 C2OUT(RC4) low'"
	nop
	; Test change in voltage detected
   .command "V3.voltage = 2.0"
	nop
   .assert "(porta & 0x04) == 0x00, 'FAILED 16f1823 compare RA2 not low volt change'"
	nop
   .assert "(portc & 0x10) == 0x10, 'FAILED 16f1823 compare RC4 not high volt change'"
	nop
	bsf 	CM1CON1,C1INTP	; interrupt on positive edge
	bsf 	CM1CON0,C1POL   ; change state
   .assert "int_cm1 == 1, 'FAILED 16f1823 cm1 +edge interrupt'"
	nop
	clrf	int_cm1		
	bcf 	CM1CON0,C1POL
   .assert "int_cm1 == 0, 'FAILED 16f1823 cm1 unexpected +edge interrupt'"
	nop
	bsf 	CM2CON1,C2INTN  ; interrupt cm2 on negative edge
	bcf 	CM2CON0,C2POL   ; change state
   .assert "int_cm2 == 1, 'FAILED 16f1823 cm2 -edge interrupt'"
	nop
	clrf	int_cm2
	bsf 	CM2CON0,C2POL   ; change state
   .assert "int_cm2 == 0, 'FAILED 16f1823 cm2 unexpected -edge interrupt'"
	nop
	nop
;RRR	bsf 	CM1CON0,C1R	; C1IN+ is voltage reference
	nop
;RRR	bsf 	CM2CON1,C1RSEL  ; Vref from Comparator Reference
	nop
;RRR	movf	CM1CON0,W	; FIXME should not be needed
	nop
	return

off_pol:
	return

dummy:
	bsf	CM1CON0,C1POL
  .assert "cmout == 1, 'FAILED 16f1823 cmout CM1 off C1POL=1'"
	nop
  .assert "cm1con0 == 0x54, 'FAILED 16f1823 cm1con0 CM1 off C1POL=1'"
	nop
	bsf	CM2CON0,C2POL
  .assert "cmout == 3, 'FAILED 16f1823 cmout CM2 off C2POL=1'"
	nop
  .assert "cm2con0 == 0x54, 'FAILED 16f1823 cm2con0 CM2 off C2POL=1'"
	nop

	bsf 	CM2CON0,C2OE	; C2OUT to RC4
   .assert "(portc & 0x10) == 0x10, 'FAILED 16f1823 CM2 C2OUT not high ON=0, POL=1'"
	nop 
	return


interrupt:

	movlb	0		;select bank 0

	btfsc   PIR2,C1IF
	  goto  int_com1
	btfsc   PIR2,C2IF
	  goto  int_com2

	btfsc	INTCON,ADIE
	 btfsc	PIR1,ADIF
	  goto	check
   .assert "'FAILED 16F1823 unexpected interrupt'"
	nop
back_interrupt:
	retfie

;;	An A/D interrupt has occurred
check:
	bsf 	t1,0		;Set a flag to indicate we got the int.
	bcf 	PIR1,ADIF	;Clear the a/d interrupt
	goto	back_interrupt

;;	Compatator 1 interrupt
int_com1:
	bsf 	int_cm1,0
	bcf 	PIR2,C1IF
	goto	back_interrupt


;;	Compatator 2 interrupt
int_com2:
	bsf 	int_cm2,0
	bcf 	PIR2,C2IF
	goto	back_interrupt





	end
