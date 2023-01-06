;
; 
; Copyright (c) 2016 Roy Rankin
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
        radix dec

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR 0

temp         RES 1
txif_int     RES 1
tx2if_int    RES 1
rcif_int     RES 1
rc2if_int    RES 1

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

;SPBRG  :  3                      : 1 Mbits/se
;TXSTA  :  10110000  (B0h) : 8-bit tran
;RCSTA  :  10010000  (90h) : 8-bit rece


;*******************************************
; Sample Code For Synchronous Mode
;*******************************************

#define ClkFreq     16000000
#define baud(X)     ((10*ClkFreq/(4*X))+5)/10 - 1
#define TXSTA_INIT  0xB0
#define RCTSA_INIT  0x90

MAIN	CODE
    .sim "scope.ch0 = 'portc6'"
    .sim "scope.ch1 = 'portc7'"
    .sim "node clock data"
    .sim "attach clock portc6 portb6"
    .sim "attach data portc7 portb7"
start
       ;set clock to 16 Mhz

	BANKSEL OSCCON
        bsf     OSCCON,6

        clrf    STATUS
	BANKSEL ANSELA
        clrf    ANSELA  ; set port to digital
	clrf	ANSELC
	bsf	TRISC,7
	bsf	TRISC,6
	bsf	TRISB,7
	bsf	TRISB,6
        call	RC_CREN_Master
	call	TXEN_Master
	call    RC_Master
	call	TX_Master
	clrf	RCSTA
	clrf	RCSTA2

  .assert "'*** PASSED 18f26k22 synchronous euart'"
	nop

TXEN_Master:
	call	Setup_Sync_TXEN_Master_Mode
	call	Setup_Sync_RC_Slave_Mode
	bsf	RCSTA2,RX9
	movlw	0x55
	movwf   TXREG
  .assert("(txsta1 & 2) == 0, '*** FAILED p18f26k22 TXMT=0 after TXREG loaded and TXEN==0'" )
	nop
        ; delay to check data not sent yet
	decfsz	WREG,W
	goto	$-1
  .assert("(txsta1 & 2) == 0, '*** FAILED p18f26k22 TXMT=0 before  TXEN==1'" )
	nop
  .assert("(pir3 & 0x20) == 0, '*** FAILED p18f26k22 rc2if=0 before  TXEN==1'" )
	nop
	bsf	TXSTA,TXEN
	btfss	TXSTA,TRMT
	goto	$-1
  .assert("(pir3 & 0x20) == 0x20, '*** FAILED p18f26k22 rc2if=1 after  TXEN==1'" )
	nop
  .assert("(rcsta2 & 0x01) == 0x01, '*** FAILED p18f26k22 RX9D=1 after transfer'")
        nop
	movf	RCREG2,W
  .assert ("W == 0x55, '*** FAILED p18f26k22 rcreg 9 bit transfer'")
	nop
	return

TX_Master:
	call	Setup_Sync_TX_Master_Mode
	call	Setup_Sync_RC_Slave_Mode
	movlw   (1<<CKTXP)
	movwf	BAUDCON
	movwf	BAUDCON2
        movlw	0x55
	movwf	TXREG
	btfss	PIR1,TXIF
	goto	$-1
	movlw   0xaa
	movwf	TXREG
	btfss	PIR1,TXIF
	goto	$-1
	btfss	PIR3,RC2IF
	goto	$-1
        movf	RCREG2,W
     .assert "W == 0x55, 'p18f26k22 Sync M=tx S=rx clk invert char 1'"
	nop

	btfss	TXSTA,TRMT
	goto	$-1
	btfss	PIR3,RC2IF
	goto	$-1
        movf	RCREG2,W
     .assert "W == 0xaa, 'p18f26k22 Sync M=tx S=rx clk invert char 2'"
	nop
	return



RC_Master:
	call    Setup_Sync_RC_Master_Mode
        call    Setup_Sync_TX_Slave_Mode
	clrf    BAUDCON
	clrf    BAUDCON2
        movlw	0x55
	movwf	TXREG2
        bsf     RCSTA,SREN		;Start single character read
	btfss	PIR1,RC1IF
	goto	$-1
	movf	RCREG1,W
     .assert "W == 0x55, 'p18f26k22 Sync M=rx S=tx char 1'"
	nop
	return

