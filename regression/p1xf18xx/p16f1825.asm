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


        ;; The purpose of this program is to test gpsim's ability to 
        ;; simulate a pic 16F1825.
        ;; Specifically, basic port operation, eerom, interrupts,
	;; a2d, dac, SR latch, capacitor sense, and enhanced instructions


	list    p=16f1825                ; list directive to define processor
	include <p16f1825.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_OFF & _CLKOUTEN_OFF
        __CONFIG _CONFIG2, _STVREN_ON ; & _WRT_BOOT

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
cmif_cnt	RES	1
tmr0_cnt	RES	1
tmr1_cnt	RES	1
eerom_cnt	RES	1
adr_cnt		RES	1
data_cnt	RES	1
inte_cnt	RES	1
intp_cnt	RES	1
iocaf_val	RES	1

 GLOBAL inte_cnt, iocaf_val, intp_cnt


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlp  high  start               ; load upper byte of 'start' label
        goto   start                     ; go to beginning of program


  .sim "module library libgpsim_modules"
  ; Use a pullup resistor as a voltage source
  .sim "module load pullup V1"
  .sim "V1.resistance = 10000.0"
  .sim "V1.capacitance = 20e-12"
  .sim "V1.voltage=1.0"

  .sim "node n1"
  .sim "attach n1 porta0 porta3"
  .sim "node n2"
  .sim "attach n2 porta1 porta4"
  .sim "node n3"
  .sim "attach n3 porta2 porta5"
  .sim "node n4"
  .sim "attach n4 portc0 V1.pin"

  .sim "p16f1825.xpos = 72"
  .sim "p16f1825.ypos = 72"

  .sim "V1.xpos = 216"
  .sim "V1.ypos = 120"


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
	; many of the core registers now saved and restored automatically
                                                                                
	clrf	BSR		; set bank 0

	btfsc	PIR2,EEIF
	    goto ee_int

  	btfsc	INTCON,T0IF
	    goto tmr0_int

  	btfsc	INTCON,IOCIF
	    goto ioc_int
	btfsc   INTCON,INTF
	    goto intf_int

	.assert "'***FAILED p16f1825 unexpected interrupt'"
	nop


; Interrupt from TMR0
tmr0_int
	incf	tmr0_cnt,F
	bcf 	INTCON,T0IF
	goto	exit_int

; Interrupt from eerom
ee_int
	incf	eerom_cnt,F
	bcf 	PIR2,EEIF
	goto	exit_int

; Interrupt from IOC pin
ioc_int
	incf	inte_cnt,F
	bcf	INTCON,GIE	; stop interrupts
	goto	exit_int

; Interrupt from INT pin
intf_int
	incf	intp_cnt,F
	bcf	INTCON,INTF	; stop interrupts
	goto	exit_int

exit_int:
                                                                                
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
;
; Timer4 setup
;
        BANKSEL T4CON
        MOVLW   1<<TMR4ON|1<<T4OUTPS3|1<<T4OUTPS2|1<<T4OUTPS1|1<<T4OUTPS0|1<<T4CKPS1|1<<T4CKPS0
;        MOVWF   T4CON           ; store timer4 config data

	;set clock to 16 MHz
	BANKSEL OSCCON
	bsf 	OSCCON,6
	btfss	OSCSTAT,HFIOFL
	goto	$-1
   .assert "(oscstat & 0x1f) == 0x19,  '*** FAILED 16f1825 HFIO bit error'"
	nop
	call	cap_sense
	BANKSEL FVRCON
	; Enable Core Temp, high range, FVR AD 2.048v 
	movlw	(1<<TSEN)|(1<<TSRNG)|(1<<FVREN)|(1<<ADFVR1)
	movwf	FVRCON
	BANKSEL STKPTR
	movf	STKPTR,W
	;
	; Check PIR bits writable
	BANKSEL PIR1
	movlw	0xff
	movwf	PIR1
   .assert "pir1 == 0xcf,  '*** FAILED 16f1825  PIR1 writable bits'"
	nop
	clrf	PIR1
	movwf	PIR2
   .assert "pir2 == 0xf8,  '*** FAILED 16f1825  PIR2 writable bits'"
	nop
	clrf	PIR2
	;
	; Check PIE bits writable
	BANKSEL PIE1
	movlw	0xff
	movwf	PIE1
   .assert "pie1 == 0xff,  '*** FAILED 16f1825  PIE1 writable bits'"
	nop
	clrf	PIE1
	movwf	PIE2
   .assert "pie2 == 0xf8,  '*** FAILED 16f1825  PIE2 writable bits'"
	nop
	clrf	PIE2
