
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

 GLOBAL temp,temp1,temp2,temp3,temp4,temp5, w_temp

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

    .assert "(w_temp) == 0x12,'*** FAILED interrupt_14bit/int_sleep, w not set'"
	nop

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
  .sim "stimulus asynchronous_stimulus "
  .sim "initial_state 0"
  .sim "start_cycle 0"
  .sim "period 305"
  .sim "{ 38,1 ,  76,0 , 114,1 , 152,0 , 190,1 , 228,0 , 266,1}"
  .sim "name portDrive"
  .sim "end"

  .sim "node stim"

  .sim "attach stim portDrive portb0"
        
	clrwdt
test_inte:

        
        bsf     STATUS,RP0
        bsf     (TRISB ^ 0x80),0        ;and portb bit 0 an input
        bcf     (OPTION_REG^0x80),NOT_RBPU  ;Enable the portb weak pull-ups
        bsf     (OPTION_REG^0x80),INTEDG    ;Interrupt on rising edge
        bcf     STATUS,RP0

	bcf	INTCON,INTF
	bsf	INTCON,INTE
	bsf	INTCON,GIE
	movlw	0x12
x:
        btfss   temp5,0
	  goto x
	clrf	temp5
	clrw
rrr:
	movlw	0x12
	sleep
	nop
	btfss	temp5,0		; did we interrupt?
	goto	failed		; no
	clrw	
	clrf	temp5

   	nop
	movlw   0x12
	sleep
	nop
	goto	done
	goto	failed		; no

done:   
	nop
  .assert  "'*** PASSED interrupt_14bit/int_sleep sleep interrupt test'"
	nop
        goto    $

failed:	
  .assert  "'*** FAILED interrupt_14bit/int_sleep sleep interrupt test'"
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
