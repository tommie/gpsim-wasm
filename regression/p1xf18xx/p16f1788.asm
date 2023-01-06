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
        ;; simulate a pic 16F1788.
        ;; Specifically, basic port operation, eerom, interrupts,
	;; a2d, dac, SR latch, capacitor sense, and enhanced instructions


	list    p=16f1788                ; list directive to define processor
	include <p16f1788.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_ON & _CLKOUTEN_OFF
        __CONFIG _CONFIG2, _STVREN_ON ; & _WRT_BOOT

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm

TSEN  EQU H'0005'
TSRNG EQU H'0004'

;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
cmif_cnt	RES	1
tmr0_cnt	RES	1
tmr1_cnt	RES	1
eerom_cnt	RES	1
adr_cnt		RES	1
data_cnt	RES	1
inte_cnt	RES	1
iocaf_val	RES	1

  GLOBAL iocaf_val

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
  .sim "attach n4 portb0 V1.pin"
  .sim "node n5"
  .sim "attach n5 porta6 porta7"
  .sim "node n6"
  .sim "attach n6 portb1 portb7"

  .sim "p16f1788.xpos = 72"
  .sim "p16f1788.ypos = 72"

  .sim "V1.xpos = 260"
  .sim "V1.ypos = 120"


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
	; many of the core registers now saved and restored automatically
                                                                                
;        movwf   w_temp
;        swapf   STATUS,W
	clrf	BSR		; set bank 0
;        movwf   status_temp

	btfsc	PIR2,EEIF
	    goto ee_int

  	btfsc	INTCON,T0IF
	    goto tmr0_int

  	btfsc	INTCON,IOCIF
	    goto inte_int

	.assert "'***FAILED p16f1788 unexpected interrupt'"
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

; Interrupt from INT pin
inte_int
	incf	inte_cnt,F
	BANKSEL	IOCAF
	movf	IOCAF,W
	movwf	iocaf_val
	xorlw	0xff
	andwf 	IOCAF, F
	goto	exit_int

exit_int:
                                                                                
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
	;set clock to 16 Mhz
	BANKSEL OSCCON
	bsf 	OSCCON,6

	call  test_pir_pie_bits

	BANKSEL FVRCON
	; Enable Core Temp, high range, FVR AD 2.048v 
	movlw	(1<<TSEN)|(1<<TSRNG)|(1<<FVREN)|(1<<ADFVR1)
	movwf	FVRCON
	BANKSEL STKPTR
	movf	STKPTR,W
;	;
	; test pins in analog mode return 0 on register read
	BANKSEL TRISA
	clrf	STATUS
	clrf	TRISA
   .assert "trisa == 0x00, '*** FAILED 16f1788  TRISA not clear '"
	nop
	BANKSEL PORTA
	movlw	0xff
	movwf	PORTA
   .assert "porta == 0xe8, '*** FAILED 16f1788  analog bits read 0'"
	nop
	movf	PORTA,W

;
; test PORTA works as expected
;
	clrf	PORTA
	BANKSEL ANSELA
	clrf	ANSELA	; set port to digital
	bcf	ANSELB,0
	BANKSEL TRISA
	movlw	0x38
	movwf	TRISA		;PORTA 0,1,2 output 3,4,5 input

	clrf	BSR	; bank 0
  .assert "porta == 0x00, 'PORTA = 0x00'"
	nop
	movlw	0x07
	movwf	PORTA		; drive 0,1,2  bits high
	bsf	PORTC,7
  .assert "porta == 0x3f, 'PORTA = 0x3f'"
	nop
	BANKSEL LATA
	movf	LATA,W
	BANKSEL TRISA
	movlw	0x07
	movwf	TRISA  	; PORTA 3, 4, 5 output 0,1,2 input
	BANKSEL PORTA
  .assert "porta == 0x00, 'PORTA = 0x00'"
	nop
	movlw	0x38
	movwf	PORTA		; drive output bits high
  .assert "porta == 0x3f, 'PORTA = 0x3f'"
	nop

	call test_eerom
	call test_int
        call read_config_data
	call test_a2d
	call test_dac
	call test_a2d_ref
 	call write_prog

  .assert  "'*** PASSED 16f1788 Functionality'"
	nop
	reset
	goto	$

