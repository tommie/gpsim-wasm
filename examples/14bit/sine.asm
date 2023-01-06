        list    p=16C84,t=ON,c=132,n=80,st=off
        radix   dec


        include "p16c84.inc"

  cblock  0x20

        f_hi    ;Low byte of frequency variable
        f_lo
        x1
        x2
        interp
        index
        product

        fstep_lo
        fstep_hi
  endc

ODD_QUADRANT    equ     4       ;In f_hi
NEG_QUADRANT    equ     5       ;In f_hi
INDEX_MASK      equ     0x0f    ;In f_hi
INTERP_MASK     equ     0xf0    ;In f_lo


        ORG     0               ;Reset Vector

        GOTO    Main

        ORG     4               ;Interrupt Vector


Main

        BCF     STATUS,RP0      ;Point to BANK 0

    ;Initialize the step in frequency
        MOVLW   0x10
        MOVWF   fstep_lo        ;One unit of fstep corresponds to 360/16384 degrees
        CLRF    fstep_hi        ;i.e. 0.02197 degrees. Set fstep = 455 (1c7) to step
                                ;10 degrees for example

        CLRF    f_hi            ;Start off at 0 degrees
        CLRF    f_lo

xxx
        CALL    sine
        NOP

    ;Now that the sine has been calculated, we could do something with it other than
    ;throwing it away.

    ;Make a frequency step. Note, the frequency step doesn't necessarily have to be
    ;constant. E.g. it could be changed as part of a PLL algorithm.

        MOVF    fstep_lo,W
        ADDWF   f_lo,F
        SKPNC
          INCF  f_hi,F
        MOVF    fstep_hi,W
        ADDWF   f_hi,F

        GOTO    xxx


;--------------------------------------------------------
;sine
;
;  The purpose of this routine is to take the sine of the
;16-bit frequency variable f_hi:f_lo to produce a signed
;8-bit result that is within +/- 1 count of the true sine
;value.
;  Only the lower 14 bits of the frequency variable are
;actually used. The frequency variable maps into degrees
;by the following transfer function:
;  degrees = (f & 0x3fff) * 360 / 0x4000
;  The sine output is approximately
;sine = int(127*sin( (f & 0x3fff) * 360 / 0x4000) )
; where
; sin() is the true sine function
; int() is the nearest integer function
;
;The technique used to obtain the sine value is a combination
;of table look-up and first order linear interpolation. Sixteen
;equidistant frequency samples of the first quadrant of sin(x) 
;are stored in sine_table.
;
; The frequency variable is broken down as follows:
; xxQQTTTT IIIIPPPP
; where 
;  xx - don't care
;  QQ - Quadrant: 00 = quadrant 1, 01 = quadrant 2, etc.
;  TTTT - Index into the sine_table.
;  IIII - Interpolation between successive entries in the table
;  PPPP - Phase accumulation (not needed in this function, it's
;         only used to increase the dynamic range in frequency
;         steps).
;Once the frequency has been broken down in to these parts, the
;sine function for the first quadrant can be calculated as follows:
;  x1 = sine_table[index]
;  x2 = sine_table[index+1]
;  sine = x1 + ((x2-x1) * interp) / 16
;The first term, x1, is seen to be a first order approximation to
;sine function. The second term improves on that approximation by
;interpolating between x1 and x2. The interpolation variable interp
;is 0 <= interp <= 15 and it is divided by 16. Consequently, the
;interpolation factor ranges between 0 and 15/16.
;
;The sine function in the other three quadrants can be obtained
;from calculations based on the first quadrant by using the following
;trig identities:
; first, let 0 <= f <= 90, i.e. f covers the first quadrant.
;  quadrant 2:  u = 90 + f,   90 < u < 180
;     sin(u-90) = sin(f)
;     x1 = sine_table(16-index), x2 = sine_table(15-index)
;  quadrant 3:  u = 180 + f,  180 < u < 270
;     sin(u) = sin(f+180) = -sin(f)
;     x1 = -sine_table(index), x2 = -sine_table(index+1)
;  quadrant 4:  u = 270 + f,  270 < u < 360
;     sin(u-90) = sin(f+180) = -sin(f)
;     x1 = -sine_table(16-index), x2 = -sine_table(15-index)
;
;Thus, for quadrants 2 and 4, the sine table is indexed in reverse
;order and for quadrants 3 and 4 the values from the sine table 
;are negated. A slight change is made on this indexing and negation
;scheme so that the operation (x2-x1) * interp / 16 only deals with
;positive numbers. This significantly simplifies the multiplication.
;The modification changes the formula for each quadrant as follows:
; quadrant 1:   (no change)
;   x1 = sine_table[index],  x2 = sine_table[index+1]
;   sine = x1 + ((x2-x1) * interp) / 16
; quadrant 2:
;   x1 = sine_table[15-index],  x2 = sine_table[16-index]
;   sine = x2 - ((x2-x1) * interp) / 16
; quadrant 3:
;   x1 = sine_table[index],  x2 = sine_table[index+1]
;   sine = -(x1 + ((x2-x1) * interp) / 16)
; quadrant 4:
;   x1 = sine_table[15-index],  x2 = sine_table[16-index]
;   sine = -(x2 - ((x2-x1) * interp) / 16)
;
;Input
; f_hi:f_lo  -  16-bit frequency variable
;Output
; W = int(127*sin( (f & 0x3fff) * 360 / 0x4000) )
;
;Execution time: 48 Cycles (for all cases)
;

