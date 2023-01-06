;
; 
; Copyright (c) 2017 Roy Rankin
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
        ;; simulate a pic 16F1709.
	;; test zero crossing detector
        ;; Specifically, basic port operation, ssp, euart, pwm, cog with pps

	LIST F=INHX8M, P=PIC16F1709
	include <p16f1709.inc>
	include <coff.inc>              ; Grab some useful macros

        __CONFIG _CONFIG1, _CP_OFF & _WDTE_OFF &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_ON & _CLKOUTEN_OFF
        __CONFIG _CONFIG2, _STVREN_ON  ;//& _ZCDDIS_OFF; & _WRT_BOOT

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


#define SDO_PORT PORTC,2
#define SDI_PORT PORTC,1
#define SCK_PORT PORTC,0
#define SS_PORT PORTB,4

#define SDO_TRIS TRISC,2
#define SDI_TRIS TRISC,1
#define SCK_TRIS TRISC,0
#define SS_TRIS TRISB,4

#define DRV_CLOCK PORTA,2
#define DRV_CLOCK_TRIS TRISA,2

;----------------------------------------------------------------------
; RAM Declarations


;
INT_VAR        UDATA   0x70

temp1		RES	1
temp2		RES	1
temp3		RES	1

tx_ptr		RES	1

rxLastByte	RES	1
rxFlag		RES	1
w_temp 		RES  1
status_temp 	RES  1
loopcnt		RES	1

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

        btfsc   PIE1,SSP1IF
	goto ssp_int

   .assert "'***FAILED p16f1709 unexpected interrupt'"
	nop

int_exit
       retfie

ssp_int
        bcf     PIE1,SSP1IF
	goto	int_exit
tx_int
    .assert "rc1reg == tx1reg, '*** FAILED p16f1709 sent character looped back'"
	nop

        BANKSEL RCREG
        movf    RCREG,W
        movwf   rxLastByte
        bsf     rxFlag,0
	goto	int_exit


    .assert "'TO HERRE'"
	nop

tmr2_int
	bcf PIR1,TMR2IF
	goto int_exit;

ccp1_int
	bcf PIR1,CCP1IF
	goto int_exit;


  .sim "scope.ch0 = 'portc0'"
  .sim "scope.ch1 = 'portc1'"
  .sim "scope.ch2 = 'portc2'"
  .sim "scope.ch3 = 'porta0'"
  .sim "scope.ch4 = 'porta1'"
  .sim "scope.ch5 = 'portc5'"
IF (0)
  .sim "scope.ch0 = 'portc5'"
  .sim "scope.ch1 = 'portc4'"
  .sim "scope.ch2 = 'portc3'"
  .sim "scope.ch3 = 'porta0'"
  .sim "scope.ch4 = 'porta1'"
ENDIF
  .sim "node npwm"
  .sim "attach npwm portc5 porta2"

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
  .sim "attach PIC_tx porta0 U1.RXPIN"
  .sim "attach PIC_rx porta1 U1.TXPIN"

   .sim "module load pu pu1"
   .sim "module load pu pu2"
   .sim "module load not not"

   .sim "not.xpos=264."
   .sim "not.ypos=180."

   .sim "pu1.xpos=252."
   .sim "pu1.ypos=120."

   .sim "pu2.xpos=264."
   .sim "pu2.ypos=252."

   .sim "node ss"
   .sim "attach ss portb4 portc6"	; Not SS
   .sim "node sck"
   .sim "attach sck portc0 porta2 pu1.pin"	; SCK
   .sim "node sdo"
   .sim "attach sdo not.in0 portc2 pu2.pin"
   .sim "node sdi"
   .sim "attach sdi portc1 not.out"
   .sim "symbol ref_cycle=0"


MAIN	CODE
start

    call setup
    call test_ssp
    call usart
    call pwm
  .assert "'*** PASSED p16f1709 PWM, COG'"
    nop

;**********************************************************************************

usart:
        BANKSEL SPBRGL
