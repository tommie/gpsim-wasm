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

;        CONFIG WDTEN=ON
;	CONFIG WDTPS=128
;        CONFIG MCLRE = INTMCLR
;        CONFIG FOSC = INTIO67
        errorlevel -302

; Printf Command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR 0

delay1	        RES	1
delay2          RES	1
counter         RES	1
counter_hi      RES	1
eerom_cnt       RES     1
adr_cnt         RES     1
data_cnt        RES     1
inte_cnt        RES     1
padd		RES	3




GPR_DATA2	UDATA 0x100
buffer  RES 64

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
	call	test_eerom
	call	prog_2_ram
	call	erase_block
	call	ram_2_prog
	call	verify_ram_2_prog
	call	read_config_data
	call	read_devid

  .assert  "'*** PASSED 18F26k22 TBL EEPROM test'"
        nop
        goto    $-1

copy_exec:	; test that copied code can work
	movf	TBLPTRL,W
	return

;
; Test reading and writing Configuration data via eeprom interface
;
read_config_data:
	movlw   0x30
	movwf   TBLPTRU
	movlw   0x00
	movwf   TBLPTRH
        movlw   0x00              ;
	movwf	TBLPTRL
	movlw	(1<<CFGS)
	movwf	EECON1
	tblrd*+             ; read into TABLAT and increment
	tblrd*+             ; read into TABLAT and increment
	tblrd*+             ; read into TABLAT and increment
	tblrd*+             ; read into TABLAT and increment
	return

read_devid:
	movlw   0x3f
	movwf   TBLPTRU
	movlw   0xFF
	movwf   TBLPTRH
        movlw   0xFE              ;
	movwf	TBLPTRL
	movlw	(1<<CFGS)
	movwf	EECON1
   .assert "tblptrl == 0xfe, '*** FAILED 18f26k22 Device ID 1 tblptrl'"
	nop
	tblrd*+             ; read into TABLAT and increment
   .assert "tablat == 0x60, '*** FAILED 18f26k22 Device ID 1'"
	nop
	movf     TABLAT, W
   .assert "tblptrl == 0xff, '*** FAILED 18f26k22 Device ID 2 tblptrl'"
	nop
	tblrd*+             ; read into TABLAT and increment
   .assert "tablat == 0x54, '*** FAILED 18f26k22 Device ID 2'"
	nop
	movf     TABLAT, W

        nop
	return


interrupt:

        movlb   0       ; BSR=0
        btfsc   PIR2,EEIF
            goto ee_int



   .assert "'FAILED 18F26k22 unexpected interrupt'"
        nop

; Interrupt from eerom
ee_int
        incf    eerom_cnt,F
        bcf     PIR2,EEIF

back_interrupt:
        retfie 1


test_eerom:
  ;
  ;	test can write and read to all 128 eeprom locations
  ;	using interrupts
        clrf    adr_cnt
        clrf    data_cnt
;  setup interrupts
        bsf     INTCON,PEIE
        bsf     INTCON,GIE
	bsf 	PIE2,EEIE
;
;	write to EEPROM starting at EEPROM address 0
;	value of address as data using interrupts to
;	determine write complete. 
;	read and verify data

l1:     
        movf    adr_cnt,W
	clrf	eerom_cnt
        movwf   EEADR 
        movf    data_cnt,W
        movwf   EEDATA
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
        goto   $-4
;
;	read what we just wrote
;
	
        movf    adr_cnt,W

	movwf   EEADR
	bcf EECON1, CFGS 	;Deselect Config space
	bcf EECON1, EEPGD	;Point to DATA memory

	bsf 	EECON1,RD	; start read operation
	movf	EEDATA,W	; Read data

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
  .assert "'***FAILED 18f26k22 eerom write/read error'"
        nop

; copy 64 bytes (32 words) of program memory into a ram buffer
prog_2_ram:
	movlw    D'64'             ; number of bytes in erase block
	movwf     counter           ; point to buffer
	movlw    high buffer       ; point to buffer
	movwf    FSR0H
	movlw    low  buffer       ; Load TBLPTR with the base
	movwf    FSR0L             ; address of the memory block

	movlw	 upper copy_exec
	movwf    TBLPTRU
	movlw	 high copy_exec
	movwf    TBLPTRH
	movlw	 low copy_exec
	movwf    TBLPTRL

