;
; Test operation of the 1-wire tmemperaure sensor ds1820 and
; is part of gpsim.
; 
; Copyright (c) 2013 Roy Rankin
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, see
; http://www.gnu.org/licenses/gpl-2.0.html
;
; MACROS and some code included the following
;************************************************************************
;*	Microchip Technology Inc. 2006
;*	02/15/06
;*	Designed to run at 20MHz
;************************************************************************
; gpsim command
.command macro x
  .direct "C", x
  endm



	list		p=16F1823
	#include	p16f1823.inc
        include <coff.inc>


	__CONFIG _CONFIG1, _CP_OFF &  _WRT_OFF & _CPD_OFF  & _LVP_OFF  & _PWRTE_ON &  _WDTE_OFF & _FOSC_HS

	errorlevel	-302
	radix dec

#define NO_PARASITE 1
#define ID1_ADDRESS 0x20
#define ID2_ADDRESS 0x30
#define DS_PORT	PORTC
#define DS_PIN	0
#define DS_TRIS TRISC

		UDATA 0x20
ID1		RES	8
scratch		RES	8
ID2		RES	8
Tlsb		RES	1
Tmsb		RES	1
Th		RES	1
Tl		RES	1
reserve		RES	2
count_rem	RES	1
count_per_c	RES	1
crc		RES	1

variables	UDATA 0x70
d1l		RES	1	; delay counter
d1h		RES	1	; delay counter
d2		RES	1	; long period counter
temp1           RES     1
temp2           RES     1
status_temp	RES	1
w_temp		RES	1
bit_cnt		RES	1	; bit count for byte I/O 
dup_devices	RES	1	; found more than 1 device
dev_select	RES	1	; select device to save

  GLOBAL Tlsb, Tmsb,Th,Tl, count_rem

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x004               ; interrupt vector location


        .assert "\"***FAILED p16f1823 unexpected interrupt\""
        nop
exit_int:

        retfie



;
MAIN	CODE
start	
   .sim "module lib libgpsim_modules"
   .sim "module lib libgpsim_extras"
   .sim "module load ds1820 dstemp1"
   .sim "module load ds1820 dstemp2"
   .sim "module load pu pu1"
   .sim "node n1"
   .sim "attach n1 pu1.pin portc0 dstemp1.pin"
   ;.sim "attach n1 pu1.pin portc0 dstemp1.pin dstemp2.pin" 
   .sim "pu1.xpos = 240"
   .sim "pu1.ypos = 188"
   .sim "pu1.resistance = 4700"
   .sim "dstemp1.xpos = 240"
   .sim "dstemp1.ypos = 50"
   .sim "dstemp1.powered = false"
   .sim "dstemp2.xpos = 240"
   .sim "dstemp2.ypos = 110"
   .sim "dstemp2.ROMCode = 0x06050403020200"
   .sim "dstemp2.temperature = -25.4"
   .sim "p16f1823.xpos = 36"
   .sim "p16f1823.ypos = 36"
   .sim "scope.ch0=\"portc0\""




	call	ProgInit		; Get everything running step-by-step
	;goto	two
	clrf	dev_select
	call	searchRom
	call	read_rom
	call	check_power
   .assert "W==0x00,\"DS1820 temp1 parasite power\""
	nop
	call	power_on_val
	call	read_temp
   .assert "Tlsb==0x32,\"Temperature Reading lsb 0x32 (25C)\""
	nop
   .assert "Tmsb==0x00,\"Temperature Reading msb 0x00 (25C)\""
	nop
	call 	write_ThTl
	call	eeprom2scratch

two
   .command "attach n1 dstemp2.pin"
	nop

 	incf	dev_select,F		; search for second ds1820
 	call	searchRom
 	call	check_power
   .assert "W==0x80,\"DS1820 temp2 external power\""
 	nop
	call	power_on_val
	call	read_temp
   .assert "Tlsb==0xcd,\"Temperature Reading lsb 0xCD (-25.4C)\""
	nop
   .assert "Tmsb==0xff,\"Temperature Reading msb 0xFF (-25.4C)\""
	nop
	call 	write_ThTl
	call	eeprom2scratch

  .assert "\"*** PASSED p16f1823 DS1820 test\""
	nop
	goto $

 ;
 ; Read Power-on scratchpad 0,1
power_on_val
	call	match_device
	movlw	0xbe			;Transmit out temperature
	call	send_byte
	call	get_byte
   .assert "W==0xAA,\"Power-on reading scratchpad[0]\""
	nop
	call	get_byte
   .assert "W==0x00,\"Power-on reading scratchpad[1]\""
	nop
	return
 ;
 ; Do Temperature conversion, and read temperature
