
   ;;  Breakpoints.asm  
   ;;
   ;;  The purpose of this program is to test gpsim's
   ;; breakpoints.
   ;;
   ;;
   ;;


	list    p=16f873                ; list directive to define processor
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

        clrf    var1
        clrf    var2
        clrf    var3



	call	delay
   .assert "var1==0"
	incf	var1,F	;This write will cause a break
   .assert "var1==4"
	movf	var1,W

        movlw   50
        movwf   var3
L_StopWatchDelayTest:
        call    delay
        decfsz  var3,F
         goto   L_StopWatchDelayTest

passed:
	clrf	failures

done:
  ; If no expression is specified, then break unconditionally

  .assert  "'*** PASSED breakpoint test'"
	nop
        goto    done


delay:
	goto	$+1
	goto	$+1
	return


  ; Don't let the simulation run forever.
   .sim "break c 0x10000"
   .sim "step 6"
   .sim "echo Breakpoints:"
   .sim "break"
   .sim "break w var1"
   .sim "run"
   .sim "var1=4"
   .sim "echo stepping over breakpoint"
   .sim "step"
   .sim "trace 5"
   .sim "stopwatch.enable = true"
   .sim "stopwatch.rollover = 300"
   .sim "break stopwatch" ;, 'Hit stopwatch breakpoint'"
   .sim "run"  ; This run will take us to the stopwatch break
   .sim "run"  ; This run will take us to the done label

  end