READ_BLOCK

        tblrd*+  
	movf     TABLAT, W       ; read into TABLAT, and inc
        movwf    POSTINC0        ; store data
        decfsz   counter,F        ; done?
        bra      READ_BLOCK      ; repeat
	return

; erase a 64 byte (32 words) of program memory
erase_block
	movlw  upper END_PROG   ; load TBLPTR with the base
	movwf  TBLPTRU          ; address of the memory block
	movlw  high END_PROG 
	movwf  TBLPTRH
	movlw  low END_PROG
	movwf  TBLPTRL    
	movlw  D'64'		; make sure block starts after program
	addwf  TBLPTRL,F
	btfss  STATUS,C
	goto   add_end
	incf   TBLPTRH,F
	btfsc  STATUS,C
	incf   TBLPTRU,F
add_end
	
	movlw  (1<<EEPGD)|(1<<WREN)|(1<<FREE)
	movwf  EECON1
	bcf    INTCON, GIE
	movlw  55h              ; write 55h
	movwf  EECON2
	movlw  0AAh             ; write 0AAh
	movwf  EECON2      
	bsf    EECON1, WR       ; start erase (CPU stall)
	bsf    INTCON, GIE      ; re-enable interrupts
	return

; copy ram buffer into program space
ram_2_prog:
	movlw    ~D'63'
	andwf	 TBLPTRL,W
	movwf	 TBLPTRL
	movwf	 padd
	movf	 TBLPTRH,W
	movwf	 PCLATH
	movf	 TBLPTRU,W
	movwf	 PCLATU
	
	tblrd*-                    ; dummy read decrement to test preincrement
        movlw    8		   ; number of write buffer groups of 8 bytes
        MOVWF    counter_hi
	movlw    high buffer       ; point to buffer
	movwf    FSR0H
	movlw    low buffer        ; number of bytes in holding register
	movwf    FSR0L             ; number of write blocks in 64 bytes
PROGRAM_LOOP                         
        movlw    8		   ; number of bytes in holding register
        movwf    counter
WRITE_WORD_TO_HREGS
        movf     POSTINC0, W       ; present data to table latch
        movwf    TABLAT
        tblwt+*			   ; write data, perform a short write
	decfsz   counter,F
	bra      WRITE_WORD_TO_HREGS

	movlw  (1<<EEPGD)|(1<<WREN) ; prepare to write latches to program memory
	movwf	EECON1
	bcf    INTCON, GIE	    ; disable interrupts
	movlw  55h              ; write 55h
	movwf  EECON2
	movlw  0AAh              ; write 0AAh
	movwf  EECON2
	bsf    EECON1, WR	 ; start program write (CPU stall)
	bsf    INTCON, GIE       ; re-enable interrupts

	decfsz counter_hi,F
	bra    PROGRAM_LOOP
	bcf    EECON1, WREN	;disable write to memory
	movf   padd,W
	movwf  PCL
	return

; verify  64 bytes (32 words) of written program memory with ram buffer
verify_ram_2_prog:
	movlw  upper END_PROG   ; load TBLPTR with the base
	movwf  TBLPTRU          ; address of the memory block
	movlw  high END_PROG 
	movwf  TBLPTRH
	movlw  low END_PROG
	movwf  TBLPTRL    
	movlw  D'64'		; make sure block starts after program
	addwf  TBLPTRL,F
	btfss  STATUS,C
	goto   add_end2
	incf   TBLPTRH,F
	btfsc  STATUS,C
	incf   TBLPTRU,F
add_end2
	movlw    ~D'63'
	andwf	 TBLPTRL,F
	movlw    D'64'             ; number of bytes in erase block
	movwf     counter           ; point to buffer
	movlw    high buffer       ; point to buffer
	movwf    FSR0H
	movlw    low  buffer       ; Load TBLPTR with the base
	movwf    FSR0L             ; address of the memory block


verify_loop

        tblrd*+                  ; read into TABLAT, and inc
	movf     TABLAT,W
        xorwf    POSTINC0,W       ; compare 
	btfss	 STATUS,Z	  ; zero?
	goto	 fail
        decfsz   counter,F        ; done?
        bra      verify_loop      ; repeat
	return

fail:
    .assert "'***FAILED p18f26k22 program- ram verify'"
	nop
	goto $-2

END_PROG
	end
