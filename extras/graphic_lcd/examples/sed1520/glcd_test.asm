
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


        include "processor.inc"
        include <coff.inc>              ; Grab some useful macros

  CONFIG  OSC = HSPLL, OSCS = ON,   WDT = ON, LVP = OFF 
  CONFIG  CP0 = ON, CP1 = ON, CPB = ON

	include "sed1520.inc"
	include "lcd100X32.inc"
	include "portdef.inc"

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;  Variable declarations
;----------------------------------------------------------------------
GPR_DATA                UDATA_ACS
temp            RES     1


temp0           RES     1
temp1           RES     1
temp2           RES     1
temp3           RES     1
  GLOBAL temp0, temp1, temp2, temp3


Tx RES 1
Ty RES 1

DISPLAY_DATA  UDATA 400
DisplayBuffer	RES 100*4
  GLOBAL DisplayBuffer

;------------------------------------------------------------------------
; Code labels

  GLOBAL done


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        bra   start                      ; go to beginning of program

;----------------------------------------------------------------------
;   ******************* INTERRUPT VECTOR LOCATION  ******************
;----------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location
	RETFIE 1

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

   ;;
   ;; Define the simulation environment.
   ;;

   .sim "module library libgpsim_extras"
   .sim "module library libgpsim_modules"

   .sim "module load LCD100X32 LCD"
   .sim "module load pullup R1"
   .sim "module load pullup R2"

   .sim "node nE1"
   .sim "node nE2"
   .sim "node nRW"
   .sim "node nA0"

   .sim "attach nE1 porte0 LCD.e1 R1.pin"
   .sim "attach nE2 porte1 LCD.e2 R2.pin"
   .sim "attach nRW porte2 LCD.rw"
   .sim "attach nA0 porta1 LCD.a0"

   .sim "node nd0 nd1 nd2 nd3 nd4 nd5 nd6 nd7"

   .sim "attach nd0 portb0 LCD.d0"
   .sim "attach nd1 portb1 LCD.d1"
   .sim "attach nd2 portb2 LCD.d2"
   .sim "attach nd3 portb3 LCD.d3"
   .sim "attach nd4 portb4 LCD.d4"
   .sim "attach nd5 portb5 LCD.d5"
   .sim "attach nd6 portb6 LCD.d6"
   .sim "attach nd7 portb7 LCD.d7"

   ;; Now position the modules in the BreadBoard viewer

   .sim "LCD.xpos=192.0"
   .sim "LCD.ypos=24.0"
   .sim "R1.xpos=216.0"
   .sim "R1.ypos=264.0"
   .sim "R2.xpos=312.0"
   .sim "R2.ypos=264.0"

   ;; Initialize the SED1520 LCD graphics controller.

	RCALL	SED1520_Init

loop:
	CLRWDT

   ; Clear Screen Test

	RCALL	LCD_ClearScreen
	RCALL	LCD_RefreshDisplay

   ; Line Test

	movlw	31
	movwf	Ty
	movlw	99
	movwf	Tx
LL
	clrwdt
	movf	Tx,W
	MOVWF	LCD_x2
	movf	Ty,W
	MOVWF	LCD_y2
        MOVLW	0
	MOVWF	PixelX
	MOVLW	1
	MOVWF	PixelY

	RCALL	LCD_Line
;	RCALL	LCD_RefreshDisplay

	decf	Tx,F
	BNN	LL

	RCALL	LCD_RefreshDisplay

        MOVLW	0
	MOVWF	PixelX
	MOVWF	PixelY
	MOVLW	99
	MOVWF	LCD_x2
	MOVLW	15
	MOVWF	LCD_y2

	RCALL	LCD_Line
	RCALL	LCD_RefreshDisplay

	MOVLW	20
	MOVWF	PixelX
	CLRF	PixelY

    .assert "\"Press run for next test\""
	nop

	RCALL	LCD_ClearScreen

	MOVLW	1
	RCALL	LCD_putBitMap
	RCALL	LCD_RefreshDisplay

    .assert "\"Press run for next test\""
	nop
	RCALL	LCD_ClearScreen

	CLRF	PixelX
	MOVLW	0
	RCALL	LCD_putBitMap
	RCALL	LCD_RefreshDisplay



	MOVLW	32
	MOVWF	PixelX

	MOVLW	2
	RCALL	LCD_putBitMap
	RCALL	LCD_RefreshDisplay

    .assert "\"Press run to repeat test\""
	nop
	bra	loop

	nop

done:

  .assert  "\"*** PASSED LCD test\""
	goto	$

FAILED:
  .assert  "\"*** FAILED LCD test\""
	goto	$



  end
