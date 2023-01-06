/*
   Copyright (C) 2006 T. Scott Dattalo
   Copyright (C) 2009, 2013, 2015 Roy R. Rankin

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#ifndef SRC_A2DCONVERTER_H_
#define SRC_A2DCONVERTER_H_

#include <glib.h>

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include <stdio.h>

#include "ioports.h"
#include "registers.h"
#include "trigger.h"
#include "stimuli.h"

class ADCON0;
class ADCON2_DIF;
class ANSEL_H;
class CPSCON0;
class ComparatorModule2;
class DACCON0;
class INTCON;
class PIR;
class Processor;

/*
 *   Class to connect modules to CDA channel of FVR module
 */
class FVR_ATTACH
{
public:
    explicit FVR_ATTACH(const char *name)
        : reg_name(name)
    {
    }
    ~FVR_ATTACH()
    {
	if (fvr_node)
	    fprintf(stderr, "***FVR_ATTACH %s detach not called***\n", fvr_name.c_str());
	if (fvr_ad_node)
	    fprintf(stderr, "***FVR_ATTACH %s detach not called***\n", fvr_ad_name.c_str());
	if (fvr_Vt_node)
	{
	    fprintf(stderr, "***FVR_ATTACH %s detach not called***\n", fvr_Vt_name.c_str());
	    printf("***FVR_ATTACH RRR %s detach not called***\n", fvr_Vt_name.c_str());
	}
    }
    void attach_cda_fvr(Stimulus_Node *_fvr_node, unsigned int chan=99);
    void attach_ad_fvr(Stimulus_Node *_fvr_node, unsigned int chan);
    void attach_Vt_fvr(Stimulus_Node *_fvr_node, unsigned int chan);
    void detach_fvr();
    virtual void set_FVR_volt(double, unsigned int){fprintf(stderr, "set_FVR_volt not defined %s\n", reg_name.c_str());}
    

private:
    std::string   reg_name;
    std::string	  fvr_name;
    stimulus      *fvr_stimulus = nullptr;
    Stimulus_Node *fvr_node = nullptr;
    std::string   fvr_ad_name;
    stimulus      *fvr_ad_stimulus = nullptr;
    Stimulus_Node *fvr_ad_node = nullptr;
    std::string   fvr_Vt_name;
    stimulus      *fvr_Vt_stimulus = nullptr;
    Stimulus_Node *fvr_Vt_node = nullptr;
};


/*
 *   Class to connect modules to CDA channel of FVR module
 */
class DAC_ATTACH
{
public:
    explicit DAC_ATTACH(const char *name)
        : reg_name(name)
    {
        std::fill_n(DAC_node, 8 , nullptr);
        std::fill_n(DAC_stimulus, 8 , nullptr);
    }

    ~DAC_ATTACH()
    {
        for(int i=0; i<8; i++)
        {
	    if (DAC_stimulus[i])
	    {
	        fprintf(stderr, "***DAC_ATTACH %s %s detach not called***\n", reg_name.c_str(), DAC_name[i].c_str());
	    }
	}
    }

    void attach_DAC(Stimulus_Node *_dacout_node, unsigned int chan, unsigned int n=1);
    void detach_DAC();
    virtual void set_DAC_volt(double, unsigned int)
    {
        fprintf(stderr, "set_DAC_volt not defined %s\n", reg_name.c_str());
    }

private:
    std::string   reg_name;
    std::string   DAC_name[8];
    stimulus      *DAC_stimulus[8];
    Stimulus_Node *DAC_node[8];

};

class DAC_STIMULUS : public stimulus
{
public:
    DAC_STIMULUS(DAC_ATTACH *arg, unsigned int _chan, const char *n = nullptr,
                 double _Vth = 0.0, double _Zth = 1e12) :
                stimulus(n, _Vth, _Zth),
                    pt_base_function(arg), chan(_chan)
    {}


    void set_nodeVoltage(double v) override;

private:
    DAC_ATTACH *pt_base_function;
    unsigned int chan;

};


/* Stimulus which directs updates FVR voltages through
 * the set_FVR_volt function
 */
class FVR_STIMULUS : public stimulus
{
public:
    FVR_STIMULUS(FVR_ATTACH *arg, unsigned int _chan, 
		 const char *name = nullptr,
                 double _Vth = 0.0, double _Zth = 1e12) :
                stimulus(name, _Vth, _Zth),
                    pt_base_function(arg), chan(_chan)
    {}


