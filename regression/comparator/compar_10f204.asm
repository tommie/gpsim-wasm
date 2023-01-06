	; 10f204 gpsim regression test
	;
	; The purpose of this test is to verify that the 'F204
	; is implemented correctly. Here's a list of specific things
	; tested:	
	;
	; 1) Reset conditions
	; 2) WDT
	; 3) Sleep
	; 4) Wakeup on PIN change.
        ; 5) Comparator.

	list	p=10f204
        include <p10f204.inc>
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


;                   +-------------------+
;                 1 |o                  | 6
;             ------|GP0/CIN+   GP3/MCLR|------
;                 2 |                   | 5
;                ---|VSS   10F204    VDD|---
;                 3 |                   | 4
;             ------|GP1/CIN-   GP2/COUT|------
;                   |                   |  
;                   +-------------------+
;

;############################################################
;# Create a stimulus to simulate a switch
;

   .sim "module lib libgpsim_modules"
   .sim "module load pulsegen MCLR"

   .sim "module load pullup VP"
   .sim "module load pullup VN"

   .sim "MCLR.initial = 5.0"

   .sim "VP.voltage = 5.0"
   .sim "VP.resistance = 1.0e3"
   .sim "VN.voltage = 5.0"
   .sim "VN.resistance = 1.0e3"

  .sim "MCLR.xpos=250.0"
  .sim "MCLR.ypos=85.0"
  .sim "VP.xpos=250.0"
  .sim "VP.ypos=130.0"
  .sim "VN.xpos=250.0"
  .sim "VN.ypos=180.0"

  ;############################################################

  .sim "node nMCLR"
  .sim "attach nMCLR gpio3 MCLR.pin"

  .sim "node nVP"
  .sim "attach nVP gpio0 VP.pin"

  .sim "node nVN"
  .sim "attach nVN gpio1 VN.pin"


  ;############################################################
  ; define symbols that can be used in the assertion expressions.
   .sim "symbol CHigh=4"
   .sim "symbol CLow=0"
   .sim "symbol Cexp=0"
   .sim "symbol CMask=4"

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

        MOVLW   (1<<GP0)|(1<<GP1)      ;Enable the comparator inputs
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
; toggleGP2
;
; The purpose of this code is to write a 1 and then a 0 to GP2. The GP2
; output pin is then checked. Note that the assertion expression are
; parametrically defined.

toggleGP2:
        movf    GPIO,W
        iorlw   1<<GP2
        movwf   GPIO

  .assert "(gpio & (1<<2))==(Cexp),'*** FAILED Comparator test'"
        movf    GPIO,W
        andlw   ~(1<<GP2)
        movwf   GPIO

  .assert "(gpio & (1<<2))==(Cexp^CMask),'*** FAILED Comparator test'"
        nop
        return
;========================================================================
;
;  PowerOnReset
;
;========================================================================
PowerOnReset:

        nop


   ;Exercise the comparator

   ; Verify that the reset condition for CMCON0 is correct
  .assert "(cmcon0&0x7f)==0x7f,'*** FAILED COMCON0 Power on reset condition '"

        nop


   ; Make VP > VN and verify that the output doesn't change due to this

  .command "VN.voltage=0.0"
  .command "VP.voltage=5.0"
        nop
  .assert "(gpio & (1<<2))==0,'*** FAILED Comparator test'"

   ; Now toggle GPIO and verify that we can write to the port.
        nop
  .command "Cexp=CHigh"
        call    toggleGP2

   ; Make VP < VN and verify that the output doesn't change

  .command "VN.voltage=0.0"
  .command "VP.voltage=5.0"

        nop

   ; Toggle GPIO again  and verify that we can still write to the port.

        call    toggleGP2

   ; Now turn on the comparator

        movlw (1<<CMPOUT) | (0<<NOT_COUTEN) | (1<<POL) | (1<<NOT_CMPT0CS) | (1<<CMPON) | (1<<CNREF)| (1<<CPREF) | (1<<NOT_CWU)
        movwf   CMCON0

   ; Make VP > VN and verify the comparator output reflects this

  .command "VN.voltage=0.0"
  .command "VP.voltage=5.0"

        nop
  .assert "(gpio & (1<<2))==(1<<2),'*** FAILED Comparator test'"
        call    delay

   ; verify that GPIO2 cannot be written to
  .command "CMask=0"
        call    toggleGP2

   ; make VP < VN 

  .command "VN.voltage=5.0"
  .command "VP.voltage=0.0"
        nop
        
  .assert "(gpio & (1<<2))==0,'*** FAILED Comparator test'"
        call    delay

   ; verify again that GPIO2 can't be written to.
  .command "Cexp=0"

        call    toggleGP2

        nop

    ; toggle the comparator output polarity and repeat the above tests.
        bcf     CMCON0,POL
  .assert "(gpio & (1<<2))==(1<<2),'*** FAILED Comparator test'"
        nop

  .command "Cexp=1<<2"

        call    toggleGP2

  .command "VN.voltage=0.0"
  .command "VP.voltage=5.0"
        nop
  .assert "(gpio & (1<<2))==0,'*** FAILED Comparator test'"
        nop
  .command "Cexp=0"
        call    toggleGP2

   ;turn off the comparator output
        bsf     CMCON0,NOT_COUTEN
  .command "Cexp=1<<2"
  .command "CMask=1<<2"
        call    toggleGP2

  .assert "'*** PASSED 10F204 Comparator test'"
        nop

;========================================================================
;
;  AwakeWDT - WDT timed out while we were sleeping.
;
;========================================================================
AwakeWDT:

  .assert "'*** FAILED -- Unexpected WDT Reset'"
        nop

;========================================================================
;
;  WDTTimeOut - WatchDog timer timed out while we were awake.
;
;========================================================================
WDTTimeOut:

  .assert "'*** FAILED -- Unexpected WDT timeout'"
        nop


;========================================================================
;
;  AwakeIO - an I/O pin changed states to awaken us from sleep.
;
;========================================================================
AwakeIO:

  .assert "'*** FAILED -- Unexpected Wakeup on I/O pin change'"
        nop


;========================================================================
;
;  AwakeMCLR - MCLR went low while we were sleeping.
;
;========================================================================
AwakeMCLR:

  .assert "'*** FAILED -- Unexpected /MCLR Reset'"
        nop

  .assert  "'*** PASSED 12c509 Sleep and Reset test'"

done:
	goto	done

;========================================================================
delay:

delay10: goto    delay8
delay8: goto    delay6
delay6: goto    delay4
delay4: return
;========================================================================

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0xff              ; processor reset vector

OSC_CAL_VALUE	EQU	0x80

	MOVLW	OSC_CAL_VALUE
	
	end
