        ;; ccp_628.asm
        ;;
        ;; This program attempts to test gpsim for correct behaviour of
        ;; TMR1 and the CCP module on a 16F628. Specifically it is looking 
	;; for a number of bugs that have crept in from time to time.
	;;
        ;; Here are the tests performed:
	;;
	;; -- TMR1 can be pre-loaded while disabled and doesn't get "reset" on enable
	;; -- TMR1 driven Fosc/4 with prescale of 1 counts correctly
	;; -- TMR1 stops counting while disabled, as assessed once enabled again
	;; -- TMR1L and TMR1H contain the correct values when the CCP compare mode trips
        ;; -- CCP1 in output compare mode actually drives the output pin

 list p=16f628
 include "p16f628.inc"
 include "coff.inc"

ifndef _CPD_OFF
#define _CPD_OFF _DATA_CP_OFF
endif
 __CONFIG _CP_OFF & _CPD_OFF & _BODEN_ON & _MCLRE_OFF & _PWRTE_ON & _WDT_OFF & _INTRC_OSC_NOCLKOUT & _LVP_OFF

 cblock 0x30
  time_l
  time_h
  failures
 endc

 org 0
 goto main

 org 4
isr:
 .assert "tmr1l == 0x81 && tmr1h == 0x02, '*** FAILED TMR1 value does not match CCP'"
 nop
 
 btfss  PORTB,3
 .assert "'*** FAILED CCP1 output compare does not set pin'"
 bsf    PORTB,7     ; external indication of problem

 bcf    PIR1,CCP1IF
 clrf   CCP1CON
 retfie


main:
 .sim "break c 0x10000"
 banksel TRISB
 movlw  0x00
 movwf  TRISB
 bsf    PIE1,CCP1IE
 bsf	INTCON,PEIE

 banksel PORTB
 clrf   PORTB

; Disable timer 1
 bcf T1CON, TMR1ON

 movlw 0x34
 movwf TMR1L
 movlw 0x12
 movwf TMR1H

; counter is set to 0x1234 - works
 movf   TMR1L,w
 nop
 .assert "W == 0x34, '*** FAILED 14bit TMR1 cannot write TMR1L'"
 nop

 movf   TMR1H,w
 nop
 .assert "W == 0x12, '*** FAILED 14bit TMR1 cannot write TMR1H'"

 nop

; Enable timer 1
 bsf T1CON, TMR1ON

 .assert "(tmr1l == 0x35 && tmr1h == 0x12), '*** FAILED 14bit TMR1 value change on enable'"
 nop

 bcf    T1CON,TMR1ON
 movf   TMR1H,w
 movwf  time_h
 movf   TMR1L,w
 movwf  time_l
 .assert "W == 0x36, '*** FAILED 14bit TMR1 not running'"
 nop
 nop
 nop
 nop

 bsf T1CON, TMR1ON
 movf   TMR1L,w
 subwf  time_l,w

 .assert "W == 0xFF, '*** FAILED 14bit TMR1 keeps counting while disabled ***'"
 nop
;
;	test t0 running after wakeup from sleep by t1
;
	banksel OPTION_REG
        bsf     OPTION_REG,PSA       ; bypass precounter
	bcf	OPTION_REG,T0CS
        bsf     PIE1,TMR1IE     ; turn on interrupt
 	bcf     INTCON,GIE
	banksel T1CON
        bsf     T1CON,NOT_T1SYNC    ;sync off
        bsf     T1CON,TMR1ON    ; start timer 1
        sleep
        movf    TMR0,W
        bcf     T1CON,NOT_T1SYNC    ;sync off
	bcf	PIR1,TMR1IF
        nop
        nop
  .assert "tmr0 != W, '*** FAILED CCP_628  test- TMR0 stopped after sleep'"
        nop


 clrf   TMR1L
 clrf   TMR1H

 movlw  0x7F
 movwf  CCPR1L
 movlw  0x02
 movwf  CCPR1H
 movlw  0x08
 movwf  CCP1CON

 nop
 btfsc  PORTB,3
 .assert  "'*** FAILED CCP_628 output set too soon'"
	nop
 bsf    PORTB,6

 bsf    INTCON,PEIE
 bsf    INTCON,GIE

 clrf   time_l
dly1:
 nop
 nop
 decfsz time_l,F
 goto   dly1

 .assert  "ccp1con == 0, '*** FAILED CCP_628 event not triggered'"

 nop
done:
 .assert  "'*** PASSED 16F628 TMR1 & CCP test'"
 goto    $

failed:
 movlw   1
 movwf   failures
 .assert  "'*** FAILED 16F628 TMR1 & CCP test'"
 goto    done

 end