RC_CREN_Master:
	call    Setup_Sync_RC_Master_Mode
        call    Setup_Sync_TX_Slave_Mode
	clrf    BAUDCON
	clrf    BAUDCON2
	bsf     INTCON,GIE
        bsf     INTCON,PEIE
        movlw	0x55
	movwf	TXREG2
        movlw   (1<<SPEN) | (1<<CREN) 	;Start continuous  character read
	movwf	RCSTA
	clrf	rcif_int
	bsf	PIE1,RC1IE
	btfss	rcif_int,0
	  goto	$-1
	movf	RCREG1,W
     .assert "W == 0x55, 'p18f26k22 Sync M=rx(CREN) S=tx char 1'"
	nop
	btfss	RCSTA,OERR
	  goto	$-1
	movf	RCREG1,W	; this should not clear OERR, because CREN is set
   .assert "(rcsta1 & 0x02) == 0x02,'p18f26k22 Sync M=rx(CREN) RCREG does not clean OERR'"
	nop 
	bcf	RCSTA,CREN
   .assert "(rcsta1 & 0x02) == 0x00,'p18f26k22 Sync M=rx(CREN) CREN=0 does clean OERR'"
	nop 
        bcf	RCSTA,SPEN
   .assert "(pir1 & 0x20) == 0x00,'p18f26k22 Sync M=rx(CREN) SPEN=0 does clean RCIF'"
	nop 
	return

Setup_Sync_TX_Master_Mode
        movlb   0
        movlw   baud(1000000)
	BANKSEL	SPBRG
        movwf   SPBRG
        movlw   (1<<CSRC) | (1<<SYNC) 
        movwf   TXSTA
        movlw   (1<<SPEN) 
        movwf   RCSTA
	bsf	TXSTA,TXEN
	return

; Setup synchronous master mode 9 bits TXEN trigger output
Setup_Sync_TXEN_Master_Mode
        movlb 0
        movlw baud(1000000)
	BANKSEL	SPBRG
        movwf SPBRG
        ; send 9 bits bit9=1
        movlw (1<<CSRC) | (1<<SYNC) | (1<<TX9) | (1<<TX9D)
        movwf TXSTA
        movlw (1<<SPEN) 
        movwf RCSTA
	return

Setup_Sync_RC_Master_Mode
        movlb   0
        movlw   baud(1000000)
	BANKSEL	SPBRG
        movwf   SPBRG
        movlw   (1<<CSRC) | (1<<SYNC) 
        movwf   TXSTA
        movlw   (1<<SPEN) 
	movwf	RCSTA
	return

Setup_Sync_RC_Slave_Mode:
        movlb   0
        movlw   baud(1000000)
	BANKSEL	SPBRG
        movwf   SPBRG
        movlw   (1<<SYNC) 
        movwf   TXSTA2
        movlw   (1<<SPEN) 
        movwf   RCSTA2
        bsf	RCSTA2,CREN
	return

Setup_Sync_TX_Slave_Mode:
        movlw   (1<<SYNC) 
        movwf   TXSTA2
        movlw   (1<<SPEN)         	;RCSTA_INIT
        movwf   RCSTA2
        bsf     TXSTA2,TXEN
	return

interrupt:

        movlb   0       ; BSR=0

        movf    PIR1,W
        andwf   PIE1,W
	movwf   temp
        btfsc   temp,TXIF
          goto  int_txif
        btfsc   temp,RCIF
          goto  int_rcif
        movf    PIR3,W
        andwf   PIE3,W
	movwf   temp
        btfsc   temp,TX2IF
          goto  int_tx2if
        btfsc   temp,RC2IF
          goto  int_rc2if
        

   .assert "'FAILED 18F26k22 unexpected interrupt'"
        nop

back_interrupt:
        retfie 1

int_txif:
        bsf    txif_int,0
	bcf    PIE1,TX1IE
        goto   back_interrupt

int_tx2if:
        bsf    tx2if_int,0
	bcf    PIE3,TX2IE
        goto   back_interrupt

int_rcif:
        bsf    rcif_int,0
	bcf    PIE1,RC1IE
        goto   back_interrupt

int_rc2if:
        bsf    rc2if_int,0
	bcf    PIE3,RC2IE
        goto   back_interrupt

    end
