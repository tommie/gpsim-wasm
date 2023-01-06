
	list	p=16f887
        include <p16f887.inc>
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

  .assert "'*** FAILED 16F887 epwm unexpected interrupt'"
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
   .sim "scope.ch0 = 'portc2'"	; P1A
   .sim "scope.ch1 = 'portd5'"	; P1B
   .sim "scope.ch2 = 'portd6'"	; P1C
   .sim "scope.ch3 = 'portd7'"	; P1D
   .sim "scope.ch4 = 'portb4'"
  .sim "node na0"
   .sim "attach na0 portb2 portb0"


    call setup
    call full_forward
    call full_reverse
    call   wait_period
    call   wait_period
    call half_mode
    call pwm_shutdown

    clrf  CCP1CON       ; turn off PWM
    clrf  CCP2CON       ; turn off PWM


  .assert "'*** PASSED 16f887 epwm test'"
	goto $-1


pwm_shutdown
    BANKSEL CM1CON0
    movlw   (1<<C1ON)|(1<<C1POL)|(1<<C1OE) ; Enable Comparator use C1IN+, C12IN0
    movwf   CM1CON0
    BANKSEL ECCPAS
    ; C1 control and tristate mode
    movlw   (1<<ECCPAS0)|(1<<PSSAC1)|(1<<PSSBD1)
    movwf  ECCPAS
    nop
    bcf	    PWM1CON,PRSEN	; Shutdown clears on condition clear
    BANKSEL CM1CON0
    bcf     CM1CON0,C1POL   ; toggle ouput polarity
    
    call   wait_period
    call   wait_period
    return

half_mode
    movlw  0xAC          ;  PWM mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    movlw  0x05		 ; Start Timer2 prescaler is 4
    movwf  T2CON
    movlw  0x08		 ; set dead-band
    banksel PWM1CON
    movwf  PWM1CON
    call   wait_period
    call   wait_period
    return

full_forward
    banksel CCP1CON
    movlw  0x6C          ;  PWM full forward mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    movlw  0x05		 ; Start Timer2 prescaler is 4
    movwf  T2CON
    banksel PR2		 ; Bank 1
    movlw  0x2F          ;
    movwf  PR2           ;
    return

full_reverse
    banksel CCP1CON
    movlw  0xEC          ;  PWM full reverse mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    return

wait_period
    banksel PIR1
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF	 ; loop until TMR2 sets IF flag
    goto   $-1
    clrf   TMR0
    return


pwm_mode
    movlw  0xAC          ;  PWM mode, 2 LSbs of Duty cycle = 10
    movwf  CCP1CON       ;
    movlw  0x05		 ; Start Timer2 prescaler is 4
    movwf  T2CON
    movlw  0x2C		 ; PWM mode, 2 LSbs of Duty cycle = 00
    movwf  CCP2CON       ;
  .assert "ccpr1l != ccpr1h, '*** FAILED 16F887 epwm CCPR1H before TRM2 reset'"
    nop
;
; The CCP1 interrupt is disabled,
; do polling on the TMR2 Interrupt flag bit
;
PWM_Period_Match
    btfss  PIR1, TMR2IF
    goto   PWM_Period_Match
    clrf   TMR0

  .assert "ccpr1l == ccpr1h, '*** FAILED 16F887 epwm CCPR1H loaded from CCPR1H'"
    nop

  .assert "(portc & 0x6) == 0x6, '*** FAILED 16F887 epwm CCP1, CCP2 are high'"
   nop
   ; loop until CCP2 goes low
   btfsc  PORTC,1
   goto   $-1
  .assert "tmr0 == 0x0f, '*** FAILED 16F887 epwm CCP2 duty cycle'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,2
   goto   $-1
  .assert "tmr0 == 0x1f, '*** FAILED 16F887 epwm CCP1 duty cycle'"
   nop
;
; Wait for end of PWM cycle
;
    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x2f, '*** FAILED 16F887 epwm TMR2 period'"
   nop
