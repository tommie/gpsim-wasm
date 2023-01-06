
   ;;  16F684 tests
   ;;
   ;; This regression test exercises the 16f684.  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  1) Verify no WDT with _WDT_OFF in __CONFIG
   ;;  2) Verify WDT with WDTCON,SWDTEN set


   list p=16f684

include "p16f684.inc"
include <coff.inc>

.command macro x
  .direct "C", x
  endm

	__CONFIG  _CP_OFF & _WDT_OFF & _MCLRE_OFF

   cblock 0x40

        temp
	flag
	

   endc



	ORG	0

  .sim "p16f684.BreakOnReset = false"
  .sim "break c 0x100000"
  .sim "p16f684.frequency=10000"

	btfss	STATUS,NOT_TO
	goto	wdt_reset

	clrf    flag
	banksel OPTION_REG
	bcf	OPTION_REG,PSA
        BANKSEL TRISA
        clrf    TRISA
;       bcf     OPTION_REG,NOT_RABPU ; enable pullups on portA
        BANKSEL PORTA
        movlw   0xff
        movwf   PORTA

	banksel PORTA
	call 	delay
	incf	flag
	banksel WDTCON
	bsf	WDTCON,SWDTEN	; turn on WDT
	bcf	OPTION_REG,PSA  
	banksel PORTA
	call	delay

   .assert "'*** FAILED p16f684 no WDT with SWDTEN'"
	nop


done:
  .assert "'*** PASSED p16f684 no WDT test'"
    nop

	GOTO	$


wdt_reset:
	btfsc	flag,0
	goto	done
    .assert "'*** FAILED p16f684 unexpected WDT triggered'"
    nop
    goto $

delay
        clrf    temp     ;
LOOP2   goto	$+1
	goto	$+1
	goto	$+1
	decfsz  temp, F   
        goto    LOOP2  
	return
  end