    void set_nodeVoltage(double v) override;

private:
    FVR_ATTACH *pt_base_function;
    unsigned int chan;

};

//---------------------------------------------------------
// ADRES
//
/*
class ADRES : public sfr_register
{
public:

  void put(int new_value);
};
*/

/*
	AD_IN_SignalControl is used to set an ADC pin as input
	regardless of the setting to the TRIS register
*/
class AD_IN_SignalControl : public SignalControl
{
public:
    AD_IN_SignalControl() {}
    ~AD_IN_SignalControl() {}
    char getState() override
    {
        return '1';
    }
    void release() override {}
};


//---------------------------------------------------------
// ADCON1
//

class ADCON1 : public sfr_register, public FVR_ATTACH, public DAC_ATTACH
{
public:

    enum PCFG_bits
    {
        PCFG0 = 1 << 0,
        PCFG1 = 1 << 1,
        PCFG2 = 1 << 2,
        PCFG3 = 1 << 3, // 16f87x etc.
        VCFG0 = 1 << 4, // 16f88
        VCFG1 = 1 << 5, // 16f88
        ADCS2 = 1 << 6,
        ADFM  = 1 << 7  // Format Result select bit
    };

    ADCON1(Processor *pCpu, const char *pName, const char *pDesc);
    ~ADCON1();

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    void setChannelConfiguration(unsigned int cfg, unsigned int bitMask);
    void setVrefLoConfiguration(unsigned int cfg, unsigned int channel);
    void setVrefHiConfiguration(unsigned int cfg, unsigned int channel);
    unsigned int getVrefHiChannel(unsigned int cfg);
    unsigned int getVrefLoChannel(unsigned int cfg);

    double getChannelVoltage(unsigned int channel);
    virtual double getVrefHi();
    virtual double getVrefLo();

    void setValidCfgBits(unsigned int m, unsigned int s);
    void setNumberOfChannels(unsigned int);
    unsigned int getNumberOfChannels() { return (m_nAnalogChannels); }
    void setIOPin(unsigned int, PinModule *);
    virtual void setVoltRef(unsigned int, float);
    int  get_cfg(unsigned int);
    virtual unsigned int get_adc_configmask(unsigned int);
    virtual void set_cfg_index(unsigned int index) { cfg_index = index; }

    void set_channel_in(unsigned int channel, bool on);
    void setADCnames();
    void setValidBits(unsigned int mask) { valid_bits = mask; }
    bool getADFM() { return adfm; }
    unsigned int getMaxCfg() { return cMaxConfigurations; }
    void set_FVR_chan(unsigned int chan) { FVR_chan = chan; }
    void set_DAC_volt(double, unsigned int) override;
    void set_FVR_volt(double, unsigned int) override;

protected:
    unsigned int  valid_bits;
    unsigned int  FVR_chan;
    bool	  adfm = false;
    PinModule     **m_AnalogPins = nullptr;
    float	  *m_voltageRef = nullptr;
    unsigned int  m_nAnalogChannels = 0;
    unsigned int  mValidCfgBits = 0;
    unsigned int  mCfgBitShift = 0;
    unsigned int  mIoMask = 0;
    unsigned int  cfg_index = 0;

    static const unsigned int cMaxConfigurations = 16;

    /* Vrefhi/lo_position - this is an array that tells which
     * channel the A/D converter's  voltage reference(s) are located.
     * The index into the array is formed from the PCFG bits.
     * The encoding is as follows:
     *   0-7:  analog channel containing Vref(s)
     *     8:  The reference is internal (i.e. Vdd)
     *  0xff:  The analog inputs are configured as digital inputs
     */
    unsigned int Vrefhi_position[cMaxConfigurations];
    unsigned int Vreflo_position[cMaxConfigurations];

    /* configuration bits is an array of 8-bit masks definining whether or not
     * a given channel is analog or digital
     */
    unsigned int m_configuration_bits[cMaxConfigurations];

    // used bt setControl to set pin direction as input
    AD_IN_SignalControl *m_ad_in_ctl = nullptr;
};


//---------------------------------------------------------
// ADCON1_16F
//

class ADCON1_16F : public ADCON1
{
public:

    enum
    {
        ADPREF0 = (1 << 0),
        ADPREF1 = (1 << 1),
        ADNREF  = (1 << 2),
        ADCS0   = (1 << 4),
        ADCS1   = (1 << 5),
        ADCS2   = (1 << 6),
        ADFM    = (1 << 7)
    };