;
; Increase  TMR2 but less than first duty cycle
;
    clrf   TMR0

    movlw   0x0D
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc   PORTC,2
    goto    $-1

  .assert "(portc & 0x6) == 0x0, '*** FAILED 16F887 epwm TMR2 put, only change period'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x23, '*** FAILED 16F887 epwm TMR2 put, only change period'"
    nop
;
; Increase  TMR2 between first and second duty cycle
;
    clrf   TMR0

    movlw   0x1D
    movwf   TMR2	; update timer 

   ; loop until CCP1 goes low
    btfsc   PORTC,2
    goto    $-1

  .assert "(portc & 0x6) == 0x2, '*** FAILED 16F887 epwm TMR2 put, between duty cycles'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1
  .assert "tmr0 == 0x13, '*** FAILED 16F887 epwm TMR2 put, between duty cycles'"
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
    btfsc  PORTC,2
    goto   $-1
  .assert "tmr0 == 0x77, '*** FAILED 16F887 epwm CCP1 duty cycle after wrap'"
    nop

    bcf    PIR1, TMR2IF
    btfss  PIR1, TMR2IF
    goto   $-1

  .assert "tmr0 == 0x80, '*** FAILED 16F887 epwm TMR2 > PR2 causes wrap'"
    nop

;
; write reduced PR2 
;
   clrf   TMR0

   ; loop until CCP2 goes low
   btfsc  PORTC,1
   goto   $-1
  .assert "tmr0 == 0x07, '*** FAILED 16F887 epwm CCP2 duty cycle PR2 to 0x20'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,2
   goto   $-1
  .assert "tmr0 == 0x0f, '*** FAILED 16F887 epwm CCP1 duty cycle PR2 to 0x20'"
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
  .assert "tmr0 == 0x10, '*** FAILED 16F887 epwm TMR2 period PR2 to 0x20'"
   nop

;
; write reduced PR2 < TRM2
;
   clrf   TMR0

   ; loop until CCP2 goes low
   btfsc  PORTC,1
   goto   $-1
  .assert "tmr0 == 0x07, '*** FAILED 16F887 epwm CCP2 duty cycle PR2 to 0x10'"
   nop
   ; loop until CCP1 goes low
   btfsc  PORTC,2
   goto   $-1
  .assert "tmr0 == 0x0f, '*** FAILED 16F887 epwm CCP1 duty cycle PR2 to 0x10'"
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
  .assert "tmr0 == 0x88, '*** FAILED 16F887 epwm TMR2 period PR2 to 0x10 wraps'"
   nop

   return

setup: 
    banksel CCP1CON
    clrf   CCP1CON       ;  CCP Module is off
    clrf   CCP2CON       ;  CCP Module is off
    clrf   TMR2          ;  Clear Timer2
    clrf   TMR0          ;  Clear Timer0
    banksel ANSEL
    clrf    ANSEL	 ; turn ports to digital
    clrf    ANSELH
    banksel CCPR1L
    movlw  0x1F          ;
    movwf  CCPR1L        ;  Duty Cycle is 25% of PWM Period
    movlw  0x0F          ;
    movwf  CCPR2L        ;  Duty Cycle is 25% of PWM Period
    clrf   INTCON        ;  Disable interrupts and clear T0IF
    banksel TRISC
    ; Make output pins
    bcf    TRISC, 1      ;  Make pin output
    bcf    TRISC, 2      ;  P1A
    bcf	   TRISD,5	 ;  P1B
    bcf	   TRISD,6	 ;  P1C
    bcf	   TRISD,7	 ;  P1D
    clrf   PIE1          ;  Disable peripheral interrupts
    movlw  0x03		 ; Port B pull-up  Tmr0 internal clock prescaler 16
    movwf  OPTION_REG
    banksel PIR1         ;  Bank0
    clrf   PIR1          ;  Clear peripheral interrupts Flags
    return
	end

