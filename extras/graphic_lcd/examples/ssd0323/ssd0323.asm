;	
;  SSD0323 Driver for the PIC 18F family.
;
;  Copyright (C) 2007 T. Scott Dattalo
;
; This driver is designed to test gpsim. However, it is also usable 
; as a stand alone driver. You're free to use this driver as you
; wish as long as the copyright is retained. 
; 
;  
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;
; The SSD0323 driver supports graphic LCD displays that are based on the
; Solomon Systech SSD0323 OLED graphics controller. The SSD0323 is an
; 128X80, 4-bit gray scale dot matrix controller.
;
;
; 
;  !!! NOTE: The driver assumes all variables reside in Bank 0!. !!!
;

        list    p=18f452,t=ON,c=132,n=80
        radix   dec



  #include <p18f452.inc>

; The defines in portdef.inc allow the LCD port definitions to be customized.

  #include "portdef.inc"
  #include "osram128x64.inc"

; Functions:

	GLOBAL	SSD0323_Init
	GLOBAL  SSD0323_WriteCmd
	GLOBAL  SSD0323_Write
	GLOBAL  LCD_RefreshDisplay
	GLOBAL  LCD_RefreshEntireDisplay
        GLOBAL  CommandLoop
        GLOBAL CommandWaitForSPI

  EXTERN temp0, temp1, temp2, temp3
  EXTERN DisplayBuffer



;========================================================================
;
;   SSD0323 Driver
;
;
;   RAM Definitions

        UDATA_ACS
    ;--------------------
    ; Mode 
    ;--------------------

SSD0323_Mode	RES	1
        GLOBAL  SSD0323_Mode
; xxxx_xxxM
;         +--- Mode 0=Command 1=Data


;------------------------------------------------------------------------
; Macros

; SetMode 
;  Selects between Command and Data mode 
mSSD0323_SetMode  macro
	BCF	LATA, RA1
	BTFSC	SSD0323_Mode,bSSD0323_MODE_CMD
	 BSF	LATA, RA1
  endm



SETA0   macro
        BSF     LCDA0_LAT,LCDA0_BIT
  endm
CLRA0   macro
        BCF     LCDA0_LAT,LCDA0_BIT
  endm

;------------------------------------------------------------------------
; SSD0323 commands
;
; The SSD0323 Commands are written to address 1 of the chip.
; The data written for the command includes the command type and optional
; parameters. The data bus is only 8-bits wide. The upper bits in general
; describe the command while the lower are for data

SSD0323_DisplayOn               EQU     0xaf
SSD0323_DisplayOff              EQU     0xae
SSD0323_DisplayStartLine        EQU     0xc0
SSD0323_SETPAGEADDRESS          EQU     0xb8
SSD0323_SetColumnAddress        EQU     0x00
SSD0323_SetDuty                 EQU     0xa8

;------------------------------------------------------------------------
;


SSD0323_CODE	CODE


SSD0323_Init:

        mInitLCD_PINS

   ; de-select the chip
        mSetLCDCS

   ; Bring the chip out of reset:
        RCALL   delay
        mSetLCDRES
        RCALL   delay

   ; Select the chip
        mClrLCDCS


CmdSetColumnAddress     EQU 0x15
CmdGraphicAccleration   EQU 0x23
CmdDrawRectangle        EQU 0x24
CmdCopy                 EQU 0x25
CmdHorizontalScroll     EQU 0x26
CmdStopMoving           EQU 0x2E
CmdStartMoving          EQU 0x2F

CmdSetRowAddress        EQU 0x75
CmdSetContrast          EQU 0x81
CmdSetQuarterCurrent    EQU 0x84
CmdSetHalfCurrent       EQU 0x85
CmdSetFullCurrent       EQU 0x86

CmdSetRemap             EQU 0xA0
CmdSetDisplayStartLine  EQU 0xA1
CmdSetDisplayOffset     EQU 0xA2
CmdSetNormalDisplay     EQU 0xA4
CmdSetAllOn             EQU 0xA5
CmdSetAllOff            EQU 0xA6
CmdSetInverse           EQU 0xA7
CmdSetMultiplexRatio    EQU 0xA8
CmdSetMasterCfg         EQU 0xAD
CmdSetDisplayOff        EQU 0xAE
CmdSetDisplayOn         EQU 0xAF

CmdSetPreChargeCompensationEnable EQU 0xB0
CmdSetPhaseLength       EQU 0xB1
CmdSetRowPeriod         EQU 0xB2
CmdSetClockDivide       EQU 0xB3
CmdSetPreChargeCompensationLevel EQU 0xB4
CmdSetGrayScaleTable    EQU 0xB8

