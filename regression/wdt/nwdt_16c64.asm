
   ;;  16C64 WDT tests
   ;;
   ;; This regression test, tests the following WDT functions
   ;;   WDT disabled by configuration word



   list p=16c64

include "p16c64.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm

   __CONFIG    _WDT_OFF & _RC_OSC 

   cblock 0x20

        temp
	tmp2
   endc



	ORG	0

  .sim "p16c64.BreakOnReset = false"
  .sim "p16c64.frequency = 10000"
  .sim "break c 0x10000"

	btfss	STATUS,NOT_TO
	goto	wdt_reset

	; Delay past time WDT expected to go off
	call delay1
	call delay1

done:
  .assert "'*** PASSED p16c64 no WDT test'"
    nop

	GOTO	$

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



wdt_reset:
    .assert "'*** FAILED p16c64 unexpected WDT triggered'"
    nop
    goto $
  end
