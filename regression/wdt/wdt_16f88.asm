
   ;;  16F88 WDT tests
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


   list p=16f88

include "p16f88.inc"
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

  .sim "p16f88.BreakOnReset = false"
  .sim "break c 0x100000"
  .sim "p16f88.frequency=10000"
  .sim "symbol cycleCounter=0"

; are we seeing a WDT reset?

	btfss	STATUS,NOT_TO
	goto	wdt_reset

;
;	WDT should be about 2.3 seconds in gpsim with default postscaler of 128
;	(on real device may be soon as 0.9 sec)
;	In the following test WDT should be longer then delay1, but shorter
;	than twice delay1. The clrwdt thus prevents the WDT from going off.

        BANKSEL	OSCCON
	movlw	0xff
	movwf	OSCCON
   .assert "osccon == 0x73, '*** FAILED 18f88 osccon writable bits'"
	nop
	bcf	OSCCON,SCS0
	movlw	0x72	; set RC clock 8MHz
	movwf	OSCCON
   .assert "osccon == 0x76, '*** FAILED 18f88 osccon IOFS OK'"
	nop
	movlw   0x12    ; set RC clock 125 kHz
	movwf	OSCCON
	sleep
	nop
	nop
    .command "cycleCounter = cycles"
	nop
	btfss	OSCCON,IOFS	; wait for stable
	goto	$-1
    ; at 125 kHz 4ms delay = 125 cycles
    .assert "((cycles - cycleCounter) >= 125) && ((cycles - cycleCounter) <= 129), '*** FAILED 16f88 RC stable delay from sleep'"
	nop
	movlw	0x02	; set RC clock 31 kHz
	movwf	OSCCON
   .assert "osccon == 0x0e, '*** FAILED 18f88 osccon IOFS OK high to low'"
	nop
	bsf	OSCCON,IRCF2	; set RC clock 1MHz  
    .command "cycleCounter = cycles"
	nop
	btfss	OSCCON,IOFS	; wait for stable
	goto	$-1
    ; at 1MHz 4ms delay = 1000 cycles
    .assert "((cycles - cycleCounter) >= 1000) && ((cycles - cycleCounter) <= 1004), '*** FAILED 16f88 RC stable delay low to high'"
	nop
	bcf	OSCCON,SCS1 	; turn off RC clock
    .assert "p16f88.frequency == 10000.,'*** FAILED 18f88 frequency '"
	nop


	call	delay1
        clrwdt
	call	delay1

;
;	My reading of the specs indicate sleep should continue (no reset)
;	when the WDT goes off sleeps continue
	sleep
	nop
  .assert "(status & 0x18) == 0x00,'*** FAILED 16f88 status after sleep'"
	nop

	incf	phase, F
;
;	Test the WDT cause a reset in under 2 * delay1
	clrwdt
  .assert "(status & 0x18) == 0x18,'*** FAILED 16f88 status after clrwdt'"
	nop
	call	delay1
	call	delay1


FAILED:
  .assert "'*** FAILED p16f88 no WDT reset'"
	goto	$

FAILED2:
  .assert "'*** FAILED p16f88 unexpected WDT reset'"
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

  .assert "(status & 0x18) == 0x08,'*** FAILED 16f88 status after WDT Reset'"
    nop
    .assert "'*** PASSED p16f88 WDT'"
    goto $
  end
