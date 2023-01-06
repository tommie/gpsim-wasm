		;; 

        radix   dec

	list    p=12ce519                ; list directive to define processor
	include <p12ce519.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros


 __CONFIG   _CP_OFF & _IntRC_OSC & _MCLRE_OFF & _WDT_ON

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA        UDATA   0x07

ee_state     RES     1
eebyte       RES     1
ee_bitcnt    RES     1
eeaddr       RES     1
eedata       RES     1
failures     RES     1
temp         RES     1
  

GPR2_DATA       UDATA   0x30

eeShadow     RES     0x10

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE

	goto	start


include "EEdriver.asm"

start:
    movlw   0xC0    ; Set up GPIO pins
    movwf   GPIO
    movlw   0x0F
    tris    GPIO
    movlw   1<<7      ; Assume that we're going to fail
    movwf   failures  ; by setting the MSB of failures

FillEEPROM:
        movlw   0
        movwf   temp
        movlw   eeShadow
        movwf   FSR

FillLoop:
        clrwdt

    ; Set the address
        movf    FSR,W
        andlw   0x0f
        movwf   eeaddr

    ; Set the data
        swapf   temp,W
        iorwf   temp,W
        movwf   eedata
        movwf   INDF

        call    write_byte
  .assert "W == 0x01, '*** FAILED Write_byte returned error'"
	nop

FillWaitForWrite:
        call    read_current
        btfss   ee_state,ee_ok
        goto    FillWaitForWrite
  .assert "W == 0x01, '*** FAILED read_current error'"
	nop

        movf    INDF,W
        xorwf   eedata,W
        skpnz
	goto loop_ok
    .assert "'*** FAILED data mis-match'"
         incf   failures,f      ; failed 

loop_ok:
        incf    FSR,F
        incf    temp,F
        btfss   temp,4
         goto   FillLoop

    ; If we reach this point then it means that we didn't
    ; get stuck in the loop. 
	bcf	failures,7
	movf	failures,W
	skpz
	 goto	failed
done:
  .assert  "'*** PASSED 12CE518 internal EEPROM test'"
        goto    done

failed:
  .assert  "'*** FAILED 12CE518 internal EEPROM test'"
	goto	$

	end