    ADCON1_16F(Processor *pCpu, const char *pName, const char *pDesc);

    void put_value(unsigned int new_value) override;
    void setAdcon0(ADCON0 *_adcon0) { adcon0 = _adcon0; }
    double getVrefHi() override;
    double getVrefLo() override;
    void setVoltRef(unsigned int, float) override;

private:
    ADCON0 *adcon0 = nullptr;
};


//---------------------------------------------------------
// ADCON0
//

class ADCON0 : public sfr_register, public TriggerObject
{
public:

    enum
    {
        ADON = 1 << 0,
        ADIF = 1 << 1,
        GO   = 1 << 2,
        CHS0 = 1 << 3,
        CHS1 = 1 << 4,
        CHS2 = 1 << 5,
        ADCS0 = 1 << 6,
        ADCS1 = 1 << 7,

    };

    enum AD_states
    {
        AD_IDLE,
        AD_ACQUIRING,
        AD_CONVERTING
    };

    ADCON0(Processor *pCpu, const char *pName, const char *pDesc);

    void start_conversion();
    void stop_conversion();
    virtual void set_interrupt();
    void callback() override;
    void put(unsigned int new_value) override;
    virtual void put_conversion();

    bool getADIF() { return (value.get() & ADIF) != 0; }
    void setAdres(sfr_register *);
    void setAdresLow(sfr_register *);
    void setAdcon1(ADCON1 *);
    void setIntcon(INTCON *);
    virtual void setPir(PIR *);
    void setA2DBits(unsigned int);
    void setChannel_Mask(unsigned int ch_mask) { channel_mask = ch_mask; }
    void setChannel_shift(unsigned int ch_shift) { channel_shift = ch_shift; }
    void setGo(unsigned int go) { GO_bit = (1 << go); }
    virtual bool get_ADFM() { return adcon1->getADFM(); }
    virtual void set_Tad(unsigned int);
    virtual double getChannelVoltage(unsigned int channel)
    {
        return (adcon1->getChannelVoltage(channel));
    }
    virtual double getVrefHi()
    {
        return (m_dSampledVrefHi  = adcon1->getVrefHi());
    }
    virtual double getVrefLo()
    {
        return (m_dSampledVrefLo  = adcon1->getVrefLo());
    }

    void setValidBits(unsigned int mask) { valid_bits = mask; }

private:
    friend class ADCON0_10;
    friend class ADCON0_12F;
    friend class ADCON0_32X;
    friend class ADCON0_DIF;

    sfr_register *adres = nullptr;
    sfr_register *adresl = nullptr;
    ADCON1 *adcon1 = nullptr;
    INTCON *intcon = nullptr;
    PIR    *m_pPir = nullptr;

    double m_dSampledVoltage = 0.0;
    double m_dSampledVrefHi = 0.0;
    double m_dSampledVrefLo = 0.0;
    unsigned int m_A2DScale = 0;
    unsigned int m_nBits = 0;
    guint64 future_cycle = 0;
    unsigned int ad_state;
    unsigned int Tad = 0;
    unsigned int channel_mask;
    unsigned int channel_shift;
    unsigned int GO_bit;
    unsigned int valid_bits = 0;
};


class ADCON0_91X : public ADCON0
{
public:
    enum
    {
        ADON = 1 << 0,
        GO   = 1 << 1,
        CHS0 = 1 << 2,
        CHS1 = 1 << 3,
        CHS2 = 1 << 4,
        VCFG0 = 1 << 5,	// P16f91x
        VCFG1 = 1 << 6,	// P16f91x
        ADFM  = 1 << 7,	// P16f91x

    };

    ADCON0_91X(Processor *pCpu, const char *pName, const char *pDesc) : ADCON0(pCpu, pName, pDesc) {}

    bool get_ADFM() override
    {
        return value.get() & ADFM;
    }
    double getVrefHi() override;
    double getVrefLo() override;

    unsigned int Vrefhi_position = 0;
    unsigned int Vreflo_position = 0;
};


/* A/D converter with 12 or 10 bit differential input with ADCON2
   32 possible input channels
 */

