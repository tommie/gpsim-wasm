	;; branching.asm
	;;
	;; The purpose of this program is goto, call, return


	list    p=16f628                ; list directive to define processor
	include <p16f628.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros


.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1

w_temp          RES     1
status_temp     RES     1

  GLOBAL done

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector

  .sim "symbol callCounter=1"
  .sim "symbol callFrom=0"
  .sim "symbol cycleCounter=0"
  .sim "break c 0x100000"

        movlw  high  start               ; load upper byte of 'start' label
  .assert "callCounter==1,'*** FAILED Call level 1'"
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

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

        call    L1
  .assert "callCounter==2,'*** FAILED Call level 1'"
        nop
        call    L2
  .assert "callCounter==4,'*** FAILED Call level 1'"
        nop
        call    L3
  .assert "callCounter==7,'*** FAILED Call level 1'"
        nop

  .command "callCounter = 2"
        call    L1
  .assert "callCounter==3,'*** FAILED Call level 1'"
        call    L1
  .assert "callCounter==4,'*** FAILED Call level 1'"
        call    L1
  .assert "callCounter==5,'*** FAILED Call level 1'"

        nop
  .command "callCounter = 2"
        call    Nest1
  .assert "callCounter==0x124,'*** FAILED Call level 3'"


        ;; Verify instruction cycle counts
  .command "cycleCounter = cycles"
        nop
  .assert "(cycleCounter+1) == cycles"
        goto    $+1
  .assert "(cycleCounter+3) == cycles"
        call    L1
  .assert "(cycleCounter+7) == cycles"

        ;;
        ;; tmr0 test
        ;;
        ;; The following block of code tests tmr0 together with interrupts.
        ;; Each prescale value (0-7) is loaded into the timer. The software
        ;; waits until the interrupt due to tmr0 rollover occurs before
        ;; loading the new prescale value.

        clrwdt

        bsf     INTCON,T0IE     ;Enable TMR0 overflow interrupts
        
        bsf     INTCON,GIE      ;Global interrupts

        clrf    temp1           ;Software flag used to monitor when the
                                ;interrupt has been serviced.

        clrw
test_tmr0:      
        option                  ;Assign new prescale value

test_flag:
        clrwdt
        call    Test1
        btfss   temp1,0         ;Wait for the interrupt to occur and
         goto   test_flag       ;get serviced

        
        clrf    temp1           ;Clear flag for the next time
        
        bsf     STATUS,RP0
        incf    (OPTION_REG^0x80),W
        bcf     STATUS,RP0
        
        andlw   0x7             ;Check the prescaler
        skpz
         goto   test_tmr0


   ;
   ; Now test 14-bit core SFRs (which should be done in an SFR test but there isn't one)
   ;

        movlw   0xFF
        movwf   PCLATH
        
   .assert "pclath == 0x1F, '*** FAILED 14-bit PCLATH masking'"
        nop
        clrf    PCLATH

        goto    done

L1:
  .command "callCounter = callCounter+1"
        return
L2:
  .command "callCounter = callCounter+2"
        return
L3:
  .command "callCounter = callCounter+3"
        return


Nest1:
  .command "callCounter = callCounter+1"
        call    Nest2
  .command "callCounter = callCounter+1"
        return
Nest2:
  .command "callCounter = callCounter+0x10"
        call    Nest3
  .command "callCounter = callCounter+0x10"
        return
Nest3:
  .command "callCounter = callCounter+0x100"
        return


Test1:
  .command "callCounter = 2"
        call    Test2
  .assert "callCounter == 7,'*** FAILED Call level 4'"
;  .command "callFrom=2"
        call    Test3
;  .assert "callCounter == 9,'*** FAILED Call level 4'"
;  .command "callFrom=3"
        call    Test4
;  .assert "callCounter == 10,'*** FAILED Call level 4'"
        return
Test2:  
;  .command "callFrom=2"
        call    Test3
  .command "callCounter = callCounter+1"
;  .command "callFrom=3"
        call    Test4
  .command "callCounter = callCounter+1"
        return
Test3:

;  .assert "callFrom==2"
;        nop
;  .command "callFrom=3"
        call    Test4
  .command "callCounter = callCounter+1"
        return
Test4:  
  .command "callCounter = callCounter+1"
;  .assert "callFrom==3"
        return


done:
  .assert  "'*** PASSED Midrange core Branching test'"
        nop

  end