sine

    ;Get the 4-bit index and add 1 to it.
        MOVF    f_hi,W
        ANDLW   INDEX_MASK
        ADDLW   1
        BTFSC   f_hi,ODD_QUADRANT
         SUBLW  17                      ;Odd quadrants, index = 16 - index
                                        ;Actually: (index + 1) = 17 - (index + 1)
                                        ;                      = 16 - index
        MOVWF   index
        CALL    sine_table              ;Get x2=sin(index+1)
        MOVWF   x2

        DECF    index,W
        CALL    sine_table              ;Get x1=sin(index)
        MOVWF   x1

        SUBWF   x2,W                    ;W=x2-x1, This is always positive. 

    ;Initialize the product of (x2-x1)*interp/16 to 1/2. Note 8/16 == 1/2
    ;(This rounds the product to the nearest integer.)
        CLRF    product
        BSF     product,3       ;(note, product and index could be aliased to
                                ; save one byte of ram).

    ;multiply interp and x2 - x1 and divide by 16. This is actually a 4 by 8
    ;bit multiplication. The division by 16 is implemented with a shift right
    ;one position for each of the four multiplication iterations.

	clrc
	btfsc	f_lo,4
         addwf  product,f       ;Then add (x2-x1) to the product
        rrf     product,f       ;Divide the product by two

	clrc
	btfsc	f_lo,5
         addwf  product,f
        rrf     product,f	;Divide the product by four

	clrc
	btfsc	f_lo,6
         addwf  product,f
        rrf     product,f	;Divide the product by eight

	clrc
	btfsc	f_lo,7
         addwf  product,f
        rrf     product,w	;Divide the product by sixteen


        BTFSS   f_hi,ODD_QUADRANT
          ADDWF   x1,W
        BTFSC   f_hi,ODD_QUADRANT
          SUBWF   x2,W
        BTFSC   f_hi,NEG_QUADRANT
          SUBLW 0
        RETURN

sine_table
        ADDWF   PCL,F
        RETLW   0       ;127*sin(0 * 90/16)
        RETLW   12      ;127*sin(1 * 90/16)
        RETLW   25      ;127*sin(2 * 90/16)
        RETLW   37      ;127*sin(3 * 90/16)
        RETLW   49      ;127*sin(4 * 90/16)
        RETLW   60      ;127*sin(5 * 90/16)
        RETLW   71      ;127*sin(6 * 90/16)
        RETLW   81      ;127*sin(7 * 90/16)
        RETLW   90      ;127*sin(8 * 90/16)
        RETLW   98      ;127*sin(9 * 90/16)
        RETLW   106     ;127*sin(10 * 90/16)
        RETLW   112     ;127*sin(11 * 90/16)
        RETLW   117     ;127*sin(12 * 90/16)
        RETLW   122     ;127*sin(13 * 90/16)
        RETLW   125     ;127*sin(14 * 90/16)
        RETLW   126     ;127*sin(15 * 90/16)
        RETLW   127     ;127*sin(16 * 90/16)

        END
