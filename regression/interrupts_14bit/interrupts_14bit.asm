
        ;; The purpose of this program is to test gpsim's ability to 
        ;; simulate interrupts on the midrange core (specifically the 'f84).


	list    p=16f84                 ; list directive to define processor
	include <p16f84.inc>            ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

 ;; Suppress warnings of using 'option' instruction.
	errorlevel -224

  __CONFIG _WDT_ON



GPR_DATA        UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
temp3           RES     1
temp4           RES     1
temp5           RES     1
adr_cnt         RES     1
data_cnt        RES     1
failures        RES     1

w_temp          RES     1
status_temp     RES     1

 GLOBAL temp,temp1,temp2,temp3,temp4,temp5

 GLOBAL start
;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
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

        bsf     temp5,1         ;Set a flag to indicate rb4-7 int occured
        bcf     INTCON,RBIF
	movf	PORTB,w
        
check_int:
        btfsc   INTCON,INTF
         btfss  INTCON,INTE
          goto  check_t0

        bsf     temp5,0         ;Set a flag to indicate rb0 int occured
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
	clrwdt
        retfie



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
        ;; Assume no failures

        clrf    failures

        ;;
        ;; tmr0 test
        ;;
        ;; The following block of code tests tmr0 together with interrupts.
        ;; Each prescale value (0-7) is loaded into the timer. The software
        ;; waits until the interrupt due to tmr0 rollover occurs before
        ;; loading the new prescale value.
        
        bsf     INTCON,T0IE     ;Enable TMR0 overflow interrupts
        
        bsf     INTCON,GIE      ;Global interrupts

        clrf    temp1           ;Software flag used to monitor when the
                                ;interrupt has been serviced.

        clrw
test_tmr0:      
        option                  ;Assign new prescale value


        btfss   temp1,0         ;Wait for the interrupt to occur and
         goto   $-1             ;get serviced

        
        clrf    temp1           ;Clear flag for the next time
        
        bsf     STATUS,RP0
        incf    (OPTION_REG^0x80),W
        bcf     STATUS,RP0
        
        andlw   0x7             ;Check the prescaler
        skpz
         goto   test_tmr0

        bcf     INTCON,T0IE     ;Disable tmr0 interrupts

        ;; Now check tmr0 with an external clock source
        ;;
        ;; It assumes that port b bit 0 is the stimulus.
        ;; This requires that the following gpsim commands be invoked:
        ;;  gpsim> node new_test_node
        ;;  gpsim> attach new_test_node porta4 portb0

        bsf     STATUS,RP0
        bcf     (TRISB ^ 0x80),0        ;portb bit 0 is an output
        bcf     STATUS,RP0

        ;; assign the prescaler to the wdt so that tmr0 counts every edge
        ;; select the clock source to be tocki
        ;; and capture low to high transitions (tose = 0)
        
        movlw   (1<<T0CS) | (1<<PSA)
;       movlw   (1<<T0CS) | (1<<PSA) | (1<<T0SE)
        option
        
        movlw   0xff
        movwf   temp2
        movwf   temp3
        movwf   temp4
        
        bcf     INTCON,T0IF     ;not necessary..., but clear pending int.
        bsf     INTCON,T0IE     ;Re-enable tmr0 interrupts
        
	clrwdt
tmr0_l1:
        bcf     temp1,0         ;Interrupt flag

        clrz    
        bcf     PORTB,0
        btfsc   temp1,0
         decf   temp2,f         ;Falling edge caused the interrupt

        bcf     temp1,0
        bsf     PORTB,0
        btfsc   temp1,0
         decf   temp3,f         ;Rising edge caused the interrupt

        ;; if either temp2 or temp3 decremented to zero, then z will be set
        skpz
         goto   tmr0_l1

        incfsz  temp4,f
         goto   test_inte
        
        ;; Now let's test external clocking with the falling edge
        
        movlw   (1<<T0CS) | (1<<PSA) | (1<<T0SE)
        option
        
        goto    tmr0_l1

        ;; tmr0 test is done.

        ;;
        ;; inte test
        ;;
        ;; The following block of code tests the interrupt on
        ;; change for port b bit 0. It assumes that port a bit 4
        ;; is the stimulus. This requires that the following
        ;; gpsim commands be invoked:
        ;;  gpsim> node new_test_node
        ;;  gpsim> attach new_test_node porta4 portb0
        ;;
        ;; Also, recall that porta bit 4 is an open collector
        ;; output (it can't drive high). So to generate the
        ;; the logic highs, the portb weak pull-up resistors
        ;; need to be enabled.
        
	clrwdt
test_inte:

        
        bsf     STATUS,RP0
        bcf     (TRISA ^ 0x80),4        ;Make porta bit 4 an output
        bsf     (TRISB ^ 0x80),0        ;and portb bit 0 an input
        bcf     (OPTION_REG^0x80),NOT_RBPU  ;Enable the portb weak pull-ups
        bsf     (OPTION_REG^0x80),INTEDG    ;Interrupt on rising edge
        bcf     STATUS,RP0

        bcf     PORTA,4
        bcf     INTCON,T0IE

        call    test_int_pin


    .assert "(temp2) == 0x40,'*** FAILED 14bit-interrupts, no int0 rising'"
	nop
    .assert "(temp1) == 0x00,'*** FAILED 14bit-interrupts, int0 interrupt on wrong edge'"
	nop

        bcf     INTCON,INTE             ;Disable inte interrupt
        bsf     STATUS,RP0
        bcf     (OPTION_REG^0x80),INTEDG    ;Interrupt on falling edge
        bcf     STATUS,RP0

        call    test_int_pin

    .assert "(temp1) == 0x40,'*** FAILED 14bit-interrupts, no int0 falling'"
	nop
    .assert "(temp2) == 0x00,'*** FAILED 14bit-interrupts, int0 interrupt on wrong edge'"
	nop



        ;;
        ;; test_rbif
        ;;
        ;; This next block tests the interrupt on change feature of
        ;; port b's I/O pins 4-7
        ;; 
	clrwdt
test_rbif:

        bcf     INTCON,INTE     ;Disable the rb0 interrupt

        bsf     STATUS,RP0
        bsf     (TRISA ^ 0x80),4        ;Porta bit 4 is now an input 
        bcf     STATUS,RP0

        clrf    temp5                   ;Interrupt flag
        clrf    temp1
        
        movlw   0x10
        movwf   temp2

	movlw	6
	movwf	PORTB

	clrwdt
rbif_l1:
        
        bcf     INTCON,RBIE
        
        movf    temp2,w
        tris    PORTB

;        clrf    PORTB
        movf    PORTB,W

        clrf    temp5                   ;Interrupt flag

        bcf     INTCON,RBIF
        bsf     INTCON,RBIE


        swapf   temp2,w
        xorwf   PORTB,f

        btfsc   temp5,1
         iorwf  temp1,f

   .assert "(temp5 & 2) == 2"

        clrc
        rlf     temp2,f

        skpc
         goto   rbif_l1


done:   
  .assert  "'*** PASSED MidRange core interrupt test'"
        goto    $

failed:	
  .assert  "'*** FAILED MidRange core interrupt test'"
        goto    $



        ;;
        ;; This routine toggles one of the porta bits with the hope of generating
        ;; an interrupt on one of the portb bits.
        ;; temp1 counts the number of interrupts due to falling edges
        ;; temp2 counts the number of interrupts due to rising edges
        ;;
test_int_pin:
        
        bsf     STATUS,RP0
        bcf     (TRISA ^ 0x80),4        ;Make porta bit 4 an output
        bsf     (TRISB ^ 0x80),0        ;and portb bit 0 an input
        bcf     (OPTION_REG^0x80),NOT_RBPU  ;Enable the portb weak pull-ups
        bcf     STATUS,RP0

        clrf    temp1
        clrf    temp2
        movlw   0x40
        movwf   temp3       ; used for counting
        movwf   temp4

        bcf     PORTA,4
        bcf     INTCON,INTF
        bsf     INTCON,INTE

        ;;
        ;; Now loop round 64 times toggling the pin and count the interrupts
        ;;
inte_edgecount:
        bcf     temp5,0         ;Interrupt flag

        clrz    
        bsf     PORTA,4         ;Falling edge

        btfsc   temp5,0
         incf   temp2,f         ;Falling edge caused the interrupt

        bcf     temp5,0
        bcf     PORTA,4         ;Rising edge

        btfsc   temp5,0
         incf   temp1,f         ;Rising edge caused the interrupt

        decfsz  temp3,f
         goto   inte_edgecount

        return


        end