class ADCON0_DIF : public ADCON0
{
    enum
    {
        ADON = 1 << 0,
        GO   = 1 << 1,
        CHS0 = 1 << 2,
        CHS1 = 1 << 3,
        CHS2 = 1 << 4,
        CHS3 = 1 << 5,
        CHS4 = 1 << 6,
        ADRMD = 1 << 7
    };

public:
    ADCON0_DIF(Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    void put_conversion() override;
    void	setAdcon2(ADCON2_DIF * _adcon2) { adcon2 = _adcon2; }

private:
    ADCON2_DIF *adcon2;
};


//---------------------------------------------------------
// ADCON0_10 register for 10f22x A2D
//

class ADCON0_10 : public ADCON0
{
public:

    enum
    {
        ADON = 1 << 0,
        GO   = 1 << 1,
        CHS0 = 1 << 2,
        CHS1 = 1 << 3,
        ANS0 = 1 << 6,
        ANS1 = 1 << 7
    };
    void put(unsigned int new_value) override;
    ADCON0_10(Processor *pCpu, const char *pName, const char *pDesc);

private:
    AD_IN_SignalControl ad_pin_input;
};


//---------------------------------------------------------
// ANSEL
//

class ANSEL : public sfr_register
{
public:
    enum
    {
        ANS0 = 1 << 0,
        ANS1 = 1 << 1,
        ANS2 = 1 << 2,
        ANS3 = 1 << 3,
        ANS4 = 1 << 4,
        ANS5 = 1 << 5,
        ANS6 = 1 << 6,
        ANS7 = 1 << 7
    };

    ANSEL(Processor *pCpu, const char *pName, const char *pDesc);

    void setAdcon1(ADCON1 *new_adcon1);
    void setAnselh(ANSEL_H *new_anselh)
    {
        anselh = new_anselh;
    }
    void put(unsigned int new_val) override;
    void setValidBits(unsigned int mask)
    {
        valid_bits = mask;
    }

private:
    ADCON1 *adcon1 = nullptr;
    ANSEL_H *anselh = nullptr;
    unsigned int valid_bits;
};


//---------------------------------------------------------
// ANSEL_H
//

class ANSEL_H : public sfr_register
{
public:
    enum
    {
        ANS8 = 1 << 0,
        ANS9 = 1 << 1,
        ANS10 = 1 << 2,
        ANS11 = 1 << 3,
        ANS12 = 1 << 4,
        ANS13 = 1 << 5,
    };

    ANSEL_H(Processor *pCpu, const char *pName, const char *pDesc);

    void setAdcon1(ADCON1 *new_adcon1);
    void setAnsel(ANSEL *new_ansel)
    {
        ansel = new_ansel;
    }
    void put(unsigned int new_val) override;
    void setValidBits(unsigned int mask)
    {
        valid_bits = mask;
    }

private:
    ADCON1 *adcon1 = nullptr;
    ANSEL *ansel = nullptr;
    unsigned int valid_bits;
};


//
//	ANSEL_P is an analog select register associated
//	with a port.
class ANSEL_P : public sfr_register
{
public:
    ANSEL_P(Processor *pCpu, const char *pName, const char *pDesc);

    void setAdcon1(ADCON1 *new_adcon1);
    void setAnsel(ANSEL_P *new_ansel);
    void put(unsigned int new_val) override;
    void setValidBits(unsigned int mask) { valid_bits = mask; }
    void config(unsigned int pins, unsigned int first_chan)
    {
        analog_pins = pins;
        first_channel = first_chan;
    }
    unsigned int get_mask() { return cfg_mask; }

private:
    ADCON1 *adcon1 = nullptr;
    ANSEL_P *ansel = nullptr;
    unsigned int valid_bits;	// register bit mask
    unsigned int analog_pins = 0;	// bit map of analog port pins
    unsigned int first_channel = 0;	// channel number for LSB of analog_pins
    unsigned int cfg_mask = 0;	// A2D mask this port only
    std::list<ANSEL_P *> ansel_list;
};


//---------------------------------------------------------
// ADCON0_12F register for 12f675 A2D
//

class ADCON0_12F : public ADCON0
{
public:

    enum
    {
        ADON = 1 << 0,
        GO   = 1 << 1,
        CHS0 = 1 << 2,
        CHS1 = 1 << 3,
        CHS2 = 1 << 4,
        VCFG = 1 << 6,
        ADFM = 1 << 7
    };
    void put(unsigned int new_value) override;
    bool get_ADFM() override { return (value.get() & ADFM); }
    void set_Tad(unsigned int _tad) override { Tad = _tad; }
    ADCON0_12F(Processor *pCpu, const char *pName, const char *pDesc);

private:
    AD_IN_SignalControl ad_pin_input;
};


//---------------------------------------------------------
// ADCON0_32X register for 10f320 A2D
//

class ADCON0_32X : public ADCON0
{
public:

