        ;; ttl595.asm
        ;;

	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros
        radix   dec                     ; Numbers are assumed to be decimal


;----------------------------------------------------------------------

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1


  GLOBAL done


;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
STARTUP    CODE	0

	bra	Start

;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location



ExitInterrupt:
	RETFIE	1


MAIN	CODE

Start:	

   .sim "module library libgpsim_modules"
   .sim "module load ttl595 t595"

   .sim "node nb0 nb1 nb2 nb3 nb4 nb5 nb6 nb7"
   .sim "node nc0 nc1 nc2 nc3 nc4 nc5 nc6 nc7"
#define Ds	LATC,0
#define RCK	LATC,1
#define MR	LATC,2
#define SCK	LATE,0
#define OE	LATE,1

   .sim "attach nc0 portc0 t595.Ds"
   .sim "attach nc1 portc1 t595.RCK"
   .sim "attach nc2 portc2 t595.MR"

   .sim "attach nb0 portb0 t595.Q0"
   .sim "attach nb1 portb1 t595.Q1"
   .sim "attach nb2 portb2 t595.Q2"
   .sim "attach nb3 portb3 t595.Q3"
   .sim "attach nb4 portb4 t595.Q4"
   .sim "attach nb5 portb5 t595.Q5"
   .sim "attach nb6 portb6 t595.Q6"
   .sim "attach nb7 portb7 t595.Q7"

   .sim "node nClk nE"
   .sim "attach nClk porte0 t595.SCK"
   .sim "attach nE porte1 t595.OE"

   .sim "node nQs"
   .sim "attach nQs porta0 t595.Qs"

   .sim "p18f452.xpos = 84"
   .sim "p18f452.ypos = 48"
   .sim "t595.xpos = 288"
   .sim "t595.ypos = 48"
   .sim "scope.ch0='portc0'"
   .sim "scope.ch1='portc1'"
   .sim "scope.ch2='portc2'"
   .sim "scope.ch3='porte0'"
   .sim "scope.ch4='porte1'"
   .sim "scope.ch5='portb0'"


	nop

	movlw	0x06	; all pins digital
	movwf	ADCON1

	CLRF	TRISC
	CLRF	TRISE
	SETF	TRISB
	bcf	INTCON2,7	; set weak pullups on portB
	

	CLRF	LATB
	CLRF	LATE
	bsf	MR

	bsf	Ds	    ; Ds = 1
	bsf	SCK	    ; clock shift register
	bcf	SCK	;RRR
   .assert "portb == 0, '***FAILED 18f452 ttl595 output without clock'"
	nop
	bsf	RCK	    ; clock shift register to output latch
	bcf	RCK
   .assert "portb == 1, '***FAILED 18f452 ttl595 output with 1 shift'"
	nop
	bsf	OE	;RRR
	bsf	SCK		; second SR clock;
	bcf	RCK
	bsf	RCK		; second load output latch
	bcf	OE	;RRR
   .assert "portb == 3, '***FAILED 18f452 ttl595 output with 2 shift'"
	nop
	bsf	OE		;OE high to disable output
   .assert "portb == 0xff, '***FAILED 18f452 ttl595 output with OE high'"
	nop
	bcf	OE		;OE low to enable output
   .assert "portb == 3, '***FAILED 18f452 ttl595 output after OE high'"
	nop
	; clear shift register
	bcf	MR
   .assert "portb == 3, '***FAILED 18f452 ttl595 output after MR low'"
	nop
	bcf	RCK		; load output from shift register
	bsf	RCK
	nop
   .assert "portb == 0, '***FAILED 18f452 ttl595 output after MR low and load'"
	nop
	
	call	test_qs

done:
  .assert  "'*** PASSED TTL-595 test'"
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "'*** FAILED TTL-595 test'"
        bra     done

test_qs:
	movlw	8
	movwf   temp
	bcf	SCK
	bcf	RCK	
	bsf	MR
qs_loop:
	bsf	SCK
	bcf	SCK
	bsf	RCK
	bcf	RCK
	decfsz   temp,F
	goto	qs_loop
    .assert "(porta & 1) == 1,'*** FAILED TTL-595 Qs'"
	nop
    .assert "portb == 0xff, '*** FAILED TTL-595 Qn all bits'"
	nop
	bcf	MR	; clear shift register, latch unchanged
	nop
	nop
    .assert "(porta & 1) == 0,'*** FAILED TTL-595 Qs cleared on RESET'"
	nop
    .assert "portb == 0xff, '*** FAILED TTL-595 Qn all latch bits unchanged on RESET'"
	nop
	
	return

 end
