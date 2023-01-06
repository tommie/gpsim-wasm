

;------------------------------------------------------------------------
;
; I/O Port definitions for the SSD0323 based graphic LCD
;
;
; The user of the graphic LCD module should configure this
; file according to their specific application.
;

LCDCS_PORT   EQU   PORTD
LCDCS_TRIS   EQU   TRISD
LCDCS_LAT    EQU   LATD
LCDCS_BIT    EQU   RD0

LCDRES_PORT  EQU   PORTD
LCDRES_TRIS  EQU   TRISD
LCDRES_LAT   EQU   LATD
LCDRES_BIT   EQU   RD1

LCDE_PORT    EQU   PORTE
LCDE_TRIS    EQU   TRISE
LCDE_LAT     EQU   LATE
LCDE_BIT     EQU   RE0

LCDRW_PORT   EQU   PORTE
LCDRW_TRIS   EQU   TRISE
LCDRW_LAT    EQU   LATE
LCDRW_BIT    EQU   RE1

LCDDC_PORT   EQU   PORTE
LCDDC_TRIS   EQU   TRISE
LCDDC_LAT    EQU   LATE
LCDDC_BIT    EQU   RE2


SSD0323_PORT EQU   PORTB
SSD0323_TRIS EQU   TRISB
SSD0323_LAT  EQU   LATB


mInitLCD_PINS macro
   ; Reset and CS are outputs
        BCF     LCDCS_TRIS,LCDCS_BIT
        BCF     LCDRES_TRIS,LCDRES_BIT


   ; Hold reset low while the ports are being initialized.
        BCF     LCDRES_LAT,LCDRES_BIT
        BSF     LCDCS_LAT,LCDCS_BIT

   ; Data bus
	CLRF	SSD0323_LAT
	CLRF	SSD0323_TRIS

   ; Control bus

  if LCDE_TRIS == LCDRW_TRIS && LCDE_TRIS == LCDDC_TRIS
	CLRF	TRISE
        SETF    LATE
  else
	BCF	LCDE_TRIS, LCDE_BIT
	BCF	LCDDC_TRIS, LCDDC_BIT
	BCF	LCDRW_TRIS, LCDRW_BIT
  endif

 endm

mSetLCDRES      macro
        BSF     LCDRES_LAT,LCDRES_BIT
 endm

mClrLCDRES      macro
        BSF     LCDRES_LAT,LCDRES_BIT
 endm

mSetLCDCS macro
        BSF     LCDCS_LAT, LCDCS_BIT
 endm

mClrLCDCS macro
        BCF     LCDCS_LAT, LCDCS_BIT
 endm