CmdSetPreChargeVoltage  EQU 0xBC
CmdSetVCOMH             EQU 0xBE
CmdSetVSL               EQU 0xBF
CmdNop                  EQU 0xE3

	MOVLW	LOW(CommandTable)
	MOVWF	TBLPTRL
	MOVLW	HIGH(CommandTable)
	MOVWF	TBLPTRH

        MOVLW   CommandTableEnd-CommandTable
        MOVWF   temp0

CommandLoop:
	TBLRD   *+
	MOVF	TABLAT,W
	rcall	SSD0323_WriteCmd
        decfsz  temp0,F
         bra    CommandLoop
  if InterfaceMode == ModeSPI

        ; In SPI mode, we need to wait for the command completes
        ; before returning. The reason is that we may send data 
        ; just after this command. Sending data changes the DC
        ; bit which in turn will screw up the command transfer.
CommandWaitForSPI:
        MOVF    SSPBUF,W
CommandSPIBusy:
        BTFSS   SSPSTAT,BF
         bra    CommandSPIBusy
  endif
        return

CommandTable:
        db CmdSetColumnAddress, 0x00, 0x3f,    CmdSetRowAddress, 0x00, 0x3f
        db CmdSetContrast, 0x6d
        db CmdSetFullCurrent,   CmdSetRemap, 0x41, CmdSetDisplayStartLine
        db 0x00, CmdSetDisplayOffset, 0x44, CmdSetNormalDisplay
        db CmdSetMultiplexRatio, 0x3f
        db CmdSetPhaseLength, 0x28
        db CmdSetPreChargeCompensationLevel, 0x07
        db CmdSetRowPeriod, 0x46
        db CmdSetClockDivide, 0x91
        db CmdSetVSL, 0x0d
        db CmdSetVCOMH, 0x02
        db CmdSetPreChargeVoltage, 0x10
        db CmdSetGrayScaleTable, 0x01,0x11,0x22, 0x32, 0x43,0x54,0x65 
        db 0x76, CmdSetMasterCfg, 02, CmdSetDisplayOn
CommandTableEnd:


CommandSetCursorPosition:
        db CmdSetColumnAddress, 0x00, 0x3f,    CmdSetRowAddress, 0x00, 0x3f
CommandSetCursorPositionEnd:

;------------------------------------------------------------
; Write either a command or data to the SSD0323
;
; Inputs: WREG - byte to be written
; Outputs:
; Mem used:  W
;

  if InterfaceMode == Mode8080

SSD0323_WriteCmd  ;<--- Entry point to write a command.
	BCF	LCDDC_LAT, LCDDC_BIT

SSD0323_Write:    ;<--- Entry point to write data.

	BCF	LCDRW_LAT, LCDRW_BIT    ;SSD0323 WR#

	MOVWF	SSD0323_LAT

	BSF	LCDRW_LAT, LCDRW_BIT    ;Rising edge writes the data
	BSF	LCDDC_LAT, LCDDC_BIT

        return
  endif

  if InterfaceMode == Mode6800

SSD0323_WriteCmd  ;<--- Entry point to write a command.
	BCF	LCDDC_LAT, LCDDC_BIT

SSD0323_Write:    ;<--- Entry point to write data.

	BCF	LCDRW_LAT, LCDRW_BIT    ;SSD0323 R/W line is low for writes

	MOVWF	SSD0323_LAT

	BCF	LCDE_LAT, LCDE_BIT      ;Falling edge of E latches the data.
        BRA     $+2
	BSF     LCDE_LAT, LCDE_BIT      ;Turn off the enable
	BSF	LCDDC_LAT, LCDDC_BIT

	return
  endif

  if InterfaceMode == ModeSPI

SSD0323_WriteCmd  ;<--- Entry point to write a command.
	BCF	LCDDC_LAT, LCDDC_BIT
        BRA     SSD0323_WriteSPI
SSD0323_Write:    ;<--- Entry point to write data.

	BSF	LCDDC_LAT, LCDDC_BIT

SSD0323_WriteSPI:

        MOVWF   SSPBUF
        btfss   SSPCON1, WCOL
         return

        BCF     SSPCON1, WCOL
        bra     SSD0323_WriteSPI

	return
  endif


d2	rcall d3
d3	return

delay:  bra $+2
        bra $+2
        bra $+2
        decfsz WREG,F
         bra delay
        return
