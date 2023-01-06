
	list	p=18f6520
        include <p18f6520.inc>
        include <coff.inc>

    CONFIG WDT=OFF

	;; The purpose of this program is to test gpsim's ability to simulate a 
        ;; pic with more than two PWM channels. The 18F6520 is such a device.

        ;; CCP1 is set with a 50% duty cycle and used to test timing
        ;; CCP2 is set with a 25% duty cycle and used to test order
        ;; CCP3 is set beyond the period and tied to INT0 to test 100% duty
        ;; CCP4 is set with a 75% duty cycle
        ;; CCP5 is set with a 75% duty cycle to test two simultaneous PWM edges

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
        goto   start                     ; go to beginning of program

INTERRUPT     CODE    0x008
	;; 
	;; Interrupt
	;; 
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp

  .assert "'*** FAILED pwm_6520 unexpected interrupt'"
	nop

check:
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie




   .sim "node pwm3"
   .sim "attach pwm3 portg0 portb0"

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:
    clrf   CCP1CON       ;  CCP Module is off
    clrf   CCP2CON       ;  CCP Module is off
    clrf   CCP3CON       ;  CCP Module is off
    clrf   CCP4CON       ;  CCP Module is off
    clrf   CCP5CON       ;  CCP Module is off
    clrf   TMR2          ;  Clear Timer2
    clrf   TMR0L         ;  Clear Timer0
    movlw  0x1F          ;
    movwf  CCPR1L        ;  Duty Cycle is 50% of PWM Period
    movlw  0x0F          ;
    movwf  CCPR2L        ;  Duty Cycle is 25% of PWM Period
    movlw  0x4F          ;
    movwf  CCPR3L        ;  Duty Cycle is 125% of PWM Period
    movlw  0x2F          ;
    movwf  CCPR4L        ;  Duty Cycle is 75% of PWM Period
    movlw  0x2F          ;
    movwf  CCPR5L        ;  Duty Cycle is 75% of PWM Period
    clrf   INTCON        ;  Disable interrupts and clear T0IF
    clrf   INTCON2       ;  external interrupt pins trigger on falling edge
    movlw  0x3F          ;
    movwf  PR2           ;
    bcf    TRISC, 1      ;  Make pin output
    bcf    TRISC, 2      ;  Make pin output
    bcf    TRISG, 0      ;  Make pin output
    bcf    TRISG, 3      ;  Make pin output
    bcf    TRISG, 4      ;  Make pin output
    clrf   PIE1          ;  Disable peripheral interrupts
    movlw  0x83		 ; Tmr0 internal clock prescaler 16
    movwf  T0CON
    clrf   PIR1          ;  Clear peripheral interrupts Flags
    movlw  0x2C          ;  PWM mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    movlw  0x06		 ; Start Timer2 prescaler is 16 (to match TMR0, of course!)
    movwf  T2CON
    movlw  0x2C          ;  PWM mode, 2 LSbs of Duty cycle = 10
    movwf  CCP2CON       ;
    movwf  CCP3CON       ;
    movwf  CCP4CON       ;
    movwf  CCP5CON       ;
;
; The CCP1 interrupt is disabled,
; do polling on the TMR2 Interrupt flag bit
;
PWM_Period_Match
    btfss  PIR1, TMR2IF
    goto   PWM_Period_Match
    clrf   TMR0L

  .assert "ccpr1l == ccpr1h, '*** FAILED pwm_6520 CCPR1H loaded from CCPR1H'"
    nop

 bcf    INTCON,INT0IF       ; INT0 is tied to CCP3, which should remain always high

  .assert "(portc & 0x6) == 0x6, '*** FAILED pwm_6520 CCP1, CCP2 are high'"
   nop
   ; loop until CCP2 goes low
   btfsc  PORTC,1
   goto   $-1
  .assert "tmr0 == 0x0f, '*** FAILED pwm_6520 CCP2 duty cycle'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,2
   goto   $-1
  .assert "tmr0 == 0x1f, '*** FAILED pwm_6520 CCP1 duty cycle'"
   nop
   ; loop until CCP4 goes low
   btfsc  PORTG,3
   goto   $-1
  .assert "tmr0 == 0x2f, '*** FAILED pwm_6520 CCP4 duty cycle'"
   nop
