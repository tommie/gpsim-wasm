
	list	p=12f683
        include <p12f683.inc>
        include <coff.inc>

  __CONFIG _WDT_OFF & _INTOSCIO & _MCLRE_OFF

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16c71.
	;; Specifically, the pwm is tested.
    
    ; derived from pwm_877a
	; (c) G.Schielke 2022-01-28
	
        errorlevel -302
        radix dec

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

;----------------------------------------------------------------------
;   ******************* INTERRUPT VECTOR LOCATION  ********************
;----------------------------------------------------------------------

INT_VECTOR   CODE    0x004               ; interrupt vector location
	;; 
	;; Interrupt
	;; 
isr:
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp

	bcf	STATUS,RP0	;adcon0 is in bank 0

  .assert "'*** FAILED pwm_683 unexpected interrupt'"
	nop

check:
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie

#ifdef _GPSIM
    .sim "scope.ch0 = \"gpio2\""
#endif
    
;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------

MAIN    CODE

start:
        movlw   0x60            ;4MHz, fosc2:0 source
        movwf   OSCCON
        
        clrf   CCP1CON      ;  CCP Module is off
        clrf   TMR2         ;  Clear Timer2
        clrf   TMR0         ;  Clear Timer0
        movlw  0x1F         ;
        movwf  CCPR1L       ;  Duty Cycle is 25% of PWM Period
        clrf   INTCON       ;  Disable interrupts and clear T0IF
        
        BANKSEL(ANSEL)      ; RB1
        movlw   0x31        ; only an0, Frc clock
        movwf   ANSEL ^ 0x80
        
    .assert "((ansel & 0x31)==0x31), \"ANSEL wrong!\""
        nop
        
        BANKSEL(CMCON0)             ; RB0
        movlw   0x07                ; comparator module off
        movwf   CMCON0
        
    .assert "((cmcon0 & 0x07)==0x07), \"CMCON0 wrong!\""
        nop
            
        BANKSEL(PR2)        ;  Bank1
        movlw  0x2F         ;
        movwf  PR2          ;
        bcf    TRISIO,TRISIO2 ;  Make pin output
        clrf   PIE1         ;  Disable peripheral interrupts
        movlw  0x83         ; Tmr0 internal clock prescaler 16
        movwf  OPTION_REG

        BANKSEL(PIR1)       ;  Bank1
        clrf   PIR1         ;  Clear peripheral interrupts Flags
        movlw  0x2C         ;  PWM mode, 2 LSbs of Duty cycle = 10
        movwf  CCP1CON      ;
        movlw  0x06         ; Start Timer2 prescaler is 16, just like TMR0
        movwf  T2CON
;
; The CCP1 interrupt is disabled,
; do polling on the TMR2 Interrupt flag bit
;
PWM_Period_Match
        btfss  PIR1, TMR2IF
        goto   PWM_Period_Match
        clrf   TMR0

    .assert "ccpr1l == ccpr1h, '*** FAILED pwm_683 CCPR1H loaded from CCPR1H'"
        nop

    .assert "(gpio & 0x4) == 0x4, '*** FAILED pwm_683 CCP1 is high'"
        nop
        
        ; loop until CCP1 goes low
        btfsc  GPIO,GP2
        goto   $-1
        
    .assert "tmr0 == 0x1f, '*** FAILED pwm_683 CCP1 duty cycle'"
        nop
;
; Wait for end of PWM cycle
;
        bcf    PIR1, TMR2IF
        btfss  PIR1, TMR2IF
        goto   $-1
        
    .assert "tmr0 == 0x2f, '*** FAILED pwm_683 TMR2 period'"
        nop
;
; Increase  TMR2 but less than first duty cycle
;
        clrf   TMR0

        movlw   0x0D
        movwf   TMR2	; update timer 

; loop until CCP1 goes low
        btfsc   GPIO,GP2
        goto    $-1

    .assert "(gpio & 0x4) == 0x0, '*** FAILED pwm_683 TMR2 put, only change period'"
        nop

        bcf    PIR1, TMR2IF
        btfss  PIR1, TMR2IF
        goto   $-1
        
    .assert "tmr0 == 0x23, '*** FAILED pwm_683 TMR2 put, only change period'"
        nop
;
; Increase  TMR2 between first and second duty cycle
;
        clrf   TMR0

        movlw   0x1D
        movwf   TMR2	; update timer 

    ; loop until CCP1 goes low
        btfsc   GPIO,GP2
        goto    $-1
        
    .assert "(gpio & 0x4) == 0x0, '*** FAILED pwm_683 TMR2 put, between duty cycles'"
        nop

        bcf    PIR1, TMR2IF
        btfss  PIR1, TMR2IF
        goto   $-1
        
    .assert "tmr0 == 0x13, '*** FAILED pwm_683 TMR2 put, between duty cycles'"
        nop
;
;  in this test TMR2 > PR2, expect TMR2 to wrap around
;
        bsf    STATUS, RP0   ;  Bank1
        movlw  0x84		 ; Tmr0 internal clock prescaler 32
        movwf  OPTION_REG
        bcf    STATUS, RP0   ;  Bank0
        clrf   TMR0

        movlw   0x31
        movwf   TMR2	; update timer above PR2+1

; loop until CCP1 goes low
        btfsc  GPIO,GP2
        goto   $-1
    
    .assert "tmr0 == 0x77, '*** FAILED pwm_683 CCP1 duty cycle after wrap'"
        nop

        bcf    PIR1, TMR2IF
        btfss  PIR1, TMR2IF
        goto   $-1

    .assert "tmr0 >= 0x79 && tmr0 <= 0x81, '*** FAILED pwm_683 TMR2 > PR2 causes wrap'"
        nop

;
; write reduced PR2 
;
        clrf   TMR0

; loop until CCP1 goes low
        btfsc  GPIO,GP2
        goto   $-1
        
    .assert "tmr0 == 0x0f, '*** FAILED pwm_683 CCP1 duty cycle PR2 to 0x20'"
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
    
    .assert "tmr0 == 0x10, '*** FAILED pwm_683 TMR2 period PR2 to 0x20'"
        nop

;
; write reduced PR2 < TRM2
;
        clrf   TMR0

; loop until CCP1 goes low
        btfsc  GPIO,GP2
        goto   $-1
    
    .assert "tmr0 == 0x0f, '*** FAILED pwm_683 CCP1 duty cycle PR2 to 0x10'"
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
    
    .assert "tmr0 == 0x88, '*** FAILED pwm_683 TMR2 period PR2 to 0x10 wraps'"
        nop

        clrf  CCP1CON       ; turn off PWM

    .assert "'*** PASSED 12f683 PWM test'"
        nop
        
        goto $-1

        end