;        movlw   25              ;9600 baud.
        movlw   0x19              ;9600 baud.
        movwf   SPBRGL

        ;; Turn on the serial port
        BANKSEL RCSTA
        movlw   (1<<SPEN) | (1<<CREN)
        movwf   RCSTA

        movf    RCREG,w          ;Clear RCIF
        bsf     INTCON,GIE
        bsf     INTCON,PEIE

        movf    RCREG,w          ;Clear RCIF
        movf    RCREG,w          ;Clear RCIF


        ;; Test TXIF, RCIF bits of PIR1 are not writable

        BANKSEL PIR1
        clrf    PIR1
        bsf     PIR1,RCIF
        bsf     PIR1,TXIF
  .assert "pir1 == 0x00, '*** FAILED 16F1709 USART TXIF, RCIF not writable'"
        nop


        ;; Enable the transmitter
        BANKSEL TXSTA
        bsf     TXSTA,TXEN
 .assert "pir1 == 0x10, '*** FAILED 16F1709 USART TXIF should now be set'"
        nop
	bsf	TXSTA,SENDB
	clrf	TXREG		; start break transfer with dummy write
	btfsc	TXSTA,SENDB
	goto 	$-1

        BANKSEL PIE1
        bsf     PIE1,RCIE       ; Enable Rx interrupts

        clrf    BSR

        ;; Now Transmit some data and verify that it is transmitted correctly.

        call    TransmitNextByte
   .assert "rxLastByte == 0x31, '*** FAILED 16F1709 USART -  sending 0x31'"
        nop

        call    TransmitNextByte
   .assert "rxLastByte == 0x32, '*** FAILED 16f1709 USART -  sending 0x32'"
        nop

        call    TransmitNextByte
   .assert "rxLastByte == 0x33, '*** FAILED 16f1709 USART -  sending 0x33'"
        nop


        ;; Disable the transmitter
        BANKSEL TXSTA
        bcf     TXSTA,TXEN
        ;; Turn off the receiver
        BANKSEL RCSTA
	bcf	RCSTA,SPEN

    return

TransmitNextByte:	
	clrf	rxFlag
	call	tx_message
	BANKSEL TXREG
	movwf	TXREG

	BANKSEL	PIR1

rx_loop:

	btfss	rxFlag,0
	 goto	rx_loop

;	clrf	temp2
;	call	delay		;; Delay between bytes.

	btfss	PIR1,TXIF
	 goto	$-1

	return

tx_message
	incf	tx_ptr,w
	andlw	0x0f
	movwf	tx_ptr
	addlw	TX_TABLE
	skpnc
	 incf	PCLATH,f
	movwf	PCL
TX_TABLE
	dt	"0123456789ABCDEF",0


setup:
	BANKSEL LATA
	bsf	LATA,0
;Bank 1
	BANKSEL TRISC
	clrf	OPTION_REG
	clrf	TRISC			;Port C outputs
;RRR	bcf	TRISA,0
    	bcf	SDO_TRIS	; SDO
    	bcf	SCK_TRIS	; SCK
	bsf	SDI_TRIS	; SDI
	movlw	9
	movwf	WDTCON			;WDT 16 ms
	movlw	0x78
	movwf	OSCCON			;fosc 16 MHz
;Bank 3
	BANKSEL ANSELC
	clrf	ANSELC			;disabling analog circuitry
	clrf	ANSELB
	clrf	ANSELA


;Bank 29
	; PPS OUTPUT PINS
	BANKSEL RC3PPS
	movlw	8
	movwf	RC3PPS			;COG main output (A) signal to RC3
	movlw	9
	movwf	RC4PPS			;COG complementary output (B) signal to RC4	
	movlw	0x0E
	movwf	RC5PPS			;PWM3OUT to RC5
        movlw   0x14			; Usart output pin
	movwf   RA0PPS
	movlw	0x10			; SCK
	movwf	RC0PPS
	movlw   0x12			; SDO
	movwf	RC2PPS
	BANKSEL TRISC
