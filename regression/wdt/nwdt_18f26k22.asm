
   ;;  18F26k22 WDT tests
   ;;
   ;; This regression test, tests the following WDT functions
   ;;   WDT disabled by configuration word
   ;;	WDT turned on by SWDTEN bit of WDTCON register



   list p=18f26k22

include "p18f26k22.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm

   CONFIG WDTEN=SWON, WDTPS = 64

   cblock 0x20

        temp
	tmp2
	phase

   endc



	ORG	0

  .sim "p18f26k22.BreakOnReset = false"
  .sim "break c 0x10000"
  .sim "p18f26k22.frequency=10000"


	btfss	RCON,NOT_TO
	goto	wdt_reset

	; Delay past time WDT expected to go off
	call delay1
	call delay1
	
	banksel WDTCON
	bsf	WDTCON,SWDTEN
	banksel temp
	incf    phase, F

	; WDT should now go off in less than one delay
	call	delay1
	nop

done:
  .assert "'*** FAILED p18f26k22 SWDTEN did not turn on WDT'"
    nop

	GOTO	$


wdt_reset:
    btfss phase,0
    goto FAILED

  .assert "(rcon & 0x0c) == 0x04,'*** FAILED 18f26k22 status after WDT Reset'"
    nop
    .assert "'*** PASSED p18f26k22 no WDT, SWDTEN'"
    goto $

FAILED:

    .assert "'*** FAILED p18f26k22 unexpected WDT triggered'"
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