read_temp
	call	ds1820init
	movlw	0xcc			;Send skip rom command
	call	send_byte
	movlw	0x44			;Issue convert T command
	call	send_byte
	;; It is not known what a real devices would do on the following
	;; status reads if at least one is on  parasite power.
	;; However this code would fail on real devices using parasite power
	;; as a hard pullup is required during the conversion
#ifdef NO_PARASITE
	call	get_byte	; status read
	call	get_byte	; status read loop
	btfss	temp1,0
	goto	$-2
#else
 	banksel DS_PORT
 	bsf	DS_PORT,DS_PIN		; we want strong pullup, so output
 	banksel DS_TRIS
 	bcf	DS_TRIS,DS_PIN
 	call	Delay750mS		; Tconv delay
#endif

	call	read_scratch
	call	get_byte		; temp lsb
	movwf	Tlsb
	call	get_byte		; temp lsb
	movwf	Tmsb
	return
 ;
 ; Write Th & Tl to scratchpad
write_ThTl
	call	match_device
	movlw	0x4e		; write Th & Tl to scratchpad
	call	send_byte
	movlw	0xa0
	call	send_byte
	movlw	0x00
	call	send_byte
	call	read_scratch
	call	get_byte		; temp lsb
	call	get_byte		; temp msb
	call	get_byte		; Th
  .assert "W==0xa0,\"Th 0xa0\""
	nop
	call	get_byte		; Tl
  .assert "W==0, \"Tl 0x00\""
	nop
	return
 ;
 ; Transfer EEPROM to scratchpad
eeprom2scratch
	call	match_device
	movlw	0xb8		; Recall EE
	call	send_byte
	call	read_scratch
	call	get_byte		; temp lsb
	movwf	Tlsb
	call	get_byte		; temp msb
	movwf	Tmsb
	call	get_byte		; Th
	movwf	Th
	call	get_byte		; Tl
	movwf	Tl
	call	get_byte
	movwf	reserve
	call	get_byte
	movwf	reserve+1
	call	get_byte
	movwf	count_rem
	call	get_byte
	movwf	count_per_c
	call	get_byte
	movwf	crc
  .assert "Th==30,\"Th 30\""
	nop
 .assert "Tl==0xfb, \"Tl -5\""
	nop
	return

read_rom
	call    ds1820init
	movlw	0x33		;read rom command
	call	send_byte
	movlw	ID2_ADDRESS		; address of scratch + 8
	movwf	FSR0
	movlw	8
	movwf	temp2
	call	get_byte
	movwi	--FSR0
	decfsz  temp2,F
	goto	$-3
	return

read_scratch
	call	match_device
	movlw	0xbe			;Transmit out temperature
	call	send_byte
	return
 ; Match_device does a skip ROM command if one device present
 ; Otherwise it does a match ROM on ID1 or ID2 depending on dev_select
match_device
	call	ds1820init
	btfsc	dup_devices,0
	goto	$+4
	movlw	0xcc		; just 1 device do skip ROM
	call	send_byte
	return

	movlw	0x55
	call	send_byte

	movlw	ID1_ADDRESS+8		; address of ID1+8
	btfsc	dev_select,0
	movlw	ID2_ADDRESS+8		;address of ID2+8
	movwf	FSR0
	movlw	8
	movwf	temp2
	moviw	--FSR0
	call	send_byte
	decfsz  temp2,F
	goto	$-3
	return


check_power:
	call	match_device
	movlw	0xb4			;Check power
	call	send_byte
	call	get_1bit
	return

searchRom:

	call	ds1820init
	movlw	0xf0			;Search ROM
	call	send_byte
	movlw	.64
	movwf	temp2
searchloop
	call	get_2bits
	call	process_bits
	decfsz	temp2,F
	goto	searchloop
	return

process_bits
	btfsc	temp1,6
	goto	b1x
	btfsc	temp1,7
	goto	b01
  ; 00	got 00 some devices sent 0 some 1 , do 0 first
	incf	dup_devices,F	; indicate duplicate 
	btfss	dev_select,0
	goto	b10
	

b01
	bcf	temp1,0	; got 01 all devices sent 0
	bcf	STATUS,C
	goto	bits_ack

b1x	btfsc	temp1,7
	goto	b11

b10
	bsf	temp1,0 ; got 10 all devices sent 1
	bsf	STATUS,C
 	goto	bits_ack

b11
	.assert "\"no devices present\""
	nop

bits_ack
	btfsc	dev_select,0
	goto	bits_ack2