;	;
	; test pins in analog mode return 0 on register read
	BANKSEL TRISA
	clrf	STATUS
	clrf	TRISA
   .assert "trisa == 0x08, '*** FAILED 16f1825  TRISA not clear except bit 3'"
	nop
	BANKSEL PORTA
	movlw	0xff
	movwf	PORTA
   .assert "porta == 0x28, '*** FAILED 16f1825  analog bits read 0'"
	nop
	movf	PORTA,W

;
; test PORTA works as expected
;
	clrf	PORTA
	BANKSEL ANSELA
	clrf	ANSELA	; set port to digital
	BANKSEL TRISA
	movlw	0x38
	movwf	TRISA		;PORTA 0,1,2 output 3,4,5 input

	call	sr_test
	clrf	BSR	; bank 0
  .assert "porta == 0x00, 'PORTA = 0x00'"
	nop
	movlw	0x07
	movwf	PORTA		; drive 0,1,2  bits high
  .assert "porta == 0x3f, 'PORTA = 0x3f'"
	nop
	BANKSEL LATA
	movf	LATA,W
	BANKSEL TRISA
	movlw	0x07
	movwf	TRISA  	; PORTA 4, 5 output 0,1,2,3 input (3 input only)
	BANKSEL PORTA
  .assert "porta == 0x09, 'PORTA = 0x09'"
	nop
	movlw	0x38
	movwf	PORTA		; drive output bits high
  .assert "porta == 0x3f, 'PORTA = 0x3f'"
	nop

	call test_eerom
	call test_ioc
	call test_intf
        call read_config_data
	call test_a2d
	call test_dac
	call test_t246
 	call write_prog

  .assert  "'*** PASSED 16f1825 Functionality'"
	nop
	reset
	goto	$

;
; Test operation of SR latch
;
sr_test:
	banksel	ANSELC
	clrf 	ANSELC
	banksel TRISC
	bcf 	TRISC,4
	banksel	SRCON1
	bsf 	SRCON1,SRSPE
	bsf 	SRCON0,SRLEN
	bsf 	SRCON0,SRQEN
	bsf 	SRCON0,SRNQEN
    .assert "(porta&0x04) == 0 && (portc&0x10) == 0x10, '*** FAILED p16f1825 RS Q=0'"
	nop
	banksel PORTA
	bsf 	PORTA,1
    .assert "(porta&0x04) == 4 && (portc&0x10) == 0x00, '*** FAILED p16f1825 RS Q=1'"
	nop
	banksel	SRCON1
	movlw	1<<SRRPE
	movwf	SRCON1
    .assert "(porta&0x04) == 0 && (portc&0x10) == 0x10, '*** FAILED p16f1825 RS Q=0 SRRPE'"
	nop
	bsf 	SRCON1,SRSPE
    .assert "(porta&0x04) == 0 && (portc&0x10) == 0x10, '*** FAILED p16f1825 RS Q=0 SRRPE & SRSPE'"
	nop
	clrf 	SRCON1
	bsf 	SRCON0,SRPS
    .assert "(porta&0x04) == 4 && (portc&0x10) == 0x00, '*** FAILED p16f1825 RS Q=1 SRPS'"
	nop
	bsf 	SRCON0,SRPR
    .assert "(porta&0x04) == 0 && (portc&0x10) == 0x10, '*** FAILED p16f1825 RS Q=0 SRPR'"
	nop

	bsf 	SRCON0,SRCLK2	; set clock every 16 instructions (64 Fosc)
	movlw	(1<<SRSCKE)
	movwf	SRCON1
	movlw	5	; loop takes 3 instruction cycles
	decfsz	WREG,W
	goto	$-1
	bsf 	SRCON1,SRRCKE	; set reset on clock (both set and reset)
	movlw	5	; loop takes 3 instruction cycles
	decfsz	WREG,W
	goto	$-1
	
	banksel PORTA
	bcf  	PORTA,1
	banksel	SRCON0
	bcf  	SRCON0,SRLEN
	clrf 	SRCON0
	clrf	SRCON1

	return

