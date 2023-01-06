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
        ;; simulate a pic 12F1840.


	list    p=12f1840          ; list directive to define processor
	include <p12f1840.inc>     ; processor specific variable definitions
        include <coff.inc>         ; Grab some useful macros

        __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_OFF & _CLKOUTEN_OFF
        __CONFIG _CONFIG2, _STVREN_ON ; & _WRT_BOOT

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm

    GLOBAL temp, temp2, temp3, A0

;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
temp            RES     1
w_temp          RES     1
temp2           RES     1
temp3           RES     1
status_temp     RES     1
cmif_cnt	RES	1
tmr0_cnt	RES	1
tmr1_cnt	RES	1
eerom_cnt	RES	1
adr_cnt		RES	1
data_cnt	RES	1
inte_cnt	RES	1
iocaf_val	RES     1

GPR_DATA2	UDATA	0xa0
A0		RES	1

  GLOBAL iocaf_val

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlp  high  start               ; load upper byte of 'start' label
        goto   start                     ; go to beginning of program


  .sim "node n1"
  .sim "attach n1 porta0 porta3"
  .sim "node n2"
  .sim "attach n2 porta1 porta4"
  .sim "node n3"
  .sim "attach n3 porta2 porta5"
  .sim "p12f1840.BreakOnReset = false"
  .sim "log w W"
  .sim "log r W"
  ;.sim "log lxt"
  .sim "log on"


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
	    goto inte_int

	.assert "'***FAILED p12f1840 unexpected interrupt'"
	nop


; Interrupt from TMR0
tmr0_int
	incf	tmr0_cnt,F
	bcf	INTCON,T0IF
	goto	exit_int

; Interrupt from eerom
ee_int
	incf	eerom_cnt,F
	bcf	PIR2,EEIF
	goto	exit_int

; Interrupt from INT pin
inte_int
	incf	inte_cnt,F
	BANKSEL IOCAF
	movf	IOCAF,W
	movwf	iocaf_val
	xorlw	0xff
	andwf 	IOCAF, F
	goto	exit_int

exit_int:
                                                                                
;; STATUS and W now restored by RETFIE
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
        BANKSEL PCON
        btfss   PCON,NOT_RI
        goto    soft_reset

	BANKSEL STKPTR
	movf	STKPTR,W
;	;
	; test pins in analog mode return 0 on register read
	BANKSEL TRISA
	clrf	STATUS
	clrf	TRISA
   .assert "trisa == 0x08, '**FAILED 12f1840  TRISA clears to 0x08'"
	nop
	BANKSEL PORTA
	movlw	0xff
	movwf	PORTA
   .assert "porta == 0x28, '**FAILED 12f1840  analog bits read 0'"
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
	call test_int
        call read_config_data
	call test_indr
	reset

soft_reset:
  .assert "cycles > 100, '*** FAILED 12f1840 Unexpected soft reset'"
	nop
  .assert  "'*** PASSED 12f1840 Functionality'"
	nop
	goto	$






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
	bsf	PIE2,EEIE
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

	bsf	EECON1,RD	; start read operation
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

eefail:
  .assert "'***FAILED 12f1840 eerom write/read error'"
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
	bsf	IOCAP,2		; set interrupt on + edge of porta2
	movlw	0xff
	movwf	IOCAF
   .assert "iocaf == 0x3f, '*** FAILED 12f1840 INT test - IOCAF writable bits'"
	nop
	clrf	IOCAF

	BANKSEL OPTION_REG
	bsf	OPTION_REG,INTEDG
	BANKSEL INTCON
        bsf     INTCON,GIE      ;Global interrupts
        bcf     INTCON,PEIE     ;No Peripheral interrupts
        bsf     INTCON,IOCIE

        clrf    inte_cnt
        bsf     PORTA,5          ; make a rising edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, '*** FAILED 12f1840 INT test - No int on rising edge'"
        nop
   .assert "iocaf_val == 0x04, '*** FAILED 12f1840 IOCAF bit 2 not set'"
	nop
        clrf    inte_cnt
        bcf     PORTA,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x00, '*** FAILED 12f1840 INT test - Unexpected int on falling edge'"
        nop


