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

	list	p=16f84
        include <p16f84.inc>
        include <coff.inc>

  __CONFIG _WDT_ON

        radix   dec

; Printf Command
.command macro x
  .direct "C", x
  endm

GPR_DATA  UDATA
ResetSequence RES 1
optionShadow  RES 1
w_temp RES 1
status_temp RES 1
temp1 RES 1

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
   .sim "module load pulsegen P_INTF"
   .sim "module load pulsegen P_RBIF"
   .sim "module load pulsegen RESET"

; Modules:
  .sim "p16f84.xpos = 156"
  .sim "p16f84.ypos = 36"

  .sim "P_INTF.xpos = 72"
  .sim "P_INTF.ypos = 144"

  .sim "P_RBIF.xpos = 300"
  .sim "P_RBIF.ypos = 132"

   .sim "RESET.xpos = 60"
   .sim "RESET.ypos = 36"
   .sim "RESET.initial = 5.0"

   .sim "P_INTF.clear = 0"   ; At cycle 0, 
;   .sim "P_INTF.set = 1"
   .sim "P_RBIF.clear = 0"   ; At cycle 0, 
   .sim "P_RBIF.set = 1"

  ;############################################################

  .sim "node nINTF"
  .sim "attach nINTF portb0 P_INTF.pin"

  .sim "node nRBIF"
  .sim "attach nRBIF portb4 P_RBIF.pin"

  .sim "node nRESET"
;  .sim "attach nRESET p16f84.MCLR RESET.pin"
  .sim "attach nRESET MCLR RESET.pin"

  ;############################################################
  .sim "symbol resetCounter=1"

   ;  NOT_GPWU = 0  - Enable wakeup if I/O pins 0,1, or 3 change states
   ;  NOT_GPPU = 1  - Disable weak pullups on I/O pins 0,1, and 3 
   ;  TOCS = 0 - Let the clock source for TMR0 be the internal fosc/4
   ;  TOSE = 0 - don't care - TMR0 source edge.
   ;  PSA = 1 - assign Prescale to the WDT
   ;  PS2:0 = 000 - prescale = 2^0=1

  .sim "optionShadow=0x48"      ; PSA=1 INTEDG=1

  .sim "ResetSequence=0"

  .sim ".BreakOnReset = false"
  .sim "p16f84.frequency=100000"

  ; Set a cycle break point far in the future in case the resets fail.
  .sim "break c 0x1000000"



RESET_VECTOR  CODE    0x000              ; processor reset vector
  .assert "option_reg==0xff,'*** FAILED 16f84 reset bad OPTION_REG'"
  .assert "(trisa&0x1f)==0x1f, '*** FAILED 16f84 reset bad TRISA'"
  .assert "(trisb&0xff)==0xff, '*** FAILED 16f84 reset bad TRISB'"


        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program




;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x004               ; interrupt vector location

        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

check_rbi:
        btfsc   INTCON,RBIF
         btfss  INTCON,RBIE
          goto  check_int

;        bsf     temp5,1         ;Set a flag to indicate rb4-7 int occured
        bcf     INTCON,RBIF
	movf	PORTB,w
        bcf     INTCON,RBIF
        
check_int:
        btfsc   INTCON,INTF
         btfss  INTCON,INTE
          goto  check_t0

;        bsf     temp5,0         ;Set a flag to indicate rb0 int occured
        bcf     INTCON,INTF

check_t0:
        btfsc   INTCON,T0IF
         btfss  INTCON,T0IE
          goto  exit_int

    ;; tmr0 has rolled over
        
        bcf     INTCON,T0IF     ; Clear the pending interrupt
        bsf     temp1,0         ; Set a flag to indicate rollover
                
exit_int:               

        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,f
        swapf   w_temp,w
        retfie





bSWITCH equ 0

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

   ; OPTION register setup

	BANKSEL	OPTION_REG
        MOVF    optionShadow,W  ;optionShadow is initialized by the gpsim script
        MOVWF   OPTION_REG

   ; GPIO setup
   ; If this is a PowerOn reset, then the state of the GPIO output register
   ; is unknown. If this is not a PowerOn reset then the GPIO pins have
   ; the same state they had prior to the reset.

   ;
   ; TRIS register setup - The I/O pins are always configured as Inputs after
   ; any reset.

