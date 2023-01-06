;
; lcd100X32.asm - Driver for 100X32 pixel graphics LCD based on 
;                 SED1520 controllers
;
;
; From a high level point of view, this driver supports basic graphing 
; and text functions (that the SED1320 lacks). The graphing functions
; include points and lines, while the text function allows 5X7 fonts
; to be written to the display.
;
; The driver is relatively inefficient with regards to memory usage. The
; entire 100X32 LCD display is buffered in the PIC's memory. The purpose
; of this is to simplify the mapping of the logical coordinates of the LCD
; display to the RAM embedded in the SED1520's.
;
;  !!! NOTE: The driver assumes all variables reside in Bank 0!. !!!

 include "processor.inc"
 include "sed1520.inc"


;  GLOBAL DisplayBuffer
  EXTERN DisplayBuffer
  GLOBAL LCD_ClearScreen
  GLOBAL SED1520_Test
  GLOBAL LCD_Line
  GLOBAL LCD_putBitMap

  EXTERN temp0, temp1, temp2, temp3

        UDATA_ACS


LCD_WritePtrLo RES 1
LCD_WritePtrHi RES 1
PixelX	RES 1
PixelY	RES 1
TempX	RES 1
TempY   RES 1

  global TempY, PixelX, PixelY


;----------------------------------------
; Private RAM area for temporary variables.

TempRam   udata_ovr	0x78
LCD_Temporaries  RES 6


TempRam   udata_ovr	0x78

LCD_x2	RES 1
LCD_y2	RES 1
slope	RES 1
dx	RES 1
dy	RES 1
dir     RES 1

  global LCD_x2, LCD_y2

;------------------------------------------------------------------------
; Display buffer
;
;GPR_ARRAYS	UDATA  400
;DisplayBuffer	RES 100*4

;------------------------------------------------------------------------
;

LCD_CODE	CODE


SED1520_Test:

TestBitMap:

	MOVLW	20
	MOVWF	PixelX
	CLRF	PixelY

	MOVLW	2
	RCALL	LCD_putBitMap

	CLRF	PixelX
	CLRF	PixelY
	MOVLW	93
	MOVWF	LCD_x2
	CLRF	LCD_y2

	RCALL	LCD_Line

	MOVLW	31
	MOVWF	LCD_x2
	MOVWF	LCD_y2

	RCALL	LCD_Line

	CLRF	PixelX
	CLRF	PixelY
 	RCALL	LCD_Line


	bra	LCD_RefreshDisplay



;------------------------------------------------------------------------
;LCD_ClearScreen

LCD_ClearScreen:

	LFSR	0, DisplayBuffer	;Get a pointer to the display
	movlw	100
	movwf	temp0
	movlw	4
cs1:
	CLRF	POSTINC0
	decfsz	temp0,F
	 bra	cs1
	decfsz	WREG,F
	 bra	cs1
	return



;------------------------------------------------------------------------
; LCD_putBitMap
;
; Input:  x1, y1 - Where the upper left hand corner of the bitmap will be placed
;         WREG   - Bitmap # (i.e. index into the BitMapTable array)
; Output
; MemUsed: temp0 - used as a counter
;          PRODL,PRODH
;          TBLPTRL,TBLPTRH
;
; Calls:   LCD_putByte

LCD_putBitMap:

   ; Point to BitMapTable entry that contains the pointer 
   ; to the desired bitmap.

	RLNCF	WREG,W
	ADDLW	LOW(BitMapTable)
	MOVWF	TBLPTRL
	MOVLW	HIGH(BitMapTable)
	SKPNC
	 ADDLW	1
	MOVWF	TBLPTRH

   ; Get the pointer to the bitmap

	TBLRD   *+
	MOVF	TABLAT,W
	TBLRD   *+
	MOVWF	TBLPTRL
	MOVF	TABLAT,W
	MOVWF	TBLPTRH

   ; The first byte of the bitmap is the width and the second is the height

	TBLRD   *+
	MOVF	TABLAT,W
	MOVWF	LCD_y2
	TBLRD   *+
	MOVF	TABLAT,W
	MOVWF	LCD_x2

   ; At this point, the TBLPTR points to the bitmap data. Now we need
   ; to fetch this data and copy it to the display.

	movf	PixelX,W
	movwf	temp1
	rrncf	PixelY,W
	rrncf	WREG,W
	rrncf	WREG,W
	RCALL	LCD_MoveCursor

