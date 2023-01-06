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


	list	p=18f26k22
        include <p18f26k22.inc>
        include <coff.inc>


        CONFIG WDTEN=ON, WDTPS=4
	CONFIG MCLRE = INTMCLR 
	CONFIG FOSC = INTIO67

	;; The purpose of this program is to test gpsim's ability to 
	;; simulate a pic 18F26k22
	;; Specifically, the comparator and gate function of Tmr1 is tested.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR 0

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
        bra   start                     ; go to beginning of program

	;; 
	;; Interrupt
	;; 
INTERRUPT_VECTOR CODE 0X008

	goto	interrupt

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

   .sim "p18f26k22.xpos = 72"
   .sim "p18f26k22.ypos = 72"

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
   .sim "V3.xpos = 84"
   .sim "V3.ypos = 288"
   .sim "V3.voltage = 4.5"

   .sim "module load pullup V4"	
   .sim "V4.resistance = 100.0"
   .sim "V4.xpos = 240"
   .sim "V4.ypos = 120"
   .sim "V4.voltage = 3.5"

   .sim "module load pullup V5"	
   .sim "V5.resistance = 100.0"
   .sim "V5.xpos = 240"
   .sim "V5.ypos = 160"
   .sim "V5.voltage = 3.5"

   .sim "node na0"
   .sim "attach na0 V3.pin porta3" 	; C1IN+
   .sim "node na1"
   .sim "attach na1 V1.pin porta1"	; C12IN1-
   .sim "node na3"
   .sim "attach na3 V2.pin porta0"	; C12IN0-
   .sim "node na4"
   .sim "attach na4 V4.pin portb3"	; C12IN2-
   .sim "node na5"
   .sim "attach na5 V5.pin portc0"	; T3G


	BANKSEL ANSELA
	movlw	0x0f
	movwf	ANSELA		; select AN0, AN1, AN2, AN3
	clrf	ANSELC  	; 
        movwf   TRISA
 	movlw	0x03
	movwf	TRISC		; portc0 (AN4) portc1(AN5) input, portc4 output

	movlw	0xff
	movwf 	CTMUICON
        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts
        bsf     PIE2,C1IE       ;CM1 interrupts
        bsf     PIE2,C2IE       ;CM2 interrupts
	call	t3_port	
	call    test_compar
	call	test_tot1	; can C2 increment TMR1
        call	test_sr
	call	test_dac
  .assert  "'*** PASSED 18F26k22 comparator test'"
	nop
	goto	$-1

test_dac:
	bsf	ADCON2,ADFM	;right justify
;	movlw   0xf0 		;right justify, Frc, Vdd ref
;	movwf   ADCON1		
	bsf     TRISC,0		;Set RC0 to input
	bsf     ANSELC,0	;Set RC0 to analog
	movlw   (0x1e<<2)|(1<<ADON)	;Select DAC output and ADC on
	movwf   ADCON0      	
	; DAC output should be Vsource- (0)
	call	a2dConvert
   .assert "(adresh==0x00) && (adresl==0x00), '*** FAILED 16f1823 DAC default'"
	nop
	; test DAC High idle state
	movlw	(1<<DACLPS)	; high reference
	movwf	VREFCON1
	movlw	0x1f		; top of ladder
	movwf	VREFCON2
	call	a2dConvert
   .assert "(adresh==0x03) && (adresl==0xff), '*** FAILED 16f1823 DAC High power off'"
	nop
	; test DAC enabled Vout = 5V * (16/32) = 2.5
	movlw	0x10
	movwf	VREFCON2
	movlw	(1<<DACEN)
	movwf	VREFCON1
	call	a2dConvert
   .assert "(adresh==0x02) && (adresl==0x00), '*** FAILED 16f1823 DAC enabled 1/2'"
	nop
  	; Test FVR = 4.065 as Vsource+ = 2.037
	movlw	(1<<FVREN)|(1<<FVRS1)|(1<<FVRS0)
	movwf	VREFCON0
	movlw	(1<<DACEN)|(1<<DACPSS1)
	movwf	VREFCON1
	call	a2dConvert
   .assert "(adresh==0x01) && (adresl==0xa3), '*** FAILED 16f1823 DAC enabled 1/2 FVR '"
	nop
	return
