
	;; ccp_819
        ;; The purpose of this program is to test gpsim's ability to simulate
        ;; the Capture Compare peripherals in a midrange pic (e.g. pic16c64).

    


        list    p=16f819
	include <p16f819.inc>
        include <coff.inc>

        __CONFIG _WDT_OFF & _CCP1_RB3

        errorlevel -302 
        radix dec
;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
failures        RES     1

status_temp	RES	1
w_temp		RES	1
interrupt_temp  RES     1

  GLOBAL failures
  GLOBAL done

	;; The capTime 16-bit register is a working register that keeps track
        ;; of the capture edge.

capTimeH RES 1
capTimeL RES 1

capTimeHb RES 1
capTimeLb RES 1

  GLOBAL capTimeH, capTimeL, capTimeHb, capTimeLb

temp1 RES 1
temp2 RES 1
t1 RES 1
t2 RES 1
t3 RES 1
kz RES 1

  GLOBAL temp1, temp2, t1, t2, t3, kz


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


;; Simulation Script

   .sim "node test_node_C"
   ;# Create an asynchronous  stimulus that's 1000 cycles long
   ;# and attach it to portb bit 3. 
   .sim "stimulus asynchronous_stimulus"
   .sim "initial_state 0"
   .sim "start_cycle 0x100"
   .sim "period 0x400"
   .sim "{ 0x200, 1}"
;   .sim "0x300, 0,"
;   .sim "0x400, 1,"
;   .sim "0x600, 0,"
;   .sim "0x700, 1,"
;   .sim "0x800, 0}"
   .sim "name asy1"
   .sim "end"
   .sim "attach test_node_C asy1 portb3"



;----------------------------------------------------------------------
;   ******************* INTERRUPT VECTOR LOCATION  ********************
;----------------------------------------------------------------------
                
INT_VECTOR   CODE    0x004               ; interrupt vector location

        ;; 
        ;; Interrupt
        ;;

        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

        ;; Are peripheral interrupts enabled?
        btfss   INTCON,PEIE
         goto   exit_int

        bsf     STATUS,RP0
        movf    PIE1 ^ 0x80,W
        bcf     STATUS,RP0
        movwf   interrupt_temp

check_tmr1:
        btfsc   PIR1,TMR1IF
         btfss  interrupt_temp,TMR1IE
          goto  check_ccp1

    ;; tmr1 has rolled over
        
        bcf     PIR1,TMR1IF     ; Clear the pending interrupt
        bsf     temp1,0         ; Set a flag to indicate rollover

check_ccp1:
        btfsc   PIR1,CCP1IF
         btfss  interrupt_temp,CCP1IE
          goto  exit_int

        bcf     PIR1,CCP1IF     ; Clear the pending interrupt
        bsf     temp1,1         ; Set a flag to indicate match

exit_int:               

        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,F
        swapf   w_temp,W

        retfie

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
        clrf    failures        ;Assume success.

        clrf    kz              ;kz == Known Zero.

        ;; disable (primarily) global and peripheral interrupts
        
        clrf    INTCON

        ;;
        ;; CCP test
        ;;

        ;; The CCP module is intricately intertwined with the TMR1
        ;; module. So first, let's initialize TMR1:

        ;; Clear all of the bits of the TMR1 control register:
        ;; this will:
        ;;      Turn the tmr off
        ;;      Select Fosc/4 as the clock source
        ;;      Disable the External oscillator feedback circuit
        ;;      Select a 1:1 prescale
        ;; In this mode, TMR1 will count instruction cycles.
        
        clrf    T1CON           ;
        clrf    PIR1            ; Clear the interrupt/roll over flag

        ;; Zero TMR1

        clrf    TMR1L
        clrf    TMR1H

        ;; Start the timer

        bsf     T1CON,TMR1ON


ccp_test1:
        movlw   (1<<GIE) | (1<<PEIE)
        movwf   INTCON

        bsf     STATUS,RP0
        bsf     TRISB ^ 0x80, 3         ;CCP bit is an input
        bsf     PIE1 ^ 0x80, CCP1IE
        bcf     STATUS,RP0

        ;;
        ;; Start the capture mode that captures every falling edge
        ;; (please refer to the mchip documentation on the details
        ;; of the various ccp modes.)
        ;; ccp = 4  <- capture every falling edge
        ;; ccp = 5  <- capture every rising edge
        ;; ccp = 6  <- capture every 4th rising edge
        ;; ccp = 7  <- capture every 16th rising edge
        ;;
        ;; Note, the capture only works if the stimulus is present
        ;; on the ccp pin!

        call    ccpWaitForPORTC2_high

        movlw   (1<<CCP1M2)	;CCP1CON = 4 <- capture every falling edge
        movwf   CCP1CON

        ;; Start the timer

        bsf     T1CON,TMR1ON


        call    ccpWaitForPORTC2_high
	call	ccpCaptureTwoEvents

  .assert "(capTimeL==0) && (capTimeH==4)"

        clrf    temp1           ;Clear the software interrupt flag.
        incf    CCP1CON,F       ;Next mode --> 5


        call    ccpWaitForPORTC2_low

        call    ccpWaitForPORTC2_high
	call	ccpCaptureTwoEvents

  .assert "(capTimeL==0) && (capTimeH==4)"

        clrf    temp1           ;Clear the software interrupt flag.
        incf    CCP1CON,F       ;Next mode --> 6

        call    ccpWaitForPORTC2_high
	call	ccpCaptureTwoEvents

  .assert "(capTimeL==0) && (capTimeH==0x10)"

        clrf    temp1           ;Clear the software interrupt flag.
        incf    CCP1CON,F       ;Next mode --> 7

        call    ccpWaitForPORTC2_high
	call	ccpCaptureTwoEvents

  .assert "(capTimeL==0) && (capTimeH==0x40)"

	goto done