RB0 equ 0
RB4 equ 4

        MOVLW   (1<<RB0) | (1<<RB4)     ;RB0 and RB4 are the only inputs.
        BANKSEL TRISB
        MOVWF   TRISB ^ 0x80    ;
        CLRF    TRISA ^ 0x80
        CLRF    STATUS

   ; Now examine why the RESET condition occurred.

	BTFSS	STATUS,NOT_TO
	 goto   TO_is_low

	BTFSS	STATUS,NOT_PD
	 goto   AwakeMCLR	;/TO=1, /PD=0 ==> /MCLR while sleeping
                                ;            -or- Interrupt wake-up from sleep

	goto	PowerOnReset	;/TO=1, /PD=1 ==> Power has just been applied

TO_is_low

	BTFSS	STATUS,NOT_PD
	 goto   AwakeWDT	;/TO=0, /PD=0 ==> WDT while sleeping

	goto	WDTTimeOut	;/TO=0, /PD=1 ==> WDT while not sleeping


	BTFSC	INTCON, RBIF	;Did we just wake up from SLEEP due
         goto   AwakeIO		;to a change in the I/O pins?


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

        movf    PORTB,W         ;Clear miss-match, if any
	bcf	INTCON, RBIF
  .assert "(intcon&1)==0, '*** FAILED p16f84 to clear INTCON'"
        nop

  .assert "resetCounter==1,'*** FAILED p16f84 Power On Reset'"
	nop
	MOVLW	eRSTSequence_PowerOnReset
	MOVWF	ResetSequence
	CLRWDT
  .command "resetCounter = resetCounter+1"
	nop
	SLEEP

   ; The WDT should cause a reset. So we shouldn't fall through
   ; (note, the instruction after the SLEEP should not be executed).
   ; RRR WDT will not cause a reset but will fall through
        
        nop
;RRR  .command "resetCounter = resetCounter+1"
        nop
;========================================================================
;
;  AwakeWDT - WDT timed out while we were sleeping.
;
;========================================================================
AwakeWDT:

  .assert "resetCounter==2,'*** FAILED WDT Reset p16f84'"
	nop
	MOVLW	eRSTSequence_AwakeWDT
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

  .assert "resetCounter==3,'*** FAILED WDT timeout p16f84'"
	nop
	MOVLW	eRSTSequence_WDTTimeOut
	MOVWF	ResetSequence

	CLRWDT

  .command "P_RBIF.clear = cycles+100"
        nop

    ; Keep interrupts enabled, but turn on RBIE. Changes on
    ; RB4-RB7 should wake us from sleep 
        MOVLW   (1<<RBIE)
        MOVWF   INTCON

	MOVF	TMR0,W
	SLEEP

        nop    ; the processor should idle at this instruction.
  .command "resetCounter = resetCounter+1"
	nop
  .assert "(tmr0 - W) <= 4, '*** FAILED 16f84 reset - TMR0 stops during sleep'"
        nop
	nop
;        MOVLW   (1<<GIE) | (1<<INTE)
        MOVLW   (1<<INTE)
        MOVWF   INTCON
  .command "P_INTF.set = cycles+10"
        nop
	sleep
	nop
	nop

;========================================================================
;
;  AwakeIO - an I/O pin changed states to awaken us from sleep.
;
;========================================================================
AwakeIO:
  .assert "resetCounter==4,'*** FAILED Wakeup on I/O pin change p16f84'"
	nop
	MOVLW	eRSTSequence_AwakeIO
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	nop

	CLRWDT

    ; reset the processor by driving /MCLR low for a few cycles 
  .command "RESET.clear = cycles+100"
	nop
  .command "RESET.set = cycles+110"
        nop

	SLEEP

;========================================================================
;
;  AwakeMCLR - MCLR went low while we were sleeping.
;
;========================================================================
AwakeMCLR:
  .assert "resetCounter==5,'*** FAILED /MCLR Reset p16f84'"
	nop

	MOVLW	eRSTSequence_AwakeMCLR
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	nop
	CLRWDT

    ; reset the processor by driving /MCLR low for a few cycles 
  .command "RESET.clear = cycles+100"
	nop
  .command "RESET.set = cycles+110"
        nop

waitForMCLR:    goto waitForMCLR


done:
  .assert  "'*** PASSED 16f84 Sleep and Reset test'"
	nop

	goto	done

	
	end
