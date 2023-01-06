
;Screens
;
;  
;***********************************************************************
;Screen driver EQUates
FIELD_START_ADDR        EQU     0x14

;Screen objects 2 or 3 bytes
; first byte FRTCCCCC
;      F = 1 if this the last screen object in a list of screen objects
;      R = Row location. 0 = Row 0, 1 = Row 1
;  CCCCC = Column location 0 = column 0, 0x13 = column 19
;      T = Type of object. 0 = String object, 1 = Field Object
; Second byte
;    If the object is a string object (T=0), then this byte is the string number
;      that the object references.
;    Otherwise, if this object is a field object (T=1), then this byte defines the 
;      the type of the field with the following bit definition:
;       xMESHHLx
;       x = don't care
;       M = Edit Mode. M=1 means field is being edited.
;       E = Editable. If E=1 the object can be edited.
;       S = Size of Variable reference by the variable pointer. S=0 for a byte S=1 for a word
;      HH = 1x Display as Hex
;         = 01 Display as Decimal
;         = 00 Display as Decimal, but first, scale Hex value to decimal
;       L = Suppress leading zeroes.
; Third byte (Field objects only)
;    For field objects, this byte is pointer to the variable that is to be displayed in
;      the field.

SCR_IS_STRING_OBJECT    EQU     0x00
SCR_IS_FIELD_OBJECT     EQU     0x20
SCR_IS_EDITABLE         EQU     0x20
SCR_IS_EDITABLEb        EQU     5
SCR_IS_WORD             EQU     0x10
SCR_IS_BYTE             EQU     0x00
SCR_IS_WORDb            EQU     4
SCR_IS_HEX              EQU     0x08
SCR_IS_HEXb             EQU     3
SCR_IS_DEC              EQU     0x00
SCR_IS_SCALED           EQU     0x04
SCR_IS_SCALEDb          EQU     2
SCR_NO_ZEROES           EQU     0x02
SCR_NO_ZEROESb          EQU     1


;LCD Module DDRAM Address is formed by adding the row and column EQU
SCR_ROW0                EQU     0x00    ;Row 0 starts at LCD DDRAM Address 0
SCR_ROW1                EQU     0x40    ;Row 1     "            "          0x40

SCR_COL0                EQU     0x00
SCR_COL1                EQU     0x01
SCR_COL2                EQU     0x02
SCR_COL3                EQU     0x03
SCR_COL4                EQU     0x04
SCR_COL5                EQU     0x05
SCR_COL6                EQU     0x06
SCR_COL7                EQU     0x07
SCR_COL8                EQU     0x08
SCR_COL9                EQU     0x09
SCR_COL10               EQU     0x0a
SCR_COL11               EQU     0x0b
SCR_COL12               EQU     0x0c
SCR_COL13               EQU     0x0d
SCR_COL14               EQU     0x0e
SCR_COL15               EQU     0x0f
SCR_COL16               EQU     0x10
SCR_COL17               EQU     0x11
SCR_COL18               EQU     0x12
SCR_COL19               EQU     0x13

SCR_STRING_OBJECT       macro   row, column, string_num
        RETLW   row | column
        RETLW   string_num
        endm

SCR_FIELD_OBJECT        macro   row, column, variable_ptr, type
        RETLW   row | column | SCR_IS_FIELD_OBJECT
        RETLW   type
        RETLW   variable_ptr
        endm

SCR_NULL_OBJECT        macro
        RETLW   0xff
        endm

