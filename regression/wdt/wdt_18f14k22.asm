
   ;;  18F14k22 WDT tests
   ;;
   ;; This regression test, tests the following WDT functions
   ;;	WDT enabled by default without configuration word
   ;;	clrwdt works
   ;;	WDT wakes up sleep without reset
   ;;	WDT causes reset
   ;;	
   ;;     The test assumes that the clock speed is about 10 kHz
   ;;


   list p=18f14k22

include "p18f14k22.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm

   CONFIG WDTEN=ON, WDTPS = 512
;   CONFIG WDTEN=ON, WDTPS = 1

   cblock 0x20

        temp
	tmp2
	phase

   endc

	ORG	0

  .sim "p18f14k22.BreakOnReset = false"
;  .sim "break c 0x100000"
  .sim "p18f14k22.frequency=10000"

; are we seeing a WDT reset?

	btfss	RCON,NOT_TO
	goto	wdt_reset

; test weak pullup on porta and portb
	clrf	ANSEL
	clrf	ANSELH
	bcf	INTCON2,NOT_RABPU

   .assert "(porta == 0x7) && (portb == 0xf0),  '*** FAILED p18f14k22 weak pullup'"
	nop

	clrf	WPUA
	clrf	WPUB	


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
	
	nop
  .assert "(rcon & 0x0c) == 0x00,'*** FAILED p18f14k22 status after sleep'"
	nop
  .assert "(tmr0l - W) <= 0x4,'*** FAILED p18f14k22 TMR0 stops during sleep'"
	nop


	incf	phase, F
;
;	Test the WDT cause a reset in under 2 * delay1
	clrwdt
  .assert "(rcon & 0x0c) == 0x0c,'*** FAILED p18f14k22 status after clrwdt'"
	nop
	call	delay1
	call	delay1


FAILED:
  .assert "'*** FAILED p18f14k22 no WDT reset'"
	nop
	goto	$

FAILED2:
  .assert "'*** FAILED p18f14k22 unexpected WDT reset'"
	nop
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

  .assert "(rcon & 0x0c) == 0x04,'*** FAILED p18f14k22 status after WDT Reset'"
    nop
    .assert "'*** PASSED p18f14k22 WDT'"
    goto $
  end
