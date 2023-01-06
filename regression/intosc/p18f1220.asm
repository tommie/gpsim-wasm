;
; 
; Copyright (c) 2022 Rob Pearce
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

	list	p=18f1220
        include <p18f1220.inc>
        include <coff.inc>

 CONFIG OSC=INTIO2, FSCM=OFF, IESO=OFF
 CONFIG MCLRE=OFF
  CONFIG WDT=OFF

	;; The purpose of this program is to test gpsim's ability to simulate a
	;; PIC18F1220
	;; Specifically, the internal oscillator is tested in various modes.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA

t0_1 RES 1
t0_2 RES 1
x  RES  1
t1 RES  1
t2 RES  1
avg_lo RES  1
avg_hi RES  1
w_temp RES  1
status_temp RES  1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

	;; 
	;; Interrupt
	;; 
INT_VECTOR   CODE    0x008               ; interrupt vector location
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp


  .assert "'FAILED 18f1220 unexpected interrupt'"
	nop

check:
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:
  .sim "node na0"
   .sim "attach na0 porta0 portb1"

                                                                                
  .assert "frequency == 31000., '*** FAILED p18f1220 initial frequency'"
    nop

    call setup

    movlw   0x70
    movwf   OSCCON
  .assert "frequency == 8000000., '*** FAILED p18f1220 initial frequency'"
    nop

    movlw   0x1f
    movwf   OSCTUNE
  .assert "frequency == 9000000., '*** FAILED p18f1220 max OSCTUNE frequency'"
    nop
    movlw   0x21
    movwf   OSCTUNE
  .assert "frequency == 7000000., '*** FAILED p18f1220 min OSCTUNE frequency'"
    nop
    clrf    OSCTUNE
    bsf     OSCTUNE,6
  .assert "frequency == 8000000., '*** FAILED p18f1220 PLL on INTOSC'"

    movlw   0x60
    movwf   OSCCON
  .assert "frequency == 4000000., '*** FAILED p18f1220 4MHz int osc'"
    nop

    movlw   0x50
    movwf   OSCCON
  .assert "frequency == 2000000., '*** FAILED p18f1220 2MHz int osc'"
    nop

    movlw   0x40
    movwf   OSCCON
  .assert "frequency == 1000000., '*** FAILED p18f1220 1MHz int osc'"
    nop

    movlw   0x30
    movwf   OSCCON
  .assert "frequency ==  500000., '*** FAILED p18f1220 500kHz int osc'"
    nop

    movlw   0x20
    movwf   OSCCON
  .assert "frequency ==  250000., '*** FAILED p18f1220 250kHz int osc'"
    nop

    movlw   0x10
    movwf   OSCCON
  .assert "frequency ==  125000., '*** FAILED p18f1220 125kHz int osc'"
    nop

    movlw   0x00
    movwf   OSCCON
  .assert "frequency ==   31000., '*** FAILED p18f1220 31kHz int osc'"
    nop


  .assert "'*** PASSED 18f1220 INTOSC test'"
	goto $-1



setup: 
    movlw  0x7f		; ports are digital
    movwf  ADCON1
    clrf   CCP1CON       ;  CCP Module is off
    clrf   TMR2          ;  Clear Timer2
    clrf   TMR0L          ;  Clear Timer0
    movlw  0x1F          ;
    movwf  CCPR1L        ;  Duty Cycle is 25% of PWM Period
    clrf   INTCON        ;  Disable interrupts and clear T0IF
    bsf    PORTA,0
    ; Make output pins
    bcf    TRISA,0      ;  drive shutdown int1
    bcf    TRISB,3      ;  P1A
    bcf	   TRISB,2	 ;  P1B
    bcf	   TRISB,6	 ;  P1C
    bcf	   TRISB,7	 ;  P1D
    clrf   PIE1          ;  Disable peripheral interrupts
    clrf   PIR1          ;  Clear peripheral interrupts Flags
    return
	end