pbm1:
	MOVF	LCD_y2,W
	MOVWF	temp0
pbm2:
	TBLRD   *+
	MOVF	TABLAT,W
	RCALL	LCD_PutByte
	DECFSZ	temp0,F
	 bra	pbm2

	MOVF	LCD_y2,W
	SUBLW	100
	ADDWF	LCD_WritePtrLo,F
	CLRW
	ADDWFC	LCD_WritePtrHi,F

	DECFSZ  LCD_x2,F
	 bra	pbm1

aret:	RETURN
;------------------------------------------------------------------------
; LCD_putc
;
; Write a single character to the LCD display buffer. The input is the ASCII
; code of the character to write. This is used as an index into the LCD font
; table. The 5 7-bit bytes that comprise this character are then written to
; the LCD display buffer. In addition, a 6th byte containing 0 is written so 
; that there will be a little space between characters. Note, the character
; will be written at the current location of the cursor. No attempt is made
; to prevent line wrapping.  
;
; Input:    WREG - ASCII value of character
; Output:   None
; Mem used: temp0  - used as a counter
;
LCD_putc:

        BTFSC	WREG,7			;The max character is 127
	 return				;leave if we're beyond this.

	ADDLW	-' '			;The first character in the table is a Space
	BNC	aret			;leave if the ascii code is smaller than this

	MULLW   5			;Each character is comprised of 5 bytes.
					;The offset into the table is the product
					;of the ASCII code and the size of each character
	CLRF	TBLPTRU			;Get a pointer to the
	MOVF	PRODL,W			;start of this character's font
	ADDLW	LOW(ASCII_5X7Table)	;
	MOVWF	TBLPTRL			;

	MOVLW	HIGH(ASCII_5X7Table)	;
	ADDWFC  PRODH,W
	MOVWF	TBLPTRH			;
					;
	MOVLW	5			; Now loop through and get all 5 bytes
	MOVWF	temp0
LCD_ASCII_L1
	TBLRD	*+			;Get one byte
	MOVF 	TABLAT,W		;
        rcall   LCD_PutByte		;Write the byte to the display buffer
	decfsz	temp0,F
	 bra    LCD_ASCII_L1

	MOVLW	0

   ;;
   ;;  !!!! Intentionally fall through to write a zero to the display
   ;; 

;------------------------------------------------------------------------
; LCD_PutByte - write a byte of data to the LCD display buffer
;
; Write the byte contained in W to the display buffer. The byte is written
; at the current cursor position. Also, the cursor position is incremented 
; after the write.
;
; INPUT: WREG
; OUTPUT:
; Mem Used: temp1
;           LCD_WritePtrLo/Hi 
;           FSR0L/H

LCD_PutByte:

	MOVWF	temp1			;Save the byte to write

	LFSR	0, DisplayBuffer	;Get a pointer to the display
	MOVF	LCD_WritePtrLo,W        ;Now point to the location
	ADDWF	FSR0L,F			;of the cursor.
	MOVF	LCD_WritePtrHi,W
	ADDWFC	FSR0H,F

	MOVFF	temp1, INDF0		;Write the byte to the display

	INFSNZ  LCD_WritePtrLo,F	;Increment the cursor position
	 INCF	LCD_WritePtrHi,F

        MOVLW   LOW(400)		;The Display buffer is only 400
        CPFSLT  LCD_WritePtrLo		;bytes. If we didn't increment
	 BTFSS	LCD_WritePtrHi,0	;beyond the end of the display
	  return			;then we're okay.

	CLRF	LCD_WritePtrHi		;oops - we've rolled over
	CLRF	LCD_WritePtrLo
	return