;
;       Start A2D conversion and wait for results
;
a2dConvert
        bsf     ADCON0,GO
        btfsc   ADCON0,GO
        goto    $-1
        movf    ADRESH,W
        return


;
; Test Comparator driving SR latch
; Assumes called with C1OUT = C2OUT = 0 and
;    C1POL = 0, C2POL = 1
;
test_sr:

	; turn on Q output pin
	movlw	(1<<SRLEN)|(1<<SRQEN)
	movwf	SRCON0
	; CM1 set, CM2 reset
	movlw	(1<<SRSC1E)|(1<<SRRC2E)
	movwf	SRCON1
	bsf	CM1CON0,C1ON	; turn on CM1
	; toggle CM1
	bsf 	CM1CON0,C1POL
    .assert "(porta&0x10) == 0x10, '*** FAILED 16f1823 CM1 set SR latch'"
	nop
	bcf 	CM1CON0,C1POL
	; toggle CM2
	bcf 	CM2CON0,C2POL
    .assert "(porta&0x10) == 0x00, '*** FAILED 16f1823 CM2 reset SR latch'"
	nop
	bsf 	CM2CON0,C2POL
	return
;
;	Test T3 port gate control
;
t3_port:
	bsf	T3CON,TMR3ON	; turn on T3
	movf	TMR3L,W		; is Timer 3 running ?
	nop
	nop
	subwf	TMR3L,W
  .assert "W != 0, '*** FAILED 18f26k22 T3 running with no gate'"
	nop
	movlw	(1<<TMR3GE)
	movwf	T3GCON
	movf	TMR3L,W		; is Timer 3 running ?
	nop
	nop
	subwf	TMR3L,W
  .assert "W == 0, '*** FAILED 18f26k22 T3 ON=1 POL=0 PIN=1'"
	nop
	bsf	T3GCON,T3GPOL	; set POL to 1
	movf	TMR3L,W		; is Timer 3 running ?
	nop
	nop
	subwf	TMR3L,W
  .assert "W != 0, '*** FAILED 18f26k22 T3 ON=1 POL=1 PIN=1'"
	nop
   .command "V5.voltage = 0.0"
	nop
	movf	TMR3L,W		; is Timer 3 running ?
	nop
	nop
	subwf	TMR3L,W
  .assert "W == 0, '*** FAILED 18f26k22 T3 ON=1 POL=1 PIN=0'"
	nop

	return
;
; Test Comparator2 gate control of Timer 1
;
test_tot1:
	movlw	(1<<FVREN)|(1<<FVRS0)|(1<<FVRS1) ; Vref 4.096
	movwf	FVRCON
	btfss	VREFCON0,FVRST ; wait for Vref ready
	goto	$-1
;	bsf 	T1GCON,TMR1GE
	movlw	(1<<TMR1ON)		; T1 on 
	movwf	T1CON
	clrf	TMR1H
	clrf	TMR1L
	movlw	(1<<TMR1GE)|(1<<T1GSS0)|(1<<T1GSS1) ; T1 gate source is SYNCC2OUT
	movwf	T1GCON
	clrf	CM1CON0		; turn off C1
	movlw	1<<C2R		; use FVR, C2IN0-
	movwf	CM2CON0
	movlw	(1<<C2RSEL)	; FVR
        movwf	CM2CON1
	bsf 	CM2CON0,C2ON	; Enable Comparator 2
;	bsf 	CM2CON0,C2POL	; toggle ouput polarity,
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, '*** FAILED 18f26k22 T1 running CM2 gate off'"
	nop
	bsf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W != 0, '*** FAILED 18f26k22 T1 CM2 gate invert'"
	nop
	bsf 	CM2CON0,C2POL	; toggle comparator ouput polarity,
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, '*** FAILED 18f26k22 T1 running gate on'"
	nop
	call	test_t0_SPM_Tog
	call	test_spm
	call	test_cm1_Tog
	return

