
;;
;;	Include file for the p18c242
;;

;----- register files------------------------------------------------------

porta		equ	0xf80
portb		equ	0xf81
portc		equ	0xf82
portd		equ	0xf83
porte		equ	0xf84

lata		equ	0xf89
latb		equ	0xf8a
latc		equ	0xf8b
latd		equ	0xf8c
late		equ	0xf8d

trisa		equ	0xf92
trisb		equ	0xf93
trisc		equ	0xf94
trisd		equ	0xf95
trise		equ	0xf96

pie1		equ	0xf9d
pir1		equ	0xf9e
ipr1		equ	0xf9f
pie2		equ	0xfa0
pir2		equ	0xfa1
ipr2		equ	0xfa2

rcsta		equ	0xfab
txsta		equ	0xfac
txreg		equ	0xfad
rcreg		equ	0xfae
spbrg		equ	0xfaf

t3con		equ	0xf91
tmr3l		equ	0xf92
tmr3h		equ	0xf93

rcon		equ	0xfd0
wdtcon		equ	0xfd1
lvdcon		equ	0xfd2
osccon		equ	0xfd3

tmr0con		equ	0xfd5
t0con		equ	0xfd5
tmr0l		equ	0xfd6
tmr0h		equ	0xfd7

status		equ	0xfd8

fsr2l		equ	0xfd9
fsr2h		equ	0xfda
plusw2		equ	0xfdb
preinc2		equ	0xfdc
postdec2	equ	0xfdd
postinc2	equ	0xfde
indf2		equ	0xfdf

bsr		equ	0xfe0

fsr1l		equ	0xfe1
fsr1h		equ	0xfe2
plusw1		equ	0xfe3
preinc1		equ	0xfe4
postdec1	equ	0xfe5
postinc1	equ	0xfe6
indf1		equ	0xfe7

wreg		equ	0xfe8
fsr0l		equ	0xfe9
fsr0h		equ	0xfea
plusw0		equ	0xfeb
preinc0		equ	0xfec
postdec0	equ	0xfed
postinc0	equ	0xfee
indf0		equ	0xfef

intcon3		equ	0xff0
intcon2		equ	0xff1
intcon		equ	0xff2

prodl		equ	0xff3
prodh		equ	0xff4

tablat		equ	0xff5
tabptrl		equ	0xff6
tabptrh		equ	0xff7
tabptru		equ	0xff8

pcl		equ	0xff9
pclath		equ	0xffa
pclatu		equ	0xffb

stkptr		equ	0xffc
tosl		equ	0xffd
tosh		equ	0xffe
tosu		equ	0xfff

;----- status bits --------------------------------------------------------

irp		equ	7
rp1		equ	6
rp0		equ	5
not_to		equ	4
to		equ	4
not_pd		equ	3
pd		equ	3
z		equ	2
dc		equ	1
c		equ	0

; intcon bits

gie		equ	7
peie		equ	6
t0ie		equ	5
tmr0ie		equ	5
int0e		equ	4
rbie		equ	3
t0if		equ	2
tmr0if		equ	2
int0f		equ	1
rbif		equ	0

; t0con bits

tmr0on          equ  7
t08bit          equ  6
t0cs            equ  5
t0se            equ  4
psa             equ  3
t0ps2           equ  2
t0ps1           equ  1
t0ps0           equ  0

; txsta bits
csrc            equ  7
tx9             equ  6
txen            equ  5
sync            equ  4
brgh            equ  2
trmt            equ  1
tx9d            equ  0


; rcsta bits
spen            equ  7
rx9             equ  6
sren            equ  5
cren            equ  4
ferr            equ  2
oerr            equ  1
rx9d            equ  0



; pir1 bits
pspif           equ  7
adif            equ  6
rcif            equ  5
txif            equ  4
sspif           equ  3
ccp1if          equ  2
tmr2if          equ  1
tmr1if          equ  0

; pie1 bits
pspie           equ  7
adie            equ  6
rcie            equ  5
txie            equ  4
sspie           equ  3
ccp1ie          equ  2
tmr2ie          equ  1
tmr1ie          equ  0

; intcon bits

gie		equ	7
gieh		equ	7
peie		equ	6
giel		equ	6
t0ie		equ	5
tmr0ie		equ	5
int0ie		equ	4
rbie		equ	3
t0if		equ	2
tmr0if		equ	2
intf		equ	1
int0f		equ	1
rbif		equ	0

; intcon2 bits
rbpu		equ	7
intedg0		equ	6
intedg1		equ	5
intedg2		equ	4
tmr0ip		equ	2
rbip		equ	0

; intcon3 bits
int2ip		equ	7
int1ip		equ	6
int2ie		equ	4
int1ie		equ	3
int2if		equ	1
int1if		equ	0


;---- Interrupt vectors -----------------------------------

#define INTERRUPT_VECTOR_LO       0x18
#define INTERRUPT_VECTOR_HI       0x08