;------------------------------------------------------------------------
;SSD_SetColumnRange - set the start and columns in the SSD0323
; In addition, the width of the virtual display (in the PIC's ram)
; is also initialized.
; 
; Inputs: W - start column
;         temp0 - end column
; Outputs: DisplaySizeX 
; MemUsed: None
;
SSD_SetColumnRange:
  GLOBAL SSD_SetColumnRange
        MOVWF   temp1
        SUBWF   temp0,W
        ADDLW   1
        MOVWF   DisplaySizeX

        CLRC
        RRCF    temp0,F
        CLRC
        RRCF    temp1,F

        MOVLW   CmdSetColumnAddress
SSD_SetRowCol:
        RCALL   SSD0323_WriteCmd

        MOVF    temp1,W
        RCALL   SSD0323_WriteCmd

        MOVF    temp0,W
        bra     SSD0323_WriteCmd
        

SSD_SetRowRange:
  GLOBAL SSD_SetRowRange
        MOVWF   temp1
        SUBWF   temp0,W
        ADDLW   1
        MOVWF   DisplaySizeY

        MOVLW   CmdSetRowAddress

        bra     SSD_SetRowCol

;------------------------------------------------------------------------
;LCD_RefreshDisplay - copy the RAM buffered display to the physical display
;
; Inputs:  None
; Outputs: None
; MemUsed: temp0
;
; The display buffer is organized differently than the physical display
; Here is a graphical mapping:
;
;                     columns
;          0  1  2  3  4  5  6  7  8
;    -----------------------------------
;     0 | a0 b0 c0 d0 e0 f0 g0 h0 i0
;     1 | a1 b1 c1 d1 e1 f1 g1 h1 i1
;     2 | a2 b2 c2 d2 e2 f2 g2 h2 i2
;  R  3 | a3 b3 c3 d3 e3 f3 g3 h3 i3
;  O  4 | a4 b4 c4 d4 e4 f4 g4 h4 i4
;  W  5 | a5 b5 c5 d5 e5 f5 g5 h5 i5
;  S  6 | a6 b6 c6 d6 e6 f6 g6 h6 i6
;     7 | a7 b7 c7 d7 e7 f7 g7 h7 i7
;     8 | A0 B0 C0 D0 E0 F0 G0 H0 I0
;     9 | A1 B1 C1 D1 E1 F1 G1 H1 I1
;
; The sequence of bytes in the PIC memory is:
;  a, b, c, and so on for the first 8 rows of pixels. The next 8 rows
; begin with the A, B, C and so on bytes.
;
; 

LCD_RefreshEntireDisplay:
	MOVLW	LOW(CommandSetCursorPosition)
	MOVWF	TBLPTRL
	MOVLW	HIGH(CommandSetCursorPosition)
	MOVWF	TBLPTRH

        MOVLW   CommandSetCursorPositionEnd-CommandSetCursorPosition
        MOVWF   temp0

        call    CommandLoop

LCD_RefreshDisplay:	
	LFSR	0, DisplayBuffer

	MOVLW	0       ; row counter
	MOVWF	temp0

        MOVLW   1
        MOVWF   temp2   ; bit mask

SSD_ref1:	

     ;row counter * number of columns
        MOVF    temp0,W
        MULWF   DisplaySizeX
     ;   MULLW   LCD_nCOLS

	LFSR	0, DisplayBuffer	;Get a pointer to the display
	MOVF	PRODL,W
	ADDWF	FSR0L,F	
	MOVF	PRODH,W
	ADDWFC	FSR0H,F

        ;MOVLW   LCD_nCOLS/2     ; # of columns - 1 byte covers 2 columns
        RRCF    DisplaySizeX,W
        ANDLW   0x7f
	MOVWF	temp1

SSD_ref2:
        movf    temp2,W         ; Check a single pixel in the display
        andwf   POSTINC0,W      ; buffer

        movlw   0               ;Assume that the pixel is off
        skpz                    ;
         movlw  0xf0            ;Pixel was on- assign it the maximum color
        movwf   temp3           ;We now have the low order pixel.

        movf    temp2,W         ;Check the pixel in the next column
        andwf   POSTINC0,W

        movlw   0               ;Again, assume it's zero
        skpz                    ;
         movlw  0x0f            ;Pixel is on
        iorwf   temp3,W         ;We now have the high order pixel

	rcall	SSD0323_Write   ;Write both pixels

	DECFSZ	temp1,F         ;Have we gone through all of the columns?
	 bra    SSD_ref2        ;... nope

       ; next row
        rlncf   temp2,F         ;Rotate the pixel mask
        btfss   temp2,0         ;Did we wrap around?
         bra    SSD_ref1        ;Nope,

        incf    temp0,F         ;Display buffer row counter.

        SWAPF   temp0,W
        RRNCF   WREG,W
        ADDLW 1
        CPFSLT  DisplaySizeY
         bra    SSD_ref1

	return

;------------------------------------------------------------

  end
