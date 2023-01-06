
        ;; The purpose of this program is to test gpsim's ability to 
        ;; simulate interrupts on the midrange core (specifically the 'f2321).


	list    p=18f2321               ; list directive to define processor
	include <p18f2321.inc>          ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

 ;; Suppress warnings of using 'option' instruction.
	errorlevel -224

 CONFIG WDT=OFF
 CONFIG MCLRE=ON, LPT1OSC=OFF, PBADEN=DIG, CCP2MX=RC1



GPR_DATA        UDATA
intflags        RES     1
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

 GLOBAL temp1,temp2,temp3,temp4,temp5,intflags

 GLOBAL start
;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        goto   start                     ; go to beginning of program




;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

HI_PRI  CODE    0x008               ; interrupt vector location
        bra     hi_pri_isr


LO_PRI  CODE    0x018               ; interrupt vector location
        movwf   w_temp
        movff   STATUS,status_temp

check_rbi:
        btfsc   INTCON,RBIF
         btfss  INTCON,RBIE
          bra   check_int

        bsf     intflags,0          ;Set a flag to indicate rb4-7 int occured
        bcf     INTCON,RBIF
	movf	PORTB,w
        
check_int:
        btfsc   INTCON,INT0IF
         btfss  INTCON,INT0IE
          bra   check_t0

        bsf     intflags,1          ;Set a flag to indicate rb0 int occured
        bcf     INTCON,INT0IF

check_t0:
        btfsc   PIR1,TMR2IF
         btfss  PIE1,TMR2IE
          bra   check_int1

    ;; tmr0 has rolled over

        bcf     PIR1,TMR2IF     ; Clear the pending interrupt
        bsf     intflags,2      ; Set a flag to indicate rollover
                
check_int1:
        btfsc   INTCON3,INT1IF
         btfss  INTCON3,INT1IE
          bra   check_int2

        bsf     temp3,0         ;Set a flag to indicate rb1 int occured
        bcf     INTCON3,INT1IF

check_int2:
        btfsc   INTCON3,INT2IF
         btfss  INTCON3,INT2IE
          bra   exit_int

        bsf     temp3,1         ;Set a flag to indicate rb2 int occured
        bcf     INTCON3,INT2IF

exit_int:               

        movf    w_temp,w
        movff   status_temp,STATUS
        retfie


hi_pri_isr:
check_int1hi:
        btfsc   INTCON3,INT1IF
         btfss  INTCON3,INT1IE
          bra   check_int2hi

        bsf     temp3,2         ;Set a flag to indicate rb1 int occured
        bcf     INTCON3,INT1IF

check_int2hi:
        btfsc   INTCON3,INT2IF
         btfss  INTCON3,INT2IE
          bra   check_rbih

        bsf     temp3,3         ;Set a flag to indicate rb2 int occured
        bcf     INTCON3,INT2IF

check_rbih:
        btfsc   INTCON,RBIF
         btfss  INTCON,RBIE
          bra   check_inth

        bsf     intflags,4          ; Set a flag to indicate rb4-7 int occured
        bcf     INTCON,RBIF
	movf	PORTB,w
        
check_inth:
        btfsc   INTCON,INT0IF
         btfss  INTCON,INT0IE
          bra   check_t0h

        bsf     intflags,5          ; Set a flag to indicate rb0 int occured
        bcf     INTCON,INT0IF

check_t0h:
        btfsc   PIR1,TMR2IF
         btfss  PIE1,TMR2IE
          bra   exit_inth

    ;; tmr1 has rolled over
        
        bcf     PIR1,TMR2IF         ; Clear the pending interrupt
        bsf     intflags,6          ; Set a flag to indicate rollover

        btfsc   temp5,7             ; Is a nesting test in progress?
        btg     PORTB,2
        nop
        nop
        btfsc   intflags,0
    .assert "'*** FAILED - low priority PORTB interrupt serviced during high priority ISR'"
        bsf     failures,7

                
exit_inth:
        retfie  FAST



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
        ;; Assume no failures

        clrf    failures
        clrf    temp5

        movlw   0Fh    ; Configure all A/D
        movwf   ADCON1 ; for digital inputs
        movwf   07h    ; Configure comparators
        movwf   CMCON  ; for digital input

        clrf    INTCON
        bsf     RCON,IPEN       ; Enable priority mode
        movlw   080h
        movwf   INTCON2
        clrf    INTCON3
        
        bsf     RCON,IPEN
        bsf     INTCON,GIE      ;Enable high priority interrupts

        ;; First we test that with only high priority interrupts enabled, 
        ;; those set to low priority are ignored
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
        
test_inte:

        
        bsf     TRISB,0        ;and portb bit 0 an input
        bcf     TRISA,4        ;Make porta bit 4 an output
        bcf     INTCON2,NOT_RBPU  ;Enable the portb weak pull-ups
        bsf     INTCON2,INTEDG0   ;Interrupt on rising edge


        movlw   0xff
        movwf   temp1
        movwf   temp2
        movwf   temp3
        movwf   temp4
                
        bcf     PORTA,4
        bcf     INTCON,INT0IF
        bsf     INTCON,INT0IE


        ;;
        ;; This routine toggles porta bit 4 with the hope of generating
        ;; an interrupt on portb bit 0. Note that INT0 is ALWAYS high priority
        ;;
inte_edgecount:
        clrf    intflags        ;Interrupt flag

        bsf     PORTA,4         ;Rising edge

        bcf     PORTA,4         ;Falling edge

        nop
        nop

    .assert "intflags == 0x20, '*** FAILED INT0 interrupt not high priority'"
        movlw   0x20
        cpfseq  intflags
         bra    failed

        bcf     INTCON,INT0IF
        bcf     INTCON,INT0IE     ;Disable the rb0 interrupt

        ;;
        ;; test_rbif
        ;;
        ;; This next block uses the interrupt on change feature of
        ;; port b's I/O pins 4-7 to confirm that a low priority interrupt
        ;; is masked.
        ;; 
test_rbif:
        bsf     TRISA,4         ; Porta bit 4 is now an input

        movlw   0xF0
        movwf   TRISB           ; Upper nibble of port B is connected to lower nibble

        movlw   0x10
        movwf   temp2

	movlw	6
	movwf	PORTB

rbif_l1:
        
        bcf     INTCON,RBIE
        
        movf    PORTB,W

        clrf    intflags         ; Interrupt flag

        bcf     INTCON,RBIF
        bcf     INTCON2,RBIP
        bsf     INTCON,RBIE

        swapf   temp2,w
        xorwf   PORTB,f

    .assert "intflags == 0, '*** FAILED low priority portB interrupt not masked'"
        tstfsz  intflags
         bra    failed

    .assert "(intcon&1) == 1, '*** FAILED Toggle of portb pin not detected'"
        btfss   INTCON,RBIF
         bra    failed

        clrc
        rlcf    temp2,f

        skpc
         bra    rbif_l1


        ;;
        ;; The following block of code tests tmr2 together with interrupts.

        bsf     PIE1,TMR2IE     ;Enable TMR2 overflow interrupts

        clrf    intflags        ;Software flag used to monitor when the
                                ;interrupt has been serviced.

        movlw   0x04
        movwf   T2CON           ;Assign new prescale value
        movlw   0x80
        movwf   PR2

        clrf    temp1
        decfsz  temp1,f         ; Hang around long enough that an overflow should occur
        goto    $-2

        btfss   PIR1,TMR2IF     ; check the overflow happened
    .assert "'*** FAILED no TMR2 overflow detected'"
         bra    failed

    .assert "intflags == 0, '*** FAILED low priority TRM2 interrupt not masked'"
        tstfsz  intflags
         bra    failed

        bcf     PIE1,TMR2IE     ;Disable TMR2 overflow interrupts
        clrf    T2CON           ; and switch off TMR2


        ;; Next we test that with low priority interrupts enabled, 
        ;; those set to low priority are handled by the low priority ISR
        ;;
        bsf     INTCON,GIEL     ;Enable low priority interrupts

        ;;
        ;; test_rbif
        ;;
        ;; This next block uses the interrupt on change feature of
        ;; port b's I/O pins 4-7 to confirm that a low priority interrupt
        ;; is responded to as such.
        ;; 
test_rbif_2:
        bsf     TRISA,4         ; Porta bit 4 is now an input

        movlw   0xF0
        movwf   TRISB           ; Upper nibble of port B is connected to lower nibble

        movlw   0x10
        movwf   temp2

	movlw	6
	movwf	PORTB

rbif_l2:
        
        bcf     INTCON,RBIE
        
        movf    PORTB,W

        clrf    intflags         ; Interrupt flag

        bcf     INTCON,RBIF
        bcf     INTCON2,RBIP
        bsf     INTCON,RBIE

        swapf   temp2,w
        xorwf   PORTB,f

    .assert "intflags == 1, '*** FAILED low priority portB interrupt not seen'"
        decfsz  intflags,w
         bra    failed

        clrc
        rlcf    temp2,f

        skpc
         bra    rbif_l2


        ;;
        ;; The following block of code tests tmr2 together with interrupts.

        bsf     PIE1,TMR2IE     ;Enable TMR2 overflow interrupts

        clrf    intflags        ;Software flag used to monitor when the
                                ;interrupt has been serviced.

        movlw   0x04
        movwf   T2CON           ;Assign new prescale value
        movlw   0x80
        movwf   PR2

        clrf    temp1
        decfsz  temp1,f         ; Hang around long enough that an overflow should occur
        goto    $-2

        btfsc   PIR1,TMR2IF     ; check the overflow happened
    .assert "'*** FAILED no TMR2 overflow detected'"
         bra    failed

    .assert "intflags == 4, '*** FAILED low priority TMR2 interrupt not masked'"

        bcf     PIE1,TMR2IE     ;Disable TMR2 overflow interrupts
        clrf    T2CON           ; and switch off TMR2

        ;; Finally test that setting peripherals to high priority causes the
        ;; right ISR to be run
        ;;
        bsf     INTCON,GIEL     ;Enable low priority interrupts

        ;;
        ;; test_rbif
        ;;
        ;; This next block uses the interrupt on change feature of
        ;; port b's I/O pins 4-7 to confirm that a low priority interrupt
        ;; is responded to as such.
        ;; 
test_rbif_3:
        bsf     TRISA,4         ; Porta bit 4 is now an input

        movlw   0xF0
        movwf   TRISB           ; Upper nibble of port B is connected to lower nibble

        movlw   0x10
        movwf   temp2

	movlw	6
	movwf	PORTB

        bsf     INTCON2,RBIP

rbif_l3:
        
        bcf     INTCON,RBIE
        
        movf    PORTB,W

        clrf    intflags         ; Interrupt flag

        bcf     INTCON,RBIF
        bsf     INTCON,RBIE

        swapf   temp2,w
        xorwf   PORTB,f

    .assert "intflags == 0x10, '*** FAILED high priority portB interrupt not seen'"
;        decfsz  intflags,w
;         bra    failed

        clrc
        rlcf    temp2,f

        skpc
         bra    rbif_l3


        ;;
        ;; The following block of code tests tmr2 together with interrupts.

        bsf     PIE1,TMR2IE     ;Enable TMR2 overflow interrupts
        bsf     IPR1,TMR2IP     ; ... as high priority

        clrf    intflags        ;Software flag used to monitor when the
                                ;interrupt has been serviced.

        movlw   0x04
        movwf   T2CON           ;Assign new prescale value
        movlw   0x80
        movwf   PR2

        clrf    temp1
        decfsz  temp1,f         ; Hang around long enough that an overflow should occur
        goto    $-2

        btfsc   PIR1,TMR2IF     ; check the overflow happened
    .assert "'*** FAILED no TMR2 overflow detected'"
         bra    failed

    .assert "intflags == 0x40, '*** FAILED high priority TMR2 interrupt not seen'"


        ;;
        ;; The following block of code tests nesting of interrupts, by generating a
        ;; high priority TMR2 interrupt with the "cause PORTB interrupt" flag set. 
        ;; The high priority ISR then triggers a low priority interrupt, which should
        ;; be handled after leaving the high priority but before doing any more of
        ;; the background stuff.

        bcf     INTCON2,RBIP
        bsf     temp5,7         ; TMR2 ISR triggers a PORTB event

        clrf    intflags        ;Software flag used to monitor when the
                                ;interrupt has been serviced.

;        clrf    temp1
;        decfsz  temp1,f         ; Hang around long enough that an overflow should occur
;        goto    $-2

        btfss   intflags,6      ; Wait for the TMR2 interrupt (must only see one)
        goto    $-2

    .assert "intflags == 0x41, '*** FAILED priority nesting test'"
	nop

        bcf     PIE1,TMR2IE     ;Disable TMR2 overflow interrupts
        clrf    T2CON           ; and switch off TMR2

	;; Test int1 and int2 
	;; 1> low priority with GIEL=0 (no inturrupt expected)
	;; 2> log priority with GIEL=1

	
        bcf     INTCON,GIEL     ;disable low priority interrupts
	movlw	0x6		; b5 drives b1 b6 drives b2
	movwf	TRISB
	bcf	PORTB,5
	bcf	PORTB,6
	clrf	temp3
	movlw	(1<<INT1IE)|(1<<INT2IE)	; enable int1, int2 inturrupt with low priority
	movwf	INTCON3
	bsf	PORTB,5
	bsf	PORTB,6
	bcf	PORTB,5
	bcf	PORTB,6
    .assert "temp3 == 0, '***FAILED GIEL not masking int1 or int2'"
	nop

	bsf	INTCON,GIEL
	bsf	PORTB,5
	bsf	PORTB,6
	bcf	PORTB,5
	bcf	PORTB,6
    .assert "temp3 == 3, '***FAILED low priority inturrupt on int1 or int2'"
	nop

done:  
  .assert  "'*** PASSED 16bit interrupt priority test'"
        goto    $

failed:	
  .assert  "'*** FAILED 16bit interrupt priority test'"
        goto    $

        end
