
   ;;  16F88 WDT tests
   ;;
   ;; This regression test, tests the following WDT functions
   ;;   WDT disabled by configuration word
   ;;	WDT turned on by SWDTEN bit of WDTCON register



   list p=16f88

include "p16f88.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm

   __CONFIG  _CONFIG1, _CP_OFF & _WDT_OFF & _FOSC_EXTRCIO  & _MCLR_OFF
   __CONFIG  _CONFIG2, _IESO_OFF & _FCMEN_OFF

   cblock 0x20

        temp
	tmp2
	phase

   endc



	ORG	0

  .sim "p16f88.BreakOnReset = false"
  .sim "break c 0x10000"
  .sim "p16f88.frequency=10000"


	btfss	STATUS,NOT_TO
	goto	wdt_reset

	; Delay past time WDT expected to go off
	call delay1
	call delay1
	
	movlw	0x07	; turn on WDT with prescale 256
	banksel WDTCON
	movwf	WDTCON
	banksel temp
	incf    phase, F

	; WDT should now go off in less than one delay
	call	delay1
	nop

done:
  .assert "'*** FAILED p16f88 SWDTEN did not turn on WDT'"
    nop

	GOTO	$


wdt_reset:
    btfss phase,0
    goto FAILED

  .assert "(status & 0x18) == 0x08,'*** FAILED 16f88 status after WDT Reset'"
    nop
    .assert "'*** PASSED p16f88 no WDT, SWDTEN'"
    goto $

FAILED:

    .assert "'*** FAILED p16f88 unexpected WDT triggered'"
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