;	Setup - edge interrupt
	BANKSEL IOCAP
	clrf	IOCAP
	bsf	IOCAN,2
	clrf	IOCAF

	BANKSEL	PORTA

        clrf    inte_cnt
        bsf     PORTA,5          ; make a rising edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x00, '*** FAILED 12f1840 INT test - Unexpected int on rising edge'"
        nop
        clrf    inte_cnt
        bcf     PORTA,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, '*** FAILED 12f1840 INT test - No int on falling edge'"
        nop
   .assert "iocaf_val == 0x04, '*** FAILED 12f1840 IOCAF bit 2 not set'"
	nop
        return

;
; Test reading and writing Configuration data via eeprom interface
;
read_config_data:
	BANKSEL  EEADRL            ; Select correct Bank
	movlw    0x06              ;
	movwf    EEADRL            ; Store LSB of address
	clrf     EEADRH            ; Clear MSB of address
	bsf      EECON1,CFGS       ; Select Configuration Space
	bcf      INTCON,GIE        ; Disable interrupts
	bsf      EECON1,RD         ; Initiate read
	nop
	nop
	bsf      INTCON,GIE        ; Restore interrupts
   .assert "eedath == 0x1b && eedatl == 0x80, '*** FAILED 12f1840 Device ID'"
        nop

  ; test write
	BANKSEL PIR2
	clrf	PIR2
	BANKSEL EECON1
	movlw    0x01              ;
	movwf    EEADRL            ; Store LSB of address
	bsf 	EECON1, WREN  ;Enable writes
	bcf	INTCON,GIE
        movlw   0x55            ;Magic sequence to enable eeprom write
        movwf   EECON2
        movlw   0xaa
        movwf   EECON2
	bsf	EECON1,WR
	nop
	nop
	bsf	INTCON,GIE
	bcf 	EECON1, WREN  ;Enable writes
	BANKSEL PIR2
	btfss 	PIR2,EEIF
        goto    $-1
  .assert "UserID2 == 0x1b80, '*** FAILED 12f1840 write to UserID2'"
        nop
	return

clear_prog:
; This row erase routine assumes the following:
; 1. A valid address within the erase block is loaded in ADDRH:ADDRL
; 2. ADDRH and ADDRL are located in shared data memory 0x70 - 0x7F
        bcf       INTCON,GIE     ; Disable ints so required sequences will execute properly
        BANKSEL   EEADRL
	movlw	0
        movwf	EEADRL
	movlw	1
        movwf   EEADRH
        bsf     EECON1,EEPGD   ;   Point to program memory
        bcf     EECON1,CFGS    ;   Not configuration space
        bsf     EECON1,FREE    ;   Specify an erase operation
        bsf     EECON1,WREN    ;   Enable writes
        movlw   0x55           ;   Start of required sequence to initiate erase
        movwf   EECON2
        movlw   0xAA 
        movwf   EECON2
        bsf     EECON1,WR      ;   Set WR bit to begin erase
        nop
        nop
        bcf     EECON1,WREN    ; Disable writes
        bsf     INTCON,GIE     ; Enable interrupts
	btfsc 	EECON1,WR
        goto    $-1
	return

write_prog:

	bcf	INTCON,GIE      ;   Disable ints so required sequences will execute properly
	BANKSEL	EEADRH          ;   Bank 3
 ;	movf	ADDRH,W         ;   Load initial address
	movlw	0x03
	movwf	EEADRH          ;
;	movf	ADDRL,W         ;
	movlw	0x00
	movwf	EEADRL          ;
	movlw	0x00
;	movlw	LOW DATA_ADDR   ;   Load initial data address
	movwf	FSR0L           ;
	movlw	0x20		;   Program memory
;	movlw	HIGH DATA_ADDR  ;   Load initial data address
	movwf	FSR0H           ;
	bsf	EECON1,EEPGD    ;   Point to program memory
	bcf	EECON1,CFGS     ;   Not configuration space
	bsf	EECON1,WREN     ;   Enable writes
	bsf	EECON1,LWLO     ;   Only Load Write Latches
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
	movlw	0x55             ;   Start of required write sequence:
	movwf	EECON2
	movlw	0xAA            ;
	movwf	EECON2
	bsf	EECON1,WR       ;   Set WR bit to begin write
	nop
	nop
	incf	EEADRL,F        ; Still loading latches Increment address
	goto	LOOP            ; Write next latches
