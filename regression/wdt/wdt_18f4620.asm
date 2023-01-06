
   ;;  18F452 WDT tests
   ;;
   ;; This regression test, tests the following WDT functions
   ;;	WDT enabled by default without configuration word
   ;;	clrwdt works
   ;;	WDT wakes up sleep without reset
   ;;	WDT causes reset
   ;;	
   ;;     The test assumes that the clock speed is about 10 kHz
   ;;


   list p=18f4620

include "p18f4620.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm


   cblock 0x20

        temp
	tmp2
	phase

   endc

	ORG	0

  .sim "p18f4620.BreakOnReset = false"
  .sim "break c 0x10000"
  .sim "p18f4620.frequency=10000"

; are we seeing a WDT reset?

	btfss	RCON,NOT_TO
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
;	During sleep, іf the WDT goes off, PC = PC + 2 
        bcf	T0CON,5	; set tmr0 as timer
	movf	TMR0L,W
	sleep
	
  .assert "(rcon & 0x0c) == 0x00,'*** FAILED p18f4620 status after sleep'"
	nop
  .assert "(tmr0l - W) <= 0x4,'*** FAILED p18f4620 TMR0 stops during sleep'"
	nop


	incf	phase, F
;
;	Test the WDT cause a reset in under 2 * delay1
	clrwdt
  .assert "(rcon & 0x0c) == 0x0c,'*** FAILED p18f4620 status after clrwdt'"
	call	delay1
	call	delay1


FAILED:
  .assert "'*** FAILED p18f4620 no WDT reset'"
	goto	$

FAILED2:
  .assert "'*** FAILED p18f4620 unexpected WDT reset'"
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

  .assert "(rcon & 0x0c) == 0x04,'*** FAILED p18f4620 status after WDT Reset'"
    nop
    .assert "'*** PASSED p18f4620 WDT'"
    goto $
  end