;------------------------------------------------------------------------
; LCD_MoveCursor - re-position the cursor. The cursor marks where the next
;          byte will be written
;
; INPUT:  WREG - row
;         temp1 - column
; OUTPUT
; MemUsed: LCD_WritePtrLo/HI
;          PRODL/H

LCD_MoveCursor:
	ANDLW   3
	MULLW	100
	MOVF	PRODL,W
	MOVWF	LCD_WritePtrLo
	MOVF	PRODH,W
	MOVWF	LCD_WritePtrHi

	MOVF	temp1,W
	ADDWF	LCD_WritePtrLo,F
	CLRW
	ADDWFC	LCD_WritePtrHi,F
	RETURN
	
;------------------------------------------------------------------------
; LCD_GetPixelPtr - convert the current (PixelX,PixelY) coordinates into
;   a pointer into the display buffer. This pointer is returned in FSR0
;   and the bit offset within the byte of the display buffer is returned 
;   is returned as bit mask in temp0.
;
;  {
;    temp0 = 1<<(PixelY & 7);
;
;    int offset = (PixelY>>3) * nLCD_COLUMNS + PixelX
;    FSR0 = &DisplayBuffer[offset];
;    return(temp0);
;  }
;
; Input:  PixelX = X location of the pixel
;         PixelY = Y location of the pixel
; Output: FSR0L:FSR0H -- will point to the byte offset in the display buffer
;         temp0 -- 2^n where n is the position within the byte.
;

LCD_GetPixelPtr:
	RRCF	PixelY,W	;Put the LSB of the current Y pixel into C.

	CLRF	temp0		;
	BSF	temp0,0		; 1<<0 -- assumes Y is even
	SKPNC			;
	 RLNCF	temp0,F		; 1<<1 -- Y is actually odd

	RRCF	WREG,W		;Put bit 1 of the current Y pixel into C
	BNC	pp1		;
	RLNCF	temp0,F		; bit 1 is set, so we need to shift the
	RLNCF	temp0,F		; mask two positions
	
pp1:	RRCF	WREG,W		;Put bit 2 of the current Y pixel into C
	SKPNC			;If it is set
	 SWAPF	temp0,F		; then shift the mask left 4 positions.

	ANDLW	3		;The lower two bits of W are the 3rd and 4th
                                ;bit of the current Y pixel. 
	MULLW	100		;Multiply by 100 (number of columns in a row)
				;to obtain the byte offset into the buffer.

	LFSR	0, DisplayBuffer	;Get a pointer to the display
	MOVF	PRODL,W
	ADDWF	FSR0L,F	
	MOVF	PRODH,W
	ADDWFC	FSR0H,F

	MOVF	PixelX,W
	ADDWF	FSR0L,F	
	MOVLW	0
	ADDWFC	FSR0H,F
ret1:	
	return

;------------------------------------------------------------------------
; LCD_Line - Draw a line 
;
; Inputs: PixelX, PixelY - start coordinates
;         LCD_x2, LCD_y2 - end coordinates
;
;  int dx = abs(x2-x1);
;  int dy = abs(y2-y1);
;
;  int bRight = 1;
;  int bUp = 1;
;
;  if (dx<0) {
;     bRight = 0;
;     dx = -dx;
;  }
;  if (dy<0) {
;     bUp = 0 ;
;     dy = -dY;
;  }
;
;  int slope = dx - dy;
;
;  do {
;
;    SetPixel(x,y);
;
;    if ( slope < 0) {
;      y++;
;      slope += dx;
;    } else {
;      x++;
;      slope -= dy;
;    }
;  } while (x!=x2 && y!=y2);
;
;
; MEM Used:	
;  dx
;  dy
;  slope
;  temp1
; Calls:
;   LCD_SetPixel
;
LCD_Line:


	MOVF	PixelX,W	;x1
	SUBWF	LCD_x2,W	;W=x2-x1
	MOVWF	dx		;dx=x2-x1
	RLCF    dir,F		;Pick up carry
	BTFSS	dir,0		;if x2<x1
         NEGF   dx		; dx = x1-x2

	MOVF	PixelY,W	;y1
	SUBWF	LCD_y2,W	;W=y2-y1
	MOVWF	dy		;dy=y2-y1
	RLCF    dir,F		;Pick up carry
	BTFSS	dir,0		;if y2<y1
         NEGF   dy		; dy = y1-y2

   ; Initialize the slope accumulator to
   ;   slope = (dx-dy)/2
	movf	dy,W
	subwf	dx,w
	rrcf	WREG,F
	xorlw	0x80
	movwf	slope


	btfsc	slope,7
	 bra	LVertical
