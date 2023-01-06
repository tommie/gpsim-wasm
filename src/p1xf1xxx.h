#ifndef SRC_P1xF1xxx_H_
#define SRC_P1xF1xxx_H_

#include "14bit-processors.h"
#include "14bit-registers.h"
#include "14bit-tmrs.h"
#include "a2dconverter.h"
#include "clc.h"
#include "comparator.h"
#include "cwg.h"
#include "dsm_module.h"
#include "eeprom.h"
#include "ioports.h"
#include "nco.h"
#include "op_amp.h"
#include "pic-processor.h"
#include "pie.h"
#include "pir.h"
#include "pps.h"
#include "registers.h"
#include "ssp.h"
#include "uart.h"
#include "zcd.h"

#define FOSC0 (1<<0)
#define FOSC1 (1<<1)
#define FOSC2 (1<<2)
#define IESO (1<<12)

class PicLatchRegister;
class PicPortBRegister;
class PicPortIOCRegister;
class PicTrisRegister;
class Processor;

class P12F1822 : public _14bit_e_processor
{
public:
    ComparatorModule2 comparator;
    PIR_SET_2 pir_set_2_def;
    PIE     pie1;
    PIR    *pir1;
    PIE     pie2;
    PIR    *pir2;
    T2CON_64  t2con;
    PR2	  pr2;
    TMR2    tmr2;
    T1CON_G   t1con_g;
    TMRL    tmr1l;
    TMRH    tmr1h;
    CCPCON	ccp1con;
    CCPRL		ccpr1l;
    CCPRH		ccpr1h;
    FVRCON	fvrcon;
    BORCON	borcon;
    ANSEL_P 	ansela;
    ADCON0  	adcon0;
    ADCON1_16F 	adcon1;
    sfr_register  adresh;
    sfr_register  adresl;
    OSCCON_2  	*osccon;
    OSCTUNE6	osctune;          // with 6-bit trim, no PLLEN
    OSCSTAT 	oscstat;
    //OSCCAL  osccal;
    WDTCON  	wdtcon;
    USART_MODULE 	usart;
    SSP1_MODULE 	ssp;
    APFCON	apfcon;
    PWM1CON	pwm1con;
    ECCPAS        ccp1as;
    PSTRCON       pstr1con;
    CPSCON0	cpscon0;
    CPSCON1	cpscon1;
    SR_MODULE	sr_module;
    EEPROM_EXTND *e;
    DSM_MODULE       dsm_module;

    WPU              *m_wpua;
    IOC              *m_iocap;
    IOC              *m_iocan;
    IOCxF            *m_iocaf;
    PicPortIOCRegister  *m_porta;
    PicTrisRegister  *m_trisa;
    PicLatchRegister *m_lata;
    DACCON0	   *m_daccon0;
    DACCON1	   *m_daccon1;

    virtual PIR *get_pir2() { return nullptr; }
    virtual PIR *get_pir1() { return pir1; }
    virtual PIR_SET *get_pir_set() { return &pir_set_2_def; }

    EEPROM_EXTND *get_eeprom() override { return (EEPROM_EXTND *)eeprom; }

    PROCESSOR_TYPE isa() override { return _P12F1822_; }

    static Processor *construct(const char *name);
    P12F1822(const char *_name = nullptr, const char *desc = nullptr);
    ~P12F1822();

    void create_sfr_map() override;
    void create_symbols() override;
    void set_out_of_range_pm(unsigned int address, unsigned int value) override;
    virtual void create_iopin_map();
    virtual void create(int ram_top, int eeprom_size, int dev_id);
    unsigned int register_memory_size() const override { return 0x1000; }
    void option_new_bits_6_7(unsigned int bits) override;
    unsigned int program_memory_size() const override { return 2048; }
    void enter_sleep() override;
    void exit_sleep() override;
    void oscillator_select(unsigned int mode, bool clkout) override;
    void program_memory_wp(unsigned int mode) override;
};


class P12LF1822 : public P12F1822
{
public:
    P12LF1822(const char *_name = nullptr, const char *desc = nullptr);
    ~P12LF1822();

    PROCESSOR_TYPE isa() override { return _P12LF1822_; }

    static Processor *construct(const char *name);

    void create(int ram_top, int eeprom_size, int dev_id) override;
};


class P12F1840 : public P12F1822
{
public:
    P12F1840(const char *_name = nullptr, const char *desc = nullptr);
    ~P12F1840();