; Test Toggle gate mode with CM1
test_cm1_Tog:
	movlw 	(1<<C1ON)	; Enable Comparator use C1IN+, C12IN0
	movwf	CM1CON0
	movlw	(1<<TMR1GE)|(1<<T1GSS1)|(1<<T1GTM)
	movwf	T1GCON
	movlw 	(1<<C1ON)	; Enable Comparator use C1IN+, C12IN0
	;movwf	CM1CON0
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
	bsf 	T1GCON,T1GPOL	; set gate to 0
	bcf 	PIR1,TMR1GIF
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, '*** FAILED 18f26k22 T1 gate toggle set, t1 stopped'"
	nop
	bcf 	T1GCON,T1GPOL	; low-high gate transition
        nop
  .assert "(t1gcon & 0x04) != 0, '*** FAILED 18f26k22 T1 gate toggle set, T1GVAL set'"
	nop
	bsf 	T1GCON,T1GPOL	; high low gate transition
	; T1 should still be running
  .assert "(t1gcon & 0x04) != 0, '*** FAILED 18f26k22 T1 gate toggle set, T1GVAL set - edge'"
	nop
	bcf 	T1GCON,T1GPOL	; low-high gate transition
	; T1 should be stopped
  .assert "(t1gcon & 0x04) == 0, '*** FAILED 18f26k22 T1 gate toggle set, T1GVAL clear 2nd +edge'"
	nop
	bsf 	T1GCON,T1GPOL	; high low gate transition
	bcf 	T1GCON,T1GPOL	; low-high gate transition
  .assert "(t1gcon & 0x04) != 0, '*** FAILED 18f26k22 T1 gate toggle set, T1GVAL set 3rd + edge'"
	nop

	return


; test Toggle with Single Pulse Mode with gate from T6 = PR6
test_t0_SPM_Tog:
	bcf	T0CON,TMR0ON	; Turn off t0
	movlw	0x80
	movwf	PR6
	movlw   (1<<TMR6ON)
	movwf	T6CON
	; gate single pulse mode from t0 overflow
	;RRR movlw	(1<<TMR1GE)|(1<<T1GSS0)|(1<<T1GSPM)|(1<<T1GGO)|(1<<T1GTM)
	;T5GCON enable single pulse from T6
	movlw   (1<<T5CKPS0)|(1<<TMR5ON)	; T5 ON prescale 1:2
	movwf	T5CON
  .command("echo DEBUG turn on T5")
	nop
	movlw	(1<<TMR5GE)|(1<<T5GSPM)|(1<<T5GSS0)|(1<<T1GTM)
	movwf	T5GCON
	bsf	T5GCON,T5GGO
  .command(" echo DEBUG set T5GGO")
	nop
	clrf	TMR5L
	movf	TMR5L,W		; is Timer 5 running ?
	nop
	nop
	subwf	TMR5L,W
  .assert "W == 0, '*** FAILED 18f26k22 T5 waiting for T6 toggle'"
	nop
	nop
	btfss	T5GCON,T5GVAL	; wait for t5gval going high
	GOTO	$-1
  .command("echo DEBUG T5GCON,T5GVAL high")
	nop
	nop
	btfsc	T5GCON,T5GVAL	; wait for t5gval going low
	GOTO	$-1
   .assert "tmr5l == 0x40, '*** FAILED 18f26k22 Tmr5 T6 gate toggle'"
	nop
	clrf	TMR5L
	; wait for next tmr6 overflow, tmr5 should not restart
	bcf 	PIR5,TMR6IF
	btfss	PIR5,TMR6IF
	goto	$-1
   .assert "tmr5l == 0x00, '*** FAILED 18f26k22 Tmr5 T6 gate toggle has stopped'"
	nop
	clrf	T6CON
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
  .assert "W == 0, '*** FAILED 18f26k22 T1 not running armed SPM'"
	nop
  .assert "(t1gcon & 0x04) == 0, '*** FAILED 18f26k22 T1GVAL clear armed SPM'"
	nop
	bcf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W != 0, '*** FAILED 18f26k22 T1 running SPM'"
	nop
  .assert "(t1gcon & 0x04) != 0, '*** FAILED 18f26k22 T1GVAL set running SPM'"
	nop
	bsf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, '*** FAILED 18f26k22 T1 stopped after SPM'"
	nop
  .assert "(t1gcon & 0x0c) == 0, '*** FAILED 18f26k22 T1GVAL,T1GGO clear after running SPM'"
	nop
	bcf 	T1GCON,T1GPOL	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, '*** FAILED 18f26k22 T1 not running 2nd pulse SPM'"
	nop
  .assert "(t1gcon & 0x04) == 0, '*** FAILED 18f26k22 T1GVAL clear 2nd pulse SPM'"
	nop
	bsf 	T1GCON,T1GPOL	; t1g_in to 0
	return