;
;	Test A/D Vref-pin and differential input
;
test_a2d_ref:
	BANKSEL ANSELA
	bsf	ANSELA,2
	bsf	ANSELA,1
	BANKSEL	TRISA
	bsf	TRISA,5
	BANKSEL	DAC1CON0
	; Pin a2 is both DAC1 output and ADC Vref-pin
	movlw	(1<<DAC1EN)|(1<<DACOE1)
	movwf	DAC1CON0
	movlw	0x10	; Ladder 16/256 * 5V = 0.3125 V
	movwf	DAC1CON1
	BANKSEL	ADCON0
	movlw	(1<<ADFM)|(7 << 4) | (1<<ADNREF) ; 2's comp, Frc, Vref-pin
	movwf	ADCON1 
	movlw	0xf		; V- = Vref-pin
	movwf	ADCON2		
	movlw	(1<<ADON)|(0xc<<2)	; V+ = AN12
	movwf   ADCON0      	
	; DAC output  (1V - 0.3125) /(5-0.3125)
	call	a2dConvert
   .assert "adresh == 0x02 && (adresl==0x59), '*** FAILED 16f1788 AN12=1V V-=Vref-pin'"
	nop

  	; Test Vdd = 5.00 as Vsource+ = 2.500 DAC4 (5 bit)
	BANKSEL DAC4CON0
	movlw	(1<<DACEN)|(1<<DAC4OE1)
	movwf	DAC4CON0
	movlw	0x10		; ladder 16/32
	movwf	DAC4CON1
	BANKSEL	ADCON0
	movlw	(1<<ADFM)|(7 << 4) | (1<<ADNREF) ; 2's comp, Frc, Vref-pin
	movwf	ADCON1 
	movlw	0x1		; V- =  AN1 (2.5 V)
	movwf	ADCON2		
	movlw	(1<<ADON)|(0xc<<2)	; V+ = AN12
	movwf   ADCON0      	
	call	a2dConvert
   .assert "adresh == 0xfa && (adresl==0xe5), '*** FAILED 16f1788 V+=AN12=1V V-=AN1=2.5V 2's comp'"
	nop
	BANKSEL	ADCON0
	movlw	(7 << 4) | (1<<ADNREF) ; sign-mag, Frc, Vref-pin
	movwf	ADCON1 
	movlw	0x1			; V- =  AN1
	movwf	ADCON2		
	movlw	(1<<ADON)|(0xc<<2)|(1<<ADRMD)	; V+ = AN12
	movwf   ADCON0      	
	call	a2dConvert
   .assert "adresh == 0x51 && (adresl==0x81), '*** FAILED 16f1788 V+=AN12=1V V-=AN1=2.5V sign-mag 10bit'"
	nop
	BANKSEL DAC4CON0
	clrf	DAC4CON0
	BANKSEL DAC1CON0
	clrf	DAC1CON0
	return


test_pir_pie_bits:
	BANKSEL PIR1
	movlw	0xff
	movwf	PIR1
   .assert "pir1 == 0xcf, '*** FAILED 16f1788 PIR1 write test'"
	nop
	clrf	PIR1
	movwf	PIR2
   .assert "pir2 == 0xff, '*** FAILED 16f1788 PIR2 write test'"
	nop
	clrf	PIR2
	movwf	PIR3
   .assert "pir3 == 0x10, '*** FAILED 16f1788 PIR3 write test'"
	nop
	clrf	PIR3

	BANKSEL	PIE1
	movwf	PIE1
   .assert "pie1 == 0xff, '*** FAILED 16f1788 PIE1 write test'"
	nop
	clrf	PIE1
	movwf	PIE2
   .assert "pie2 == 0xff, '*** FAILED 16f1788 PIE2 write test'"
	nop
	clrf	PIE2
	movwf	PIE3
   .assert "pie3 == 0x10, '*** FAILED 16f1788 PIE3 write test'"
	nop
	clrf	PIE3
	return