bits_ack1
	banksel ID1
 	rrf	ID1,F
	rrf	ID1+1,F
	rrf	ID1+2,F
	rrf	ID1+3,F
	rrf	ID1+4,F
	rrf	ID1+5,F
	rrf	ID1+6,F
	rrf	ID1+7,F
	call	send_1bit
	return
bits_ack2
	banksel ID2
 	rrf	ID2,F
	rrf	ID2+1,F
	rrf	ID2+2,F
	rrf	ID2+3,F
	rrf	ID2+4,F
	rrf	ID2+5,F
	rrf	ID2+6,F
	rrf	ID2+7,F
	call	send_1bit
	return
	




;****************** Initialize Registers and Variables  *****************
;************************************************************************
ProgInit

	banksel	DS_PORT
	movlw	0xff
	movwf	DS_PORT			; Set all bits high on Port 
	banksel DS_TRIS
	clrf	DS_TRIS
	banksel ANSELC
	clrf	ANSELC
	
	return


ds1820init:				; DS1820 init sequence
	banksel	DS_PORT
	bsf	DS_PORT,DS_PIN			; Pin high
	banksel DS_TRIS
	bcf	DS_TRIS,DS_PIN			; Pin as output
	banksel DS_PORT
	bcf	DS_PORT,DS_PIN			; Pin low
	call	Delay500us
	banksel DS_TRIS
	bsf	DS_TRIS,DS_PIN			; Pin as input (pullup to high)
	banksel DS_PORT
	call	Delay70us
	movf	DS_PORT,W
    .assert "(W & 1)==0,\"DS1820, no presents pulse\""
	nop
	call	Delay500us
	return

	; send 1 bit from temp1
send_1bit
	movlw	1
	goto	send_bits

	; send the byte in W
send_byte
	movwf	temp1
	movlw	8
send_bits
	movwf	bit_cnt
	banksel DS_PORT		; we want bit low, high is by pullup
	bcf	DS_PORT,DS_PIN
send_loop
	rrf	temp1,F		; rotate lSB into carry
	btfss	STATUS,C	; is bit 0 or 1
	goto	send_0
	goto	send_1
send_chk
	decfsz	bit_cnt,F		; have we send all bits
	goto	send_loop	; NO
	banksel DS_PORT
	return			; YES

send_0
	banksel DS_TRIS
	bcf	DS_TRIS,DS_PIN
	call	Delay70us
	bsf	DS_TRIS,DS_PIN
	goto	send_chk

send_1
	banksel DS_TRIS
	bcf	DS_TRIS,DS_PIN
	call	Delay10us
	bsf	DS_TRIS,DS_PIN
	call	Delay70us
	goto	send_chk

	; read 1 bit of data
get_1bit
	movlw	1
	goto	get_bits

	; read 2 bits of data
get_2bits
	movlw	2
	goto	get_bits

	; read a data byte and return it in W
get_byte
	movlw	8	; input bit counter
get_bits
	movwf	bit_cnt
	clrf	temp1
        banksel DS_PORT
	bcf	DS_PORT,DS_PIN
get_loop
	banksel DS_TRIS
	bcf	DS_TRIS,DS_PIN	; init read timeslot by pulling line low
	nop			; delay for at least 1 usec
	nop
	nop
	nop
	nop
	bsf	DS_TRIS,DS_PIN	; release line
	call	Delay10us
	banksel	DS_PORT
	bsf	STATUS,C	; set carry incase of 1
	btfss	DS_PORT,DS_PIN	; read port data
	bcf	STATUS,C	; its 0, so clear carry
	rrf	temp1,f		; roll data into data word
 	call    Delay70us	; this could be 45us
	btfss	DS_PORT,DS_PIN	; check port pin
	goto	$-1		; loop until high
	decfsz	bit_cnt,F
	goto	get_loop
	movf	temp1,W
	return


Delay750mS                               

	movlw	.75
	movwf	d2
loop750ms
	call	Delay10mS
	decfsz	d2,F
	  goto loop750ms
	return

Delay10mS
                movlw   0x50
                movwf   d1l
                movlw   0x28
                movwf   d1h
		goto	Delay_0

Delay500us
		movlw	0x02
		movwf	d1h
		movlw	0xfe
		movwf	d1l
		goto	Delay_0
Delay70us
		movlw	0x01
		movwf	d1h
		movlw	0x44
		movwf	d1l
		goto	Delay_0
Delay10us
		movlw	0x01
		movwf	d1h
		movlw	0x08
		movwf	d1l
		goto	Delay_0
Delay_0
                decfsz  d1l, f
                goto    $+2
                decfsz  d1h, f
                goto    Delay_0

                        ;2 cycles
                goto    $+1
                return



	end	