    enum
    {
        ADON = 1 << 0,
        GO   = 1 << 1,
        CHS0 = 1 << 2,
        CHS1 = 1 << 3,
        CHS2 = 1 << 4,
        ADCS0 = 1 << 5,
        ADCS1 = 1 << 6,
        ADCS2 = 1 << 7,
    };
    void put(unsigned int new_value) override;

    ADCON0_32X(Processor *pCpu, const char *pName, const char *pDesc);

private:
    AD_IN_SignalControl ad_pin_input;
};


//---------------------------------------------------------
// ANSEL_12F
//

class ANSEL_12F : public sfr_register
{
public:
    enum
    {
        ANS0 = 1 << 0,
        ANS1 = 1 << 1,
        ANS2 = 1 << 2,
        ANS3 = 1 << 3,
        ADCS0 = 1 << 4,
        ADCS1 = 1 << 5,
        ADCS2 = 1 << 6
    };

    ANSEL_12F(Processor *pCpu, const char *pName, const char *pDesc);

    void setAdcon0(ADCON0_12F *new_adcon0) { adcon0 = new_adcon0; }
    void setAdcon1(ADCON1 *new_adcon1) { adcon1 = new_adcon1; }
    void put(unsigned int new_val) override;
    void set_tad(unsigned int);

private:
    ADCON1 *adcon1 = nullptr;
    ADCON0_12F *adcon0 = nullptr;
};


// set voltage from stimulus
class a2d_stimulus : public stimulus
{
public:
    a2d_stimulus(ADCON1 *arg, int chan, const char *n = nullptr,
                 double _Vth = 0.0, double _Zth = 1e12
                );

    ADCON1 *_adcon1;
    int	   channel;

    void set_nodeVoltage(double v) override;
};



//---------------------------------------------------------
// FVRCON register for Fixed Voltage Reference
// 2 channels with temperature output


class FVRCON : public sfr_register, public TriggerObject
{
public:

    enum
    {
        ADFVR0 	= 1 << 0,
        ADFVR1 	= 1 << 1,
        CDAFVR0 = 1 << 2,
        CDAFVR1 = 1 << 3,
        TSRNG	= 1 << 4,
        TSEN	= 1 << 5,
        FVRRDY	= 1 << 6,
        FVREN	= 1 << 7,
    };
    FVRCON(Processor *, const char *pName, const char *pDesc = nullptr, unsigned int bitMask = 0xff);
    ~FVRCON();

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    Stimulus_Node *get_node_cvref()   { return node_cvref;}
    Stimulus_Node *get_node_adcvref() { return node_adcvref;}
    Stimulus_Node *get_node_Vtref() { return node_Vtref;}

    virtual double compute_VTemp(unsigned int);	//Voltage of core temperature setting
    virtual double compute_FVR_AD(unsigned int);	//Voltage reference for ADC
    virtual double compute_FVR_CDA(unsigned int);	//Voltage reference for Comparators, DAC, CPS
//private:
    void callback() override;
    void callback_print() override;

    guint64       future_cycle = 0;
    ADCON1        *adcon1 = nullptr;
    DACCON0       *daccon0 = nullptr;
    unsigned int   mask_writable;
    Stimulus_Node *node_cvref;
    Stimulus_Node *node_adcvref;
    Stimulus_Node *node_Vtref;
    stimulus	  *volt_cvref;	
    stimulus	  *volt_adcvref;
    stimulus	  *volt_Vtref;
};

// FVR single output using bits 4,5 with no temp measurements
class FVRCON_V2 : public sfr_register, public TriggerObject
{
public:

    enum {
        FVRS0   = 1 << 4,
        FVRS1   = 1 << 5,
        FVRRDY  = 1 << 6,
        FVREN   = 1 << 7,
    };
    FVRCON_V2(Processor *, const char *pName, const char *pDesc = nullptr, unsigned int bitMask = 0xf0);
    ~FVRCON_V2();

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    double compute_FVR_CDA(unsigned int); 
    Stimulus_Node *get_node_cvref()   { return node_cvref;}
    void callback() override;
    void callback_print() override;