test_dac:
	BANKSEL ADCON1      
	movlw   (1<<ADFM)| 0x70 ;A2D 2's comp, Frc, Vdd ref+, Vss ref-
	movwf   ADCON1		
	BANKSEL TRISC		;
	bsf     TRISC,0		;Set RC0 to input
	BANKSEL ANSELC		;
	bsf     ANSELC,0	;Set RC0 to analog
	BANKSEL ADCON0      	;
	movlw   (0x1e<<2)|(1<<ADON)	;Select A2D input from DAC1  and ADC on
	movwf   ADCON0      	
	; DAC output should be Vsource- (0)
	call	a2dConvert
   .assert "(adresh==0x00) && (adresl==0x00), '*** FAILED 16f1788 DAC default'"
	nop
	; test DAC enabled Vout = 5V * (128/256) = 2.5
	BANKSEL DAC1CON0
	movlw	0x80
	movwf	DAC1CON1
	movlw	(1<<DACEN)
	movwf	DAC1CON0
	BANKSEL ADCON0
	call	a2dConvert
   .assert "(adresh==0x08) && (adresl==0x00), '*** FAILED 16f1788 DAC enabled 1/2'"
	nop
  	; Test  FVR = 4.096 as Vsource+ = 2.048
	BANKSEL	FVRCON
	movlw	(1<<FVREN)|(1<<CDAFVR1)|(1<<CDAFVR0)
	movwf	FVRCON
	movlw	(1<<DACEN) | (1<<DACPSS1)
	movwf	DAC1CON0
	BANKSEL ADCON0
	call	a2dConvert
   .assert "(adresh==0x06) && (adresl==0x8d), '*** FAILED 16f1788 DAC enabled 1/2 FVR '"
	nop
	banksel TRISB
	bsf	TRISB,7
	clrf	TRISE
	
  	; Test Vdd = 5.00 as Vsource+ = 2.500 DAC4 (5 bit)
	BANKSEL DAC4CON0
	movlw	(1<<DACEN)|(1<<DAC4OE1) | (1<<DAC4OE2)
	movwf	DAC4CON0
	movlw	0x10		; ladder 16/32
	movwf	DAC4CON1
	BANKSEL ADCON0
	movlw   (0x18<<2)|(1<<ADON)	;Select A2D input from DAC4  and ADC on
	movwf   ADCON0      	
	call	a2dConvert
   .assert "(adresh==0x08) && (adresl==0x00), '*** FAILED 16f1788 DAC4 enabled 1/2 Vdd '"
	nop
	

	; Read DAC4 from output pin. (will be slightly lower than 2.500 V)
	BANKSEL ADCON0
	movlw   (0x0a<<2)|(1<<ADON)	;Select A2D input from DAC4 output pin
	movwf   ADCON0      	
	call	a2dConvert
   .assert "(adresh==0x07) && (adresl==0xfd), '*** FAILED 16f1788 DAC4 output pin'"
	nop
	

	BANKSEL DAC4CON0
	clrf	DAC4CON0

	return

test_eerom:
  ;
  ;	test can write and read to all 128 eeprom locations
  ;	using intterupts
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
	movlw   0x70 		;Sign-magnatude, Frc, Vdd ref+, Vss ref-
	movwf   ADCON1		
	movlw	0x0f		;negative input is negative reference
	movwf	ADCON2
	BANKSEL TRISC		;
	bsf     TRISC,0		;Set RC0 to input
	BANKSEL ANSELC		;
	bsf     ANSELC,0	;Set RC0 to analog
	BANKSEL ADCON0      	;
	movlw   (0xc << 2)|(1<<ADON)	;12bit, Select channel AN12 and ADC on
	movwf   ADCON0      	
	call	a2dConvert
   .assert "adresh == 0x33 && (adresl==0x30), '*** FAILED 16f1788 AN12=1V'"
	nop
  ; measure Core Temperature (v= 2.733 for 30C)
	bsf 	ADCON1,ADFM	;  2's compliment (right justified)
        movlw	(0x1d << 2) | (1<<ADON) ; Core Temp channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x08) && (adresl==0xbf), '*** FAILED 16f1788 ADC Core Temp'"
	nop

  ; measure FVR (2.048)
        movlw	(0x1f << 2) | (1<<ADON) ; FVR channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x06) && (adresl==0x8d), '*** FAILED 16f1788 ADC FVR'"
	nop

  ; measure FVR using FVR Reference
	movlw	0xf3	; use FVR reference
	movwf	ADCON1
        movlw	(0x1f << 2) | (1<<ADON) ; FVR channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x0f) && (adresl==0xff), '*** FAILED 16f1788 ADC FVR with FVR Ref'"
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
  .assert "'***FAILED 16f1788 eerom write/read error'"
	nop

