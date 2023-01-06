        ;; ttl377.asm
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
   .sim "module load ttl377 U1"

   .sim "node nb0 nb1 nb2 nb3 nb4 nb5 nb6 nb7"
   .sim "node nc0 nc1 nc2 nc3 nc4 nc5 nc6 nc7"

   .sim "attach nb0 portb0 U1.D0"
   .sim "attach nb1 portb1 U1.D1"
   .sim "attach nb2 portb2 U1.D2"
   .sim "attach nb3 portb3 U1.D3"
   .sim "attach nb4 portb4 U1.D4"
   .sim "attach nb5 portb5 U1.D5"
   .sim "attach nb6 portb6 U1.D6"
   .sim "attach nb7 portb7 U1.D7"

   .sim "attach nc0 portc0 U1.Q0"
   .sim "attach nc1 portc1 U1.Q1"
   .sim "attach nc2 portc2 U1.Q2"
   .sim "attach nc3 portc3 U1.Q3"
   .sim "attach nc4 portc4 U1.Q4"
   .sim "attach nc5 portc5 U1.Q5"
   .sim "attach nc6 portc6 U1.Q6"
   .sim "attach nc7 portc7 U1.Q7"

   .sim "node nClk nE"
   .sim "attach nClk porte0 U1.CP"
   .sim "attach nE porte1 U1.E"

   .sim "p18f452.xpos = 24.00000000000000"
   .sim "p18f452.ypos = 24.00000000000000"
   .sim "U1.xpos = 156.0000000000000"
   .sim "U1.ypos = 24.00000000000000"

	nop

	CLRF	TRISB
	CLRF	TRISE
	SETF	TRISC

	CLRF	LATB
	CLRF	LATE

	BSF	LATE,0      ;Clock data into the 377

	COMF	LATB
	BCF	LATE,0
	BSF	LATE,0

   .assert "latb == portc"
	COMF	LATB
	BCF	LATE,0
	BSF	LATE,0

   .assert "latb == portc"
	nop
done:
  .assert  "'*** PASSED TTL-377 test'"
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "'*** FAILED TTL-377 test'"
        bra     done

 end
