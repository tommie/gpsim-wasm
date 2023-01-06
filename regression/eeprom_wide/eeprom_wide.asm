        list    p=p16f873
        radix   dec
;
;	Test EEPROM and FLASH program single byte reads and writes.
;	on 16f873 which uses wide mode

  __config _WDT_OFF

	include "p16f873.inc"
        include <coff.inc>              ; Grab some useful macros


bank0_data udata 0x20
adr_cnt		RES 1
data_cnt	RES 1
status_temp	RES 1
data_high		RES 1
data_low   	RES 1
ee_int		RES 1
    global data_low
    global data_high

    ; W shadowing for interrupts
    ; When an interrupt occurs, we don't know what the current bank settings
    ; are. The solution here is to declare two temporaries that have the
    ; same base address. That way we don't need to worry about the bank setting.

  cblock        0x70
        w_temp          ; W is stored here during interrupts if the
                        ; bank bits point to bank 0 or 2
  endc
  cblock        0xf0
        w_temp_shadow   ; W is stored here during interrupts if the
                        ; bank bits point to bank 0 or 2
  endc

;;========================================================================
;;========================================================================
;
;       Start 
;
        org     0

        goto    start

        org     4

    ;;************** 
    ;; Interrupt
    ;;*************

        movwf   w_temp
        swapf   STATUS,W
        clrf    STATUS          ;Bank 0
        movwf   status_temp

	btfss	PIR2,EEIF
        goto   check

;;; eeprom has interrupted
	bcf	PIR2,EEIE
	incf	ee_int,F
	goto	i_ret

check:
  .assert "'***  FAILED wide EEPROM unexpected interrupt'"
	nop

i_ret:
        clrf    STATUS          ;bank 0
        swapf   status_temp,W
        movwf   STATUS
        swapf   w_temp,F
        swapf   w_temp,W
        retfie
    ;;*************
    ;; end of interrupt
    ;;*************
     

start:  
        clrf    STATUS          ;Point to Bank 0
        clrf    adr_cnt
        clrf    data_cnt
 ;       bsf     INTCON,EEIE
        bsf     INTCON,PEIE
        bsf     INTCON,GIE
	bsf	STATUS,RP0	;Bank 1
	bsf	PIR2,EEIE
	bcf	STATUS,RP0	;Bank 0
;
;	write to EEPROM starting at EEPROM address 0
;	value of address as data using interrupts to
;	determine write complete. 
;	read and verify data

l1:     
	bcf	PIR2,EEIF
        movf    adr_cnt,W
	clrf	ee_int
        bcf     STATUS,RP0
        bsf     STATUS,RP1	;Bank 2
        movwf   EEADR
        movf    data_cnt,W
        movwf   EEDATA

        bcf     INTCON,GIE      ;Disable interrupts while enabling write

        bsf     STATUS,RP0	; Bank 3
        bcf     EECON1,EEPGD   ;Point to data Memory
        bsf     EECON1,WREN    ;Enable eeprom writes

        movlw   0x55            ;Magic sequence to enable eeprom write
        movwf   EECON2
        movlw   0xaa
        movwf   EECON2

        bsf     EECON1,WR      ;Begin eeprom write
	nop
	nop
        bsf     INTCON,GIE      ;Re-enable interrupts
        
        clrf    STATUS          ; Bank 0
        movf   ee_int,W
	skpnz
        goto   $-2
;
;	read what we just wrote
;
	
        movf    adr_cnt,W

	bsf	STATUS,RP1
	bcf	STATUS,RP0      ; Bank 2
	movwf   EEADR
	bsf	STATUS,RP0      ; Bank 3
	bcf	EECON1,EEPGD	; point ot data memory
	bsf	EECON1,RD	; start read operation
	nop
	nop
	bcf	STATUS,RP0      ; Bank 2
	movf	EEDATA,W	; Read data
	clrf	STATUS		; Bank 0

	xorwf	data_cnt,W	; did we read what we wrote ?
	skpz
	goto fail

        incf    adr_cnt,W
        andlw   0x7f
        movwf   adr_cnt
	movwf	data_cnt

        skpz
         goto   l1
	
	goto flash

fail:
  .assert  "'***  FAILED eeprom_wide Compare written and read'"
        goto    $

;
;	test program FLASH read and writes
;	    read low program memory (0x00XX)  
;	    write to higher memory (0x01XX)
;	    read and compare higher memory (0x1XX)

flash:

	bsf	STATUS,RP1
	bcf	STATUS,RP0      ; Bank 2
loop_prg:     
;
;	read low memory 0x00XX
;
	bsf	STATUS,RP1
	bcf	STATUS,RP0      ; Bank 2
	movwf	adr_cnt
	movwf   EEADR
	clrf	EEADRH
	bsf	STATUS,RP0      ; Bank 3
	bsf	EECON1,EEPGD	; point to program memory
	bsf	EECON1,RD	; start read operation
	nop
	nop
	bcf	STATUS,RP0      ; Bank 2
	movf	EEDATA,W	; save read data
	movwf	data_low
	movf	EEDATH,W
	movwf	data_high
;
;  Write to High address (0x01XX) data alredy in EEDATA, EEDATAH
;
	movlw	0x01
	movwf	EEADRH
	movf	adr_cnt,W
        movwf   EEADR

        bcf     INTCON,GIE      ;Disable interrupts while enabling write
        bsf     STATUS,RP0	; Bank 3
        bsf     EECON1,EEPGD   ;Point to program Memory
        bsf     EECON1,WREN    ;Enable eeprom writes
        movlw   0x55            ;Magic sequence to enable write
        movwf   EECON2
        movlw   0xaa
        movwf   EECON2

        bsf     EECON1,WR      ;Begin write
	nop
	nop

        bsf     INTCON,GIE      ;Re-enable interrupts
        bcf     EECON1,WREN    ;Disable writes
        
        btfsc   EECON1,WR
         goto   $-1
;
;	read what we just wrote
;

	bcf	STATUS,RP0      ; Bank 2
        movf    adr_cnt,W
	movwf   EEADR
	movlw	0x01
	movwf	EEADRH
	bsf	STATUS,RP0      ; Bank 3
	bsf	EECON1,EEPGD	; point to program memory
	bsf	EECON1,RD	; start read operation
	nop
	nop
	bcf	STATUS,RP0      ; Bank 2
	movf	EEDATA,W	; Read data
    .assert "W == data_low, '*** FAILED eeprom_wide, program FLASH Compare written and read - low byte'"
	nop
	movf	EEDATH,W	; Read data
    .assert "W == data_high, '*** FAILED eeprom_wide, program FLASH Compare written and read - high byte'"
	nop

        incf    adr_cnt,W
        andlw   0x3f
        movwf   adr_cnt
        skpz
         goto   loop_prg

  .assert  "'*** PASSED eeprom_wide and program FLASH read-write test'"
        goto    $

        org     0x2100
	de	"0123456789ABCDEF"
        de      "Linux is cool!",0
        de      0xaa,0x55,0xf0,0x0f
        de      'g','p','s','i','m'

   end