test_tmr0:
	return
	





test_int:
	BANKSEL TRISA
	bsf     TRISA,2
        bcf     TRISA,5
	BANKSEL PORTA
	clrf	PORTA
	BANKSEL IOCAP
	bsf 	IOCAP,2		; set interrupt on + edge of porta2

	BANKSEL OPTION_REG
	bsf 	OPTION_REG,INTEDG
	BANKSEL INTCON
	movlw	0x7f
	movwf	INTCON
   .assert "intcon == 0x7e, '*** FAILED 16f1788 INT test - INTCON:IOCIF read only'"
	nop
	BANKSEL	IOCAF 
	movlw	0xff
	movwf	IOCAF
 .assert "iocaf == 0xff, '*** FAILED 16f1788 INT test - IOCAF writable bits'"
	nop
	clrf	IOCAF
	movwf	IOCBF
 .assert "iocbf == 0xff, '*** FAILED 16f1788 INT test - IOCBF writable bits'"
	nop
	clrf	IOCBF
	movwf	IOCCF
 .assert "ioccf == 0xff, '*** FAILED 16f1788 INT test - IOCCF writable bits'"
	nop
	clrf	IOCCF
	movwf	IOCEF
 .assert "iocef == 0x08, '*** FAILED 16f1788 INT test - IOCEF writable bits'"
	nop
	clrf	IOCEF
	clrf	INTCON
        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,IOCIE

 	BANKSEL PORTA

        clrf    inte_cnt
        bsf     PORTA,5          ; make a rising edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, '*** FAILED 16f1788 INT test - No int on rising edge'"
        nop
   .assert "iocaf_val == 0x04, '*** FAILED 16f1788 IOCAF bit 2 not set'"
	nop
        clrf    inte_cnt
        bcf     PORTA,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x00, '*** FAILED 16f1788 INT test - Unexpected int on falling edge'"
        nop


;	Setup - edge interrupt
	BANKSEL IOCAP
	clrf	IOCAP
	bsf 	IOCAN,2
	movlw	0xff
	clrf	IOCAF
	BANKSEL INTCON
        bcf     INTCON,IOCIF     ;Clear flag

        clrf    inte_cnt
        bsf     PORTA,5          ; make a rising edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x00, '*** FAILED 16f1788 INT test - Unexpected int on rising edge'"
        nop
        clrf    inte_cnt
        bcf     PORTA,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, '*** FAILED 16f1788 INT test - No int on falling edge'"
        nop
   .assert "iocaf_val == 0x04, '*** FAILED 16f1788 IOCAF bit 2 not set'"
	nop


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
   .assert "eedath == 0x30 && eedatl == 0x2b, '*** FAILED 16f1788 Device ID'"
	nop
	
  ; test write which is in EEDATAH and EEDATAL to userID1
	BANKSEL PIR2
	clrf	PIR2
	BANKSEL EECON1
	movlw	0x01              ;
	movwf	EEADRL            ; Store LSB of address
	bsf 	EECON1, WREN  	   ;Enable writes
	bcf 	INTCON,GIE	   ; disable interupts
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
  .assert "UserID2 == 0x302b, '*** FAILED 16f1788 write to UserID2'"
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

 	org 0x200
rrDATA
	dw 0x01, 0x02, 0x03
  end
