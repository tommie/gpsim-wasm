	; 12C509 gpsim regression test
	;
	; The purpose of this test is to verify that the 'C509
	; is implemented correctly. Here's a list of specific things
	; tested:	
	;
	; 1) Reset conditions
	; 2) WDT
	; 3) Sleep
	; 4) Wakeup on PIN change.

	list	p=12c509
        include <p12c509.inc>
        include <coff.inc>

  __CONFIG _WDT_ON & _MCLRE_ON

        radix   dec

; Printf Command
.command macro x
  .direct "C", x
  endm

GPR_DATA  UDATA
ResetSequence RES 1
optionShadow  RES 1

  GLOBAL optionShadow, ResetSequence


    ; Define the reset conditions to be checked.

eRSTSequence_PowerOnReset	equ	1
eRSTSequence_AwakeMCLR		equ	2
eRSTSequence_AwakeWDT		equ	3
eRSTSequence_AwakeIO		equ	4
eRSTSequence_WDTTimeOut		equ	5

;----------------------------------------------------------------------
;   ********************* STARTUP LOCATION  ***************************
;----------------------------------------------------------------------
START  CODE    0x000                    ; 


;############################################################
;# Create a stimulus to simulate a switch
;

   .sim "module lib libgpsim_modules"
   .sim "module load pulsegen PG1"
   .sim "module load pulsegen MCLR"

   .sim "MCLR.initial = 5.0"

   .sim "PG1.clear = 0"   ; At cycle 0, 
   .sim "PG1.set = 1"
;   .sim "PG1.clear = 0x200"
;   .sim "PG1.period = 0x400"

  ;############################################################

  .sim "node nSwitch"
  .sim "attach nSwitch gpio0 PG1.pin"

  .sim "node nMCLR"
  .sim "attach nMCLR gpio3 MCLR.pin"

  ;############################################################
  .sim "symbol resetCounter=1"

   ;  NOT_GPWU = 0  - Enable wakeup if I/O pins 0,1, or 3 change states
   ;  NOT_GPPU = 1  - Disable weak pullups on I/O pins 0,1, and 3 
   ;  TOCS = 0 - Let the clock source for TMR0 be the internal fosc/4
   ;  TOSE = 0 - don't care - TMR0 source edge.
   ;  PSA = 1 - assign Prescale to the WDT
   ;  PS2:0 = 000 - prescale = 2^0=1

  .sim "optionShadow=8"      ; PSA=1

  .sim "ResetSequence=0"

  .sim ".BreakOnReset = false"

  ; Set a cycle break point far in the future in case the resets fail.
  .sim "break c 0x1000000"

;break e START
;break e Activate
;break e AbortActivate
;break e SendIsComplete
;break e SwitchWasJustPressed









bSWITCH equ 0

  .assert "option==0xff"
  .assert "(tris&0x3f)==0x3f"
        MOVWF   OSCCAL          ; put calibration into oscillator cal reg


   ; OPTION register setup

        MOVF    optionShadow,W  ;optionShadow is initialized by the gpsim script
        OPTION

   ; GPIO setup
   ; If this is a PowerOn reset, then the state of the GPIO output register
   ; is unknown. If this is not a PowerOn reset then the GPIO pins have
   ; the same state they had prior to the reset.

   ;
   ; TRIS register setup - The I/O pins are always configured as Inputs after
   ; any reset.

        MOVLW   1<<bSWITCH      ;Only the switch is an input
        TRIS    GPIO            ;

   ; Now examine why the RESET condition occurred.

	BTFSC	STATUS,GPWUF	;Did we just wake up from SLEEP due
         goto   AwakeIO		;to a change in the I/O pins?

	BTFSS	STATUS,NOT_TO
	 goto   TO_is_low

	BTFSS	STATUS,NOT_PD
	 goto   AwakeMCLR	;/TO=1, /PD=0 ==> /MCLR while sleeping

	goto	PowerOnReset	;/TO=1, /PD=1 ==> Power has just been applied

TO_is_low

	BTFSS	STATUS,NOT_PD
	 goto   AwakeWDT	;/TO=0, /PD=0 ==> WDT while sleeping

	goto	WDTTimeOut	;/TO=0, /PD=1 ==> WDT while not sleeping


;========================================================================
;
;  PowerOnReset
;
;========================================================================
PowerOnReset:

        movf    ResetSequence,W
        XORLW   eRSTSequence_AwakeMCLR
        skpnz
         goto   done

  .assert "resetCounter==1,'*** FAILED Power On Reset'"
	MOVLW	eRSTSequence_PowerOnReset
	MOVWF	ResetSequence
  .command "resetCounter = resetCounter+1"
	CLRWDT
	SLEEP

;========================================================================
;
;  AwakeWDT - WDT timed out while we were sleeping.
;
;========================================================================
AwakeWDT:

	MOVLW	eRSTSequence_AwakeWDT
  .assert "resetCounter==2,'*** FAILED WDT Reset'"
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	CLRWDT

    ; loop until WDT times out
here:   goto    here


;========================================================================
;
;  WDTTimeOut - WatchDog timer timed out while we were awake.
;
;========================================================================
WDTTimeOut:

  .assert "resetCounter==3,'*** FAILED WDT timeout'"
	MOVLW	eRSTSequence_WDTTimeOut
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	CLRWDT

  .command "PG1.clear = cycles+100"
        nop
    
	SLEEP


;========================================================================
;
;  AwakeIO - an I/O pin changed states to awaken us from sleep.
;
;========================================================================
AwakeIO:
	MOVLW	eRSTSequence_AwakeIO
  .assert "resetCounter==4,'*** FAILED Wakeup on I/O pin change'"
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"

	CLRWDT

    ; reset the processor by driving /MCLR low for a few cycles 
  .command "MCLR.clear = cycles+100"
  .command "MCLR.set = cycles+110"
        nop

	SLEEP

;========================================================================
;
;  AwakeMCLR - MCLR went low while we were sleeping.
;
;========================================================================
AwakeMCLR:

	MOVLW	eRSTSequence_AwakeMCLR
  .assert "resetCounter==5,'*** FAILED /MCLR Reset'"
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	CLRWDT

    ; reset the processor by driving /MCLR low for a few cycles 
  .command "MCLR.clear = cycles+100"
  .command "MCLR.set = cycles+110"
        nop

waitForMCLR:    goto waitForMCLR


  .assert  "'*** PASSED 12c509 Sleep and Reset test'"

done:
	goto	done

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x3ff              ; processor reset vector

OSC_CAL_VALUE	EQU	0x80

	MOVLW	OSC_CAL_VALUE
	
	end