    static Processor *construct(const char *name);
    unsigned int program_memory_size() const override { return 4096; }
    void create(int ram_top, int eeprom_size, int dev_id) override;
    PROCESSOR_TYPE isa() override { return _P12F1840_; }

    sfr_register *vregcon;
};


class P12LF1840 : public P12F1840
{
public:
    P12LF1840(const char *_name = nullptr, const char *desc = nullptr);
    ~P12LF1840();

    static Processor *construct(const char *name);
    void create(int ram_top, int eeprom_size, int dev_id) override;
    PROCESSOR_TYPE isa() override { return _P12LF1840_; }
};


class P16F1503 : public _14bit_e_processor
{
public:
    P16F1503(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F1503();

    ComparatorModule2 comparator;
    PIR_SET_2 pir_set_2_def;
    PIE     pie1;
    PIR    *pir1;
    PIE     pie2;
    PIR    *pir2;
    PIE     pie3;
    PIR    *pir3;
    T2CON_64	  t2con;
    PR2	  pr2;
    TMR2    tmr2;
    T1CON_G   t1con_g;
    TMRL    tmr1l;
    TMRH    tmr1h;
    FVRCON	fvrcon;
    BORCON	borcon;
    ANSEL_P 	ansela;
    ANSEL_P 	anselc;
    ADCON0  	adcon0;
    ADCON1_16F 	adcon1;
    ADCON2_TRIG	adcon2;
    sfr_register  adresh;
    sfr_register  adresl;
    OSCCON_2  	*osccon;
    OSCSTAT 	oscstat;
    WDTCON  	wdtcon;
    SSP1_MODULE 	ssp;
    APFCON	apfcon1;
    PWMxCON	pwm1con;
    sfr_register  pwm1dcl;
    sfr_register  pwm1dch;
    PWMxCON	pwm2con;
    sfr_register  pwm2dcl;
    sfr_register  pwm2dch;
    PWMxCON	pwm3con;
    sfr_register  pwm3dcl;
    sfr_register  pwm3dch;
    PWMxCON	pwm4con;
    sfr_register  pwm4dcl;
    sfr_register  pwm4dch;
    CWG4		cwg;
    NCO		nco;
    CLCDATA	clcdata;
    CLC		clc1;
    CLC		clc2;
    OSC_SIM	frc;
    OSC_SIM	lfintosc;
    OSC_SIM	hfintosc;


    EEPROM_EXTND     *e;
    sfr_register     vregcon;
    WPU              *m_wpua;
    IOC              *m_iocap;
    IOC              *m_iocan;
    IOCxF            *m_iocaf;
    PicPortIOCRegister  *m_porta;
    PicTrisRegister  *m_trisa;
    PicLatchRegister *m_lata;
    DACCON0	     *m_daccon0;
    DACCON1 	     *m_daccon1;

    PicPortBRegister  *m_portc;
    PicTrisRegister  *m_trisc;
    PicLatchRegister *m_latc;

    CLC_BASE::data_in lcxd1_1[8] =
    {
        CLC_BASE::CLCxIN0, CLC_BASE::CLCxIN1, CLC_BASE::C1OUT, CLC_BASE::C2OUT,
        CLC_BASE::FOSCLK, CLC_BASE::T0_OVER, CLC_BASE::T1_OVER, CLC_BASE::T2_MATCH
    };
    CLC_BASE::data_in lcxd1_2[8] =
    {
        CLC_BASE::CLCxIN0, CLC_BASE::CLCxIN1, CLC_BASE::C1OUT, CLC_BASE::C2OUT,
        CLC_BASE::FOSCLK, CLC_BASE::T0_OVER, CLC_BASE::T1_OVER, CLC_BASE::T2_MATCH
    };
    CLC_BASE::data_in lcxd2_1[8] =
    {
        CLC_BASE::FOSCLK, CLC_BASE::T0_OVER, CLC_BASE::T1_OVER, CLC_BASE::T2_MATCH,
        CLC_BASE::LC1, CLC_BASE::LC2, CLC_BASE::UNUSED, CLC_BASE::UNUSED
    };
    CLC_BASE::data_in lcxd2_2[8] =
    {
        CLC_BASE::FOSCLK, CLC_BASE::T0_OVER, CLC_BASE::T1_OVER, CLC_BASE::T2_MATCH,
        CLC_BASE::LC1, CLC_BASE::LC2, CLC_BASE::UNUSED, CLC_BASE::UNUSED
    };
    CLC_BASE::data_in lcxd3_1[8] =
    {
        CLC_BASE::LC1, CLC_BASE::LC2, CLC_BASE::UNUSED, CLC_BASE::UNUSED,
        CLC_BASE::NCOx, CLC_BASE::HFINTOSC, CLC_BASE::PWM3, CLC_BASE::PWM4
    };
    CLC_BASE::data_in lcxd3_2[8] =
    {
        CLC_BASE::LC1, CLC_BASE::LC2, CLC_BASE::UNUSED, CLC_BASE::UNUSED,
        CLC_BASE::LFINTOSC, CLC_BASE::FRC_IN, CLC_BASE::PWM1, CLC_BASE::PWM2
    };
    CLC_BASE::data_in lcxd4_1[8] =
    {
        CLC_BASE::NCOx, CLC_BASE::HFINTOSC, CLC_BASE::PWM3, CLC_BASE::PWM4,
        CLC_BASE::CLCxIN0, CLC_BASE::CLCxIN1, CLC_BASE::C1OUT, CLC_BASE::C2OUT
    };
    CLC_BASE::data_in lcxd4_2[8] =
    {
        CLC_BASE::LFINTOSC, CLC_BASE::FRC_IN, CLC_BASE::PWM1, CLC_BASE::PWM2,
        CLC_BASE::CLCxIN0, CLC_BASE::CLCxIN1, CLC_BASE::C1OUT, CLC_BASE::C2OUT
    };


    virtual PIR *get_pir2() { return nullptr; }
    virtual PIR *get_pir1() { return pir1; }
    virtual PIR_SET *get_pir_set() { return &pir_set_2_def; }
    virtual EEPROM_EXTND *get_eeprom() { return (EEPROM_EXTND *)eeprom; }
    unsigned int program_memory_size() const override { return 2048; }
    unsigned int register_memory_size() const override { return 0x1000; }

    virtual void create_iopin_map();
    void create_sfr_map() override;
    void create_symbols() override;
    void set_out_of_range_pm(unsigned int address, unsigned int value) override;
    virtual void create(int ram_top, int dev_id);
    void option_new_bits_6_7(unsigned int bits) override;
    void enter_sleep() override;
    void exit_sleep() override;
    void oscillator_select(unsigned int mode, bool clkout) override;
    void program_memory_wp(unsigned int mode) override;
    static Processor *construct(const char *name);

    unsigned int ram_size;
};


class P16LF1503 : public P16F1503
{
public:
    static Processor *construct(const char *name);
    P16LF1503(const char *_name = nullptr, const char *desc = nullptr);
    ~P16LF1503() {}
};

class P16F170x : public _14bit_e_processor
{
public:
    ComparatorModule2 comparator;
    PIR_SET_2 pir_set_2_def;
    PIE     	pie1;
    PIR    	*pir1;
    PIE     	pie2;
    PIR    	*pir2;
    PIE     	pie3;
    PIR    	*pir3;
    T2CON_64	t2con;
    PR2	  	pr2;
    TMR2    	tmr2;
    T2CON_64	t4con;
    PR2	  	pr4;
    TMR2    	tmr4;
    T2CON_64  	t6con;
    PR2	  	pr6;
    TMR2    	tmr6;
    T1CON_G   	t1con_g;
    TMRL    	tmr1l;
    TMRH    	tmr1h;
    FVRCON	fvrcon;
    BORCON	borcon;
    ANSEL_P 	ansela;
    ANSEL_P 	anselc;
    ADCON0  	adcon0;
    ADCON1_16F 	adcon1;
    ADCON2_TRIG	adcon2;
    sfr_register  adresh;
    sfr_register  adresl;
    OSCCON_2  	*osccon;
    OSCTUNE6	osctune;          // with 6-bit trim, no PLLEN
    OSCSTAT 	oscstat;
    WDTCON  	wdtcon;
    USART_MODULE 	usart;
    SSP1_MODULE 	ssp;
    CCPCON  ccp1con;
    CCPRL   ccpr1l;
    CCPRH   ccpr1h;
    CCPCON  ccp2con;
    CCPRL   ccpr2l;
    CCPRH   ccpr2h;
    CCPTMRS14 ccptmrs;
    PWMxCON_PPS	pwm3con;
    sfr_register  pwm3dcl;
    sfr_register  pwm3dch;
    PWMxCON_PPS	pwm4con;
    sfr_register pwm4dcl;
    sfr_register pwm4dch;
    COG		 cog;
    CLCDATA	 clcdata;
    CLC_4SEL	 clc1;
    CLC_4SEL  	 clc2;
    CLC_4SEL	 clc3;
    OSC_SIM	 frc;
    OSC_SIM	 lfintosc;
    OSC_SIM	 hfintosc;
    PPS		 pps;
    PPSLOCK      ppslock;
    ZCDCON	 zcd1con;
    sfr_register slrcona;
    sfr_register slrconc;
    OPA		 opa1con;
    OPA		 opa2con;


