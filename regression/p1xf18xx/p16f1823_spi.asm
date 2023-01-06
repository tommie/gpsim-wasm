;
; 
; Copyright (c) 2013 Roy Rankin
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


	;; The purpose of this program is to test gpsim's ability to 
	;; simulate a pic 16f1823.
	;; Specifically, SPI

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

  __CONFIG _CONFIG2, _PLLEN_OFF

#define SDO_PORT PORTC,2
#define SDI_PORT PORTC,1
#define SCK_PORT PORTC,0
#define SS_PORT PORTC,3

#define SDO_TRIS TRISC,2
#define SDI_TRIS TRISC,1
#define SCK_TRIS TRISC,0
#define SS_TRIS TRISC,3

#define DRV_CLOCK PORTA,1
#define DRV_CLOCK_TRIS TRISA,1


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR

w_temp RES  1
status_temp RES  1
loopcnt	RES	1



;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlp  high  start               ; load upper byte of 'start' label
        goto   start                     ; go to beginning of program

	;; 
	;; Interrupt
	;; 
INT_VECTOR   CODE    0x004               ; interrupt vector location
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp

        btfss   PIE1,SSP1IF
        goto    check	                ; other Interrupts
;        bsf     _TIME_OUT_              ; MUST set this Flag
        bcf     PIE1,SSP1IF


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

   .sim "module lib libgpsim_modules"
   .sim "module load pu pu1"
   .sim "module load pu pu2"
   .sim "module load not not"
;   .sim "p16f88.xpos = 132."
;   .sim "p16f88.ypos = 96."

   .sim "not.xpos=264."
   .sim "not.ypos=180."

   .sim "pu1.xpos=252."
   .sim "pu1.ypos=96."

   .sim "pu2.xpos=264."
   .sim "pu2.ypos=252."

   .sim "node ss"
   .sim "attach ss portc3 porta2"	; Not SS
   .sim "node sck"
   .sim "attach sck portc0 porta1 pu1.pin"	; SCK
   .sim "node sdo"
   .sim "attach sdo not.in0 portc2 pu2.pin"
   .sim "node sdi"
   .sim "attach sdi portc1 not.out"

    BANKSEL	ANSELA
    clrf	ANSELA
    clrf	ANSELC
    BANKSEL	OSCCON
    movlw	(0xe<<3)|3	; set internal RC to 8 Mhz
    movwf	OSCCON
    BANKSEL	TRISC
    bcf		SDO_TRIS	; SDO
    bcf		SCK_TRIS	; SCK
    movlw	0xff
    BANKSEL	SSPSTAT	
    movwf	SSPSTAT
  .assert "ssp1stat == 0xC0, 'SPI sspstat only SMP, CKE writable'"
    nop
    clrf	SSPSTAT


;
;  	Test SPI Master mode
;
    movlw	0x21	; SSPEN | SPI master Fosc/16
    movwf	SSPCON1
    movlw	0xab
    movwf	SSPBUF
    BANKSEL	PIR1
    bcf		PIR1,SSP1IF

loop:
    btfss	PIR1,SSP1IF
    goto	loop

  .assert "(ssp1stat & 1) == 1, 'FAILED MSSP SPI Master BF not set'"
    nop
    BANKSEL	SSPBUF
    movf	SSPBUF,W
  .assert "(ssp1stat & 1) == 0, 'FAILED MSSP SPI Master BF not cleared'"
    nop
  .assert "W == 0x54, 'FAILED MSSP SPI Master wrong data'"
    nop

;
;	TEST SPI Slave mode with SS
;
    clrf	SSPCON1
    BANKSEL	TRISC
    bcf		DRV_CLOCK_TRIS 	; external SCK drive
    bsf		SCK_TRIS	; SCK
    bsf		SS_TRIS 	; SS
    BANKSEL	PORTA
    bcf		DRV_CLOCK
    bcf		PIR1,SSP1IF
    banksel	SSPCON1
    movlw	0x24	; SSPEN | SPI slave mode SS enable
    movwf	SSPCON1
    movlw	0xab
    movwf	SSPBUF
    BANKSEL	PORTA
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    BANKSEL	SSPBUF
    movwf	SSPBUF	; test WCOL set
  .assert "(ssp1con & 0x80) == 0x80, 'FAILED MSSP SPI WCOL set'"
    nop
    bcf		SSPCON1,WCOL	; clear WCOL bit
  .assert "(ssp1con & 0x80) == 0x00, 'FAILED MSSP SPI WCOL was cleared'"
    nop
    clrf	loopcnt
    BANKSEL	PORTA
