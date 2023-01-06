;
; 
; Copyright (c) 2017 Roy Rankin
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


        ;; The purpose of this program is to test gpsim's ability to 
        ;; simulate a pic 16F1705.
        ;; Specifically, the cog with pps
    
    ; Tests in this file have been validated against a real device,
    ; hence the slightly unusual format of some of them

	LIST F=INHX8M, P=PIC16F1705
	include <p16f1705.inc>
	include <coff.inc>              ; Grab some useful macros

        __CONFIG _CONFIG1, _CP_OFF & _WDTE_OFF &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_ON & _CLKOUTEN_OFF
        __CONFIG _CONFIG2, _STVREN_ON  ;//& _ZCDDIS_OFF; & _WRT_BOOT

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


#define COG_A_TRIS  TRISA,4
#define COG_B_TRIS  TRISA,5
#define COG_C_TRIS  TRISA,0
#define COG_D_TRIS  TRISA,1
#define COG_IN_TRIS TRISA,2

OPTION_IV   equ 0x00
OSCCON_IV   equ 0x78        ; Fosc at 16MHz

PORTA_IV    equ 0x08
TRISA_IV    equ 0x0C
PORTC_IV    equ 0x10
TRISC_IV    equ 0x27        ; Port is inputs except for TX & clk

#define DRV_CLOCK   PORTC,3



;----------------------------------------------------------------------
; RAM Declarations


;
INT_VAR        UDATA   0x70

ErrorBits1      res 1
ErrorBits2      res 1
ErrorBits3      res 1
ErrorBits4      res 1

rxLastByte	RES	1
rxFlag		RES	1

 global  rxLastByte


;**********************************************************************************
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlp  high  start               ; load upper byte of 'start' label
        goto   start                     ; go to beginning of program

;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
       clrf    BSR             ; set bank 0

       btfsc PIR1,TMR2IF
	goto tmr2_int

       btfsc PIR1,CCP1IF
	goto ccp1_int

     btfsc PIR1,TXIF
	goto tx_int

   .assert "'***FAILED p16f1705 unexpected interrupt'"
	nop

int_exit
       retfie

tx_int
    .assert "rc1reg == tx1reg, '*** FAILED p16f1705 sent character looped back'"
	nop

        BANKSEL RCREG
        movf    RCREG,W
        movwf   rxLastByte
        bsf     rxFlag,0
	goto	int_exit


    .assert "'TO HERE'"
	nop

tmr2_int
	bcf PIR1,TMR2IF
	goto int_exit;

ccp1_int
	bcf PIR1,CCP1IF
	goto int_exit;


  .sim "scope.ch0 = 'porta0'"
  .sim "scope.ch1 = 'porta1'"
  .sim "scope.ch2 = 'porta4'"
  .sim "scope.ch3 = 'porta5'"
  .sim "scope.ch4 = 'porta2'"
  .sim "scope.ch5 = 'portc4'"
  .sim "scope.ch6 = 'portc5'"
  .sim "node npwm"
  .sim "attach npwm portc3 porta2"

  .sim "module library libgpsim_modules"
  .sim "module load usart U1"
  .sim "U1.xpos = 250.0"
  .sim "U1.ypos = 80.0"
   ;; Set the USART module's Baud Rate

  .sim "U1.txbaud = 9600"
  .sim "U1.rxbaud = 9600"
  .sim "U1.loop = true"
  .sim "break c 0x1000000"


  .sim "node PIC_tx"
  .sim "node PIC_rx"

  ;; Tie the USART module to the PIC
  .sim "attach PIC_tx portc4 U1.RXPIN"
  .sim "attach PIC_rx portc5 U1.TXPIN"

   .sim "symbol ref_cycle=0"


MAIN	CODE
start

    call setup
    call test_cog
  .assert "'*** PASSED p16f1705 COG'"
    nop

;**********************************************************************************

setup:
    banksel LATA
    movlw   PORTA_IV
    movwf   LATA
    movlw   PORTC_IV
    movwf   LATC

    banksel OPTION_REG
    movlw   OPTION_IV
    movwf   OPTION_REG      ; Set timer 0 and RB pull-up options
    movlw   TRISA_IV
    movwf   TRISA           ; Configure port A
    movlw   TRISC_IV
    movwf   TRISC           ; Configure port C
    movlw   OSCCON_IV
    movwf   OSCCON
    movlw   9
    movwf   WDTCON			;WDT 16 ms
;Bank 3
	banksel ANSELC
	clrf	ANSELC			;disabling analog circuitry
	clrf	ANSELA
;Bank 29
        banksel RA4PPS
        movlw   0x08        ; COG1A  -->   RA4
        movwf   RA4PPS
        movlw   0x09        ; COG1B  -->   RA5
        movwf   RA5PPS
        movlw   0x0A        ; COG1C  -->   RA0
        movwf   RA0PPS
        movlw   0x0B        ; COG1D  -->   RA1
        movwf   RA1PPS
	movlw   0x15			; RC4
        movwf	RXPPS			; Usart input pin
        banksel COGINPPS
        movlw   0x02        ; RA2 (CLK_IN)  -->   COGIN
        movwf   COGINPPS