; Test Capacitor sense functionality
cap_sense:
	; Measure capacitor sense with 20pF + 5pF for analog pin
	; The time to charge a capacitor with a current source is
	; t = CV/I
	; for this test C = 25pF V = 3.8V I = 18 uA so t = 5.28 uSec
	; for 16MHz clock that is 21 instruction cycles
	; T0 run from Fosc/4
	BANKSEL OPTION_REG
	bcf	OPTION_REG,TMR0CS
	BANKSEL CPSCON0
	; CPS uses channel 4
	movlw	(1<<CPSCH2)
	movwf	CPSCON1
	; turn capacitance sense on, 18 uA, Internal volt ref
	movlw	(1<<CPSON)|(1<<CPSRNG1)|(1<<CPSRNG0)
	movwf	CPSCON0
	clrf 	TMR0
	btfss	CPSCON0,CPSOUT
	goto	$-1
   .assert "(tmr0 >= 21 && tmr0 < 25), '*** FAILED 16f1825 CPSCON TMR1 Counting'"
	nop
	; set tmr0 for RA4/T0CKI, but CPS not feeding clock
	BANKSEL OPTION_REG
	bsf	OPTION_REG,TMR0CS

	BANKSEL TMR0
	movlw	0x00
	movwf	TMR0
	; DAC non-idle output (0.16V)
	BANKSEL DACCON0
	movlw	(1<<DACEN)
	movwf	DACCON0
	movlw	0x01		; bottom of ladder
	movwf	DACCON1
  	; FVR = 4.065 
	movlw	(1<<FVREN)|(1<<CDAFVR1)|(1<<CDAFVR0)
	movwf	FVRCON
  .assert "tmr0 == 0x00, '*** FAILED 16f1825 CPSCON TMR0 Counting without T0XCS'"
	nop
	BANKSEL CPSCON0
	; Test T0 incrementing due to CPS and use T1 to measure time
	; t = 25pF * (4.065 - 0.16) / 30 uA = 3.254 uSec per slope
	;  or 26.04 cycles (16MHz)
	; So T1 at /8 = 0x341
	; turn capaitance sense on, in high range 30 uA to T0
	movlw	(1<<CPSON)|(1<<CPSRM)|(1<<CPSRNG1)|(1<<T0XCS)
	movwf	CPSCON0
;  	bcf 	INTCON,T0IF
;  	btfss	INTCON,T0IF
;	goto	$-1
	; clear t1 and set clock = Fosc/4, prescale 1:8
 	clrf 	TMR1L
	movlw	(1<<T1CKPS1)|(1<<T1CKPS0)|(1<<TMR1ON)
	movwf	T1CON
  	bcf 	INTCON,T0IF
  	btfss	INTCON,T0IF
	goto	$-1
    .assert "tmr1l == 0x40 && tmr1h == 3,'*** FAILED p16f1825 CPS with 25pF'"
	nop
    ; Add 5pF cap period should increase (frequency decrease) by ratio 30/25
    .command "V1.capacitance = 25e-12"
	nop
	clrf 	TMR1L
	clrf 	TMR1H
  	bcf 	INTCON,T0IF
  	btfss	INTCON,T0IF
	goto	$-1
    .assert "tmr1l == 0x00 && tmr1h == 4,'*** FAILED p16f1825 capacitance shift'"
	nop
	clrf	CPSCON0
	clrf 	T1CON
	BANKSEL	FVRCON
	clrf 	FVRCON
	clrf 	DACCON0
	return

