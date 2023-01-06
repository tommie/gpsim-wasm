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

	list	p=18f14k22
        include <p18f14k22.inc>
        include <coff.inc>

  CONFIG FOSC=IRC, FCMEN=OFF, IESO=OFF
  CONFIG MCLRE=OFF
  CONFIG WDTEN=OFF

	;; The purpose of this program is to test gpsim's ability to simulate a
	;; PIC18F26K22
	;; Specifically, the internal oscillator is tested in various modes.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA 0

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


  .assert "'FAILED 18F14K22 unexpected interrupt'"
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

                                                                                
  .assert "frequency == 1000000., '*** FAILED 18F14K22 initial frequency'"
    nop

    call setup

    movlw   0x70
    movwf   OSCCON
  .assert "frequency == 16000000., '*** FAILED 18F14K22 max base frequency'"
    nop

    movlw   0x1f
    movwf   OSCTUNE
  .assert "frequency == 18000000., '*** FAILED 18F14K22 max OSCTUNE frequency'"
    nop
    movlw   0x21
    movwf   OSCTUNE
  .assert "frequency == 14000000., '*** FAILED 18F14K22 min OSCTUNE frequency'"
    nop
    clrf    OSCTUNE
    bsf	    OSCTUNE,PLLEN
  .assert "frequency == 64000000., '*** FAILED 18F14K22 PLL on INTOSC'"

    movlw   0x60
    movwf   OSCCON
  .assert "frequency == 32000000., '*** FAILED 18F14K22 lower PLL frequency'"
    nop

    movlw   0x50
    movwf   OSCCON
  .assert "frequency == 4000000., '*** FAILED 18F14K22 4MHz int osc / PLL'"
    nop

    bcf	    OSCTUNE,PLLEN

    movlw   0x40
    movwf   OSCCON
  .assert "frequency == 2000000., '*** FAILED 18F14K22 2MHz int osc'"
    nop

    movlw   0x30
    movwf   OSCCON
  .assert "frequency == 1000000., '*** FAILED 18F14K22 1MHz int osc'"
    nop

    movlw   0x20
    movwf   OSCCON
  .assert "frequency ==  500000., '*** FAILED 18F14K22 500kHz int osc'"
    nop

    movlw   0x10
    movwf   OSCCON
  .assert "frequency ==  250000., '*** FAILED 18F14K22 250kHz int osc'"
    nop

    movlw   0x00
    movwf   OSCCON
  .assert "frequency ==   31000., '*** FAILED 18F14K22 31kHz int osc'"
    nop


  .assert "'*** PASSED 18F14K22 INTOSC test'"
	goto $-1



setup: 
    clrf   CCP1CON       ;  CCP Module is off
    clrf   TMR2          ;  Clear Timer2
    clrf   TMR0L          ;  Clear Timer0
    clrf   ADCON1	 ; turn ports to digital
    movlw  0x1F          ;
    movwf  CCPR1L        ;  Duty Cycle is 25% of PWM Period
    clrf   INTCON        ;  Disable interrupts and clear T0IF
    ; Make output pins
    bcf    TRISC, 1      ;  Make pin output
    bcf    TRISC, 2      ;  P1A
;    bcf	   TRISD,5	 ;  P1B
;    bcf	   TRISD,6	 ;  P1C
;    bcf	   TRISD,7	 ;  P1D
    clrf   PIE1          ;  Disable peripheral interrupts
    clrf   PIR1          ;  Clear peripheral interrupts Flags
    return
	end

