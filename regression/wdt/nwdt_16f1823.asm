
   ;;  16F88 WDT tests
   ;;
   ;; This regression test, tests the following WDT functions
   ;;   WDT disabled by configuration word
   ;;	WDT turned on by SWDTEN bit of WDTCON register



   list p=16f1823

include "p16f1823.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm

   __CONFIG  _CONFIG1, _CP_OFF & _WDTE_OFF &  _MCLRE_OFF &  _IESO_OFF & _FCMEN_OFF
   __CONFIG  _CONFIG2, _WRT_OFF

   cblock 0x20

        temp
	tmp2
	phase

   endc



	ORG	0

  .sim "p16f1823.BreakOnReset = false"
  .sim "break c 0x20000"
  .sim "p16f1823.frequency=10000"


	btfss	STATUS,NOT_TO
	goto	wdt_reset

	BANKSEL	OSCCON
   .assert "osccon == 0x38, '*** FAILED p16f1823 osccon pow value'"
	nop
	movlw	0xff
	movwf	OSCCON
   .assert "osccon == 0xfb, '*** FAILED p16f1823 osccon write mask'"
	nop

	; WDT should not go off, delay past time WDT would be expected to go off
	call delay1
	call delay1
	
	movlw	0x07	; Set SWDTEN and prescale 256
	banksel WDTCON
	movwf	WDTCON
	banksel temp
	incf    phase, F

	; WDT should not go off, wait one delay
	call	delay1
	nop

done:
  .assert "'*** PASSED p16f1823 config WDTE_OFF'"
    nop

	GOTO	$


wdt_reset:
    
    goto FAILED

FAILED:

    .assert "'*** FAILED p16f1823 unexpected WDT triggered'"
    nop
    goto $

;       delay about 1.85 seconds
delay1
        movlw   0x06
        movwf   tmp2
Oloop
        clrf    temp     ;
LOOP1
        decfsz  temp, F
        goto    LOOP1

        decfsz  tmp2,F
        goto    Oloop
        return


  end