test_compar:
	movlw	0xff
	movwf   CM2CON1		; test writable bits
  .assert "cm2con1 == 0x3f, '*** FAILED 18f26k22 cm2con1 writable bits'"
	nop
        clrf	CM2CON1

	bcf 	CM1CON0,C1POL	; wake up CM1, bug ?
  .assert "cm1con0 == 0x08, '*** FAILED 18f26k22 cm1con0 off non-invert'"
	nop
	bsf 	CM1CON0,C1POL	; set ouput polarity, not ON
  .assert "cm1con0 == 0x58, '*** FAILED 18f26k22 cm1con0 off invert'"
	nop

	movlw 	(1<<C1ON)	; Enable Comparator use C1IN+, C12IN0
	movwf	CM1CON0
  .assert "cm1con0  == 0xC0, '*** FAILED 18f26k22 cm1con0 ON=1 C1OUT=1'"
	nop
	bsf 	CM1CON0,C1POL	; set ouput polarity
  .assert "cm1con0  == 0x90, '*** FAILED 18f26k22 cm1con0 ON=1 POL=1 C1OUT=0'"
	nop
	bsf 	CM2CON0,C2POL	; set ouput polarity, not ON
  .assert "cm2con0 == 0x58, '*** FAILED 18f26k22 cm2con0 ON=0 POL=1'"
	nop
	bsf 	CM1CON0,C1OE	; C1OUT to RA4
   .assert "(porta & 0x10) == 0x00, '*** FAILED 18f26k22 compare RA4  low'"
	nop 
	bcf 	CM1CON0,C1POL	; clear ouput polarity
   .assert "(porta & 0x10) == 0x10, '*** FAILED 18f26k22 compare RA4 high'"
	nop
	bsf 	CM2CON0,C2OE	; C2OUT to RA5
   .assert "(porta & 0x20) == 0x20, '*** FAILED 18f26k22 compare RA5 ON=0'"
	nop 
	; Test change in voltage detected
   .command "V3.voltage = 2.0"
	nop
   .assert "(porta & 0x30) == 0x20, '*** FAILED 18f26k22 compare RA4 not low volt change'"
	nop
	bsf 	CM1CON0,C1POL   ; change state
   .assert "int_cm1 == 1, '*** FAILED 18f26k22 cm1 +edge interrupt'"
	nop
	clrf	int_cm1		
	bcf 	CM1CON0,C1POL
   .assert "int_cm1 == 0, '*** FAILED 18f26k22 cm1 unexpected +edge interrupt'"
	nop
	bsf	CM2CON0,C2ON
	clrf	int_cm2
	bcf 	CM2CON0,C2POL   ; change state
   .assert "int_cm2 == 0, '*** FAILED 18f26k22 cm2 -edge no-interrupt'"
	nop
	clrf	int_cm2
	bsf 	CM2CON0,C2POL   ; change state
   .assert "int_cm2 == 1, '*** FAILED 18f26k22 cm2 expected +edge interrupt'"
	nop
	movlw	(1<<FVREN) | (1<<FVRS1)  ; turn on Vref for Comp/A2D to 2.048 v
	movwf	VREFCON0
	btfss	VREFCON0,FVRST ; wait for Vref ready
	goto	$-1
	nop
	bsf	CM2CON1,C2RSEL  ; use FVR rather than DAC
        movlw   (1<<C2ON)|(1<<C2R)|(1<<C2CH1) ; FVR, C12IN2- (AN9, RB3)
	movwf	CM2CON0

;   .assert "int_cm4 == 1, '*** FAILED 18f26k22 cm4 -edge interrupt'"
;	nop
	return



interrupt:

	movlb   0	; BSR=0

	btfsc   PIR2,C1IF
	  goto  int_com1
	btfsc   PIR2,C2IF
	  goto  int_com2

	btfsc	INTCON,ADIE
	 btfsc	PIR1,ADIF
	  goto	check
   .assert "'*** FAILED 18F26K22 unexpected interrupt'"
	nop
back_interrupt:
	retfie 1

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