START_WRITE
	bcf	EECON1,LWLO     ; No more loading latches - Actually start Flash program
	;	memory write
	movlw	0x55            ;   Start of required write sequence:
	movwf	EECON2
	movlw	0xAA
	movwf	EECON2
	bsf	EECON1,WR       ;   Set WR bit to begin write
	nop
	nop
	bcf	EECON1,WREN     ; Disable writes
	bsf	INTCON,GIE      ; Enable interrupts
	return

test_indr:
	movlb	0		; select bank 0
	; read low byte of program memory starting at 0x300
	movlw	0x83
	movwf	FSR1H
	clrf    FSR1L
	addfsr	FSR1,1
  .assert "fsr1h == 0x83 && fsr1l == 0x01, '*** FAILED 12f1840 addfsr fsr1,1'"
	nop
	addfsr	FSR1,-1		; undo above instruction
  .assert "fsr1h == 0x83 && fsr1l == 0x00, '*** FAILED 12f1840 addfsr fsr1,-1'"
	nop
	clrf	temp
	moviw   FSR1++
	addwf	temp,F
	moviw   FSR1++
	addwf	temp,F
	moviw   FSR1++
	addwf	temp,F
	moviw   FSR1++
	addwf	temp,F
	moviw   FSR1++
   .assert "(status & 0x4) == 0x0, '*** FAILED 12f1840 Z bit is 1 after MOVIW'"
	nop
	addwf	temp,F
   .assert "temp == 0xff, '*** FAILED 12f1840 read program memory'"
	nop
	moviw   FSR1++		; Read a 0
   .assert "(status & 0x4) == 0x4, '*** FAILED 12f1840 Z bit is 0 after MOVIW'"
	nop
	; put address of temp into temp, FSR0 and 0x20
	movlw	temp
	movwf   temp
	movwf	FSR0L
	movwf   0x20
	clrf	FSR0H
	; put address of w_temp into w_temp
	movlw   temp+1
	movwf	w_temp
	moviw   ++FSR0
  .assert "W == 0x71, '*** FAILED 12f1840 ++FSR0 read of w_temp'"
	nop
	moviw	-1[FSR0]
  .assert "W == 0x70, '*** FAILED 12f1840 -1[FSR0] read of temp'"
	nop
	moviw	-20[FSR1]
	moviw	0[FSR0]
	movlw	0xf1
	movwi   ++FSR0
  .assert "temp2 == 0xf1, '*** FAILED 12f1840 ++FSR0 write'"
	nop
	addlw	1
	movwi	1[FSR0]
  .assert "temp3 == 0xf2, '*** FAILED 12f1840 1[FSR0] write'"
	nop
	movwi	10[FSR1]	; not valid to write indirect to program memory
	moviw	10[FSR1]	; read unchanged program memory
  .assert "W == 0xff, '*** FAILED 12f1840 10[FSR1] W/R program memory(no write)'"
	nop
	movlw	0x20
	movwf	FSR1L
	clrf 	FSR1H
	movf 	INDF0,W
	addwf	INDF1,W
  .assert "W == 0x61, '*** FAILED 12f1840 INDF operations'"
	nop
	clrf 	FSR1L
	movlw	0x80
	movwf	FSR1H
	incf 	FSR1L,F
	movf    INDF1,W
  .assert "W == 0x1a, '*** FAILED 12f1840 INDF operations program mem'"
	nop
	movlw	0x20		; linear memory address 0x20000 == 0x20
	movwf	FSR0H
	movlw	0x6f
	movwf	0x6f
	movlw	0x4f
	movwf	FSR0L		; 0x204f points to  0x6f
	
	movwf	INDF0		; write 0x4f to 0x6f (bank 0)
	incf	FSR0L,F		; 0x2050 points to 0xa0
	movwf	INDF0		; write 0x4f to 0xa0 (bank 1)
  .assert "A0 == 0x4f, '*** FAILED 12f1840 INDF operations linear memory'"
	nop
	return


 	org 0x300
rrDATA
	dw 0x01, 0x02, 0x03, 0x04, 0xf5, 0
  end