;------------------------------------------------------------------------
;ccpCaptureTwoEvents
;
; Capture two events for the current CCP setting and return the time
; difference between them
ccpCaptureTwoEvents:	
        call    ccpWaitForCapture

	movf	CCPR1L,W
	movwf	capTimeLb
	movf	CCPR1H,W
	movwf	capTimeHb

	call    ccpWaitForCapture

	movf	CCPR1L,W
	movwf	capTimeL
	movf	CCPR1H,W
	movwf	capTimeH

   ; Subtract the time of the most recently captured edge from the previous one

	movf	capTimeLb,W
	subwf	capTimeL,F

	movf	capTimeHb,W
	skpc
	 incfsz capTimeHb,W
          subwf	capTimeH,F


        return

	
;------------------------------------------------------------------------
;ccpWaitForCapture
;
; Spin loop that polls an interrupt flag that is set whenever a capture 
; interrupt triggers.

ccpWaitForCapture:

        clrf    t1              ;A 16-bit software timeout counter
        clrf    t2
ccpWaitLoop:	
        ;; The watchdog counter ensures we don't loop forever!
        call    ccpWDCounter

        ;;
        ;; when an edge is captured, the interrupt routine will
        ;; set a flag:
        ;; 

        btfss   temp1,1
         goto    ccpWaitLoop

	bcf	temp1,1
        return




;------------------------------------------------------------------------
ccpWaitForPORTC2_high:

	btfsc	PORTB,3
         return
	call	ccpWDCounter
	goto	ccpWaitForPORTC2_high
ccpWaitForPORTC2_low:

	btfss	PORTB,3
         return
	call	ccpWDCounter
	goto	ccpWaitForPORTC2_high

;------------------------------------------------------------------------
; ccpWDCounter
;  a 16bit watch dog counter is incremented by 1. If it rolls over then we failed.

ccpWDCounter:

	incfsz  t1,f
         return
        incfsz  t2,f
         return

        goto    failed          ;If we get here then we haven't caught anything!
                                ;Either a) there's a gpsim bug or b) the stimulus
                                ;file is incorrect.


;        movlw   1               ;This 16-bit software counter
;        addwf   t1,f            ;will time out if there's something wrong,
;        rlf     kz,w
;        addwf   t2,f
;	skpc
;         return


        ;;
        ;; Compare
        ;;
        ;; Now for the compare mode. 
ccp_test2:

	goto    done

  ;;##########################################


        clrf    T1CON
        clrf    TMR1L
        clrf    TMR1H

        bsf     STATUS,RP0
        bcf     PORTB,3         ;CCP bit is an output
        bcf     STATUS,RP0

        ;; Start off the compare mode by setting the output on a compare match
        ;;
        ;; ccp = 8  <- Set output on match
        ;; ccp = 9  <- Clear output on match
        ;; ccp = 10  <- Just set the ccp1if flag, but don't change the output
        ;; ccp = 11  <- Reset tmr1 on a match

        movlw   0x8
        movwf   CCP1CON

        ;;
        clrf    PIR1
        
        ;; Initialize the 16-bit compare register:

        movlw   0x34
        movwf   CCPR1L
        movlw   0x12
        movwf   CCPR1H

        ;; Clear the interrupt flag
        clrf    temp1

tt3:
        ;; Stop and clear tmr1
        clrf    T1CON
        clrf    TMR1L
        clrf    TMR1H

        ;; Now start it
        bsf     T1CON,TMR1ON

        ;; Wait for the interrupt routine to set the flag:
        btfss   temp1,1
         goto   $-1

        bcf     temp1,1
        
        ;; Try the next capture mode
        incf    CCP1CON,F

        ;; If bit 2 of ccp1con is set then we're through with capture modes
        ;; (and are starting pwm modes)

        btfsc   CCP1CON,2
         goto   done

        goto    tt3

failed:	
  .assert  "'*** FAILED CCP 819 test'"
	incf	failures,F
	goto	$
done:
  .assert  "'*** PASSED CCP 819 test'"

        goto    $

        end