    EEPROM_EXTND     *e;
    WPU              *m_wpua;
    IOC              *m_iocap;
    IOC              *m_iocan;
    IOCxF            *m_iocaf;
    PicPortIOCRegister  *m_porta;
    PicTrisRegister  *m_trisa;
    PicLatchRegister *m_lata;
    ODCON	     *m_odcona;
    INLVL	     *m_inlvla;
    DACCON0	     *m_daccon0;
    DACCON1	     *m_daccon1;

    PicPortBRegister *m_portc;
    PicTrisRegister  *m_trisc;
    PicLatchRegister *m_latc;
    WPU              *m_wpuc;
    IOC              *m_ioccp;
    IOC              *m_ioccn;
    IOCxF            *m_ioccf;
    ODCON	     *m_odconc;
    INLVL	     *m_inlvlc;

    xxxPPS	   *m_intpps;
    xxxPPS	   *m_ccp1pps;
    xxxPPS	   *m_ccp2pps;
    xxxPPS	   *m_coginpps;
    xxxPPS	   *m_t0ckipps;
    xxxPPS	   *m_t1ckipps;
    xxxPPS	   *m_t1gpps;
    xxxPPS	   *m_sspclkpps;
    xxxPPS	   *m_sspdatpps;
    xxxPPS	   *m_sspsspps;
    xxxPPS	   *m_rxpps;
    xxxPPS	   *m_ckpps;
    xxxPPS	   *m_clcin0pps;
    xxxPPS	   *m_clcin1pps;
    xxxPPS	   *m_clcin2pps;
    xxxPPS	   *m_clcin3pps;
    RxyPPS	   *m_ra0pps;
    RxyPPS	   *m_ra1pps;
    RxyPPS	   *m_ra2pps;
    RxyPPS	   *m_ra4pps;
    RxyPPS	   *m_ra5pps;
    RxyPPS	   *m_rc0pps;
    RxyPPS	   *m_rc1pps;
    RxyPPS	   *m_rc2pps;
    RxyPPS	   *m_rc3pps;
    RxyPPS	   *m_rc4pps;
    RxyPPS	   *m_rc5pps;

