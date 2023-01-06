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


        list    p=18f26k22
        include <p18f26k22.inc>
	include <coff.inc>

        CONFIG WDTEN=ON
	CONFIG WDTPS=128
        CONFIG MCLRE = INTMCLR
        CONFIG FOSC = XT
        CONFIG IESO = ON
        errorlevel -302

; Printf Command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR 0

delay1	RES	1
delay2  RES	1

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        bra   start                     ; go to beginning of program

        ;; 
        ;; Interrupt
        ;; 
INTERRUPT_VECTOR CODE 0X008

        goto    interrupt

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

  .sim "p18f26k22.xpos = 84"
  .sim "p18f26k22.ypos = 24"
  .sim ".frequency = 8000000."


	call	test_osccon

  .assert  "'*** PASSED 18F26k22 OSCCON test'"
        nop
        goto    $-1

test_osccon:

	BANKSEL OSCCON	;select bank 15

	btfss	OSCCON,OSTS
	goto	$-1
    .assert "cycles == 1028, '***FAILED p18f26k22 2 speed clock to XT'"
	nop
   .assert "osccon == 0x38, '***FAILED p18f26k22  osccon after 2 speed clock'"
	nop
   .assert "osccon2 == 0x04, '***FAILED p18f26k22 osccons after 2 speed clock'"
	nop
   .assert "p18f26k22.frequency==8000000., '***FAILED p18f26k22 XT freq'"
	nop
	bsf	OSCCON,SCS1	; switch to intrc

   .assert "p18f26k22.frequency==1000000., '***FAILED p18f26k22 def RC f=1e6'"
	nop
   .assert "osccon == 0x3e, '***FAILED p18f26k22 def RC osccon=0x3e'"
	nop
   .assert "osccon2 == 0x04, '***FAILED p18f26k22 def RC osccon=0x04'"
	nop
	bsf	OSCCON,IRCF2	; set 16MHz RC
   .assert "p18f26k22.frequency==16000000., '***FAILED p18f26k22 osccon=0x70 f=16e6'"
	nop
   .assert "osccon == 0x7e, '***FAILED p18f26k22  osccon=0x7e'"
	nop
   .assert "osccon2 == 0x04, '***FAILED p18f26k22 osccon=0x04'"
	nop

	movlw	0x03	; LFINTOSC
	movwf	OSCCON

	bsf	OSCTUNE,INTSRC ; HFINTOSC
   .assert "osccon == 0x0b, '***FAILED p18f26k22  osccon=0x0b'"
	nop
	bsf	OSCCON2,MFIOSEL
	nop

	return
; When edge1 is set, CTPLS goes high and  the - input of C2 is changed
; by current source of CTMU. When C2 output changes to 1 CTPLS goes low
; and the current source is turned off.  
pulse_delay:

	; In pulse mode we have C = 5pF, V = 2.048, I = 0.55 uA
	; so pulse should last t = CV/I = 18.6 uSec
	; Note Gpsim only resolves time to Instruction rate
	; for 16MHz clock tmr5l = 0x4A
	; C2 on + = Vref - = C12IN- (connected to CTMU)
	movlw	(1<<C2ON)|(1<<C2CH0)|(1<<C2R)|(1<<C2POL)
	movwf	CM2CON0
	clrf	CM2CON1
	bsf	CM2CON1,C2RSEL	; Vref = FVR
	movlw	(1<<CTMUEN)|(1<<TGEN)	; enable CTMU with edge delay generation
	movwf	CTMUCONH
	movlw	(1<<IRNG0)
	movwf	CTMUICON	; set current to 0.55 uA
	bcf	PIR2,C2IF
	call	discharge
	; T5 Gate enable, positive active
	movlw	(1<<TMR5GE)|(1<<T5GPOL)
	movwf	T5GCON
	; T5 Fosc/4 prescale=1 ON
	movlw	(1<<TMR5ON)
	movwf	T5CON
        bsf	CTMUCONL,EDG1STAT	; begin charging the circuit
	btfss	PIR2,C2IF
	goto	$-2
   .assert "tmr5l == 0x4c, ''*** FAILED 18f26k22 pulse mode'"

	nop
	

	bcf	CTMUCONH,TGEN		; turn off TGEN
	clrf	CM2CON0			; turn off comparators
	return

discharge:
	bsf     CTMUCONH,IDISSEN        ; drain charge on the circuit
        call    delay                   ; for 125 usec
	bcf     CTMUCONH,IDISSEN        ; end drain of circuit
	return

