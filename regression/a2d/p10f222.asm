	list      p=10F222            ; list directive to define processor
	include <p10f222.inc>        ; processor specific variable definitions
 	include <coff.inc>

	__CONFIG   _MCLRE_OFF & _CP_OFF & _WDT_ON & _MCPU_OFF & _IOFSCS_8MHZ

  ; The purpose of this test is to check the functioning of the ADC for a P10F222
  ;  1> IO pin control
  ;  2> Channel 0
  ;  3> Channel 1
  ;  4> Internal reference on channels 2 and 3
  ;  5> Sleep stops ADC and sets ADCON0 bits on waking

; Printf Command
.command macro x
  .direct "C", x
  endm


;**********************************************************************
RESET_VECTOR	CODE   0x1FF       ; processor reset vector

; Internal RC calibration value is placed at location 0xFF by Microchip
; as a movlw k, where the k is a literal value.

MAIN	CODE	  0x000
   .sim "p10f222.BreakOnReset = false"
   .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
   .sim "module load pullup V1"
   .sim "V1.resistance = 100.0"
   .sim "node na1"
   .sim "attach na1 V1.pin gpio0 gpio3"
   .sim "V1.voltage=1.0"
   .sim "module load pullup V2"
   .sim "V2.resistance = 1000.0"
   .sim "node na2"
   .sim "attach na2 V2.pin gpio1 gpio2"
   .sim "V2.voltage=2.0"
   .sim "V1.xpos = 72"
   .sim "V1.ypos = 36"
   .sim "V2.xpos = 72"
   .sim "V2.ypos = 156"



; are we seeing a WDT reset?

        btfss   STATUS,NOT_TO
        goto    after_sleep


	movlw	0x7f
	movwf 	  OSCCAL        ; update register with factory cal value
	movlw	0x81
	movwf 	  OSCCAL        ; update register with factory cal value
	bcf	  OSCCAL,0	; Make sure GP2 is not FOSC4
	GOTO	  START

;================================================================

START
;			No wakeup or pullup on pins
;			clock T0 high to low, and T0 prescale 1
	MOVLW		1<<NOT_GPWU | 1<<NOT_GPPU | 1<<T0SE |  0x0
	OPTION				

	CLRF		GPIO	
	; Set gpio0 and gpio1 as output but ADC should keep them as input
	MOVLW		B'000001100'
	TRIS		GPIO				
;if all pins input V1 and V2 pull voltage high
    .assert "gpio != 0x0,'FAILED 10f222 a2d holding pins as input'"
	nop
	CLRF		ADCON0	
; TRIS back in control pins 0,1 pull output low
    .assert "gpio == 0x0,'FAILED 10f222 a2d release pin control'"
	nop
; If ADON os off GO cannot be set
	MOVLW		0xc0
	MOVWF		ADCON0	
	BSF		ADCON0,GO
 	MOVF		ADCON0,W
   .assert "W == 0xc0, 'FAILED 10f222 a2d GO set when ADON clear'"
	NOP
; read AN0 1V
	MOVLW		0xc1
	MOVWF		ADCON0	
	BSF		ADCON0,GO
 	MOVF		ADCON0,W

        btfsc   ADCON0,GO        ;Wait for the !DONE to be clear
        goto   $-1

		
   .assert "adres == 0x33, 'FAILED 10f222 AN0=1V'"
        nop

; read AN1 2V
	MOVLW		0xc5
	MOVWF		ADCON0	
	BSF		ADCON0,GO
 	MOVF		ADCON0,W

        btfsc   ADCON0,GO        ;Wait for the !DONE to be clear
        goto   $-1

		
   .assert "adres == 0x66, 'FAILED 10f222 AN1=2V'"
        nop

; read 0.6v CHS=10
	MOVLW		0xc9
	MOVWF		ADCON0	
	BSF		ADCON0,GO
 	MOVF		ADCON0,W

        btfsc   ADCON0,GO        ;Wait for the !DONE to be clear
        goto   $-1

		
   .assert "adres == 0x1f, 'FAILED 10f222 read 0.6V CHS=10'"
        nop

; read 0.6v CHS=11
	MOVLW		0xcf
	MOVWF		ADCON0	

        btfsc   ADCON0,GO        ;Wait for the !DONE to be clear
        goto   $-1

		
   .assert "adres == 0x1f, 'FAILED 10f222 read 0.6V CHS=11'"
        nop

;	A2D should stop in sleep and change adcon0 bits
	MOVLW		0xcf
	MOVWF		ADCON0	
  	MOVF		TMR0,W
	SLEEP
after_sleep:
	NOP
  .assert "(tmr0 - W) == 0, 'FAILED 10f222 tmr0 after sleep'"
	nop

  .assert "(adcon0 & 0xcf) == 0xcc, 'FAILED 10f222 adcon0 after sleep'"
	nop


  .assert  "'*** PASSED 10F222 a2d test'"
        goto    $-1

	END