test_dac:
	BANKSEL ADCON1      
	movlw   0xf0 		;right justify, Frc, Vdd ref
	movwf   ADCON1		
	BANKSEL TRISC		;
	bsf     TRISC,0		;Set RC0 to input
	BANKSEL ANSELC		;
	bsf     ANSELC,0	;Set RC0 to analog
	BANKSEL ADCON0      	;
	movlw   (0x1e<<2)|(1<<ADON)	;Select DAC output and ADC on
	movwf   ADCON0      	
	; DAC output should be Vsource- (0)
	call	a2dConvert
   .assert "(adresh==0x00) && (adresl==0x00), '*** FAILED 16f1825 DAC default'"
	nop
	; test DAC High idle state
	BANKSEL DACCON0
	movlw	(1<<DACLPS)	; high reference
	movwf	DACCON0
	movlw	0x1f		; top of ladder
	movwf	DACCON1
	BANKSEL ADCON0
	call	a2dConvert
   .assert "(adresh==0x03) && (adresl==0xff), '*** FAILED 16f1825 DAC High power off'"
	nop
	; test DAC enabled Vout = 5V * (16/32) = 2.5
	BANKSEL DACCON0
	movlw	0x10
	movwf	DACCON1
	movlw	(1<<DACEN)
	movwf	DACCON0
	BANKSEL ADCON0
	call	a2dConvert
   .assert "(adresh==0x02) && (adresl==0x00), '*** FAILED 16f1825 DAC enabled 1/2'"
	nop
  	; Test FVR = 4.065 as Vsource+ = 2.037
	BANKSEL	FVRCON
	movlw	(1<<FVREN)|(1<<CDAFVR1)|(1<<CDAFVR0)
	movwf	FVRCON
	movlw	(1<<DACEN)|(1<<DACPSS1)
	movwf	DACCON0
	BANKSEL ADCON0
	call	a2dConvert
   .assert "(adresh==0x01) && (adresl==0xa3), '*** FAILED 16f1825 DAC enabled 1/2 FVR '"
	nop
	


	return

test_eerom:
  ;
  ;	test can write and read to all 128 eeprom locations
  ;	using interrupts
        clrf    adr_cnt
        clrf    data_cnt
;  setup interrupts
        bsf     INTCON,PEIE
        bsf     INTCON,GIE
	BANKSEL PIE1
	bsf 	PIE2,EEIE
	BANKSEL	PIR1
;
;	write to EEPROM starting at EEPROM address 0
;	value of address as data using interrupts to
;	determine write complete. 
;	read and verify data

l1:     
        movf    adr_cnt,W
	clrf	eerom_cnt
	BANKSEL	EEADRL
        movwf   EEADRL 
        movf    data_cnt,W
        movwf   EEDATL
	bcf 	EECON1, CFGS  ;Deselect Configuration space
	bcf 	EECON1, EEPGD ;Point to DATA memory
	bsf 	EECON1, WREN  ;Enable writes


        bcf     INTCON,GIE      ;Disable interrupts while enabling write

        movlw   0x55            ;Magic sequence to enable eeprom write
        movwf   EECON2
        movlw   0xaa
        movwf   EECON2

        bsf     EECON1,WR      	;Begin eeprom write

        bsf     INTCON,GIE      ;Re-enable interrupts
	bcf 	EECON1, WREN 	;Disable writes

        
     ;;   clrf   BSR           ; Bank 0
        movf   eerom_cnt,W
	skpnz
        goto   $-2
;
;	read what we just wrote
;
	
        movf    adr_cnt,W

	BANKSEL	EEADRL
	movwf   EEADRL
	bcf EECON1, CFGS 	;Deselect Config space
	bcf EECON1, EEPGD	;Point to DATA memory

	bsf 	EECON1,RD	; start read operation
	movf	EEDATL,W	; Read data
	BANKSEL	PIR1

	xorwf	data_cnt,W	; did we read what we wrote ?
	skpz
	goto eefail

        incf    adr_cnt,W
        andlw   0x7f
        movwf   adr_cnt
	movwf	data_cnt

        skpz
         goto   l1

	return