;	movlw	0x0E
;	movwf	RC5PPS			;PWM3OUT to RC5
        movlw   0x14			; Usart output pin
	movwf   RC4PPS

	; Next configure the COG input data selection
	banksel COG1CON0
	movlw	0x04        ; Use half-bridge mode
	movwf	COG1CON0
	movlw	0x00        ; No deadbands initially
	movwf	COG1DBR
	movwf	COG1DBF
	movlw	0x00        ; No event blanking, for now
	movwf	COG1BLKR
	movwf	COG1BLKF
	movlw	0x00        ; All outputs normal polarity
	movwf	COG1CON1
	movlw	0x01        ; Feed CLK_IN...
	movwf	COG1RIS     ; ... to the rising edge detector
	movwf	COG1FIS     ; ... and the falling edge detector
	movlw	0x00        ; Use immediate events
	movwf	COG1RSIM
	movwf	COG1FSIM
	movlw	0xA8        ; No auto-restart, both pins low, in shutdown
	movwf	COG1ASD0
	movlw	0x00        ; No hardware auto-shutdown yet
	movwf	COG1ASD1
	movlw	0x0F        ; Steer outputs to the pins
	movwf	COG1STR
	movlw	0x00        ; No phase delay
	movwf	COG1PHR
	movwf	COG1PHF

	return



test_cog:
;
;   TEST: COG auto-shutdown in half-bridge mode
;
; Check ASE bit can be set with COG disabled
; Check ASE results in no output (the ASD modes are set to "logic 0")
; Check that software clearing results in resumed output but only at the 
; next rising event
;

 banksel COG1CON0
 btfss  COG1ASD0,G1ASE
 bsf    ErrorBits1,0
 bsf    COG1CON0,G1EN       ; Enable the COG
  .assert "ErrorBits1 == 0, '*** FAILED p16f1705 ASE settable without COGEN'"

 bsf    COG1ASD0,G1ASE
 nop
 banksel PORTA
 btfss  PORTA,4
 btfsc  PORTA,5
 bsf    ErrorBits1,1
  .assert "ErrorBits1 == 0, '*** FAILED p16f1705 ASE forces outputs low'"

 bsf    DRV_CLOCK
 nop
 btfss  PORTA,4
 btfsc  PORTA,5
 bsf    ErrorBits1,2
  .assert "ErrorBits1 == 0, '*** FAILED p16f1705 rising edge with ASE'"

 bcf    DRV_CLOCK
 nop
 btfss  PORTA,4
 btfsc  PORTA,5
 bsf    ErrorBits1,3
  .assert "ErrorBits1 == 0, '*** FAILED p16f1705 falling edge with ASE'"

 banksel COG1CON0
 bcf    COG1ASD0,G1ASE
 nop
 btfsc  COG1ASD0,G1ASE
 bsf    ErrorBits1,4
  .assert "ErrorBits1 == 0, '*** FAILED p16f1705 clear ASE flag'"

 banksel PORTA
 btfss  PORTA,4
 btfsc  PORTA,5
 bsf    ErrorBits1,5
  .assert "ErrorBits1 == 0, '*** FAILED p16f1705 clear ASE wait for edge'"

 bsf    DRV_CLOCK
 nop
 btfsc  PORTA,4
 btfsc  PORTA,5
 bsf    ErrorBits1,6
  .assert "ErrorBits1 == 0, '*** FAILED p16f1705 outputs enabled on rising edge'"

 bcf    DRV_CLOCK
 nop
 btfss  PORTA,4
 btfss  PORTA,5
 bsf    ErrorBits1,7
  .assert "ErrorBits1 == 0, '*** FAILED p16f1705 outputs on falling edge'"



;
;   TEST: COG double buffer and load bit
;
; Check changes to DBx registers do not take effect immediately
; Check LD bit is honoured on rising edge only
; Check LD bit gets cleared once dealt with
;

	banksel COG1DBF
	movlw   4
	movwf   COG1DBR
	movlw   8
	movwf   COG1DBF
	nop
	; As we haven't asserted GxLD, the new values don't take effect yet
	banksel PORTA
	bsf     DRV_CLOCK
	nop
  .assert "((porta&0x33)==0x11), '*** FAILED p16f1705 rising deadband double buffer'"
	movf    PORTA,w
	andlw   0x33
	xorlw   0x11
	skpz
	bsf     ErrorBits2,0

	bcf     DRV_CLOCK
	nop
  .assert "((porta&0x33)==0x22), '*** FAILED p16f1705 falling deadband double buffer'"
	movf    PORTA,w
	andlw   0x33
	xorlw   0x22
	skpz
	bsf     ErrorBits2,1

	; now assert LD and check it works
	banksel COG1CON0
	bsf     COG1CON0,G1LD
	nop
  .assert "((cog1con0&0x40)==0x40), '*** FAILED p16f1705 set GxLD bit'"
	btfss   COG1CON0,G1LD
	bsf     ErrorBits2,2

	banksel PORTA
	bsf     DRV_CLOCK
	nop
	bcf     DRV_CLOCK
	nop
	bsf     DRV_CLOCK
	nop
  .assert "((porta&0x33)==0x00), '*** FAILED p16f1705 rising deadband delay'"
	movf    PORTA,w
	andlw   0x33
	skpz
	bsf     ErrorBits2,4
	nop
  .assert "((porta&0x33)==0x11), '*** FAILED p16f1705 rising deadband expiry'"
	movf    PORTA,w
	andlw   0x33
	xorlw   0x11
	skpz
	bsf     ErrorBits2,5

	bcf     DRV_CLOCK
	nop
	nop
	nop
	nop
	nop
  .assert "((porta&0x33)==0x00), '*** FAILED p16f1705 falling deadband delay'"
	movf    PORTA,w
	andlw   0x33
	skpz
	bsf     ErrorBits2,6
	nop
  .assert "((porta&0x33)==0x22), '*** FAILED p16f1705 falling deadband expiry'"
	movf    PORTA,w
	andlw   0x33
	xorlw   0x22
	skpz
	bsf     ErrorBits2,7


	banksel COG1CON0
  .assert "((cog1con0&0x40)==0x00), '*** FAILED p16f1705 GxLD bit self-clear'"
	btfsc   COG1CON0,G1LD
	bsf     ErrorBits2,3


    return







	end