;; TEST RRR
    	bsf	SDO_TRIS	; SDO
    	bcf	SDO_TRIS	; SDO

	; PPS INPUT PINS
	BANKSEL RXPPS
	movlw   0x01			; RA1
        movwf	RXPPS			; Usart input pin
	movlw	0x11			; RC1
	movwf	SSPDATPPS		; SDI
	movlw   0x10			; RC0
	movwf   SSPCLKPPS		; SCK
	movlw	0x0c			; RB4
	movwf	SSPSSPPS		; not SS
    BANKSEL SSPCON1
    movlw       0x24    ; SSPEN | SPI master Fosc/16
    movwf       SSPCON1

	return

pwm:
;Bank 5
	BANKSEL CCPTMRS
	movlw	0x40
	movwf	CCPTMRS			;timer 2 for PWM 3
;Bank 12
	BANKSEL PWM3DCL
	movlw	0x80			;set PWM 3 Duty-Cycle
	movwf	PWM3DCL
	movlw	1
	movwf	PWM3DCH
	bsf	PWM3CON,PWM3EN		;enable PWM 3
;Bank 13
	BANKSEL COG1CON0
	clrf	COG1CON0
	movlw	(1<<G1POLB)
	movwf	COG1CON1		;COG1 clock invert COG1B
	movlw	0x40			; PWM3
	movwf	COG1RIS			;PWM3 signal as rising event of COG
	movwf	COG1RSIM		;PWM3 transition used for COG change
  	movlw	0x01			; COGxPPS
	movwf	COG1FIS			; signal as falling event of COG
	movwf	COG1FSIM		; transition used for COG change
	movlw	0x14
	movwf	COG1ASD0	;COG not in shutdown, Auto-restart disabled, tristate @ shutdown
	clrf	COG1ASD1
	movlw	0x0F
	movwf	COG1STR			;COG waveform, static low
	clrf	COG1DBR			;dead-band of rising edge 0
	clrf	COG1DBF			;dead-band of falling edge 0
	clrf	COG1BLKR		;blanking time of rising edge 0
	clrf	COG1BLKF		;blanking time of falling edge 0
	clrf	COG1PHR			;zero phase shift of rising edge 0
	clrf	COG1PHF			;zero phase shift of falling edge 0
	movlw	(1<<G1EN) | (1<<G1CS1) 
	movwf	COG1CON0		;Enable COG, HFINTOSC, steered PWM
	bsf	COG1ASD0,7		;COG shutdown
	bcf	COG1ASD0,7		;Exit COG shutdown
;Bank 0
	BANKSEL TMR2
	clrf	TMR2
	movlw	4
	movwf	PR2			;PWM 3 period (20 clock cycles)
	movlw	(1<<TMR2ON) | (1<<T2CKPS1)
	movwf	T2CON			;enable timer 2 with prescaler 16, postscaler 1
;**********************************************************************************

    clrwdt
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
    clrf   TMR0
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 40, '*** FAILED p16f1709 PR2+1 != 80'"
    nop
  .assert "(portc & 0x38) == 0x28, '***FAILED p16f1709 unexpected COG PWM=1'"
    nop
    btfsc  PORTC,5
    goto   $-1
   ; duty_cycle = 6 or 6 * prescale(16)/4 = 24 so T0 = (80 + 24)/2 = 52
  .assert "(tmr0 >= 51) && (tmr0 <= 53), '***FAILED p16f1709 CCP1 duty cycle after wrap'"
    nop
  .assert "(portc & 0x38) == 0x10, '***FAILED p16f1709 unexpected COG PWM=0'"
    nop

    bcf	T2CON,TMR2ON
    return








test_ssp
    BANKSEL	SSPSTAT	
    movlw	0xff
    movwf	SSPSTAT
  .assert "ssp1stat == 0xC0, '*** FAILED p16f1709 SPI sspstat only SMP, CKE writable'"
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

  .assert "(ssp1stat & 1) == 1, '*** FAILED MSSP SPI Master BF not set'"
    nop
    BANKSEL	SSPBUF
    movf	SSPBUF,W
  .assert "(ssp1stat & 1) == 0, '*** FAILED MSSP SPI Master BF not cleared'"
    nop
  .assert "W == 0x54, '*** FAILED MSSP SPI Master wrong data'"
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
    return

	end
