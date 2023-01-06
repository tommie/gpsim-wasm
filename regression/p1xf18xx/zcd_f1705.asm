;
; 
; Copyright (c) 2019 Roy Rankin
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
        ;; simulate a pic 16LLF1705.
	;; test zero crossing detector


	list    p=16lf1705                ; list directive to define processor
	include <p16lf1705.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_ON & _CLKOUTEN_OFF
        __CONFIG _CONFIG2, _STVREN_ON  ; & _ZCDDIS_OFF; & _WRT_BOOT

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
intf_cnt	RES	1
iocaf_val	RES	1
ioccf_val	RES	1
zcd_cnt		RES	1

 GLOBAL intf_cnt, iocaf_val, ioccf_val, zcd_cnt


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

  ; Use a pullup resistor as a voltage source
  .sim "module load pullup V2"
  .sim "V2.resistance = 10000.0"
  .sim "V2.capacitance = 20e-12"
  .sim "V2.voltage=2.5"

  .sim "node n1"
  .sim "attach n1 porta2 V2.pin"
  .sim "node n2"
  .sim "attach n2 porta1 porta4"
  .sim "node n3"
  .sim "attach n3 porta0 porta5"
;  .sim "attach n3 ZCD porta5"
  .sim "node n4"
  .sim "attach n4 portc0 V1.pin"

  .sim "p16lf1705.xpos = 72"
  .sim "p16lf1705.ypos = 72"

  .sim "V1.xpos = 216"
  .sim "V1.ypos = 120"

  .sim "V2.xpos = 216"
  .sim "V2.ypos = 60"


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
	; many of the core registers now saved and restored automatically
                                                                                
	clrf	BSR		; set bank 0

;	btfsc	PIR2,EEIF
;	    goto ee_int

  	btfsc	INTCON,T0IF
	    goto tmr0_int

  	btfsc	INTCON,IOCIF
	    goto ioc_int

  	btfsc	INTCON,INTF
	    goto intf_int

	btfsc	PIR3,ZCDIF
	    goto zcd_int

	.assert "'***FAILED p16lf1705 unexpected interrupt'"
	nop


; Interrupt from TMR0
tmr0_int
	incf	tmr0_cnt,F
	bcf 	INTCON,T0IF
	goto	exit_int

; Interrupt from eerom
ee_int
	incf	eerom_cnt,F
;	bcf 	PIR2,EEIF
	goto	exit_int

; Interrupt from IOC pins
ioc_int
	banksel IOCAF
	movf 	IOCAF,W
	movwf	iocaf_val
	clrf	IOCAF
	clrf	IOCCF
	goto	exit_int

; Interrupt from INT pin
intf_int
	incf	intf_cnt,F
	bcf	INTCON,INTF	; stop interrupts
	goto	exit_int

zcd_int
	bcf	PIE3,ZCDIF
	incf	zcd_cnt,F
	goto	exit_int

exit_int:
                                                                                
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
   .assert "p16lf1705.frequency == 500000., 'FALIED 16LF1705 POR clock 500kHz'"
	nop


	BANKSEL ANSELA
	movlw	0xfc
	movwf	ANSELA
	;clrf	ANSELA
        BANKSEL TRISA
	movlw	0x07
	movwf	TRISA
	BANKSEL RA5PPS
	movlw	0x4		;LC1 output on RA4, RA5
	movwf	RA5PPS
	movwf	RA4PPS
	call	setup_clc
   	BANKSEL PIE3
	bsf	PIE3,ZCDIE
	BANKSEL INTCON
	bsf	INTCON,PEIE
	bsf	INTCON,GIE
	BANKSEL	ZCD1CON
	clrf	zcd_cnt
	bsf	ZCD1CON,ZCD1INTP
	bsf	ZCD1CON,ZCD1EN
   .assert "zcd1con == 0xa2, '*** FALIED 16LF1705 ZCD1OUT V>0.75'"
	nop
   .assert "zcd_cnt == 1, '*** FALIED 16LF1705 1st interrupt'"
	nop
;        BANKSEL TRISA
;	clrf	TRISA
;	movlw	0xff
;	movwf	TRISA
	BANKSEL ANSELA
;	clrf	ANSELA
	BANKSEL	ZCD1CON
	bsf	ZCD1CON,ZCD1POL
   .assert "zcd1con == 0x92, '*** FALIED 16LF1705 ZCD1POL ZCD1OUT V>0.75'"
	nop
   .assert "zcd_cnt == 2, '*** FALIED 16LF1705 2st interrupt'"
	nop
   	nop
   .command "V2.voltage=0.74"
	nop
   .assert "zcd1con == 0xb2, '*** FALIED 16LF1705 ZCD1POL ZCD1OUT V<0.75'"
	nop
   .assert "zcd_cnt == 3, '*** FALIED 16LF1705 interrupt OUT+'"
	nop
	nop
   .command "V2.voltage=0.76"
	nop
   .assert "zcd1con == 0x92, '*** FALIED 16LF1705 ZCD1POL ZCD1OUT V>0.75'"
	nop
   .assert "zcd_cnt == 3, '*** FALIED 16LF1705 no interrupt OUT-'"
	nop
	bcf	ZCD1CON,ZCD1EN
	nop


   .assert  "'*** PASSED 16LF1705 ZCD Functionality'"
	nop
	goto $
setup_clc:
	; select inputs
	BANKSEL CLC1SEL0
	movlw	0x13		; ZCD_out
	movwf	CLC1SEL0
	movwf	CLC1SEL1
	movwf	CLC1SEL2
	movwf	CLC1SEL3

	movlw	0x02		; LCxG1D1T
        movwf	CLC1GLS0
        movwf	CLC1GLS1
	clrf	CLC1GLS2
	clrf	CLC1GLS3
        clrf	CLC1POL
	; AND-OR and active
	movlw	0x80
	movwf   CLC1CON

	return
  end