LHorizontal:
	RCALL	LCD_SetPixel	;Turn on (PixelX,PixelY)

	movf	PixelX,W
	xorwf	LCD_x2,W
	bz	aret
	tstfsz  slope
	 btfsc	slope,7
	  rcall	LVertStep
	rcall	LHoriStep
	bra	LHorizontal

LVertical:
	RCALL	LCD_SetPixel	;Turn on (PixelX,PixelY)
  rcall LCD_RefreshDisplay
	movf	PixelY,W
	xorwf	LCD_y2,W
	bz	aret
	btfss	slope,7
	 rcall	LHoriStep
	rcall	LVertStep
	bra	LVertical


LHoriStep:
	movf	dy,W
	subwf	slope,F
 	btfsc	dir,1
	 incf	PixelX,F
 	btfss	dir,1
	 decf	PixelX,F
	return

LVertStep:
	movf	dx,W
	addwf	slope,F
 	btfsc	dir,0
	 incf	PixelY,F
 	btfss	dir,0
	 decf	PixelY,F
	return


;

  if 0
LineLoop:
	RCALL	LCD_SetPixel	;Turn on (PixelX,PixelY)

;  rcall LCD_RefreshDisplay

	btfsc	slope,7		;If the slope accumulator is negative
	 bra	LVertStep	; then we're taking a vertical step

	movf	dy,W		;Assume we're taking a horizontal step

	TSTFSZ	slope		;If the slope accumulator is not zero
	 bra	LHoriStep	;Then the assumption is correct.

    ; The slope accumulator  is zero. If dx==dy too then that
    ; means we're drawing a diagonal line.

	cpfseq  dx		;if dx!=dy 
	 bra    LnotDiag	;Then we're not drawing a diagonal line.

LineDiag:
	rcall	LHorizontal
	rcall	LVertical
	bra	LineLoop
LnotDiag:
	cpfsgt	dx
	 bra	LVertStep

LHoriStep:
	rcall	LHorizontal	;Advance one step horizontally
	btfsc	slope,7		;If the slope accumulator changed signs
	 rcall	LVertical	; Then advance one step vertically
	bra	LineLoop	;

    ; LHorizontal subroutine
    ; Input:  W=dy
    ; Output: PixelX is either incremented or decremented
    ;         slope accumulator is reduced by dy.
    ; If Pixel exceeds x2, then the line routine is aborted.
LHorizontal:
	subwf	slope,F
 	btfss	dir,1
	 bra	LHoriLeftStep
LHoriRightStep:
	incf    PixelX,F
	movf	PixelX,W
	cpfslt  LCD_x2
         return
	decf    PixelX,F
	pop
	return
LHoriLeftStep
	decf    PixelX,W
	bnc	LLine1
	movwf	PixelX
	cpfsgt  LCD_x2
         return
	incf    PixelX,F
	pop
	return



LVertStep:
	rcall	LVertical
	movf	dy,W
	btfss	slope,7
	 rcall	LHorizontal
	bra	LineLoop
LVertical:
	movf	dx,W
	addwf	slope,F

 	btfss	dir,0
	 bra	LVertDownStep