    CLC_BASE::data_in lcxd[32] =
    {
        CLC_BASE::CLCxIN0, CLC_BASE::CLCxIN1, CLC_BASE::CLCxIN2, CLC_BASE::CLCxIN3,
        CLC_BASE::LC1, CLC_BASE::LC2, CLC_BASE::LC3, CLC_BASE::UNUSED,
        CLC_BASE::C1OUT, CLC_BASE::C2OUT, CLC_BASE::COG1A, CLC_BASE::COG1B,
        CLC_BASE::CCP1, CLC_BASE::CCP2, CLC_BASE::PWM3, CLC_BASE::PWM4,
        CLC_BASE::MSSP_SCK, CLC_BASE::UNUSED, CLC_BASE::MSSP_SDO, CLC_BASE::ZCD_OUT,
        CLC_BASE::TX_CLK, CLC_BASE::UART_DT, CLC_BASE::T4_MATCH, CLC_BASE::T6_MATCH,
        CLC_BASE::T0_OVER, CLC_BASE::T1_OVER, CLC_BASE::T2_MATCH, CLC_BASE::IOCIF,
        CLC_BASE::ADCRC, CLC_BASE::LFINTOSC, CLC_BASE::HFINTOSC, CLC_BASE::FOSCLK
    };

    virtual PIR *get_pir2() { return pir2; }
    virtual PIR *get_pir1() { return pir1; }
    virtual PIR_SET *get_pir_set() { return &pir_set_2_def; }
    virtual EEPROM_EXTND *get_eeprom() { return ((EEPROM_EXTND *)eeprom); }
    unsigned int program_memory_size() const override { return 8192; }
    unsigned int register_memory_size () const override { return 0x1000; }

