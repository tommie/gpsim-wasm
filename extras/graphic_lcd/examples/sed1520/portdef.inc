

;------------------------------------------------------------------------
;
; I/O Port definitions for the SED1520 based graphic LCD
;
;
; The user of the graphic LCD module should configure this
; file according to their specific application.
;

LCDA0_PORT   EQU   PORTA
LCDA0_TRIS   EQU   TRISA
LCDA0_LAT    EQU   LATA
LCDA0_BIT    EQU   RA1

LCDE1_PORT   EQU   PORTE
LCDE1_TRIS   EQU   TRISE
LCDE1_LAT    EQU   LATE
LCDE1_BIT    EQU   RE0

LCDE2_PORT   EQU   PORTE
LCDE2_TRIS   EQU   TRISE
LCDE2_LAT    EQU   LATE
LCDE2_BIT    EQU   RE1

LCDRW_PORT   EQU   PORTE
LCDRW_TRIS   EQU   TRISE
LCDRW_LAT    EQU   LATE
LCDRW_BIT    EQU   RE2


SED1520_PORT			EQU	PORTB
SED1520_TRIS			EQU	TRISB
SED1520_LAT			EQU	LATB


