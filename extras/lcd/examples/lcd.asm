;********************************************************************************
;
; The file contains code to control an LCD module.
;
; T. Scott Dattalo
;
;********************************************************************************

;*******************************************************************
;
; EQUates


T0CKI           equ     H'0004'
UF_SELECTED_FIELD       equ     0x0f
UF_SELECT_MODE          equ     4
UF_EDIT_MODE            equ     5
UF_ON_SELECTED_FIELD    equ     6
UF_IS_EDITABLE          equ     7

UF_SELECT_NEXT          equ     0
UF_SELECT_PREV          equ     1

;*******************************************************************
;


;LCD_INITIALIZE
;  Right now, the display could be either in 8-bit mode if we 
; just powered up, or it could be in 4-bit mode if we just 
; experienced a reset. So to begin, we have to initialize the 
; display to a known state: the 8-bit mode. This is done by 
; sending the LCD command "Function Set" with the 8-bit mode 
; bit set. We need to do this 3 times!

LCD_INITIALIZE

   ;Initialize the LCD_PORT control and data lines to outputs

        MOVLW   ~LCD_CONTROL_MASK
        ANDWF   LCD_CONTROL_PORT,F      ;Clear the control lines
        BSF     STATUS, RP0             ;Select Register page 1
        ANDWF   LCD_CONTROL_TRIS,F      ;Make LCD_PORT IO lines outputs
        BCF     STATUS, RP0             ;Select Register page 0

        MOVLW   ~LCD_DATA_MASK
        ANDWF   LCD_DATA_PORT,F         ;Clear the control lines
        BSF     STATUS, RP0             ;Select Register page 1
        ANDWF   LCD_DATA_TRIS,F         ;Make LCD_PORT IO lines outputs
        BCF     STATUS, RP0             ;Select Register page 0

        MOVLW   3
        MOVWF   LCD_init_loop   ;Use temp as a loop counter
        BCF     LCD_CONTROL_PORT, LCD_E
        BCF     LCD_CONTROL_PORT, LCD_RS
        BCF     LCD_CONTROL_PORT, LCD_R_W

init_8bit:
        CALL    LCD_DELAY

        MOVF    LCD_DATA_PORT,W
        ANDLW   ~LCD_DATA_MASK
        IORLW   (LCD_CMD_FUNC_SET | LCD_8bit_MODE) >> LCD_DATA_SHIFT
        MOVWF   LCD_DATA_PORT           ;Put command on the bus

        GOTO    $+1
        BSF     LCD_CONTROL_PORT, LCD_E ;Enable the LCD, i.e. write the command.
        GOTO    $+1                     ;NOP's are only needed for 20Mhz crystal
        GOTO    $+1
        BCF     LCD_CONTROL_PORT, LCD_E ;Disable the LCD

        DECFSZ  LCD_init_loop, F
        GOTO    init_8bit


   ;We should now have the LCD module in 8-bit mode. Now let's put it in 4-bit mode
        CALL    LCD_DELAY

        MOVF    LCD_DATA_PORT,W
        ANDLW   ~LCD_DATA_MASK
        IORLW   (LCD_CMD_FUNC_SET | LCD_4bit_MODE) >> LCD_DATA_SHIFT
        MOVWF   LCD_DATA_PORT        ;Put command on the bus

        GOTO    $+1
        BSF     LCD_CONTROL_PORT, LCD_E ;Enable the LCD, i.e. write the command.
        GOTO    $+1             ;NOP's are only needed for 20Mhz crystal
        GOTO    $+1
        BCF     LCD_CONTROL_PORT, LCD_E ;Disable the LCD
        CALL    LCD_DELAY

   ;Now we are in 4-bit mode. This means that all reads and writes of bytes have to be done
   ;a nibble at a time. But that's all taken care of by the read/write functions.
   ;Set up the display to have 2 lines and the small (5x7 dot) font.

        MOVLW   LCD_CMD_FUNC_SET | LCD_4bit_MODE | LCD_2_LINES | LCD_SMALL_FONT
        CALL    LCD_WRITE_CMD

   ;Turn on the display and turn off the cursor. Set the cursor to the non-blink mode

        MOVLW   LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF
        CALL    LCD_WRITE_CMD

   ;Clear the display memory. This command also moves the cursor to the home position.

        MOVLW   LCD_CMD_CLEAR_DISPLAY
        CALL    LCD_WRITE_CMD

   ;Set up the cursor mode.

        MOVLW   LCD_CMD_ENTRY_MODE | LCD_INC_CURSOR_POS | LCD_NO_SCROLL
        CALL    LCD_WRITE_CMD

   ;Set the Display Data RAM address to 0

        MOVLW   LCD_CMD_SET_DDRAM
        CALL    LCD_WRITE_CMD

        CALL    LCD_CLEAR_SCREEN
        RETURN

