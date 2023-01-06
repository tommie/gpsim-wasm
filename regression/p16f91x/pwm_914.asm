
	list	p=16f914
        include <p16f914.inc>
        include <coff.inc>

  __CONFIG _WDT_OFF

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16c71.
	;; Specifically, the pwm is tested.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR

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

INT_VECTOR   CODE    0x004               ; interrupt vector location
	;; 
	;; Interrupt
	;; 
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp

	bcf	STATUS,RP0	;adcon0 is in bank 0

  .assert "'FAILED 16F914 unexpected interrupt'"
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
    clrf   CCP1CON       ;  CCP Module is off
    clrf   CCP2CON       ;  CCP Module is off
    clrf   TMR2          ;  Clear Timer2
    clrf   TMR0          ;  Clear Timer0
    movlw  0x1F          ;
    movwf  CCPR1L        ;  Duty Cycle is 25% of PWM Period
    movlw  0x0F          ;
    movwf  CCPR2L        ;  Duty Cycle is 25% of PWM Period
    clrf   INTCON        ;  Disable interrupts and clear T0IF
    bsf    STATUS, RP0   ;  Bank1
    movlw  0x2F          ;
    movwf  PR2           ;
    bcf    TRISC, 5      ;  Make CCP1 pin an output
    bcf    TRISD, 2      ;  Make CCP2 pin an output
    clrf   PIE1          ;  Disable peripheral interrupts
    movlw  0x83		 ;  Tmr0 internal clock prescaler 16
    movwf  OPTION_REG
    bcf    STATUS, RP0   ;  Bank0
    clrf   PIR1          ;  Clear peripheral interrupts Flags
    movlw  0x2C          ;  PWM mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    movlw  0x06		 ; Start Timer2 prescaler is 16, just like TMR0
    movwf  T2CON
    movlw  0x2C		 ; PWM mode, 2 LSbs of Duty cycle = 00
    movwf  CCP2CON       ;
  .assert "ccpr1l != ccpr1h, 'CCPR1H before TRM2 reset'"
    nop
;
; The CCP1 interrupt is disabled,
; do polling on the TMR2 Interrupt flag bit
;
PWM_Period_Match
    btfss  PIR1, TMR2IF
    goto   PWM_Period_Match
    clrf   TMR0

  .assert "ccpr1l == ccpr1h, 'CCPR1H loaded from CCPR1H'"
    nop

  .assert "(portc & 0x20) == 0x20, 'CCP1 is high start PWM_Period_Match'"
   nop
  .assert "(portd & 0x04) == 0x04, 'CCP2 is high start PWM_Period_Match'"
   nop
   ; loop until CCP2 goes low
   btfsc  PORTD,2
   goto   $-1
  .assert "tmr0 == 0x0f, 'CCP2 duty cycle PWM_Period_Match'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,5
   goto   $-1
  .assert "tmr0 == 0x1f, 'CCP1 duty cycle PWM_Period_Match'"
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
    clrf   TMR0

    movlw   0x0D
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc   PORTC,5
    goto    $-1

  .assert "(portc & 0x20)==0, 'CCP1 is low TMR2 put, only change period'"
   nop
  .assert "(portd & 0x04)==0, 'CCP2 is low TMR2 put, only change period'"
   nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x23, 'TMR2 put, only change period'"
    nop
;
; Increase  TMR2 between first and second duty cycle
;
    clrf   TMR0

    movlw   0x1D
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc   PORTC,5
    goto    $-1

  .assert "(portc & 0x20) == 0x00, 'CCP1 is low - TMR2 put between duty cycles'"
   nop
  .assert "(portd & 0x04) == 0x04, 'CCP2 is high - TMR2 put between duty cycles'"
   nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x13, 'TMR2 put, between duty cycles'"
    nop
;
;  in this test TMR2 > PR2, expect TMR2 to wrap around
;
    bsf    STATUS, RP0   ;  Bank1
    movlw  0x84		 ; Tmr0 internal clock prescaler 32
    movwf  OPTION_REG
    bcf    STATUS, RP0   ;  Bank0
    clrf   TMR0

    movlw   0x30
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc  PORTC,5
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
   clrf   TMR0

   ; loop until CCP2 goes low
   btfsc  PORTD,2
   goto   $-1
  .assert "tmr0 == 0x07, 'CCP2 duty cycle PR2 to 0x20'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,5
   goto   $-1
  .assert "tmr0 == 0x0f, 'CCP1 duty cycle PR2 to 0x20'"
   nop
    bsf    STATUS, RP0   ;  Bank1
    movlw  0x20
    movwf  PR2
    bcf    STATUS, RP0   ;  Bank0
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
   clrf   TMR0

   ; loop until CCP2 goes low
   btfsc  PORTD,2
   goto   $-1
  .assert "tmr0 == 0x07, 'CCP2 duty cycle PR2 to 0x10'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,5
   goto   $-1
  .assert "tmr0 == 0x0f, 'CCP1 duty cycle PR2 to 0x10'"
   nop
    bsf    STATUS, RP0   ;  Bank1
    movlw  0x10
    movwf  PR2
    bcf    STATUS, RP0   ;  Bank0
;
; Wait for end of PWM cycle
;
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x88, 'TMR2 period PR2 to 0x10 wraps'"
   nop

    clrf  CCP1CON       ; turn off PWM
    clrf  CCP2CON       ; turn off PWM
    nop


  .assert "'*** PASSED p16f914 PWM test'"
	goto $-1
	end
