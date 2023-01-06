
   ;;  16F685 WDT tests
   ;;
   ;; This regression test, tests the following WDT functions
   ;;	WDT enabled by default without configuration word
   ;;   OPTION_REG Postscaler Rate select bits work
   ;;	clrwdt works
   ;;	WDT wakes up sleep without reset
   ;;	WDT causes reset
   ;;	
   ;;     The test assumes that the clock speed is about 10 KHz
   ;;


   list p=16f685

include "p16f685.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm


   cblock 0x40

        temp
	tmp2
	phase

   endc

	ORG	0

  .sim "p16f685.BreakOnReset = false"
  .sim "break c 0x10000"
  .sim "p16f685.frequency=10000"

; are we seeing a WDT reset?

	btfss	STATUS,NOT_TO
	goto	wdt_reset

;
;	WDT should be about 2.3 seconds in gpsim with default postscaler of 128
;	(on real device may be soon as 0.9 sec)
;	In the following test WDT should be longer then delay1, but shorter
;	than twice delay1. The clrwdt thus prevents the WDT from going off.

	call	delay1
        clrwdt
	call	delay1

;
;	My reading of the specs indicate sleep should continue (no reset)
;	when the WDT goes off sleeps continue
	sleep
	nop
  .assert "(status & 0x18) == 0x00,'*** FAILED 16f685 status after sleep'"
	nop

	incf	phase, F
;
;	Test the WDT cause a reset in under 2 * delay1
	clrwdt
  .assert "(status & 0x18) == 0x18,'*** FAILED 16f685 status after clrwdt'"
	nop
	call	delay1
	call	delay1


FAILED:
  .assert "'*** FAILED p16f685 no WDT reset'"
	goto	$

FAILED2:
  .assert "'*** FAILED p16f685 unexpected WDT reset'"
	goto	$

;	delay about 1.85 seconds
delay1
	movlw	0x06
	movwf	tmp2
Oloop
        clrf    temp     ;
LOOP1
        decfsz  temp, F
        goto    LOOP1
	
	decfsz	tmp2,F
	goto	Oloop
	return

wdt_reset:

    btfss phase,0
    goto FAILED2

  .assert "(status & 0x18) == 0x08,'*** FAILED 16f685 status after WDT Reset'"
    nop
    .assert "'*** PASSED p16f685 WDT'"
    goto $
  end
