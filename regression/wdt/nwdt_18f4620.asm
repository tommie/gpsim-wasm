
   ;;  18F4620 WDT tests
   ;;
   ;; This regression test, tests the following WDT functions
   ;;   WDT disabled by configuration word
   ;;	WDT turned on by SWDTEN bit of WDTCON register



   list p=18f4620

include "p18f4620.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm

   CONFIG WDT=OFF, WDTPS=64

   cblock 0x20

        temp
	tmp2
	phase

   endc



	ORG	0

  .sim "p18f4620.BreakOnReset = false"
  .sim "break c 0x10000"
  .sim "p18f4620.frequency=10000"


	btfss	RCON,NOT_TO
	goto	wdt_reset

	; Delay past time WDT expected to go off
	call delay1
	call delay1
	
	movlw	0x01	; turn on WDT 
	banksel WDTCON
	movwf	WDTCON
	banksel temp
	incf    phase, F

	; WDT should now go off in less than one delay
	call	delay1
	nop

done:
  .assert "'*** FAILED p18f4620 SWDTEN did not turn on WDT'"
    nop

	GOTO	$


wdt_reset:
    btfss phase,0
    goto FAILED

  .assert "(rcon & 0x0c) == 0x04,'*** FAILED 18f4620 status after WDT Reset'"
    nop
  .assert "'*** PASSED p18f4620 no WDT, SWDTEN'"
    nop
    goto $

FAILED:

  .assert "'*** FAILED p18f4620 unexpected WDT triggered'"
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
