
        list    p=16c64
  __config _WDT_OFF

        ;; The purpose of this program is to test gpsim's ability to simulate the
        ;; Capture Compare peripherals in a midrange pic (e.g. pic16c64). See
        ;; p16c64_tmr1.asm for additional tmr1 examples.
        ;; NOTE: The capture mode will only work if there is something to capture!
        ;; Hence this file should loaded into gpsim with the startup command file
        ;; p16c64_ccp.stc - this will give you a square wave that ccp can capture.

    include "p16c64.inc"

  cblock  0x20
        status_temp,w_temp
        interrupt_temp

        temp1,temp2
        t1,t2,t3
        kz
  endc

  cblock 0xa0
        status_temp_alias,w_temp_alias
  endc

        
  org   0
        goto    main

  org   4
                
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


main:

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
        bsf     TRISC ^ 0x80, 2         ;CCP bit is an input
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
        ;; on the ccp pin. (Try invoking gpsim with 'p16c64_ccp.stc'
        ;; to get the proper stimulus defined.)
        
        movlw   4
        movwf   CCP1CON

        ;; Start the timer

        bsf     T1CON,TMR1ON
        clrf    t1              ;A 16-bit software timeout counter
        clrf    t2

ccp_t1:

        ;;
        ;; when an edge is captured, the interrupt routine will
        ;; set a flag:
        ;; 
        btfsc   temp1,1
         goto   ccp_next_capture_mode

        movlw   1               ;This 16-bit software counter
        addwf   t1,f            ;will time out if there's something wrong,
        rlf     kz,w
        addwf   t2,f
        skpc
         goto   ccp_t1

        goto    ccp_test2       ;If we get here then we haven't caught anything!
                                ;Either a) there's a gpsim bug or b) the stimulus
                                ;file is incorrect (maybe even the wrong cpu).

ccp_next_capture_mode:
        movlw   7               ;if we just processed the 16th rising edge capture mode
        xorwf   CCP1CON,W       ;
        skpnz                   ;
         goto   ccp_test2       ;Then go to the next test.

        clrf    temp1
        incf    CCP1CON,F       ;Next mode
        goto    ccp_t1 - 2      ;clear t1 and t2 too.


        ;;
        ;; Compare
        ;;
        ;; Now for the compare mode. 
ccp_test2:

        clrf    T1CON
        clrf    TMR1L
        clrf    TMR1H

        bsf     STATUS,RP0
        bcf     PORTC,2         ;CCP bit is an output
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
	clrf	temp1

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
         goto   die

        goto    tt3
        
die:

        goto    $
        end