;; The following is a direct copy from the MPLAB include file. (Please
;; don't tell microchip ;). Immediately following are some of my
;; redefinitions that match the data sheet.

;==========================================================================
;
;       Configuration Bits
;
;==========================================================================
;
;Configuration Byte 0 Options
_CP_ON_0          EQU  H'00'    ; code protect    
_CP_OFF_0         EQU  H'FF'

;Configuration Byte 1 Options
_PLL_4MHZ_1       EQU  H'3F'    ; phase locked loop frequency
_PLL_5MHZ_1       EQU  H'7F'
_PLL_6MHZ_1       EQU  H'BF'
_PLL_8MHZ_1       EQU  H'FF'

_LPSCEN_ON_1      EQU  H'DF'    ; low power system clock enable
_LPSCEN_OFF_1     EQU  H'FF'

_LP_OSC_1         EQU  H'F8'    ; oscillator type
_XT_OSC_1         EQU  H'F9'
_HS_OSC_1         EQU  H'FA'
_RC_OSC_1         EQU  H'FB'
_EC_OSC_1         EQU  H'FC'    ; w/OSC2 div by 4
_ECRA6_OSC_1      EQU  H'FD'    ; w/OSC2 as RA6
_HSPLL_OSC_1      EQU  H'FE'    ; HS PLL
_RCRA6_OSC_1      EQU  H'FF'    ; RC w/OSC2 as RA6

;Configuration Byte 2 Options
_BOREN_ON_2       EQU  H'FF'    ; brown-out reset enable
_BOREN_OFF_2      EQU  H'FD'
_PWRTE_OFF_2      EQU  H'FF'    ; Power up Timer enable
_PWRTE_ON_2       EQU  H'FE'
_BORV_25_2        EQU  H'FF'    ; BOREN Voltage - 2.5v
_BORV_27_2        EQU  H'FB'    ;                 2.7v
_BORV_42_2        EQU  H'F7'    ;                 4.2v
_BORV_45_2        EQU  H'F3'    ;                 4.5v

;Configuration Byte 3 Options
_WDT_ON_3         EQU  H'FF'    ; watch dog timer
_WDT_OFF_3        EQU  H'FE'
_WDPS_128_3       EQU  H'FF'    ; watch dog timer postscaler
_WDPS_64_3        EQU  H'FD'
_WDPS_32_3        EQU  H'FB'
_WDPS_16_3        EQU  H'F9'
_WDPS_8_3         EQU  H'F7'
_WDPS_4_3         EQU  H'F5'
_WDPS_2_3         EQU  H'F3'
_WDPS_1_3         EQU  H'F1'

;Configuration Byte 5 Options
_CCP2MX_ON_5      EQU  H'FF'    ;
_CCP2MX_OFF_5     EQU  H'FE'

;Configuration Byte 6 Options
_LV_ON_6          EQU  H'FD'    ; low voltage
_LV_OFF_6         EQU  H'FF'
_STVRE_ON_6       EQU  H'FF'    ; stack over/underflow reset enable
_STVRE_OFF_6      EQU  H'FE'

; To use the Configuration Bits, place the following lines in your source code
;  in the following format, and change the configuration value to the desired 
;  setting (such as CP_OFF to CP_ON).  These are currently commented out here
;  and each __CONFIG line should have the preceding semicolon removed when
;  pasted into your source code.
;  The following is a assignment of address values for all of the configuration
;  registers for the purpose of table reads
_CONFIG0        EQU    H'300000'
_CONFIG1        EQU    H'300001'
_CONFIG2        EQU    H'300002'
_CONFIG3        EQU    H'300003'
_CONFIG4        EQU    H'300004'
_CONFIG5        EQU    H'300005'
_CONFIG6        EQU    H'300006'
_CONFIG7        EQU    H'300007'

;Program Configuration Register 0
;               __CONFIG    _CONFIG0,_CP_OFF_0

;Program Configuration Register 1
;               __CONFIG    _CONFIG1,_PLL_8MHZ_1 & _LPSCEN_OFF_1 & _RCRA6_OSC_1

;Program Configuration Register 2
;               __CONFIG    _CONFIG2, _BORV_25_2 & _BODEN_ON_2 & _PWRTE_OFF_2

;Program Configuration Register 3
;               __CONFIG    _CONFIG3, _WDPS_128_3 & _WDT_ON_3

;Program Configuration Register 5
                ;__CONFIG    _CONFIG5, _CCP2MX_ON_5

;Program Configuration Register 6
;               __CONFIG    _CONFIG6, _LV_OFF_6 & _STVRE_ON_6


;==========================================================================

CONFIG1L        EQU    _CONFIG0
CONFIG1H        EQU    _CONFIG1
CONFIG2L        EQU    _CONFIG2
CONFIG2H        EQU    _CONFIG3
CONFIG4         EQU    _CONFIG4  ;; not used according to data sheet.
CONFIG3H        EQU    _CONFIG5
CONFIG4L        EQU    _CONFIG6
CONFIG7         EQU    _CONFIG7  ;; not used according to data sheet.

DEVID1		EQU	0x3ffffe
DEVID2		EQU	0x3fffff
