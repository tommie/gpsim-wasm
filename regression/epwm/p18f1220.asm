
	list	p=18f1220
        include <p18f1220.inc>
        include <coff.inc>

  CONFIG WDT=OFF

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16c71.
	;; Specifically, the pwm is tested.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA

t0_1 RES 1
t0_2 RES 1
x  RES  1
t1 RES  1
t2 RES  1
avg_lo RES  1
avg_hi RES  1
w_temp RES  1
status_temp RES  1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

	;; 
	;; Interrupt
	;; 
INT_VECTOR   CODE    0x008               ; interrupt vector location`
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp


  .assert "'FAILED 18f1220 unexpected interrupt'"
	nop

check:
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:
   .sim "scope.ch0 = 'portb3'"	; P1A
   .sim "scope.ch1 = 'portb2'"	; P1B
   .sim "scope.ch2 = 'portb6'"	; P1C
   .sim "scope.ch3 = 'portb7'"	; P1D
   .sim "scope.ch4 = 'portb1'"
  .sim "node na0"
   .sim "attach na0 porta0 portb1"


    call setup
    call full_forward
    call full_reverse
    call   wait_period
    call   wait_period
    call half_mode
    call pwm_shutdown

    clrf  CCP1CON       ; turn off PWM


  .assert "'*** PASSED 18f1220 PWM test'"
	goto $-1


pwm_shutdown
    ; INT1 low
    bcf	    PORTA,0
    ; C1 control and 0 output
    movlw   (1<<ECCPAS0)
    movwf  ECCPAS
    nop
    bsf	    PWM1CON,PRSEN	; Shutdown clears on condition clear
    bsf	    PORTA,0		; turn off condition
    
    call   wait_period
    call   wait_period
    return

half_mode
    movlw  0xAC          ;  PWM mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    movlw  0x05		 ; Start Timer2 prescaler is 4
    movwf  T2CON
    movlw  0x08		 ; set dead-band
    movwf  PWM1CON
    call   wait_period
    call   wait_period
    return

full_forward
    movlw  0x6C          ;  PWM full forward mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    movlw  0x05		 ; Start Timer2 prescaler is 4
    movwf  T2CON
    movlw  0x2F          ;
    movwf  PR2           ;
    return

full_reverse
    movlw  0xEC          ;  PWM full reverse mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    return

wait_period
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF	 ; loop until TMR2 sets IF flag
    goto   $-1
    clrf   TMR0L
    return


pwm_mode
    movlw  0xAC          ;  PWM mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    movlw  0x05		 ; Start Timer2 prescaler is 4
    movwf  T2CON
  .assert "ccpr1l != ccpr1h, 'CCPR1H before TRM2 reset'"
    nop
;
; The CCP1 interrupt is disabled,
; do polling on the TMR2 Interrupt flag bit
;
PWM_Period_Match
    btfss  PIR1, TMR2IF
    goto   PWM_Period_Match
    clrf   TMR0L

  .assert "ccpr1l == ccpr1h, 'CCPR1H loaded from CCPR1H'"
    nop

  .assert "(portc & 0x6) == 0x6, 'CCP1, CCP2 are high'"
   nop
   ; loop until CCP2 goes low
;   btfsc  PORTC,1
;  goto   $-1
; .assert "tmr0 == 0x0f, 'CCP2 duty cycle'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTB,3
   goto   $-1
  .assert "tmr0 == 0x1f, 'CCP1 duty cycle'"
   nop
;
; Wait for end of PWM cycle
;
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x2f, 'TMR2 period'"
   nop
;
; Increase  TMR2 but less than first duty cycle
;
    clrf   TMR0L

    movlw   0x0D
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc   PORTB,3
    goto    $-1

  .assert "(portc & 0x6) == 0x0, 'TMR2 put, only change period'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x23, 'TMR2 put, only change period'"
    nop
;
; Increase  TMR2 between first and second duty cycle
;
    clrf   TMR0L

    movlw   0x1D
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc   PORTB,3
    goto    $-1

  .assert "(portc & 0x6) == 0x2, 'TMR2 put, between duty cycles'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x13, 'TMR2 put, between duty cycles'"
    nop
;
;  in this test TMR2 > PR2, expect TMR2 to wrap around
;
    ;Tmr1 on 8 bit internal clock prescale 32
    movlw (1<<TMR0ON)|(1<<T08BIT)|(1<<T0PS2)
    movwf  T0CON
    clrf   TMR0L

    movlw   0x30
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc  PORTB,3
    goto   $-1
  .assert "tmr0 == 0x77, 'CCP1 duty cycle after wrap'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1

  .assert "tmr0 == 0x80, 'TMR2 > PR2 causes wrap'"
    nop

;
; write reduced PR2 
;
   clrf   TMR0L

   ; loop until CCP2 goes low
;   btfsc  PORTC,1
;   goto   $-1
;  .assert "tmr0 == 0x07, 'CCP2 duty cycle PR2 to 0x20'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTB,3
   goto   $-1
  .assert "tmr0 == 0x0f, 'CCP1 duty cycle PR2 to 0x20'"
   nop
    movlw  0x20
    movwf  PR2
;
; Wait for end of PWM cycle
;
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x10, 'TMR2 period PR2 to 0x20'"
   nop

;
; write reduced PR2 < TRM2
;
   clrf   TMR0L

   ; loop until CCP2 goes low
;  btfsc  PORTC,1
;  goto   $-1
; .assert "tmr0 == 0x07, 'CCP2 duty cycle PR2 to 0x10'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTB,3
   goto   $-1
  .assert "tmr0 == 0x0f, 'CCP1 duty cycle PR2 to 0x10'"
   nop
    movlw  0x10
    movwf  PR2
;
; Wait for end of PWM cycle
;
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x88, 'TMR2 period PR2 to 0x10 wraps'"
   nop

   return

setup: 
    movlw  0x7f		; ports are digital
    movwf  ADCON1
    clrf   CCP1CON       ;  CCP Module is off
    clrf   TMR2          ;  Clear Timer2
    clrf   TMR0L          ;  Clear Timer0
    movlw  0x1F          ;
    movwf  CCPR1L        ;  Duty Cycle is 25% of PWM Period
    clrf   INTCON        ;  Disable interrupts and clear T0IF
    bsf    PORTA,0
    ; Make output pins
    bcf    TRISA,0      ;  drive shutdown int1
    bcf    TRISB,3      ;  P1A
    bcf	   TRISB,2	 ;  P1B
    bcf	   TRISB,6	 ;  P1C
    bcf	   TRISB,7	 ;  P1D
    clrf   PIE1          ;  Disable peripheral interrupts
    clrf   PIR1          ;  Clear peripheral interrupts Flags
    return
	end