LVertUpStep:
	incf    PixelY,F
	movf	PixelY,W
	cpfslt  LCD_y2
         return
	decf    PixelY,F
	pop
	return
LVertDownStep
	decf    PixelY,W
	bnc	LLine1
	movwf	PixelY
	cpfsgt  LCD_y2
         return
	incf    PixelY,F
LLine1:	pop
	return

  endif
;------------------------------------------------------------------------
; LCD_SetPixel - turn on a single Pixel
;
; Inputs:
;   PixelX - X coordinate
;   PixelY - Y coordinate
;
; Outputs
; Mem Used:
;   temp0
; Calls
;   LCD_GetPixelPtr
LCD_SetPixel:
	RCALL	LCD_GetPixelPtr
	MOVF	temp0,W
	IORWF	INDF0,F
	return

;------------------------------------------------------------------------
; LCD_ClearPixel - turn off a single Pixel

LCD_ClearPixel:
	RCALL	LCD_GetPixelPtr
	COMF	temp0,W
	ANDWF	INDF0,F
	return

;========================================================================
LCD_TABLES	CODE  0x3000

;------------------------------------------------------------------------
; This ASCII table contains the 5X7 font for characters 32-127. It was
; created from a modified version of the perl script that is distributed
; with gpsim's LCD module.
ASCII_5X7Table:
	db  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00  ; 32, 33
	db  0x00, 0x07, 0x00, 0x07, 0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14  ; 34, 35
	db  0x24, 0x2a, 0x7f, 0x2a, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62  ; 36, 37
	db  0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x05, 0x03, 0x00, 0x00  ; 38, 39
	db  0x00, 0x1c, 0x22, 0x41, 0x00, 0x00, 0x41, 0x22, 0x1c, 0x00  ; 40, 41
	db  0x14, 0x08, 0x3e, 0x08, 0x14, 0x08, 0x08, 0x3e, 0x08, 0x08  ; 42, 43
	db  0x00, 0x50, 0x30, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08  ; 44, 45
	db  0x00, 0x60, 0x60, 0x00, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02  ; 46, 47
	db  0x3e, 0x51, 0x49, 0x45, 0x3e, 0x00, 0x42, 0x7f, 0x40, 0x00  ; 48, 49
	db  0x42, 0x61, 0x51, 0x49, 0x46, 0x21, 0x41, 0x45, 0x4b, 0x31  ; 50, 51
	db  0x18, 0x14, 0x12, 0x7f, 0x10, 0x27, 0x45, 0x45, 0x45, 0x39  ; 52, 53
	db  0x3c, 0x4a, 0x49, 0x49, 0x30, 0x01, 0x71, 0x09, 0x05, 0x03  ; 54, 55
	db  0x36, 0x49, 0x49, 0x49, 0x36, 0x06, 0x49, 0x49, 0x29, 0x1e  ; 56, 57
	db  0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x56, 0x36, 0x00, 0x00  ; 58, 59
	db  0x08, 0x14, 0x22, 0x41, 0x00, 0x14, 0x14, 0x14, 0x14, 0x14  ; 60, 61
	db  0x41, 0x22, 0x14, 0x08, 0x00, 0x02, 0x01, 0x51, 0x09, 0x06  ; 62, 63
	db  0x32, 0x49, 0x79, 0x41, 0x3e, 0x7e, 0x11, 0x11, 0x11, 0x7e  ; 64, 65
	db  0x7f, 0x49, 0x49, 0x49, 0x36, 0x3e, 0x41, 0x41, 0x41, 0x22  ; 66, 67
	db  0x7f, 0x41, 0x41, 0x41, 0x3e, 0x7f, 0x49, 0x49, 0x49, 0x41  ; 68, 69
	db  0x7f, 0x09, 0x09, 0x09, 0x01, 0x3e, 0x41, 0x49, 0x49, 0x7a  ; 70, 71
	db  0x7f, 0x08, 0x08, 0x08, 0x7f, 0x00, 0x41, 0x7f, 0x41, 0x00  ; 72, 73
	db  0x20, 0x40, 0x41, 0x3f, 0x01, 0x7f, 0x08, 0x14, 0x22, 0x41  ; 74, 75
	db  0x7f, 0x40, 0x40, 0x40, 0x40, 0x7f, 0x01, 0x02, 0x01, 0x7f  ; 76, 77
	db  0x7f, 0x04, 0x08, 0x10, 0x7f, 0x3e, 0x41, 0x41, 0x41, 0x3e  ; 78, 79
	db  0x7f, 0x09, 0x09, 0x09, 0x06, 0x3e, 0x41, 0x51, 0x21, 0x5e  ; 80, 81
	db  0x7f, 0x09, 0x19, 0x29, 0x46, 0x46, 0x49, 0x49, 0x49, 0x31  ; 82, 83
	db  0x01, 0x01, 0x7f, 0x01, 0x01, 0x3f, 0x40, 0x40, 0x40, 0x3f  ; 84, 85
	db  0x1f, 0x20, 0x40, 0x20, 0x1f, 0x3f, 0x40, 0x38, 0x40, 0x3f  ; 86, 87
	db  0x63, 0x14, 0x08, 0x14, 0x63, 0x07, 0x08, 0x70, 0x08, 0x07  ; 88, 89
	db  0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x7f, 0x41, 0x41, 0x00  ; 90, 91
	db  0x15, 0x16, 0x7c, 0x16, 0x15, 0x00, 0x41, 0x41, 0x7f, 0x00  ; 92, 93
	db  0x04, 0x02, 0x01, 0x02, 0x04, 0x40, 0x40, 0x40, 0x40, 0x40  ; 94, 95
	db  0x00, 0x01, 0x02, 0x04, 0x00, 0x20, 0x54, 0x54, 0x54, 0x78  ; 96, 97
	db  0x7f, 0x48, 0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x20  ; 98, 99
	db  0x38, 0x44, 0x44, 0x48, 0x7f, 0x38, 0x54, 0x54, 0x54, 0x18  ; 100, 101
	db  0x08, 0x7e, 0x09, 0x01, 0x02, 0x06, 0x49, 0x49, 0x49, 0x3f  ; 102, 103
	db  0x7f, 0x08, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7d, 0x40, 0x00  ; 104, 105
	db  0x20, 0x40, 0x42, 0x3f, 0x00, 0x7f, 0x10, 0x28, 0x44, 0x00  ; 106, 107
	db  0x00, 0x41, 0x7f, 0x40, 0x00, 0x7c, 0x04, 0x18, 0x04, 0x78  ; 108, 109
	db  0x7c, 0x08, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38  ; 110, 111
	db  0x7c, 0x14, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14, 0x18, 0x7c  ; 112, 113
	db  0x7c, 0x08, 0x04, 0x04, 0x08, 0x48, 0x54, 0x54, 0x54, 0x24  ; 114, 115
	db  0x02, 0x3f, 0x42, 0x40, 0x20, 0x3c, 0x40, 0x40, 0x20, 0x7c  ; 116, 117
	db  0x1c, 0x20, 0x40, 0x20, 0x1c, 0x3c, 0x40, 0x30, 0x40, 0x3c  ; 118, 119
	db  0x44, 0x28, 0x10, 0x28, 0x44, 0x0c, 0x50, 0x50, 0x50, 0x3c  ; 120, 121
	db  0x44, 0x64, 0x54, 0x4c, 0x44, 0x00, 0x08, 0x36, 0x41, 0x00  ; 122, 123
	db  0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x41, 0x36, 0x08, 0x00  ; 124, 125
	db  0x08, 0x08, 0x2a, 0x1c, 0x08, 0x08, 0x1c, 0x2a, 0x08, 0x08  ; 126, 127


  ; Include the bitmaps.

  include "bitmaps.asm"

  end