    unsigned int  mask_writable;
    guint64       future_cycle = 0;
    Stimulus_Node *node_cvref;
    stimulus	  *volt_cvref;
};
//
//  DAC module
//

class DACCON0 : public sfr_register, public FVR_ATTACH
{
public:
    enum
    {
        DACNSS  = (1 << 0),	// Negative source select bit (18f26k22)
        DACPSS0	= (1 << 2),
        DACPSS1	= (1 << 3),
        DACOE2	= (1 << 4),
        DACOE	= (1 << 5),
        DACLPS  = (1 << 6),
        DACEN	= (1 << 7),
    };

    DACCON0(Processor *, const char *pName, const char *pDesc = nullptr, unsigned int bitMask = 0xe6, unsigned int bit_res = 32);
    ~DACCON0();

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    virtual void set_adcon1(ADCON1 *_adcon1) { adcon1 = _adcon1; }
    void set_cpscon0(CPSCON0 *_cpscon0) { cpscon0 = _cpscon0; }
    void set_dcaccon1_reg(unsigned int reg);
    void set_FVR_volt(double volt, unsigned int chan) override { FVR_CDA_volt = volt; }
    void setDACOUT(PinModule *pin1, PinModule *pin2 = nullptr)
    {
        output_pin[0] = pin1;
        output_pin[1] = pin2;
    }
    virtual void	compute_dac(unsigned int value);
    virtual double 	get_Vhigh(unsigned int value);
    Stimulus_Node	*get_node_dacout(){return node_dacout;}

protected:
    void  set_dacoutpin(bool output_enabled, int chan, double Vout);
    ADCON1	*adcon1 = nullptr;
    ComparatorModule2 *cmModule = nullptr;
    int	cm_index;
    std::vector<ComparatorModule2 *> cmModule_list;
    CPSCON0	  *cpscon0 = nullptr;
    unsigned int  bit_mask;
    unsigned int  bit_resolution;
    unsigned int  daccon1_reg = 0;
    double	  FVR_CDA_volt = 0.0;
    bool	  Pin_Active[2];
    double	  Vth[2];
    double	  Zth[2];
    bool	  driving[2];
    PinModule	  *output_pin[2];
    IOPIN 	  *pin = nullptr;
    Stimulus_Node *node_dacout;
    stimulus 	  *volt_dacout;
    std::string   name_dacout;
};


class DACCON1 : public sfr_register
{
public:
    DACCON1(Processor *, const char *pName, const char *pDesc = nullptr, unsigned int bitMask = 0x1f, DACCON0 *_daccon0 = nullptr);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    void set_daccon0(DACCON0 *_daccon0) { daccon0 = _daccon0; }

private:
    unsigned int bit_mask;
    DACCON0	*daccon0;
};


// Differential input and trigger conversion start
class ADCON2_DIF : public sfr_register, public TriggerObject
{
public:

    enum
    {
        CHSNS0   = 1 << 0,
        CHSNS1   = 1 << 1,
        CHSNS2   = 1 << 2,
        CHSNS3   = 1 << 3,
        TRIGSEL0 = 1 << 4,
        TRIGSEL1 = 1 << 5,
        TRIGSEL2 = 1 << 6,
        TRIGSEL3 = 1 << 7,
    };

    ADCON2_DIF(Processor *pCpu, const char *pName, const char *pDesc);
};


#ifdef RRR
// A2D up to 32 channels
class ADCON0_32 : public ADCON0
{
    enum
    {
        ADON = 1 << 0,
        GO   = 1 << 1,
        CHS0 = 1 << 2,
        CHS1 = 1 << 3,
        CHS2 = 1 << 4,
        CHS3 = 1 << 5,
        CHS4 = 1 << 6,
    };

public:
    ADCON0_32(Processor *pCpu, const char *pName, const char *pDesc);
    void put(unsigned int new_value) override;
    void put_conversion() override;
};


#endif //RRR
//
//
// Trigger conversion start (16f1503)
class ADCON2_TRIG : public sfr_register
{
public:

    enum
    {
        TRIGSEL0 = 1 << 4,
        TRIGSEL1 = 1 << 5,
        TRIGSEL2 = 1 << 6,
        TRIGSEL3 = 1 << 7,
    };

    ADCON2_TRIG(Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    void setValidBits(unsigned int mask)
    {
        valid_bits = mask;
    }
    void setAdcon0(ADCON0 *_adcon0)
    {
        m_adcon0 = _adcon0;
    }
    void setCMxsync(unsigned int cm, bool output);
    void t0_overflow();

private:
    unsigned int 	valid_bits;
    bool 		CMxsync[4];
    ADCON0	*m_adcon0 = nullptr;
};

#endif // SRC_A2DCONVERTER_H_
