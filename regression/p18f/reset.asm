	; 18f4321 gpsim regression test
	;
	; The purpose of this test is to verify that the 16bit processors
	; is implemented correctly. Here's a list of specific things
	; tested:	
	;
	; 1) Reset conditions
	; 2) WDT
	; 3) Sleep
	; 4) Wakeup on PIN change.

	list	p=18f4321
        include <p18f4321.inc>
        include <coff.inc>

   CONFIG WDT=ON, WDTPS=128

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
  GLOBAL temp1


    ; Define the reset conditions to be checked.

eRSTSequence_PowerOnReset	equ	1
eRSTSequence_AwakeMCLR		equ	2
eRSTSequence_AwakeWDT		equ	3
eRSTSequence_AwakeIO		equ	4
eRSTSequence_WDTTimeOut		equ	5
eRSTSequence_STKUNF		equ	6
eRSTSequence_STKFUL		equ	7

;----------------------------------------------------------------------
;   ********************* STARTUP LOCATION  ***************************
;----------------------------------------------------------------------
START  CODE    0x000                    ; 


;############################################################
;# Create a stimulus to simulate a switch
;

   .sim "p18f4321.xpos = 84"
   .sim "p18f4321.ypos = 96"

   .sim "module lib libgpsim_modules"
   .sim "module load pulsegen P_INTF"
   .sim "module load pulsegen P_RBIF"
   .sim "module load pulsegen RESET"

   .sim "RESET.initial = 5.0"
   .sim "RESET.xpos = 84"
   .sim "RESET.ypos = 36"

   .sim "P_INTF.clear = 0"   ; At cycle 0, 
   .sim "P_INTF.set = 1"
   .sim "P_INTF.xpos = 240"
   .sim "P_INTF.ypos = 192"

   .sim "P_RBIF.clear = 0"   ; At cycle 0, 
   .sim "P_RBIF.set = 1"
   .sim "P_RBIF.xpos = 240"
   .sim "P_RBIF.ypos = 132"

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

  .sim "optionShadow=8"      ; PSA=1

  .sim "ResetSequence=0"

  .sim ".BreakOnReset = false"
  .sim "p18f4321.frequency=100000"

  ; Set a cycle break point far in the future in case the resets fail.
  .sim "break c 0x1000000"



RESET_VECTOR  CODE    0x000              ; processor reset vector

        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program




;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location

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
        
check_int:
        btfsc   INTCON,INT0IF
         btfss  INTCON,INT0IE
          goto  check_t0

;        bsf     temp5,0         ;Set a flag to indicate rb0 int occured
        bcf     INTCON,INT0IF

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

  .assert "(trisa&0x1f)==0x1f, '*** FAILED 18f4321 reset bad TRISA'"
	nop
  .assert "trisb==0xff, '*** FAILED 18f4321 reset bad TRISB'"
	nop
  .assert "trisc==0xff, '*** FAILED 18f4321 reset bad TRISC'"
	nop
  .assert "trisd==0xff, '*** FAILED 18f4321 reset bad TRISD'"
	nop
  .assert "(trise&0x03)==0x03, '*** FAILED 18f4321 reset bad TRISE'"
	nop
   ; OPTION register setup

        MOVF    optionShadow,W  ;optionShadow is initialized by the gpsim script

   ; GPIO setup
   ; If this is a PowerOn reset, then the state of the GPIO output register
   ; is unknown. If this is not a PowerOn reset then the GPIO pins have
   ; the same state they had prior to the reset.

   ;
   ; TRIS register setup - The I/O pins are always configured as Inputs after
   ; any reset.


        MOVLW   (1<<RB0) | (1<<RB4)     ;RB0 and RB4 are the only inputs.
        MOVWF   TRISB     ;
        CLRF    TRISA 
        CLRF    STATUS
	CLRF	TRISC
	CLRF	TRISD
	CLRF	TRISE
	MOVLW	(1<<TMR0ON) | (1<<T08BIT)
	MOVWF	T0CON	; start T0

   ; Now examine why the RESET condition occurred.

	BTFSS	RCON,NOT_TO
	 goto   TO_is_low

	BTFSS	RCON,NOT_PD
	 goto   AwakeMCLR	;/TO=1, /PD=0 ==> /MCLR while sleeping
                                ;            -or- Interrupt wake-up from sleep

        btfsc   STKPTR,STKUNF   ; Stack undeflow has occured
	goto	stack_underflow

        btfsc   STKPTR,STKFUL   ; Stack overflow has occured
	goto	stack_overflow

	goto	PowerOnReset	;/TO=1, /PD=1 ==> Power has just been applied