;*******************************************************************
;LCD_DELAY
; This routine takes the calculated times that the delay loop needs to
;be executed, based on the LCD_INIT_DELAY EQUate that includes the
;frequency of operation.
;
LCD_DELAY   MOVLW   LCD_INIT_DELAY  ;
A_DELAY     MOVWF   tmp             ; Use tmp and temp
            CLRF    temp            ;
LOOP2       DECFSZ  temp, F         ; Delay time = tmp * ((3 * 256) + 3) * Tcy
            GOTO    LOOP2           ;            = tmp * 154.2 (20Mhz clock)
            DECFSZ  tmp, F          ;
            GOTO    LOOP2           ;
            RETURN

;*******************************************************************
;LCD_TOGGLE_E
;  This routine toggles the "E" bit (enable) on the LCD module. The contents
;of W contain the state of the R/W and RS bits along with the data that's
;to be written (that is if data is to be written). The contents of the LCD port
;while E is active are returned in W.

LCD_TOGGLE_E
        BCF  LCD_CONTROL_PORT,LCD_E     ;Make sure E is low
;  movlw 0
LTE1:   GOTO    $+1                     ;Delays needed primarily for 10Mhz and faster clocks
        GOTO    $+1
        GOTO    $+1
        BTFSC   LCD_CONTROL_PORT, LCD_E ;E is low the first time through the loop
          goto  LTE2

        BSF     LCD_CONTROL_PORT, LCD_E ;Make E high and go through the loop again
        GOTO    LTE1

LTE2:
;  addlw 1
;  skpz 
;    goto LTE2

        MOVF    LCD_DATA_PORT, W        ;Read the LCD Data bus
        ANDLW   LCD_DATA_MASK           ;We're only interested in the data lines
        BCF     LCD_CONTROL_PORT, LCD_E ;Turn off E
        RETURN

LCD_DROP_E
        BCF  LCD_CONTROL_PORT,LCD_E     ;Make sure E is low
        GOTO    $+1                     ;Delays needed primarily for 10Mhz and faster clocks
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        RETURN

LCD_RAISE_E
        BSF  LCD_CONTROL_PORT,LCD_E     ;Make sure E is high
        GOTO    $+1                     ;Delays needed primarily for 10Mhz and faster clocks
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        RETURN


;*******************************************************************
;LCD_WRITE_DATA - Sends a character to LCD
;  This routine splits the character into the upper and lower
;nibbles and sends them to the LCD, upper nibble first.
;
; Memory used:
;    LCD_CHAR,
; Calls
;    LCD_TOGGLE_E
;
LCD_WRITE_DATA
        MOVWF   LCD_CHAR            ;Character to be sent is in W
        CALL    LCD_BUSY_CHECK      ;Wait for LCD to be ready

        BSF     LCD_CONTROL_PORT,LCD_RS
        BCF     LCD_CONTROL_PORT,LCD_R_W

LCD_WRITE
  if LCD_DATA_MASK == 0x0f
        SWAPF   LCD_CHAR,F
  endif

    ;First, write the upper nibble
        CALL    LCD_RAISE_E
        MOVF    LCD_CHAR, w
        ANDLW   LCD_DATA_MASK
        MOVWF   temp
        MOVF    LCD_DATA_PORT,W
        ANDLW   ~LCD_DATA_MASK
        IORWF   temp,W
        MOVWF   LCD_DATA_PORT
        ;CALL    LCD_TOGGLE_E
        CALL    LCD_DROP_E

    ;Next, write the lower nibble
        CALL    LCD_RAISE_E
        SWAPF   LCD_CHAR, w
        ANDLW   LCD_DATA_MASK
        MOVWF   temp
        MOVF    LCD_DATA_PORT,W
        ANDLW   ~LCD_DATA_MASK
        IORWF   temp,W
        MOVWF   LCD_DATA_PORT
        ;GOTO    LCD_TOGGLE_E
        GOTO    LCD_DROP_E