test_a2d
	banksel ANSELC
	clrf	ANSELC
	BANKSEL ADCON1      
	movlw   0x70 		;Left justify, Frc, Vdd ref
	movwf   ADCON1		
	BANKSEL TRISC		;
	bsf     TRISC,0		;Set RC0 to input
	BANKSEL ANSELC		;
	bsf     ANSELC,0	;Set RC0 to analog
	BANKSEL ADCON0      	;
	movlw   0x10|(1<<ADON)	;Select channel AN4 and ADC on
	movwf   ADCON0      	
	call	a2dConvert
   .assert "adresh == 0x33, '*** FAILED 16f1825 AN4=1V'"
	nop
  ; measure Core Temperature
	bsf 	ADCON1,ADFM	; right justify result
        movlw	(0x1d << 2) | (1<<ADON) ; Core Temp channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x02) && (adresl==0x2f), '*** FAILED 16f1825 ADC Core Temp'"
	nop

  ; measure FVR 
        movlw	(0x1f << 2) | (1<<ADON) ; FVR channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x01) && (adresl==0xa3), '*** FAILED 16f1825 ADC FVR'"
	nop

  ; measure FVR using FVR Reference
	movlw	0xf3	; use FVR reference
	movwf	ADCON1
        movlw	(0x1f << 2) | (1<<ADON) ; FVR channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x03) && (adresl==0xff), '*** FAILED 16f1825 ADC FVR with FVR Ref'"
	nop

	return

;
;	Start A2D conversion and wait for results
;
a2dConvert
	bsf 	ADCON0,GO
	btfsc	ADCON0,GO
	goto	$-1
	movf	ADRESH,W
	return


eefail:
  .assert "'***FAILED 16f1825 eerom write/read error'"
	nop

test_tmr0:
	return
	
test_intf:	; test INT pin interrupts
	BANKSEL TRISA
	bsf     TRISA,2
        bcf     TRISA,5
	bsf	OPTION_REG,INTEDG	; raising edge interrupts
	BANKSEL PORTA
	clrf	PORTA
	bcf	INTCON,INTF
	bsf     INTCON,GIE      ;Global interrupts
        bcf     INTCON,PEIE     ;No Peripheral interrupts
	bsf	INTCON,INTE
        clrf    intp_cnt
        bsf     PORTA,5          ; make a rising edge
        nop
   .assert "intp_cnt == 0x01, '*** FAILED 16f1825 INT test - missing int on rising edge'"
	nop
	return






test_ioc:
	BANKSEL TRISA
	bsf     TRISA,2
        bcf     TRISA,5
	BANKSEL PORTA
	clrf	PORTA
	BANKSEL IOCAP
	bsf 	IOCAP,2		; set interrupt on + edge of porta2

	BANKSEL OPTION_REG
	bsf 	OPTION_REG,INTEDG
	BANKSEL IOCAF
	movlw	0xff
	movwf	IOCAF
   .assert "iocaf == 0x3f, '*** FAILED 16f1825 IOC test - IOCAF writable bits'"
	nop
	clrf	IOCAF		; Clear intcon IOCIF flag 
	movlw	0x7f
	movwf	INTCON
   .assert "intcon == 0x7e, '*** FAILED 16f1825 IOC test - INTCON:IOCIF read only'"
	nop
	clrf	INTCON
        bsf     INTCON,GIE      ;Global interrupts
        bcf     INTCON,PEIE     ;No Peripheral interrupts
        bsf     INTCON,IOCIE

	BANKSEL PORTA
        clrf    inte_cnt
        bsf     PORTA,5          ; make a rising edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, '*** FAILED 16f1825 INT test - No int on rising edge'"
        nop
   .assert "iocaf == 0x04, '*** FAILED 16f1825 IOCAF bit 2 not set'"
	nop
	bcf	INTCON,IOCIF
   .assert "(intcon & 1) == 1, '*** FAILED 16f1825 INTCON,IOCIF read only'"
	nop 
	BANKSEL	IOCAF
	bcf	IOCAF,2
   .assert "(intcon & 1) == 0, '*** FAILED 16f1825 INTCON,IOCIF not cleared from IOCAF'"
	nop 
	clrf	inte_cnt
	bsf	INTCON,GIE	; Turn interrupts back on
	BANKSEL	PORTA
        bcf     PORTA,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "inte_cnt == 0x00, '*** FAILED 16f1825 IOC test - Unexpected int on falling edge'"
        nop


