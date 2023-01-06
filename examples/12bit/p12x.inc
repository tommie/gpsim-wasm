	LIST
; P12C508 & P12C509 Header file
	NOLIST



;----- register files


indf		equ	0x0
tmr0		equ	0x1
pcl		equ	0x2
status		equ	0x3
fsr		equ	0x4
osccal		equ	0x5
gpio		equ	0x6


;----- status bits --------------------------------------------------------

pa2		equ	7
gpwuf		equ	7
pa1		equ	6
pa0		equ	5
not_to		equ	4
to		equ	4
not_pd		equ	3
pd		equ	3
z		equ	2
dc		equ	1
c		equ	0


;----- option bits --------------------------------------------------------

not_gpwu	equ	7	; gpwu is active low, but it's confusing to
rbpu		equ	7	; specify this so verbosely.
not_gppu	equ	6
gppu		equ	6
t0cs		equ	5
t0se		equ	4
psa		equ	3
ps2		equ	2
ps1		equ	1
ps0		equ	0


;	configuration bits

_cp_on		equ	0xff7
_cp_off		equ	0xfff
_pwrte_on	equ	0x3fff
_pwrte_off	equ	0x3ff7
_wdt_on		equ	0xfff
_wdt_off	equ	0xffb

_lp_osc		equ	0xffc
_xt_osc		equ	0xffd
_intrc_osc	equ	0xffe
_extrc_osc	equ     0xfff


;==========================================================================
;
;       Processor-dependent Definitions
;
;==========================================================================

        ifdef __12c508
           __maxram h'01f'
        endif

        ifdef __12c509
           __maxram h'03f'
        endif

	LIST