;*******************************************************************
;LCD_WRITE_CMD
;
;  This routine splits the command into the upper and lower
;nibbles and sends them to the LCD, upper nibble first.

LCD_WRITE_CMD
        MOVWF   LCD_CHAR        ;Character to be sent is in W

        CALL    LCD_BUSY_CHECK  ;Wait for LCD to be ready

   ;Both R_W and RS should be low
        BCF     LCD_CONTROL_PORT,LCD_RS
        BCF     LCD_CONTROL_PORT,LCD_R_W
        GOTO    LCD_WRITE

;*******************************************************************
;LCD_READ_DATA
;This routine will read 8 bits of data from the LCD. Since we're using
;4-bit mode, two passes have to be made. On the first pass we read the
;upper nibble, and on the second the lower.
;
LCD_READ_DATA
        CALL    LCD_BUSY_CHECK

   ;For a data read, RS and R/W should be high
        BSF     LCD_CONTROL_PORT,LCD_RS
        BSF     LCD_CONTROL_PORT,LCD_R_W
LCD_READ
        BSF     STATUS, RP0             ;Select Register page 1
        MOVF    LCD_DATA_TRIS,W         ;Get the current setting for the whole register
        IORLW   LCD_DATA_MASK           ;Set the TRIS bits- make all of the data
        MOVWF   LCD_DATA_TRIS           ;   lines inputs.
        BCF     STATUS, RP0             ;Select Register page 0

        CALL    LCD_TOGGLE_E            ;Toggle E and read upper nibble
        MOVWF   tmp                     ;Save the upper nibble
        CALL    LCD_TOGGLE_E            ;Toggle E and read lower nibble
        MOVWF   temp                    ;Save the lower nibble
        SWAPF   temp, W                 ;Put the lower nibble of data in lower half of W
        IORWF   tmp, F                  ;Combine nibbles

        BSF     STATUS, RP0             ;Select Register page 1

        MOVLW   ~LCD_DATA_MASK          ;Clear the TRIS bits- make all of the data
        ANDWF   LCD_DATA_TRIS,F         ;   lines outputs.

        BCF     STATUS, RP0             ;Select Register page 0
        MOVF    tmp,W
  if LCD_DATA_MASK == 0x0f
        SWAPF   tmp,W
  endif

        RETURN

;*******************************************************************
;LCD_BUSY_CHECK
;This routine checks the busy flag, returns when not busy

LCD_BUSY_CHECK

   ;For a busy check, RS is low and R/W is high
        BCF     LCD_CONTROL_PORT,LCD_RS
        BSF     LCD_CONTROL_PORT,LCD_R_W

        CALL    LCD_READ
        ANDLW   0x80                    ;Check busy flag, high = busy

        SKPNZ
          RETURN
        MOVLW   5
        CALL    A_DELAY
        GOTO    LCD_BUSY_CHECK          ;If busy, check again

;*******************************************************************
;LCD_CLEAR_SCREEN
LCD_CLEAR_SCREEN
        MOVLW   20-1
        MOVWF   buffer0
        MOVLW   LCD_CMD_SET_DDRAM | SCR_ROW0 | SCR_COL0
lcs1:   CALL    LCD_WRITE_CMD
lcs2:   MOVLW   0x20                    ;ASCII space
        CALL    LCD_WRITE_DATA
        DECF    buffer0,W
        MOVWF   buffer0
        BTFSS   buffer0,6          ;See if we generated a borrow
          goto  lcs2

        BTFSS   buffer0,7          ;First time through, borrow should be in 7 too
          RETURN

        MOVLW   20-1 + 0x80        ;Reload count and set bit 7 (borrow will clear it)
        MOVWF   buffer0
        MOVLW   LCD_CMD_SET_DDRAM | SCR_ROW1 | SCR_COL0
        GOTO    lcs1