;	Setup - edge interrupt
	BANKSEL IOCAP
	clrf	IOCAP
	bsf 	IOCAN,2
	clrf	IOCAF
	BANKSEL INTCON

        clrf    inte_cnt
	clrf	iocaf_val
        bsf     PORTA,5          ; make a rising edge
        movf    inte_cnt,w
   .assert "W == 0x00, '*** FAILED 16f1825 IOC test - Unexpected int on rising edge'"
        nop
        clrf    inte_cnt
        bcf     PORTA,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, '*** FAILED 16f1825 IOC test - No int on falling edge'"
        nop
   .assert "iocaf == 0x04, '*** FAILED 16f1825 IOCAF bit 2 not set'"
	nop
	BANKSEL	IOCAF
	CLRF	IOCAF
	bsf	INTCON,GIE
	BANKSEL	PORTA
        return

; read STKPTR, TOSL and cause underflow
rrr:
	nop
	BANKSEL STKPTR
	movf 	STKPTR,W
	movf	TOSL,W
  ;	clrf	TOSL
	call	rrr2
	return

; use STKPTR to cause stack underflow
rrr2:
	BANKSEL STKPTR
	movlw	0x1f
	movwf	STKPTR	;; cause stack underflow
	return

; overflow stack with recursive call
rrr3:
	call rrr3
	return
	

;
; Test reading and writing Configuration data via eeprom interface
read_config_data:
	;Read DeviceID at address 0x06 from config data
	BANKSEL  EEADRL            ; Select correct Bank
	movlw	0x06              ;
	movwf	EEADRL            ; Store LSB of address
	clrf	EEADRH            ; Clear MSB of address
	bsf 	EECON1,CFGS       ; Select Configuration Space
	bcf 	INTCON,GIE        ; Disable interrupts
	bsf 	EECON1,RD         ; Initiate read
	nop
	nop
	bsf 	INTCON,GIE        ; Restore interrupts
   .assert "eedath == 0x27 && eedatl == 0x60, '*** FAILED 16f1825 Device ID'"
	nop
	
  ; test write which is in EEDATAH and EEDATAL to userID1
	BANKSEL PIR2
	clrf	PIR2
	BANKSEL EECON1
	movlw	0x01              ;
	movwf	EEADRL            ; Store LSB of address
	bsf 	EECON1, WREN  	   ;Enable writes
	bcf 	INTCON,GIE	   ; disable interrupts
        movlw	0x55               ;Magic sequence to enable eeprom write
        movwf	EECON2
        movlw	0xaa
        movwf	EECON2
	bsf 	EECON1,WR
	nop
	nop
	bsf  	INTCON,GIE
	bcf 	EECON1, WREN  ;Enable writes
	BANKSEL PIR2
	btfss	PIR2,EEIF
        goto	$-1
  .assert "UserID2 == 0x2760, '*** FAILED 16f1825 write to UserID2'"
	nop
	return

clear_prog:
; This row erase routine assumes the following:
; 1. A valid address within the erase block is loaded in ADDRH:ADDRL
; 2. ADDRH and ADDRL are located in shared data memory 0x70 - 0x7F
	bcf       INTCON,GIE     ; Disable ints so required sequences will execute properly
        BANKSEL   EEADRL
 	movlw	0
        movwf   EEADRL
	movlw	3
        movwf   EEADRH
        bsf     EECON1,EEPGD   ;   Point to program memory
        bcf     EECON1,CFGS    ;   Not configuration space
        bsf     EECON1,FREE    ;   Specify an erase operation
        bsf     EECON1,WREN    ;   Enable writes
        movlw   0x55           ;   Start of required sequence to initiate erase
        movwf   EECON2
        movlw   0xAA           ;
        movwf   EECON2
        bsf     EECON1,WR      ;   Set WR bit to begin erase
        nop
        nop
        bcf     EECON1,WREN    ; Disable writes
        bsf 	INTCON,GIE     ; Enable interrupts
	btfsc 	EECON1,WR
        goto   $-2
	return

write_prog:

	bcf 	INTCON,GIE      ;   Disable ints so required sequences will execute properly
	BANKSEL	EEADRH          ;   Bank 3
	movlw	0x03
	movwf	EEADRH          ;
	movlw	0x00
	movwf	EEADRL          ;
	movlw	0x00
	movwf	FSR0L           ;
	movlw	0x20		;   Program memory
	movwf	FSR0H           ;
	bsf 	EECON1,EEPGD    ;   Point to program memory
	bcf 	EECON1,CFGS     ;   Not configuration space
	bsf 	EECON1,WREN     ;   Enable writes
	bsf 	EECON1,LWLO     ;   Only Load Write Latches
