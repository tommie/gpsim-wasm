;************************************************************************
;
; Assertion test
;
;************************************************************************

	list      p=16f873              ; list directive to define processor
	include <p16f873.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG (_CP_OFF & _WDT_ON & _BODEN_ON & _PWRTE_ON & _HS_OSC & _WRT_ENABLE_ON & _LVP_OFF & _CPD_OFF)

        errorlevel -302 

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
var1            RES     1
var2            RES     1
var3            RES     1
failures        RES     1

  GLOBAL var1,var2,var3,failures
  GLOBAL done
;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

  ; Create a script for controlling the simulation
  ; Any valid gpsim command can appear in the quotes (fixme - load???)
  ; When this file is loaded into gpsim, all of the commands are
  ; are collected. When the processor has been initialized, then
  ; the commands are played back. 


   .sim "failures=1"     ; Assume the test fails
   .sim "run"            ; runs to the first assertion (that's designed to fail)
   .sim "failures=failures+1"
   .sim "run"            ; runs to the second assertion
   .sim "failures=failures+1"
   .sim "run"
   .sim "failures"

  if 0
   .sim "failures=42"    ; gpasm should ignore this script command.
  endif

        clrf    var1
        clrf    var2
        clrf    var3

  ; var1 and var2 have just been cleared, so the following two
  ; assertions should fail. However the script controlling the
  ; this regression test expects this and will continue running

  .assert  "var1!=0"
        nop
  .assert  "var2==1"
        nop

  ; compound expression - this one shouldn't halt the simulation
  .assert  "var2==0 || var1==0"
        nop

     ; failures has been set to 3 by the script.
     ; Let's set it back to 0. Note that if any of the
     ; assertions fail to behave as expected, then the 
     ; value of failures (when the simulation does eventually
     ; stop) will be non-zero.

        movlw   3
        xorwf   failures,F

done:
  ; If no expression is specified, then break unconditionally
  .assert  ""
        goto    done


        nop
  end
