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
        CONFIG FOSC = INTIO67
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
hlvd_int RES	1

  GLOBAL hlvd_int

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

  .sim "module library libgpsim_modules"
  .sim "module load pulldown R"
  .sim "R.resistance = 10."
  .sim "R.voltage = 5."
  .sim "R.xpos = 84"
  .sim "R.ypos = 228"
  .sim "node na0"
  .sim "attach na0 R.pin  porta5"

	movlw	0x70		;set 16MHz
	movwf	OSCCON
	movlw	(1<<TMR0ON)|(1<<PSA)
	movwf	T0CON
	; Enable HLVD voltage >= 1.024 on HLVDIN
	movlw	(1<<HLVDEN)|(1<<VDIRMAG)|0xf
	movwf	HLVDCON
	; IRVST set in 20 uS or 80 cycles at 16MHz
	clrf	TMR0L
	; HLVDIF should go high when IRVST is set
	btfss	PIR2,HLVDIF
	goto	$-2
  .assert "tmr0l == 0x55, '***FAILED 18f26k22 time for IRVST to go high'"
	nop
	bcf	PIR2,HLVDIF		; clear FLAG
	bcf	HLVDCON,VDIRMAG		; Trigger voltage <= 1.024
  .assert "(pir2 & 0x04) == 0x00, '***FAILED 18f26k22 unexpected HLDVIF'"
	nop
  .command "R.voltage=0."
	nop
  .assert "(pir2 & 0x04) == 0x04, '***FAILED 18f26k22 HLDVIF VDIRMAG=0, v < 1.024'"
	nop
	bcf	PIR2,HLVDIF
	; Enable HLVD test Vdd >= 4.74V 
	movlw	(1<<HLVDEN)|(1<<VDIRMAG)|0xe
	movwf	HLVDCON
  .assert "(pir2 & 0x04) == 0x04, '***FAILED 18f26k22 HLDVIF VDIRMAG=1, Vdd > 4.74'"
	nop
	bcf     PIR2,HLVDIF
	bcf	HLVDCON,VDIRMAG		; under-voltage test
  .assert "(pir2 & 0x04) == 0x00, '***FAILED 18f26k22 no HLDVIF VDIRMAG=0, Vdd > 4.74'"
	nop
	; enable HLVD interrupts
	bsf	PIE2,HLVDIF
	bsf	INTCON,GIE
	bsf	INTCON,PEIE
	clrf	hlvd_int
  .command "p18f26k22.Vdd=3.30"
	nop
	nop
  .assert "hlvd_int != 0, '***FAILED 18f26k22 interrupt VDIRMAG=0, Vdd < 4.74'"
	nop
	nop
  .assert  "'*** PASSED 18F26k22 HLVD test'"
        nop
        goto    $-1

interrupt:

        movlb   0       ; BSR=0
	
	btfsc	PIR2,HLVDIF
	  goto	int_hlvd

   .assert "'FAILED 18F26k22 unexpected interrupt'"
        nop

back_interrupt:
        retfie 1

int_hlvd:
	bsf	hlvd_int,0
	bcf	PIR2,HLVDIF
	goto	back_interrupt


	end
