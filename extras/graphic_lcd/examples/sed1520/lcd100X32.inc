

; Definitions for the 100X32 graphic LCD
;


  EXTERN  LCD_ClearScreen
  EXTERN  SED1520_Test

  EXTERN  LCD_Line
  EXTERN  LCD_putBitMap

LCD_nROWS	EQU	32
LCD_nCOLS	EQU	100

;------------------------------------------------------------------------
; External RAM declarations

  EXTERN PixelX    ;Current X & Y coordinates for graphic's operations. E.g.
  EXTERN PixelY    ;starting point for lines, upper left hand corner of rectangles.

  EXTERN LCD_x2    ;End point for lines, 
  EXTERN LCD_y2    ;


