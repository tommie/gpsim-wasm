	; 16f676 gpsim regression test
	;
	; The purpose of this test is to verify that the 16f676
	; is implemented correctly. Here's a list of specific things
	; tested:	
	;
	; 1) Power On Reset
	; 2) WDT during sleep
	; 3) WDT during execution
	; 4) Interrupt on PIN change during sleep
	; 5) MCLR during sleep
	; 6) MCLR during execution

	list	p=16f676
        include <p16f676.inc>
        include <coff.inc>

  __CONFIG _WDT_ON & _MCLRE_ON

        radix   dec

; Printf Command
.command macro x
  .direct "C", x
  endm

GPR_DATA  UDATA_SHR
ResetSequence	RES 1
optionShadow 	RES 1
aint_cnt	RES 1
w_temp		RES 1
status_temp	RES 1

  GLOBAL optionShadow, ResetSequence, aint_cnt


    ; Define the reset conditions to be checked.

eRSTSequence_PowerOnReset	equ	1
eRSTSequence_AwakeMCLR		equ	2
eRSTSequence_AwakeWDT		equ	3
eRSTSequence_AwakeIO		equ	4
eRSTSequence_WDTTimeOut		equ	5

;----------------------------------------------------------------------
;   ********************* Reset Vector LOCATION  ***************************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

NT_VECTOR   CODE    0x004               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

        btfsc   INTCON,RAIF
            goto raif_int

        .assert "'***FAILED p16f676 unexpected interrupt'"
        nop
exit_int:
                                                                                
        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,f
        swapf   w_temp,w
        retfie


; Interrupt from porta state change
raif_int
        incf    aint_cnt,F
	bcf	INTCON,RAIF
  .assert "(intcon & 1) == 1, '***FAILED p16f676 RAIF not cleared with match condition'"
	nop 
	movf	PORTA,W		; reading ports should clear match condition
	bcf	INTCON,RAIF
  .assert "(intcon & 1) == 0, '***FAILED p16f676 RAIF is clear'"
	nop 
        goto    exit_int

;############################################################
;# Create a stimulus to simulate a switch
;

   .sim "module lib libgpsim_modules"
   .sim "p16f676.xpos = 72"
   .sim "p16f676.ypos = 72"

   .sim "module load pulsegen PG1"
   .sim "PG1.clear = 0"
   .sim "PG1.period = 0"
   .sim "PG1.set = 1"
   .sim "PG1.xpos = 228"
   .sim "PG1.ypos = 84"

   .sim "module load pulsegen MCLRE"
   .sim "MCLRE.clear = 0"
   .sim "MCLRE.period = 0"
   .sim "MCLRE.set = 1"
   .sim "MCLRE.xpos = 84"
   .sim "MCLRE.ypos = 204"
   .sim "MCLRE.initial = 5.0"


  ;############################################################

  .sim "node nSwitch"
  .sim "attach nSwitch porta0 PG1.pin"

  .sim "node nMCLR"
  .sim "attach nMCLR MCLR MCLRE.pin"

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


bSWITCH equ 0
start

  .assert "option_reg==0xff, '***FAILED p16f676 reset, option 0xff'"
  .assert "(trisa&0x3f)==0x3f, '***FAILED p16f676 reset, trisa 0x3f'"
;RRR        MOVWF   OSCCAL          ; put calibration into oscillator cal reg

   ; PORTA setup
   ; If this is a PowerOn reset, then the state of the PORTA output register
   ; is unknown. If this is not a PowerOn reset then the PORTA pins have
   ; the same state they had prior to the reset.

   ;
   ; TRIS register setup - The I/O pins are always configured as Inputs after
   ; any reset.

        MOVF    optionShadow,W  ;optionShadow is initialized by the gpsim script
	BANKSEL	TRISA
	MOVWF	OPTION_REG
        MOVLW   1<<bSWITCH      ;Only the switch is an input
	MOVWF	TRISA
	CLRF	ANSEL		; turn off ADC
	MOVLW	1		; interrupt in porta0 state change
	MOVWF	IOCA
	BANKSEL PORTA
	MOVLW	7
	MOVWF	CMCON		; turn off comparator
	MOVLW	(1 << GIE) |  (1 << RAIE)
	MOVWF	INTCON		; Enable porta interrupts

   ; Now examine why the RESET condition occurred.

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
	nop
	MOVLW	eRSTSequence_PowerOnReset
	MOVWF	ResetSequence
  .command "resetCounter = resetCounter+1"
	nop
	
	SLEEP
	nop

;========================================================================
;
;  AwakeWDT - WDT timed out while we were sleeping.
;
;========================================================================
AwakeWDT:

	MOVLW	eRSTSequence_AwakeWDT
  .assert "resetCounter==2,'*** FAILED WDT Reset'"
	nop
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	nop
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
	nop
	MOVLW	eRSTSequence_WDTTimeOut
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	nop
	CLRWDT

  .command "PG1.clear = cycles+100"
        nop
    
	CLRF	aint_cnt 
	CLRF	CMCON		; porta0, porta1 analog
	SLEEP
	nop

;========================================================================
;
;  AwakeIO - an I/O pin changed state when in analog mode
;	     This should not wakeup from sleep (continue with WDT wakeup)
;
;========================================================================
    .assert "aint_cnt==0, '*** FAILED Wakeup on analog pin change'"
	nop

	MOVLW	7
	MOVWF	CMCON		; porta0, porta1 in digital mode
	CLRF	aint_cnt	; we may get an interrupt on this change

  .command "PG1.set = cycles+100"
        nop

	SLEEP
	nop
;========================================================================
;
;  AwakeIO - an I/O pin changed states to awaken us from sleep.
;
;========================================================================
AwakeIO:
  .assert "aint_cnt==1, '*** FAILED No Wakeup on I/O pin change'"
        nop
	MOVLW	eRSTSequence_AwakeIO
  .assert "resetCounter==4,'*** FAILED Unexpected Wakeup on I/O pin change'"
	nop
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	nop

	CLRWDT

    ; reset the processor by driving /MCLR low for a few cycles 
  .command "MCLRE.clear = cycles+100"
  .command "MCLRE.set = cycles+110"
        nop

	SLEEP
	nop

  .assert "'*** FAILED Unexpected Wakeup waiting for MCLR'"
	nop

;========================================================================
;
;  AwakeMCLR - MCLR went low while we were sleeping.
;
;========================================================================
AwakeMCLR:

	MOVLW	eRSTSequence_AwakeMCLR
  .assert "resetCounter==5,'*** FAILED /MCLR Reset'"
	nop

	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	nop
	CLRWDT

    ; reset the processor by driving /MCLR low for a few cycles 
  .command "MCLRE.clear = cycles+100"
  .command "MCLRE.set = cycles+110"
        nop

waitForMCLR:    goto waitForMCLR


  .assert  "'*** PASSED 16f676 Sleep and Reset test'"

done:
	goto	done

	end