TO_is_low

	BTFSS	RCON,NOT_PD
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
        goto   to_underflow

        movf    PORTB,W         ;Clear RBIF
  .assert "(intcon&1)==0, '*** FAILED to clear INTCON'"
        nop

  .assert "resetCounter==1,'*** FAILED Power On Reset'"
	nop
	MOVLW	eRSTSequence_PowerOnReset
	MOVWF	ResetSequence
	CLRWDT
  .command "resetCounter = resetCounter+1"
	SLEEP

   ; The WDT should cause a reset. So we shouldn't fall through
   ; (note, the instruction after the SLEEP should not be executed).
   ; RRR WDT will not cause a reset but will fall through
        
        nop
  .assert "(trisa&0x1f)==0x00, '*** FAILED 18f4321 WDT sleep bad TRISA'"
	nop
  .assert "trisb==0x11, '*** FAILED 18f4321 WDT sleep bad TRISB'"
	nop
  .assert "trisc==0x00, '*** FAILED 18f4321 WDT sleep bad TRISC'"
	nop
  .assert "trisd==0x00, '*** FAILED 18f4321 WDT sleep bad TRISD'"
	nop
  .assert "(trise&0x03)==0x00, '*** FAILED 18f4321 WDT sleep bad TRISE'"
	nop
;RRR  .command "resetCounter = resetCounter+1"
        nop
;========================================================================
;
;  AwakeWDT - WDT timed out while we were sleeping.
;
;========================================================================
AwakeWDT:

  .assert "resetCounter==2,'*** FAILED WDT Reset'"
	nop
	MOVLW	eRSTSequence_AwakeWDT
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
	nop
	MOVLW	eRSTSequence_WDTTimeOut
	MOVWF	ResetSequence

	CLRWDT

  .command "P_RBIF.clear = cycles+100"
        nop

    ; Keep interrupts enabled, but turn on RBIE. Changes on
    ; RB4-RB7 should wake us from sleep 
        MOVLW   1<<RBIE
        MOVWF   INTCON

	MOVF	TMR0L,W
	SLEEP

        nop    ; the processor should idle at this instruction.
  .command "resetCounter = resetCounter+1"
  .assert "(tmr0l - W) <= 1, '*** FAILED 18f4321 reset - TMR0 stops during sleep'"
        nop

;========================================================================
;
;  AwakeIO - an I/O pin changed states to awaken us from sleep.
;
;========================================================================
AwakeIO:
  .assert "resetCounter==4,'*** FAILED Wakeup on I/O pin change'"
	nop
	MOVLW	eRSTSequence_AwakeIO
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"

	CLRWDT

    ; reset the processor by driving /MCLR low for a few cycles 
  .command "RESET.clear = cycles+100"
  .command "RESET.set = cycles+110"
        nop

	SLEEP

;========================================================================
;
;  AwakeMCLR - MCLR went low while we were sleeping.
;
;========================================================================
AwakeMCLR:
  .assert "resetCounter==5,'*** FAILED /MCLR Reset'"
	nop

	MOVLW	eRSTSequence_AwakeMCLR
	MOVWF	ResetSequence

  .command "resetCounter = resetCounter+1"
	CLRWDT

    ; reset the processor by driving /MCLR low for a few cycles 
  .command "RESET.clear = cycles+100"
  .command "RESET.set = cycles+110"
        nop

waitForMCLR:    goto waitForMCLR





stack_underflow:
  .assert "resetCounter==6,'*** FAILED Stack undeflow Reset'"
	nop
  .command "resetCounter = resetCounter+1"
	CLRWDT
	bcf     STKPTR,STKUNF
	clrf	temp1
	call	full

full:
	incf	temp1,F
	call full
	return		; never gets executed
	nop

stack_overflow:
  .assert "resetCounter==7,'*** FAILED Stack overflow Reset'"
	nop
  .assert "temp1==31, '*** FAILED Stack overflow size'"
	nop
  .assert  "'*** PASSED 18f4321 Sleep and Reset test'"
	nop
	nop

to_underflow:
	return	; RRR stack undeflow
	
	end