;
; Positive edge on pin CTED1 followed by CTED2 0.5 uS later
; charges capacitor with 5.5 uA current source t = C * V / I
time_measure:
	movlw	(1<<IRNG1)
	movwf	CTMUICON	; set current to 5.5 uA
	movlw	(1<<CTMUEN)|(1<<EDGEN)	; enable CTMU with edge enabled
	movwf	CTMUCONH
	; select A/D channel 1 and turn ON
	movlw	(1<<CHS0)|(1<<ADON)
	movwf	ADCON0
	movlw	0xdc		; edge1 CTED1 edge2 CTED2 both positive edge
	movwf	CTMUCONL
	call	discharge

	bcf	PIR1,ADIF		; make sure A/D IF not set
	bsf	CTMUCONH,CTTRIG		; use trigger to start A/D
	bsf	ADCON1,TRIGSEL		; 
	bsf	PORTB,1			; start measurement
	nop
	bsf	PORTB,5			; end measurement
	btfss	PIR1,ADIF		; wait for conversion
	goto    $-2
	; time = 0.5 usec current = 5.5 uA Cad = 5pF so voltage = 0.55 V
	; or 0x113 with V+ref = 2.048V V-ref = 0.0V
  .assert "(adresh==0x01) && ((adresl&0xf8)==0x10), '*** FAILED 18f26k22 time calculation'"
	nop
	return

; measure track and pin capacitance by measuring rate of change
; of voltage. Then C = I*t/V
; for I = 5.5 uA t = 4.5 uS V=1.54V gives C = 16.07 pF
cap_stray:
	clrf	CTMUCONL	; Set Edge status bits to zero
	movlw	2		; 5.5uA, Nominal - No Adjustment
	movwf   CTMUICON

	; select A/D channel 3 and turn ON
	movlw	(1<<CHS0)|(1<<CHS1)|(1<<ADON)
	movwf	ADCON0
	call	discharge
        bsf	CTMUCONL,EDG1STAT	; begin charging the circuit
	movlw	5
	movwf	delay1
	decfsz  delay1,F
	goto	$-2
	bcf	CTMUCONL,EDG1STAT	; stop charging circuit
	bcf	PIR1,ADIF		; make sure A/D IF not set
	bsf	ADCON0,GO		; start conversion
	btfss	PIR1,ADIF		; wait for conversion
	goto    $-2

   ; for C = 16pF V = 1.54V Vref = 2.048V 1.54/2.048 * 1024 = 770 (0x302)

  .assert "(adresh==0x03) && ((adresl&0xf8)==0x00), '*** FAILED 18f26k22 cap_stray'"
	nop
	clrf	ADCON0
	return

; A known resistor is on AN2. This allows us to get a calibration
; measurement of the CTMU current source
; I = V / R
current_measure:
	; select A/D channel 2 and turn ON
	movlw	(1<<CHS1)|(1<<ADON)
	movwf	ADCON0
	clrf	ADCON1			; Use Vdd, Vss no trigger
	clrf	CTMUCONL		; Set Edge status bits to zero
	movlw	2			; 5.5uA, Nominal - No Adjustment
	movwf   CTMUICON

	call	discharge

        bsf	CTMUCONL,EDG1STAT	; begin charging the circuit
	call	delay			; for 125 usec

	bcf	PIR1,ADIF		; make sure A/D IF not set
	bsf	ADCON0,GO		; start conversion
	btfss	PIR1,ADIF		; wait for conversion
	goto    $-2
	bcf	CTMUCONL,EDG1STAT	; stop charging circuit

        ; expect 420K x 5.5 uA = 2.31 V or 2.31/5 * 1024 = 0x1D9

  .assert "(adresh==0x01) && ((adresl&0xf0)==0xd0), '*** FAILED 18f26k22 current calibration'"
	nop
  	return

	; 125us delay
delay:
	movlw	0x51
	movwf	delay1
	decfsz  delay1,F
	goto	$-2
	nop
	return

	; 1.75 ms delay
ldelay:
	MOVLW	0xf4
	MOVWF	delay1
	MOVLW	0x01
	MOVWF	delay2
_00118_DS_:
	MOVLW	0xff
	ADDWF	delay1, F
	ADDWFC	delay2, F
	MOVF	delay1, W
	IORWF	delay2, W
	BNZ	_00118_DS_
	return

interrupt:

        movlb   0       ; BSR=0

   .assert "'FAILED 18F26k22 unexpected interrupt'"
        nop

back_interrupt:
        retfie 1

	end
