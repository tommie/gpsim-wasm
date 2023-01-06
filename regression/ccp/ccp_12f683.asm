
	;; ccp_819
        ;; The purpose of this program is to test gpsim's ability to simulate
        ;; the Capture Compare peripherals in a midrange pic (e.g. pic16c64).

    ;; ccp_683
		;; derived for 12f683 
		;; ccp maybe corrupt in gpsim 0.31.1
    ; (c) G.Schielke 2022-01-27

    list    p=12f683

	include <p12f683.inc>
    include <coff.inc>

    __CONFIG _WDT_OFF & _INTOSCIO & _MCLRE_OFF

    errorlevel -302 
    radix dec

;----------------------------------------------------------------------

.command macro x
    .direct "c", x
    endm
    
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
failures        RES 1

status_temp	    RES	1
w_temp		    RES	1
pie1_temp       RES 1

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
    ;messg "Create some test environment"

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
   .sim "attach test_node_C asy1 gpio2"
   ;
   .sim "scope.ch0 = 'gpio1'"
   .sim "scope.ch1 = 'gpio2'"


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

        BANKSEL(PIE1)
        movf    PIE1 ^ 0x80,W
        BANKSEL(STATUS)
        movwf   pie1_temp

check_tmr1:
        btfsc   PIR1,TMR1IF
         btfss  pie1_temp,TMR1IE
          goto  check_ccp1

    ;; tmr1 has rolled over
        
        bcf     PIR1,TMR1IF     ; Clear the pending interrupt
        bsf     temp1,0         ; Set a flag to indicate rollover

check_ccp1:
        btfsc   PIR1,CCP1IF
         btfss  pie1_temp,CCP1IE
          goto  check_gpif

        bcf     PIR1,CCP1IF     ; Clear the pending interrupt
        bsf     temp1,1         ; Set a flag to indicate match

check_gpif:
        btfss   INTCON,GPIF
          goto  exit_int

        btfsc   GPIO,GP2        ; invert signal at gp2 on GP1
        goto    gpio_clr2
         
        bsf     GPIO,GP1        ; set pin
        goto gpio_end
        
gpio_clr2:
        bcf     GPIO,GP1        ; clear pin

gpio_end:
        nop
        bcf     INTCON,GPIF
        bcf     INTCON,INTF
        nop

    .assert "((intcon & 0x01)==0x00), 'GPIF should be 0!'"
        nop
        
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
        movlw   0x70            ;8MHz, fosc2:0 source
        movwf   OSCCON
        
        clrf    failures        ;Assume success.

        clrf    kz              ;kz==Known Zero.

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
        movlw   (1<<GIE) | (1<<PEIE) | (1<<GPIE)
        movwf   INTCON
    .assert "((intcon & 0xC8)==0xC8), \"GIE, PEIE or GPIE wrong!\""
        nop

        BANKSEL(ANSEL)              ; RB1
        movlw   0x31                ; only an0, Frc clock
        movwf   ANSEL ^ 0x80
    .assert "((ansel & 0x31)==0x31), \"ANSEL wrong!\""
        nop
        
        BANKSEL(CMCON0)             ; RB0
        movlw   0x07                ; comparator module off
        movwf   CMCON0
    .assert "((cmcon0 & 0x07)==0x07), \"CMCON0 wrong!\""
        nop
            
        BANKSEL(IOC)                ; RB1
        bsf     IOC ^ 0x80,IOC2     ;int on pinchange GP2
    .assert "((ioc & 0x04)==0x04), \"IOC wrong!\""
        nop
            
        bsf     TRISIO ^ 0x80,TRISIO0   ;an0 bit is an input
        bcf     TRISIO ^ 0x80,TRISIO1   ;GP1 bit is an output
        bsf     TRISIO ^ 0x80,TRISIO2   ;CCP bit is an input
    .assert "((trisio & 0x07)==0x05), \"TRISIO wrong!\""
        nop

        bsf     PIE1 ^ 0x80, CCP1IE
    .assert "(pie1 == 0x20), \"CCP1IE failed!\""
        nop

        BANKSEL(STATUS)

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

    .command "echo \"wait for rising edge\""
        nop

        call    ccpWaitForPORTC2_high

        movlw   (1<<CCP1M2)	;CCP1CON = 4 <- capture every falling edge
        movwf   CCP1CON

    .command "echo \"capture every falling edge\""
        nop

        ;; Start the timer

        bsf     T1CON,TMR1ON


        call    ccpWaitForPORTC2_high
        call	ccpCaptureTwoEvents

    .assert "(capTimeL==0) && (capTimeH==4)"
        nop

        clrf    temp1           ;Clear the software interrupt flag.
        incf    CCP1CON,F       ;Next mode --> 5

    .command "echo \"capture every rising edge\""
        nop

        call    ccpWaitForPORTC2_low

        call    ccpWaitForPORTC2_high
        call	ccpCaptureTwoEvents

    .assert "(capTimeL==0) && (capTimeH==4)"
        nop

        clrf    temp1           ;Clear the software interrupt flag.
        incf    CCP1CON,F       ;Next mode --> 6

    .command "echo \"capture every 4th rising edge\""
        nop

        call    ccpWaitForPORTC2_high
        call	ccpCaptureTwoEvents

    .assert "(capTimeL==0) && (capTimeH==0x10)"
        nop

        clrf    temp1           ;Clear the software interrupt flag.
        incf    CCP1CON,F       ;Next mode --> 7

    .command "echo \"capture every 16th rising edge\""
        nop
        
        call    ccpWaitForPORTC2_high
        call	ccpCaptureTwoEvents

    .assert "(capTimeL==0) && (capTimeH==0x40)"
        nop

        ;goto done
        goto ccp_test2

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

        btfsc	GPIO,2
            return
        call	ccpWDCounter
        goto	ccpWaitForPORTC2_high
	
ccpWaitForPORTC2_low:

        btfss	GPIO,2
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

  ;;##########################################

        ;;
        ;; Compare
        ;;
        ;; Now for the compare mode. 
ccp_test2:
    .command "echo \"Enter Compare modes ...\""
    
        clrf    T1CON
        clrf    TMR1L
        clrf    TMR1H

        bsf     STATUS,RP0
        bcf     TRISIO ^0x80,2         ;CCP bit is an output
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

    .command "echo \"Set output on match\""
        nop

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

    .command "echo \"next mode =>\""
    .command "ccp1con"
        goto    tt3

failed:	
    .assert  "'*** FAILED CCP 12f683 test'"
        nop
        
        incf	failures,F
        goto	$
        
done:
    .assert  "'*** PASSED CCP 12f683 test'"
        nop
        
        goto    $

    end