;
; Wait for end of PWM cycle
;
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1

  .assert "(intcon & 2) == 0, '*** FAILED pwm_6520 PWM duty > PR2 : pin goes low'"
   nop
  .assert "(tmr0 >= 0x3f)&&(tmr0 <= 0x40), '*** FAILED pwm_6520 TMR2 period'"
   nop
;
; Increase  TMR2 but less than first duty cycle
;
    movlw   0x0D
    movwf   TMR2	; update timer 
    clrf   TMR0L

   ; loop until CCP1 goes low
    btfsc   PORTC,2
    goto    $-1

  .assert "(portc & 0x6) == 0x0, '*** FAILED pwm_6520 TMR2 put, only change period'"
    nop

   ; loop until CCP5 goes low
    btfsc   PORTG,4
    goto    $-1

  .assert "tmr0 == 0x22, '*** FAILED pwm_6520 TMR2 put, only change period'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x33, '*** FAILED pwm_6520 TMR2 put, only change period'"
    nop
;
; Increase  TMR2 between first and second duty cycle
;
    clrf   TMR0L

    movlw   0x1D
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc   PORTC,2
    goto    $-1

  .assert "(portc & 0x6) == 0x2, '*** FAILED pwm_6520 TMR2 put, between duty cycles'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x23, '*** FAILED pwm_6520 TMR2 put, between duty cycles'"
    nop
;
;  in this test TMR2 > PR2, expect TMR2 to wrap around
;
    movlw  0x84		 ; Tmr0 internal clock prescaler 32
    movwf  T0CON
    clrf   TMR0L

    movlw   0x41
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc  PORTC,2
    goto   $-1
  .assert "tmr0 == 0x6F, '*** FAILED pwm_6520 CCP1 duty cycle after wrap'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1

  .assert "tmr0 == 0x7F, '*** FAILED pwm_6520 TMR2 > PR2 causes wrap'"
    nop

;
; write reduced PR2 
;
   clrf   TMR0L

   ; loop until CCP2 goes low
   btfsc  PORTC,1
   goto   $-1
  .assert "tmr0 == 0x07, '*** FAILED pwm_6520 CCP2 duty cycle PR2 to 0x20'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,2
   goto   $-1
  .assert "tmr0 == 0x0f, '*** FAILED pwm_6520 CCP1 duty cycle PR2 to 0x20'"
   nop
    movlw  0x20
    movwf  PR2
;
; Wait for end of PWM cycle
;
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x10, '*** FAILED pwm_6520 TMR2 period PR2 to 0x20'"
   nop

;
; write reduced PR2 < TRM2
;
   clrf   TMR0L

   ; loop until CCP2 goes low
   btfsc  PORTC,1
   goto   $-1
  .assert "tmr0 == 0x07, '*** FAILED pwm_6520 CCP2 duty cycle PR2 to 0x10'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,2
   goto   $-1
  .assert "tmr0 == 0x0f, '*** FAILED pwm_6520 CCP1 duty cycle PR2 to 0x10'"
   nop
    movlw  0x10
    movwf  PR2
;
; Wait for end of PWM cycle
;
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x88, '*** FAILED pwm_6520 TMR2 period PR2 to 0x10 wraps'"
   nop

    clrf  CCP1CON       ; turn off PWM
    clrf  CCP2CON       ; turn off PWM
    clrf  CCP3CON       ; turn off PWM
    clrf  CCP4CON       ; turn off PWM


  .assert "'*** PASSED 18f6520 PWM test'"
	goto $-1
	end