    P16F170x(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F170x();

    void create_sfr_map() override;
    void program_memory_wp(unsigned int mode) override;
    void oscillator_select(unsigned int mode, bool clkout) override;
    void option_new_bits_6_7(unsigned int bits) override;
    unsigned int ram_size;
};

class P16F1705 : public P16F170x
{
public:
    P16F1705(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F1705() {}

    static Processor *construct(const char *name);
    virtual void create(int ram_top, int dev_id);
    virtual void create_iopin_map();
    void create_sfr_map() override;
};

class P16LF1705 : public P16F1705
{
public:
    P16LF1705(const char *_name = nullptr, const char *desc = nullptr);
    ~P16LF1705() {}

    static Processor *construct(const char *name);
};

class P16F1709 : public P16F170x
{
public:
    P16F1709(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F1709();

    static Processor *construct(const char *name);
    virtual void create(int ram_top, int dev_id);
    virtual void create_iopin_map();
    void create_sfr_map() override;

    ANSEL_P 	      anselb;
    sfr_register      slrconb;
    PicPortBRegister  *m_portb;
    PicTrisRegister  *m_trisb;
    PicLatchRegister *m_latb;
    WPU              *m_wpub;
    IOC              *m_iocbp;
    IOC              *m_iocbn;
    IOCxF            *m_iocbf;
    ODCON	     *m_odconb;
    INLVL	     *m_inlvlb;
    RxyPPS	     *m_rb4pps;
    RxyPPS	     *m_rb5pps;
    RxyPPS	     *m_rb6pps;
    RxyPPS	     *m_rb7pps;
    RxyPPS	     *m_rc6pps;
    RxyPPS	     *m_rc7pps;
};

class P16LF1709 : public P16F1709
{
public:
    P16LF1709(const char *_name = nullptr, const char *desc = nullptr);
    ~P16LF1709() {}

    static Processor *construct(const char *name);
};

class P16F178x : public _14bit_e_processor
{
public:
    P16F178x(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F178x();

    ComparatorModule2 comparator;
    PIR_SET_2 pir_set_2_def;
    PIE     pie1;
    PIR    *pir1;
    PIE     pie2;
    PIR    *pir2;
    PIE     pie3;
    PIR    *pir3;
    PIE     pie4;
    PIR    *pir4;
    T2CON_64	  t2con;
    PR2	  pr2;
    TMR2    tmr2;
    T1CON_G   t1con_g;
    TMRL    tmr1l;
    TMRH    tmr1h;
    CCPCON	ccp1con;
    CCPRL		ccpr1l;
    CCPRH		ccpr1h;
    FVRCON	fvrcon;
    BORCON	borcon;
    ANSEL_P 	ansela;
    ANSEL_P   	anselb;
    ANSEL_P 	anselc;
    ADCON0_DIF  	adcon0;
    ADCON1_16F 	adcon1;
    ADCON2_DIF	adcon2;
    sfr_register  adresh;
    sfr_register  adresl;
    OSCCON_2  	*osccon;
    OSCTUNE6	osctune;          // with 6-bit trim, no PLLEN
    OSCSTAT 	oscstat;
    WDTCON  	wdtcon;
    USART_MODULE 	usart;
    SSP1_MODULE 	ssp;
    APFCON	apfcon1;
    APFCON	apfcon2;
    PWM1CON	pwm1con;
    ECCPAS        ccp1as;
    PSTRCON       pstr1con;
    EEPROM_EXTND *e;
    sfr_register     vregcon;


    WPU              *m_wpua;
    IOC              *m_iocap;
    IOC              *m_iocan;
    IOCxF            *m_iocaf;
    PicPortIOCRegister  *m_porta;
    PicTrisRegister  *m_trisa;
    PicLatchRegister *m_lata;
    IOC              *m_iocep;
    IOC              *m_iocen;
    IOCxF            *m_iocef;
    PicPortIOCRegister  *m_porte;
    PicTrisRegister  *m_trise;
    WPU              *m_wpue;
    DACCON0	   *m_daccon0;
    DACCON1	   *m_daccon1;
    DACCON0	   *m_dac2con0;
    DACCON1	   *m_dac2con1;
    DACCON0	   *m_dac3con0;
    DACCON1	   *m_dac3con1;
    DACCON0	   *m_dac4con0;
    DACCON1	   *m_dac4con1;
    IOC              *m_iocbp;
    IOC              *m_iocbn;
    IOCxF            *m_iocbf;
    PicPortBRegister  *m_portb;
    PicTrisRegister  *m_trisb;
    PicLatchRegister *m_latb;
    WPU              *m_wpub;

    IOC              *m_ioccp;
    IOC              *m_ioccn;
    IOCxF            *m_ioccf;
    PicPortBRegister  *m_portc;
    PicTrisRegister  *m_trisc;
    PicLatchRegister *m_latc;
    WPU              *m_wpuc;

    virtual PIR *get_pir2() { return nullptr; }
    virtual PIR *get_pir1() { return pir1; }
    virtual PIR_SET *get_pir_set() { return &pir_set_2_def; }

    EEPROM_EXTND *get_eeprom() override { return (EEPROM_EXTND *)eeprom; }

    void create_sfr_map() override;
    void create_symbols() override;
    void set_out_of_range_pm(unsigned int address, unsigned int value) override;
    virtual void create(int ram_top, int eeprom_size);
    void option_new_bits_6_7(unsigned int bits) override;
    void enter_sleep() override;
    void exit_sleep() override;
    void oscillator_select(unsigned int mode, bool clkout) override;
    void program_memory_wp(unsigned int mode) override;

    unsigned int ram_size;
};


class P16F1788 : public P16F178x
{
public:
    P16F1788(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F1788();

    PROCESSOR_TYPE isa() override { return _P16F1788_; }

    static Processor *construct(const char *name);
    void create_sfr_map() override;
    virtual void create_iopin_map();
    virtual void create(int ram_top, int eeprom_size, int dev_id);
    unsigned int program_memory_size() const override { return 16384; }
    unsigned int register_memory_size() const override { return 0x1000; }
};


class P16LF1788 : public P16F1788
{
public:
    P16LF1788(const char *_name = nullptr, const char *desc = nullptr);
    ~P16LF1788();

