;	
;  SED1520 Driver for the PIC 18F family.
;
;  Copyright (C) 2005 T. Scott Dattalo
;
; This driver is designed to test gpsim. However, it is also usable 
; as a stand alone driver. You're free to use this driver as you
; wish as long as the copyright is retained. 
; 
;  
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;
; The SED1520 driver supports graphic LCD displays that are based on the
; S-MOS Systems SED1520 LCD graphics controller. The SED1520 is a
; relatively primitive LCD graphics controller. It contains 320 bytes
; of RAM which can be mapped to 2560 on/off pixels. It doesn't have
; hardware support for fonts or drawing primitives. 
;
; The primary target of this driver is a Varitronix 100X32 LCD display
; which contains a pair of SED1520's. The upper half of the display
; is supported by one SED1520 and the lower half by the other. Only a
; portion of the SED1520 memory is used, but this driver doesn't take
; advantage of the extra memory
;
; 
;  !!! NOTE: The driver assumes all variables reside in Bank 0!. !!!
;

        list    p=18f452,t=ON,c=132,n=80
        radix   dec



  #include <p18f452.inc>

; The defines in portdef.inc allow the LCD port definitions to be customized.

  #include "portdef.inc"
  #include "lcd100X32.inc"

; Functions:

	GLOBAL	SED1520_Init
	GLOBAL  SED1520_WriteCmd
	GLOBAL  LCD_RefreshDisplay	

  EXTERN temp0, temp1, temp2, temp3
  EXTERN DisplayBuffer



;========================================================================
;
;   SED1520 Driver
;
;
;   RAM Definitions

        UDATA_ACS
    ;--------------------
    ; Mode 
    ;--------------------

SED1520_Mode	RES	1
        GLOBAL  SED1520_Mode
; xxxx_xxCM
;        |+--- Mode 0=Command 1=Data
;        +---- Chip 0=1st chip on Display, 1=2nd chip
bSED1520_MODE_CHIP	EQU 1<<1


;------------------------------------------------------------------------
; Macros

; SetMode 
;  Selects between Command and Data mode 
mSED1520_SetMode  macro
	BCF	LATA, RA1
	BTFSC	SED1520_Mode,bSED1520_MODE_CMD
	 BSF	LATA, RA1
  endm

mSED1520_SelectChip  macro
	BTFSS	SED1520_Mode,bSED1520_MODE_CHIP
 	 BCF	LATE, RE0
;	BTFSC	SED1520_Mode,bSED1520_MODE_CHIP
;	 BCF	LATE, RE1
  endm

mSED1520_deSelectChip  macro
  	BTFSS	SED1520_Mode,bSED1520_MODE_CHIP
 	 BSF	LATE, RE0
 	BTFSC	SED1520_Mode,bSED1520_MODE_CHIP
	 BSF	LATE, RE1
  endm



SETA0   macro
        BSF     LCDA0_LAT,LCDA0_BIT
  endm
CLRA0   macro
        BCF     LCDA0_LAT,LCDA0_BIT
  endm

;------------------------------------------------------------------------
; SED1520 commands
;
; The SED1520 Commands are written to address 1 of the chip.
; The data written for the command includes the command type and optional
; parameters. The data bus is only 8-bits wide. The upper bits in general
; describe the command while the lower are for data

SED1520_DisplayOn               EQU     0xaf
SED1520_DisplayOff              EQU     0xae
SED1520_DisplayStartLine        EQU     0xc0
SED1520_SETPAGEADDRESS          EQU     0xb8
SED1520_SetColumnAddress        EQU     0x00
SED1520_SetDuty                 EQU     0xa8

;------------------------------------------------------------------------
;


SED1520_CODE	CODE


SED1520_Init:

	BCF  TRISA, RA1
	CLRF	SED1520_TRIS

  if LCDE1_TRIS == LCDE2_TRIS && LCDE1_TRIS == LCDRW_TRIS
	CLRF	TRISE
  else
	BCF	LCDE1_TRIS, LCDE1_BIT
	BCF	LCDE2_TRIS, LCDE2_BIT
	BCF	LCDRW_TRIS, LCDRW_BIT
  endif

    ; Select the first chip and set the mode to command

	CLRF    SED1520_Mode
	RCALL	LSED1520_Init

	BSF	SED1520_Mode, bSED1520_MODE_CHIP

    ; Fall through to initialize the second chip

LSED1520_Init:
	MOVLW	SED1520_DisplayStartLine
	rcall	SED1520_WriteCmd

	MOVLW   SED1520_SETPAGEADDRESS
	rcall	SED1520_WriteCmd

	MOVLW   SED1520_SetColumnAddress
	rcall	SED1520_WriteCmd

	MOVLW   SED1520_SetDuty|1
	rcall	SED1520_WriteCmd

	MOVLW   SED1520_DisplayOn
	bra	SED1520_WriteCmd

;------------------------------------------------------------
; Write either a command or data to the SED1520
;
; Inputs: WREG - byte to be written
; Outputs:
; Mem used:  W
;

SED1520_WriteCmd  ;<--- Entry point to write a command.
	BCF	LATA, RA1

SED1520_Write:    ;<--- Entry point to write data.


	BCF	LATE, RE2	;SED1520 R/W' = write

	rcall d2

	MOVWF	SED1520_LAT
	rcall d2
	BTFSS	SED1520_Mode,bSED1520_MODE_CHIP
 	 BSF	LATE, RE0
	BTFSC	SED1520_Mode,bSED1520_MODE_CHIP
 	 BSF	LATE, RE1

	rcall d2

 	BCF	LATE, RE0
 	BCF	LATE, RE1

	BSF	LATA, RA1

	return
d2	rcall d3
d3	return



;------------------------------------------------------------------------
;LCD_RefreshDisplay - copy the RAM buffered display to the physical display
;
; Inputs:  None
; Outputs: None
; MemUsed: temp0

LCD_RefreshDisplay:	
	LFSR	0, DisplayBuffer

	RCALL	SED1520_Init

	CLRF    SED1520_Mode

	MOVLW	LCD_nROWS/8
	MOVWF	temp0

SED_ref1:
	BCF	SED1520_Mode,bSED1520_MODE_CHIP
	RCALL	SED_ref2
	BSF	SED1520_Mode,bSED1520_MODE_CHIP
	RCALL	SED_ref2

	DECFSZ	temp0,F
	 bra	SED_ref1
	return

SED_ref2:	
	MOVLW   SED1520_SetColumnAddress
	rcall	SED1520_WriteCmd

	MOVF	temp0,W
	SUBLW	LCD_nROWS/8

	ANDLW   3
	IORLW	SED1520_SETPAGEADDRESS
	rcall	SED1520_WriteCmd

	MOVLW	50
	MOVWF	temp1

SED_ref3:
	MOVF	POSTINC0,W
	rcall	SED1520_Write

	DECFSZ	temp1,F
	 bra    SED_ref3

	return

;------------------------------------------------------------

  end