LOOP
	moviw	FSR0++          ; Load first data byte into lower
	movwf	EEDATL          ;
	moviw	FSR0++          ; Load second data byte into upper
	movwf	EEDATH          ;
	movf	EEADRL,W        ; Check if lower bits of address are '000'
	xorlw	0x07            ; Check if we're on the last of 8 addresses
	andlw	0x07            ;
	btfsc	STATUS,Z        ; Exit if last of eight words,
	goto	START_WRITE     ;
	movlw	0x55            ;   Start of required write sequence:
	movwf	EECON2
	movlw	0xAA
	movwf	EECON2
	bsf 	EECON1,WR       ;   Set WR bit to begin write
	nop
	nop
	incf	EEADRL,F        ; Still loading latches Increment address
	goto	LOOP            ; Write next latches
START_WRITE
	bcf 	EECON1,LWLO     ; No more loading latches - Actually start Flash program
	;	memory write
	movlw	0x55             ;   Start of required write sequence:
	movwf	EECON2
	movlw	0xAA            ;
	movwf	EECON2
	bsf 	EECON1,WR       ;   Set WR bit to begin write
	nop
	nop
	bcf 	EECON1,WREN     ; Disable writes
	bsf 	INTCON,GIE      ; Enable interrupts
	return


test_t246:
	
 .sim "symbol cycleCounter=0"

	clrwdt
	BANKSEL	T2CON
	clrf	PR2
;	movlw	0xff
;	movwf	PR2
	MOVLW	1<<TMR2ON|1<<T2OUTPS3|1<<T2OUTPS2|1<<T2OUTPS1|1<<T2OUTPS0|1<<T2CKPS1|1<<T2CKPS0
  .command "cycleCounter = cycles" 
	nop
	MOVWF	T2CON		; store timer2 config data
	BANKSEL PIR1
	bcf	PIR1,TMR2IF
	btfss	PIR1,TMR2IF
	goto	$-1
   .command  "cycleCounter = cycles - cycleCounter" 
	nop
   .assert "(cycleCounter >= 1024) && (cycleCounter <= 1030), '*** FAILED 16f1825 T2 count'"
	nop
	BANKSEL T2CON
	bcf	T2CON,TMR2ON
	NOP
	clrwdt
	BANKSEL	T4CON
	MOVLW	1<<TMR4ON|1<<T4OUTPS3|1<<T4OUTPS2|1<<T4OUTPS1|1<<T4OUTPS0|1<<T4CKPS1|1<<T4CKPS0
  .command "cycleCounter = cycles" 
	nop
	MOVWF	T4CON		; store timer4 config data
	BANKSEL PIR3
	bcf	PIR3,TMR4IF
	btfss	PIR3,TMR4IF
	goto	$-1
   .command  "cycleCounter = cycles - cycleCounter" 
	nop
   .assert "(cycleCounter >= 262144) && (cycleCounter <= 262150), '*** FAILED 16f1825 T4 count'"
	nop
	BANKSEL T4CON
	bcf	T4CON,TMR4ON
	NOP
	BANKSEL	T6CON
;	postscale = 8, prescale = 64 PR6 = 255 so cycles  8 * 64 *256 = 131072
	MOVLW	1<<TMR6ON|1<<T6OUTPS2|1<<T6OUTPS1|1<<T6OUTPS0|1<<T6CKPS1|1<<T6CKPS0
	clrwdt
  .command "cycleCounter = cycles" 
	nop
	MOVWF	T6CON		; store timer6 config data
	BANKSEL PIR3
	bcf	PIR3,TMR6IF
	btfss	PIR3,TMR6IF
	goto	$-1
   .command  "cycleCounter = cycles - cycleCounter" 
	nop
   .assert "(cycleCounter >= 131072) && (cycleCounter <= 131076), '*** FAILED 16f1825 T6 count'"
	nop
	return 

 	org 0x400
rrDATA
	dw 0x01, 0x02, 0x03
  end