    PROCESSOR_TYPE isa() override { return _P16LF1788_; }

    static Processor *construct(const char *name);
    void create(int ram_top, int eeprom_size, int dev_id) override;
};


class P16F1823 : public P12F1822
{
public:
    P16F1823(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F1823();

    ANSEL_P   anselc;
    PROCESSOR_TYPE isa() override { return _P16F1823_; }

    static Processor *construct(const char *name);
    void create_sfr_map() override;
    void create_iopin_map() override;
    void create(int ram_top, int eeprom_size, int dev_id) override;

    PicPortBRegister  *m_portc;
    PicTrisRegister  *m_trisc;
    PicLatchRegister *m_latc;
    WPU              *m_wpuc;
};


class P16LF1823 : public P16F1823
{
public:
    P16LF1823(const char *_name = nullptr, const char *desc = nullptr);
    ~P16LF1823();

    PROCESSOR_TYPE isa() override { return _P16LF1823_; }
    static Processor *construct(const char *name);
    void create(int ram_top, int eeprom_size, int dev_id) override;
};


class P16F1825 : public P16F1823
{
public:
    P16F1825(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F1825();

    static Processor *construct(const char *name);
    unsigned int program_memory_size() const override { return 8 * 1024; }
    void create(int ram_top, int eeprom_size, int dev_id) override;
    PROCESSOR_TYPE isa() override { return _P16F1825_; }

    PIE     pie3;
    PIR    *pir3;
    T2CON_64        t4con;
    PR2     pr4;
    TMR2    tmr4;
    T2CON_64        t6con;
    PR2     pr6;
    TMR2    tmr6;
    CCPCON	ccp2con;
    CCPRL		ccpr2l;
    CCPRH		ccpr2h;
    PWM1CON	pwm2con;
    ECCPAS        ccp2as;
    PSTRCON       pstr2con;
    CCPCON	ccp3con;
    CCPRL		ccpr3l;
    CCPRH		ccpr3h;
    CCPCON	ccp4con;
    CCPRL		ccpr4l;
    CCPRH		ccpr4h;
    CCPTMRS14     ccptmrs;
    APFCON	apfcon0;
    APFCON	apfcon1;
    sfr_register  inlvla;
    sfr_register  inlvlc;
};


class P16LF1825 : public P16F1825
{
public:
    P16LF1825(const char *_name = nullptr, const char *desc = nullptr);
    ~P16LF1825();

    static Processor *construct(const char *name);
    void create(int ram_top, int eeprom_size, int dev_id) override;
    PROCESSOR_TYPE isa() override { return _P16LF1825_; }
};


#endif // SRC_P1xF1xxx_H_
