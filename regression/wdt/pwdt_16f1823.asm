
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

   __CONFIG  _CONFIG1, _CP_OFF & _WDTE_SWDTEN &  _MCLRE_OFF &  _IESO_OFF & _FCMEN_OFF
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

	; WDT should not go off, delay past time WDT would be expected to go off
	call delay1
	call delay1
	
	movlw	0x06|(1>>SWDTEN)	; turn on WDT with prescale 256
	banksel WDTCON
	movwf	WDTCON
	banksel temp
	incf    phase, F

	; WDT should now go off in less than one delay
	call	delay1
	nop

done:
  .assert "'*** FAILED p16f1823 SWDTEN did not turn on WDT'"
    nop

	GOTO	$


wdt_reset:
    btfsc phase,0
    goto  sleep_test
    
    goto FAILED


sleep_test:
    movlw	0x06|(1>>SWDTEN)	; turn on WDT with prescale 256
    banksel WDTCON
    movwf   WDTCON
    banksel phase
    incf    phase, F
    sleep
    nop

    movlw   0x25	; test max bits
    banksel WDTCON
    movwf   WDTCON
  .assert "wdtcon == 0x25, '*** FAILED 16f1823 writable bits of WDTCON'"
   nop

    .assert "'*** PASSED p16f1823 config WDTE_SWDTEN'"
    nop
    goto $

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
