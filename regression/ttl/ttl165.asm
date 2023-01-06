        ;; ttl165.asm
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
   .sim "module load TTL165 U1"

   .sim "node nd0 nd1 nd2 nd3 nd4 nd5 nd6 nd7"

   .sim "attach nd0 portb0 U1.D0"
   .sim "attach nd1 portb1 U1.D1"
   .sim "attach nd2 portb2 U1.D2"
   .sim "attach nd3 portb3 U1.D3"
   .sim "attach nd4 portb4 U1.D4"
   .sim "attach nd5 portb5 U1.D5"
   .sim "attach nd6 portb6 U1.D6"
   .sim "attach nd7 portb7 U1.D7"

   .sim "node nClk nE nPL nDs nQ nQb"
   .sim "attach nClk porte0 U1.CP"
   .sim "attach nE   porte1 U1.CE"
   .sim "attach nPL  porte2 U1.PL"
   .sim "attach nQ   portc0 U1.Q7"
   .sim "attach nQb  portc1 U1.nQ7"
   .sim "attach nDs  portc2 U1.Ds"

   .sim "p18f452.xpos = 100.00"
   .sim "p18f452.ypos = 100.00"
   .sim "U1.xpos = 300.00"
   .sim "U1.ypos = 200.00"

	nop

	clrf    TRISB
	clrf    TRISE
        movlw   3
	movwf   TRISC

	clrf    LATB
	clrf    LATC
	clrf    LATE

	bsf     LATE,0
        bsf     LATE,2      ; Latch data into the 165

	comf    LATB,f
   .assert "portc == 2"

	bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit

   .assert "portc == 2"

	bcf	LATE,0
	bsf	LATE,0

   .assert "portc == 2"
	nop

        movlw   0xAA
        movwf   LATB
   .assert "portc == 2"
        nop
        bcf     LATE,2      ; Reload the shift register
   .assert "portc == 1"
        nop
        movlw   0xA5
        movwf   LATB
        nop
        bsf     LATE,2
   .assert "portc == 1"     ; SR=0xA5
        nop
        bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit
   .assert "portc == 2"     ; SR=0x4A
        nop
        bsf     LATC,2      ; Set the Ds pin
        bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit
   .assert "portc == 5"     ; SR=0x95
        nop
        bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit
   .assert "portc == 6"     ; SR=0x2B
        nop
        bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit
        nop
   .assert "portc == 6"     ; SR=0x57
        bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit
        nop
   .assert "portc == 5"     ; SR=0xAF
        bcf     LATC,2      ; Clear the Ds pin
        bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit
   .assert "portc == 2"     ; SR=0x5E
        nop
        bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit
   .assert "portc == 1"     ; SR=0xBC
        nop
        bcf     LATE,0
	bsf     LATE,0      ; Shift the data one bit
   .assert "portc == 2"     ; SR=0x78
        nop
        bsf     LATE,1      ; Disable the clock
        bcf     LATE,0
	bsf     LATE,0      ; No shift 
   .assert "portc == 2"     ; SR=0x78
        nop
        
done:
  .assert  "'*** PASSED TTL-165 test'"
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "'*** FAILED TTL-165 test'"
        bra     done

 end
