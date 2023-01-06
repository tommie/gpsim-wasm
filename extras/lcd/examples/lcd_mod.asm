        list    p=16C64,t=ON,c=132,n=80
        title   "lcd module test"
        radix   dec
;********************************************************************************
        include "p16c64.inc"
	include <coff.inc>
  __CONFIG _CP_OFF & _PWRTE_ON & _WDT_OFF& _HS_OSC



;LCD Module DDRAM Address is formed by adding the row and column EQU
SCR_ROW0                EQU     0x00    ;Row 0 starts at LCD DDRAM Address 0
SCR_ROW1                EQU     0x40    ;Row 1     "            "          64
SCR_ROW2                EQU     0x14    ;Row 2     "            "          20
SCR_ROW3                EQU     0x54    ;Row 3     "            "          84

SCR_COL0                EQU     0x00

START_OF_RAM_LO equ  0x20
END_OF_RAM_LO   equ  0x7f
START_OF_RAM_HI equ  0xa0
END_OF_RAM_HI   equ  0xbf

  cblock  START_OF_RAM_LO
        test
        index1,index2
        max1,max2
        loop_count_lo,loop_count_hi
        N_4_lo,N_4_hi

        buffer0,buffer1,buffer2,buffer3
        buffer4,buffer5,buffer6,buffer7
        LCD_CHAR      ;A Temporary register.
        LCD_CTRL      ;YATR
        LCD_init_loop
        temp          ;Temporary reg
        tmp           ;Temporary reg
        field_flag

  endc

        include "lcd.inc"
        include "screen.inc"

RESET_VECTOR  CODE    0x000              ; processor reset vector
	goto	main

INT_VECTOR   CODE    0x004               ; interrupt vector location
	nop
;***********************************************************************
main


        BCF     STATUS,RP0      ;Point to BANK 0
        CLRF    FSR             ;Clear File Select Register
        CALL    Clear_Regs      ;Clear General Purpose Registers 20-0x7f, A0-C0

        BSF     STATUS,RP0      ;Point to BANK 1
        MOVLW   (1 << T0SE) | (1 << PSA)
        MOVWF   OPTION_REG & 0x7f

        MOVLW   0x1             ;Make all of PORTA an input
        MOVWF   TRISA ^ 0x80
        MOVLW   0
        MOVWF   TRISB ^ 0x80
        MOVLW   0
        MOVWF   TRISC ^ 0x80
        MOVLW   0
        MOVWF   TRISD ^ 0x80
        MOVLW   0
        MOVWF   TRISE ^ 0x80

        BCF     STATUS,RP0      ;Point to BANK 0

        CALL    LCD_INITIALIZE

	movlw	0
	call	createChar
	movlw	1
	call	createChar
	movlw	2
	call	createChar
t1
;        CALL    LCD_CLEAR_SCREEN

	movlw	1
	call	LCD_WRITE_CMD

	;; write a string on row 0

	movlw	LCD_CMD_SET_DDRAM | SCR_ROW0 | SCR_COL0
	call	LCD_WRITE_CMD

	movlw	0
	call	write_string

	;; write another string, but to row 1
	movlw	LCD_CMD_SET_DDRAM | SCR_ROW1 | SCR_COL0
	call	LCD_WRITE_CMD

	movlw	1
	call	write_string

	;; write another string, but to row 2
	movlw	LCD_CMD_SET_DDRAM | SCR_ROW2 | SCR_COL0
	call	LCD_WRITE_CMD

	movlw	2
	call	write_string

	;; write another string, but to row 3
	movlw	LCD_CMD_SET_DDRAM | SCR_ROW3 | SCR_COL0
	call	LCD_WRITE_CMD

	movlw	3
	call	write_string

  .assert "\"Check if display OK\""
	nop
	call LCD_DELAY
	call LCD_DELAY
	call LCD_DELAY
	goto	t1


;***********************************************************************
;    Clear File Registers 20-0x7fh, A0-C0 in PIC's memory


Clear_Regs

        MOVLW   START_OF_RAM_LO ;First regs to clear in Bank 0
        MOVWF   FSR
cr1     CLRF    INDF            ;Clear reg
        INCF    FSR,F           ;point to next reg to clear
        MOVLW   END_OF_RAM_LO+1 ;
        SUBWF   FSR,W
        SKPC
         goto   cr1

        MOVLW   START_OF_RAM_HI ;First regs to clear in Bank 1
        MOVWF   FSR
cr2     CLRF    INDF            ;Clear reg
        INCF    FSR, F          ;point to next reg to clear
        MOVLW   END_OF_RAM_HI+1 ;
        SUBWF   FSR,W
        SKPC
          goto  cr2

        RETURN

        include "lcd.asm"
        include "screen.asm"
	include "icons.inc"

        END