loop2:
    incf	loopcnt,F
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    btfss	PIR1,SSP1IF
    goto	loop2

    BANKSEL	SSPBUF
    movf	SSPBUF,W
  .assert "W == 0x54, 'FAILED MSSP SPI Slave data'"
    nop
;
;	Test Slave receive overrun
;
   movlw	0x10
   movwf	loopcnt
   BANKSEL	PORTA
loop4:
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    decfsz	loopcnt,F
    goto	loop4
  .assert "(ssp1con & 0x40) == 0x40, 'FAILED MSSP SPI SSPOV'"
    nop

;
;  	Test SPI Master mode TMR2
;
    BANKSEL	SSPCON1
    clrf	SSPCON1
    BANKSEL	TRISA
    bsf		DRV_CLOCK_TRIS	; external SCK drive off
    bcf		SCK_TRIS	; SCK output
    BANKSEL	PR2
    movlw	0x1
    movwf	PR2
    clrf	TMR2
    movlw	0x3C	; prescale = 1 postscale 16
    movwf	T2CON

    bcf		PIR1,SSP1IF
    BANKSEL	SSPCON1
    movlw	0x23	; SSPEN | SPI master TMR2
    movwf	SSPCON1
    movlw	0xab
    movwf	SSPBUF

    BANKSEL	PIR1
loop3:
    btfss	PIR1,SSP1IF
    goto	loop3

  .assert "(ssp1stat & 1) == 1, 'FAILED MSSP SPI Master TMR2, BF not set'"
    nop
    BANKSEL	SSPBUF
    movf	SSPBUF,W
  .assert "(ssp1stat & 1) == 0, 'FAILED MSSP SPI Master TMR2, BF not cleared'"
    nop
  .assert "W == 0x54, 'FAILED MSSP SPI Master TMR2 wrong data'"
    nop

;
;  	Test SPI Master mode with different clock phasing
;
    movlw	0x31	; SSPEN | SPI master Fosc/16 / CKP=1
    movwf	SSPCON1
    bsf         SSPSTAT,CKE     ; Delayed clock phasing
    movlw	0xc9
    movwf	SSPBUF
    BANKSEL	PIR1
    bcf		PIR1,SSP1IF

loop5:
    btfss	PIR1,SSP1IF
    goto	loop5

  .assert "(ssp1stat & 1) == 1, 'FAILED MSSP SPI Master BF not set'"
    nop
    BANKSEL	SSPBUF
    movf	SSPBUF,W
  .assert "(ssp1stat & 1) == 0, 'FAILED MSSP SPI Master BF not cleared'"
    nop
  .assert "W == 0x36, 'FAILED MSSP SPI Master (CKP) wrong data'"
    nop


;
;  	Test SPI Master mode with BOEN set (MSSP1)
;
    bsf		SSPCON3,BOEN
    movlw	0x3
    movwf	SSPADD		; Fosc/16 (sspadd+1)*4
    movlw	(1<<SSPEN)|0x0a	; SSPEN | SPI master Fosc/4*(spadd+1)
    ;movlw	(1<<SSPEN)|0x01	; SSPEN | SPI master Fosc/4*(spadd+1)
    movwf	SSPCON1
    bcf         SSPSTAT,CKE     ; Delayed clock phasing
    movlw	0xc9
    movwf	SSPBUF
    BANKSEL	PIR1
    bcf		PIR1,SSP1IF

    btfss	PIR1,SSP1IF
    goto	$-1

  .assert "(ssp1stat & 1) == 0, 'FAILED MSSP SPI Master BF not set BOEN=1'"
    nop
    BANKSEL	SSPBUF
    movf	SSPBUF,W
  .assert "W == 0x36, 'FAILED MSSP SPI Master sspadd (CKP) wrong data'"
    nop



  .assert "'*** PASSED 16f1823 MSSP SPI test'"
    nop
    
    goto $

	end
