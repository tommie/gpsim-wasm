/*
   Copyright (C) 2006 T. Scott Dattalo
   Copyright (C) 2009, 2013, 2017 Roy R. Rankin

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

#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <string>

#include "a2dconverter.h"

#include "14bit-processors.h"
#include "gpsim_time.h"
#include "intcon.h"
#include "ioports.h"
#include "pir.h"
#include "processor.h"
#include "trace.h"
#include "ui.h"

#define p_cpu ((Processor *)get_module())

static PinModule AnInvalidAnalogInput;

//#define DEBUG
#if defined(DEBUG)
#include <config.h>
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//------------------------------------------------------
// ADCON0
//
ADCON0::ADCON0(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc),
      ad_state(AD_IDLE),
      channel_mask(7), channel_shift(3), GO_bit(GO)
{
}


/*
 * Link PIC register for High Byte A/D result
 */
void ADCON0::setAdres(sfr_register *new_adres)
{
    adres = new_adres;
}


/*
 * Link PIC register for Low Byte A/D result
 */
void ADCON0::setAdresLow(sfr_register *new_adresl)
{
    adresl = new_adresl;
}


/*
 * Link PIC register for ADCON1
 */
void ADCON0::setAdcon1(ADCON1 *new_adcon1)
{
    adcon1 = new_adcon1;
}


/*
 * Link PIC register for INTCON
 */
void ADCON0::setIntcon(INTCON *new_intcon)
{
    intcon = new_intcon;
}


/*
 * Link PIC register for PIR
 * If set ADIF in PIR otherwise ADIF in ADCON0
 */
void ADCON0::setPir(PIR *pPir)
{
    m_pPir = pPir;
}


/*
 * Set resolution of A2D converter
 */
void ADCON0::setA2DBits(unsigned int nBits)
{
    m_A2DScale = (1 << nBits) - 1;
    m_nBits = nBits;
}


void ADCON0::start_conversion()
{
    Dprintf(("starting A/D conversion at 0x%" PRINTF_GINT64_MODIFIER "x\n", get_cycles().get()));

    if (!(value.get() & ADON))
    {
        Dprintf((" A/D converter is disabled\n"));
        stop_conversion();
        return;
    }

    put(value.get() | GO_bit);
    uint64_t fc = get_cycles().get() + (2 * Tad) /
                 p_cpu->get_ClockCycles_per_Instruction();
    Dprintf(("ad_state %u fc %" PRINTF_GINT64_MODIFIER "x now %" PRINTF_GINT64_MODIFIER "x\n", ad_state, fc, get_cycles().get()))

    if (ad_state != AD_IDLE)
    {
        // The A/D converter is either 'converting' or 'acquiring'
        // in either case, there is callback break that needs to be moved.
        stop_conversion();
        get_cycles().reassign_break(future_cycle, fc, this);

    }
    else
    {
        get_cycles().set_break(fc, this);
    }

    future_cycle = fc;
    ad_state = AD_ACQUIRING;
}


void ADCON0::stop_conversion()
{
    Dprintf(("stopping A/D conversion\n"));
    ad_state = AD_IDLE;
}


void ADCON0::set_Tad(unsigned int new_value)
{
    // Get the A/D Conversion Clock Select bits
    //
    // This switch case will get the ADCS bits and set the Tad, or The A/D
    // converter clock period. Tad is the number of the oscillator periods
    //  rather instruction cycle periods. ADCS2 only exists on some processors,
    // but should be 0 where it is not used.
    switch (new_value & (ADCS0 | ADCS1))
    {
    case 0:
        Tad = (adcon1->value.get() & ADCON1::ADCS2) ? 4 : 2;
        break;

    case ADCS0:
        Tad = (adcon1->value.get() & ADCON1::ADCS2) ? 16 : 8;
        break;

    case ADCS1:
        Tad = (adcon1->value.get() & ADCON1::ADCS2) ? 64 : 32;
        break;

    case (ADCS0|ADCS1):	// typical 4 usec, convert to osc cycles
        if (p_cpu)
        {
            Tad = (unsigned int)(4.e-6  * p_cpu->get_frequency());
            Tad = Tad < 2 ? 2 : Tad;

        }
        else
        {
            Tad = 6;
        }

        break;
    }
}


void ADCON0::put(unsigned int new_value)
{
    emplace_value_trace<trace::WriteRegisterEntry>();
    set_Tad(new_value);
    unsigned int old_value = value.get();
    // SET: Reflect it first!
    value.put(new_value);

    if (new_value & ADON)
    {
        // The A/D converter is being turned on (or maybe left on)
        if ((new_value & ~old_value) & GO_bit)
        {
            if (verbose)
            {
                printf("starting A2D conversion\n");
            }

            Dprintf(("starting A2D conversion\n"));
            // The 'GO' bit is being turned on, which is request to initiate
            // and A/D conversion
            start_conversion();
        }

    }
    else
    {
        stop_conversion();
    }
}


void ADCON0::put_conversion()
{
    double dRefSep = m_dSampledVrefHi - m_dSampledVrefLo;
    double dNormalizedVoltage;
    dNormalizedVoltage = (dRefSep > 0.0) ?
                         (m_dSampledVoltage - m_dSampledVrefLo) / dRefSep : 0.0;
    dNormalizedVoltage = dNormalizedVoltage > 1.0 ? 1.0 : dNormalizedVoltage;
    unsigned int converted = (unsigned int)(m_A2DScale * dNormalizedVoltage + 0.5);
    Dprintf(("put_conversion: Vrefhi:%.4f Vreflo:%.4f conversion:%u normV:%f\n",
             m_dSampledVrefHi, m_dSampledVrefLo, converted, dNormalizedVoltage));

    if (verbose)
    {
        printf("result=0x%02x\n", converted);
    }

    Dprintf(("%u-bit result 0x%x ADFM %d\n", m_nBits, converted, get_ADFM()));

    if (adresl)    // non-null for more than 8 bit conversion
    {
        if (get_ADFM())
        {
            adresl->put(converted & 0xff);
            adres->put((converted >> 8) & 0x3);

        }
        else
        {
            adresl->put((converted << 6) & 0xc0);
            adres->put((converted >> 2) & 0xff);
        }

    }
    else
    {
        adres->put((converted) & 0xff);
    }
}


// ADCON0 callback is called when the cycle counter hits the break point that
// was set in ADCON0::put.

void ADCON0::callback()
{
    int channel;
    Dprintf((" ADCON0 Callback: 0x%" PRINTF_GINT64_MODIFIER "x ad_state=0x%x\n", get_cycles().get(), ad_state));

    //
    // The a/d converter is simulated with a state machine.
    //

    switch (ad_state)
    {
    case AD_IDLE:
        Dprintf(("ignoring ad callback since ad_state is idle\n"));
        break;

    case AD_ACQUIRING:
        channel = (value.get() >> channel_shift) & channel_mask;
        m_dSampledVoltage = getChannelVoltage(channel);
        m_dSampledVrefHi  = getVrefHi();
        m_dSampledVrefLo  = getVrefLo();
        Dprintf(("Acquiring channel:%d V=%g reflo=%g refhi=%g\n",
                 channel, m_dSampledVoltage, m_dSampledVrefLo, m_dSampledVrefHi));
        future_cycle = get_cycles().get() + (m_nBits * Tad) / p_cpu->get_ClockCycles_per_Instruction();
        get_cycles().set_break(future_cycle, this);

        if (verbose)
            printf("A/D %u bits channel:%d Vin=%.4f Refhi=%.4f Reflo=%.4f ", m_nBits,
                   channel, m_dSampledVoltage, m_dSampledVrefHi, m_dSampledVrefLo);

        ad_state = AD_CONVERTING;
        break;

    case AD_CONVERTING:
        put_conversion();
        // Clear the GO/!DONE flag.
        value.put(value.get() & (~GO_bit));
        set_interrupt();
        ad_state = AD_IDLE;
    }
}


//------------------------------------------------------
// If the ADIF bit is in the PIR1 register, call setPir()
// in the ADC setup. Otherwise, ADIF is assumed to be in
// the ADCON0 register
//
// Chips like 10f220 do not have interupt and intcon is not defined.
// Thus no interrupt needs be generated
//
void ADCON0::set_interrupt()
{
    if (m_pPir)
    {
        m_pPir->set_adif();

    }
    else if (intcon)
    {
        value.put(value.get() | ADIF);
        intcon->peripheral_interrupt();
    }
}


double ADCON0_91X::getVrefHi()
{
    if (value.get() & VCFG0)
    {
        return getChannelVoltage(Vrefhi_position);

    }
    else
    {
        return p_cpu->get_Vdd();
    }
}


double ADCON0_91X::getVrefLo()
{
    if (value.get() & VCFG1)
    {
        return getChannelVoltage(Vreflo_position);

    }
    else
    {
        return 0.0;
    }
}


//------------------------------------------------------
// ADCON0
//
ADCON0_DIF::ADCON0_DIF(Processor *pCpu, const char *pName, const char *pDesc)
    : ADCON0(pCpu, pName, pDesc), adcon2(nullptr)
{
}


void ADCON0_DIF::put(unsigned int new_value)
{
    emplace_value_trace<trace::WriteRegisterEntry>();

    if (new_value & ADRMD)  	// 10 Bit
    {
        setA2DBits(10);

    }
    else
    {
        setA2DBits(12);
    }

    set_Tad(new_value);
    unsigned int old_value = value.get();
    // SET: Reflect it first!
    value.put(new_value);

    if (new_value & ADON)
    {
        // The A/D converter is being turned on (or maybe left on)
        if ((new_value & ~old_value) & GO_bit)
        {
            if (verbose)
            {
                printf("starting A2D conversion\n");
            }

            Dprintf(("starting A2D conversion\n"));
            // The 'GO' bit is being turned on, which is request to initiate
            // and A/D conversion
            start_conversion();
        }

    }
    else
    {
        stop_conversion();
    }
}


void ADCON0_DIF::put_conversion(void)
{
    int channel = adcon2->value.get() & 0x0f;
    double dRefSep = m_dSampledVrefHi - m_dSampledVrefLo;
    double dNormalizedVoltage;
    double m_dSampledVLo;

    if (channel == 0x0e)   // shift AN21 to adcon0 channel
    {
        channel = 0x15;
    }

    if (channel == 0x0f)  	// use ADNREF for V-
    {
        m_dSampledVLo = getVrefLo();

    }
    else
    {
        m_dSampledVLo = getChannelVoltage(channel);
    }

    dNormalizedVoltage = (m_dSampledVoltage - m_dSampledVLo) / dRefSep;
    dNormalizedVoltage = dNormalizedVoltage > 1.0 ? 1.0 : dNormalizedVoltage;
    int converted = (int)(m_A2DScale * dNormalizedVoltage + 0.5);
    Dprintf(("put_conversion: V+:%g V-:%g Vrefhi:%g Vreflo:%g conversion:%d normV:%g\n",
             m_dSampledVoltage, m_dSampledVLo,
             m_dSampledVrefHi, m_dSampledVrefLo, converted, dNormalizedVoltage));

    if (verbose)
    {
        printf("result=0x%02x\n", converted);
    }

    Dprintf(("%u-bit result 0x%x ADFM %d\n", m_nBits, converted, get_ADFM()));

    if (!get_ADFM())   // signed
    {
        int sign = 0;

        if (converted < 0)
        {
            sign = 1;
            converted = -converted;
        }

        converted = ((converted << (16 - m_nBits)) % 0xffff) | sign;
    }

    adresl->put(converted & 0xff);
    adres->put((converted >> 8) & 0xff);
}


ADCON1_16F::ADCON1_16F(Processor *pCpu, const char *pName, const char *pDesc)
    : ADCON1(pCpu, pName, pDesc)
{
    valid_bits = 0x70;
}


void ADCON1_16F::put_value(unsigned int new_value)
{
    unsigned int masked_value = new_value & valid_bits;
    unsigned int Tad = 6;
    setADCnames();

    switch (masked_value & (ADCS0 | ADCS1))
    {
    case 0:
        Tad = (new_value & ADCS2) ? 4 : 2;
        break;

    case ADCS0:
        Tad = (new_value & ADCS2) ? 16 : 8;
        break;

    case ADCS1:
        Tad = (new_value & ADCS2) ? 64 : 32;
        break;

    case (ADCS0|ADCS1):	// typical 4 usec, convert to osc cycles
        if (p_cpu)
        {
            Tad = (unsigned int)(4.e-6  * p_cpu->get_frequency());
            Tad = Tad < 2 ? 2 : Tad;

        }
        else
        {
            Tad = 6;
        }

        break;
    }

    adcon0->set_Tad(Tad);

    if (ADFM & valid_bits)
    {
        adfm = ADFM & masked_value;
    }

    //RRR FIXME handle ADPREF
    value.put(new_value & valid_bits);
}


double ADCON1_16F::getVrefLo()
{
    if (ADNREF & value.get())
    {
        if (Vreflo_position[cfg_index] < m_nAnalogChannels)
        {
            return getChannelVoltage(Vreflo_position[cfg_index]);
        }

        std::cerr << "WARNING Vreflo pin not configured\n";
        return -1.0;
    }

    return 0.0;	//Vss
}


double ADCON1_16F::getVrefHi()
{
    if (ADPREF0 & valid_bits)
    {
        unsigned int mode = value.get() & (ADPREF1 | ADPREF0);

	// Positive reference voltage
        switch (mode)
        {
        case 0:	//Vdd
            return p_cpu->get_Vdd();

        case 1:  // reserved
            std::cerr << "*** WARNING " << __FUNCTION__ <<" reserved value for ADPREF\n";
            return -1.0;

        case 2:  // Vref+ IO pin
            if (Vrefhi_position[cfg_index] < m_nAnalogChannels)
            {
                return getChannelVoltage(Vrefhi_position[cfg_index]);
            }

            std::cerr << "*** WARNING Vrefhi pin not configured\n";
            return -1.0;

        case 3:	// FVR buffer 1
            if (FVR_chan < m_nAnalogChannels)
            {
                Dprintf(("%s FVR_chan =%u V=%.3f\n", __FUNCTION__, FVR_chan, getChannelVoltage(FVR_chan)));
                return getChannelVoltage(FVR_chan);
            }

            std::cerr << "*** WARNING " << __FUNCTION__ <<" FVR_chan not set " << FVR_chan << " "<< name() <<"\n";
            return -1.0;
        };
    }


    if (Vrefhi_position[cfg_index] < m_nAnalogChannels)
    {
        return getChannelVoltage(Vrefhi_position[cfg_index]);
    }

    return p_cpu->get_Vdd();
}
void ADCON1_16F::setVoltRef(unsigned int chan, float v)
{
    if (chan != FVR_chan)
	m_voltageRef[chan] = v;
}


//------------------------------------------------------
// ADCON1
//
ADCON1::ADCON1(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), FVR_ATTACH(pName), DAC_ATTACH(pName),
	valid_bits(0xff), FVR_chan(99)
{
    for (int i = 0; i < (int)cMaxConfigurations; i++)
    {
        setChannelConfiguration(i, 0);
        setVrefLoConfiguration(i, 0xffff);
        setVrefHiConfiguration(i, 0xffff);
    }
}


ADCON1::~ADCON1()
{

    delete [] m_voltageRef;

    if (m_AnalogPins)
    {
        if (m_ad_in_ctl)
        {
            for (unsigned int i = 0; i < m_nAnalogChannels; i++)
            {
                m_AnalogPins[i]->setControl(0);
            }

            delete m_ad_in_ctl;
        }

        delete [] m_AnalogPins;
    }
}


void ADCON1::put(unsigned int new_value)
{
    unsigned int masked_value = new_value & valid_bits;
    emplace_value_trace<trace::WriteRegisterEntry>();
    put_value(masked_value);
}


void ADCON1::put_value(unsigned int new_value)
{
    unsigned int masked_value = new_value & valid_bits;
    cfg_index = get_cfg(masked_value);
    setADCnames();
    adfm = masked_value & ADFM;
    value.put(masked_value);
}


// Obtain new mIoMask and set pin names as per function
void ADCON1::setADCnames()
{
    unsigned int new_mask = m_configuration_bits[cfg_index];
    unsigned int diff = mIoMask ^ new_mask;
    Dprintf(("ADCON1::setADCnames - cfg_index=%u new_mask %02X channels %u\n",
             cfg_index, new_mask, m_nAnalogChannels));
    char newname[20];

    for (unsigned int i = 0; i < m_nAnalogChannels; i++)
    {
        if ((diff & (1 << i)) && m_AnalogPins[i] != &AnInvalidAnalogInput)
        {
            if (new_mask & (1 << i))
            {
                snprintf(newname, sizeof(newname), "an%u", i);
                m_AnalogPins[i]->AnalogReq(this, true, newname);

            }
            else
            {
                m_AnalogPins[i]->AnalogReq(this, false, m_AnalogPins[i]->getPin()->name().c_str());
            }
        }
    }

    mIoMask = new_mask;
}


/*
 * If A2D uses PCFG, call for each PCFG value (cfg 0 to 15) with
 * each set bit of bitMask indicating port is an analog port
 * (either A2D input port or Vref). Processors which use an A2D
 * method that uses ANSEL register will not call this.
 *
 * As an example, for the following Port Configuration Control bit:
 * PCFG   AN7   AN6   AN5   AN4   AN3   AN2   AN1   AN0
 * ----   ---- ----- -----  ----- ----- ----- ----- -----
 * 1100   D    D     D      A     Vref+ Vref- A     A
 *
 *  then call setChannelConfiguration with cfg = 12 , bitMask = 0x1f
 * */
void ADCON1::setChannelConfiguration(unsigned int cfg, unsigned int bitMask)
{
    if (cfg < cMaxConfigurations)
    {
        m_configuration_bits[cfg] = bitMask;
    }
}


unsigned int ADCON1::getVrefLoChannel(unsigned int cfg)
{
    if (cfg < cMaxConfigurations)
    {
        return Vreflo_position[cfg];
    }

    return 0xffff;
}


unsigned int ADCON1::getVrefHiChannel(unsigned int cfg)
{
    Dprintf(("ADCON1::getVrefHiChannel cfg=%u %u\n", cfg, Vrefhi_position[cfg]));

    if (cfg < cMaxConfigurations)
    {
        return Vrefhi_position[cfg];
    }

    return 0xffff;
}


/*
 * Call for each configuration mode that uses an І/O pin as Vref-
 * to declare which port is being used for this.
 */
void ADCON1::setVrefLoConfiguration(unsigned int cfg, unsigned int channel)
{
    if (cfg < cMaxConfigurations)
    {
        Vreflo_position[cfg] = channel;
    }
}


/*
 * Call for each configuration mode that uses an І/O pin as Vref+
 * to declare which port is being used for this.
 */
void ADCON1::setVrefHiConfiguration(unsigned int cfg, unsigned int channel)
{
    if (cfg < cMaxConfigurations)
    {
        Vrefhi_position[cfg] = channel;
    }
}


/*
 * Number of A2D channels processor supports
 */
void ADCON1::setNumberOfChannels(unsigned int nChannels)
{
    PinModule **save = nullptr;	// save existing pins when nChannels increases

    if (!nChannels || nChannels <= m_nAnalogChannels)
    {
        return;  // do nothing if NChannels decreases
    }

    if (m_nAnalogChannels && nChannels > m_nAnalogChannels)
    {
        save = m_AnalogPins;
    }

    delete [] m_voltageRef;

    m_voltageRef = new float [nChannels];
    m_AnalogPins = new PinModule *[nChannels];

    for (unsigned int i = 0; i < nChannels; i++)
    {
        m_voltageRef[i] = -1.0;

        if (i < m_nAnalogChannels)
        {
            if (save)
            {
                m_AnalogPins[i] = save[i];
            }

        }
        else
        {
            m_AnalogPins[i] = &AnInvalidAnalogInput;
        }
    }

    delete [] save;

    m_nAnalogChannels = nChannels;
}


/*
 * Configure use of adcon1 register
 * 	The register is first anded with mask and then shifted
 * 	right shift bits. The result being either PCFG or VCFG
 * 	depending on the type of a2d being used.
 */
void ADCON1::setValidCfgBits(unsigned int mask, unsigned int shift)
{
    mValidCfgBits = mask;
    mCfgBitShift = shift;
}


unsigned int ADCON1::get_adc_configmask(unsigned int reg)
{
    return m_configuration_bits[get_cfg(reg)];
}


int ADCON1::get_cfg(unsigned int reg)
{
    return ((reg & mValidCfgBits) >> mCfgBitShift);
}


/*
 * Associate a processor I/O pin with an A2D channel
 */
void ADCON1::setIOPin(unsigned int channel, PinModule *newPin)
{
    if (channel < m_nAnalogChannels &&
            m_AnalogPins[channel] == &AnInvalidAnalogInput && newPin != 0)
    {
        m_AnalogPins[channel] = newPin;

    }
    else
    {
        printf("%s:%d WARNING invalid channel number config for ADCON1 %u num %u\n", __FILE__, __LINE__, channel, m_nAnalogChannels);
    }
}

void ADCON1::setVoltRef(unsigned int channel, float in_value)
{
    if (channel < m_nAnalogChannels)
    {
        m_voltageRef[channel] = in_value;

    }
    else
    {
        printf("ADCON1::%s invalid channel number %u\n", __FUNCTION__, channel);
    }
}

void ADCON1::set_FVR_volt(double in_value, unsigned int channel)
{
    Dprintf(("set_FVR_volt %s %.3f %u\n", name().c_str(), in_value, channel));
    if (channel < m_nAnalogChannels)
    {
        m_voltageRef[channel] = in_value;

    }
    else
    {
        fprintf(stderr, "ADCON1::%s invalid channel number %u\n", __FUNCTION__, channel);
    }
}


//------------------------------------------------------
double ADCON1::getChannelVoltage(unsigned int channel)
{
    double voltage = 0.0;

    if (channel < m_nAnalogChannels)
    {
        if ((1 << channel) & m_configuration_bits[cfg_index])
        {
            PinModule *pm = m_AnalogPins[channel];

            if (pm != &AnInvalidAnalogInput)
            {
                voltage = pm->getPin()->get_nodeVoltage();

            }
            else
            {
                std::cerr << "ADCON1::getChannelVoltage channel " << channel <<
                          " not valid analog input\n";
                std::cerr << "Please raise a Gpsim bug report\n";
            }

        }
        else  	// use voltage reference
        {
            Dprintf(("ADCON1::getChannelVoltage channel=%u voltage %f\n", \
                     channel, m_voltageRef[channel]));
            voltage = m_voltageRef[channel];

            if (voltage < 0.0)
            {
                std::cout << "ADCON1::getChannelVoltage channel " << channel <<
                          " not a configured input\n";
                voltage = 0.0;
            }
        }

    }
    else
    {
        std::cerr << "ADCON1::getChannelVoltage channel " << channel <<
                  " >= "
                  << m_nAnalogChannels << " (number of channels)\n";
        std::cerr << "Please raise a Gpsim bug report\n";
    }

    return voltage;
}


double ADCON1::getVrefHi()
{
    if (Vrefhi_position[cfg_index] < m_nAnalogChannels)
    {
        return getChannelVoltage(Vrefhi_position[cfg_index]);
    }

    return p_cpu->get_Vdd();
}


double ADCON1::getVrefLo()
{
    if (Vreflo_position[cfg_index] < m_nAnalogChannels)
    {
        return getChannelVoltage(Vreflo_position[cfg_index]);
    }

    return 0.0;
}


//	if on is true, set pin as input regardless of TRIS state
//	else restore TRIS control
//
void ADCON1::set_channel_in(unsigned int channel, bool on)
{
    Dprintf(("channel=%u on=%d m_ad_in_ctl=%p\n", channel, on, m_ad_in_ctl));

    if (on && (m_ad_in_ctl == nullptr))
    {
        m_ad_in_ctl = new AD_IN_SignalControl();
    }

    if (on)
    {
        m_AnalogPins[channel]->setControl(m_ad_in_ctl);

    }
    else
    {
        m_AnalogPins[channel]->setControl(0);
    }

    m_AnalogPins[channel]->updatePinModule();
}

void ADCON1::set_DAC_volt(double voltage, unsigned int chan)
{
    Dprintf(("ADCON1::set_DAC_volt v=%.3f chan=%u\n", voltage, chan));
    m_voltageRef[chan] = voltage;
}

ANSEL::ANSEL(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), valid_bits(0x7f)
{
}


void ANSEL::setAdcon1(ADCON1 *new_adcon1)
{
    adcon1 = new_adcon1;
}


void ANSEL::put(unsigned int new_value)
{
    assert(adcon1);
    unsigned int cfgmax = adcon1->getNumberOfChannels();
    unsigned int i;
    unsigned int mask = (new_value & valid_bits);

    if (anselh)
    {
        mask |= anselh->value.get() << 8;
    }

    emplace_value_trace<trace::WriteRegisterEntry>();

    /*
    Generate ChannelConfiguration from ansel register
    */
    for (i = 0; i < cfgmax; i++)
    {
        adcon1->setChannelConfiguration(i, mask);
    }

    value.put(new_value & valid_bits);
    adcon1->setADCnames();
}


ANSEL_H::ANSEL_H(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), valid_bits(0x3f)
{
}


void ANSEL_H::setAdcon1(ADCON1 *new_adcon1)
{
    adcon1 = new_adcon1;
}


void ANSEL_H::put(unsigned int new_value)
{
    unsigned int cfgmax = adcon1->getNumberOfChannels();
    unsigned int mask = ((new_value & valid_bits) << 8) ;
    emplace_value_trace<trace::WriteRegisterEntry>();

    if (ansel)
    {
        mask |= ansel->value.get();
    }

    /*
    Generate ChannelConfiguration from ansel register
    */
    for (unsigned int i = 0; i < cfgmax; i++)
    {
        adcon1->setChannelConfiguration(i, mask);
    }

    value.put(new_value & valid_bits);
    adcon1->setADCnames();
}


ANSEL_P::ANSEL_P(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), valid_bits(0x3f)
{
}


void ANSEL_P::setAdcon1(ADCON1 *new_adcon1)
{
    adcon1 = new_adcon1;
}

void ANSEL_P::setAnsel(ANSEL_P *new_ansel)
{
    ansel = new_ansel;
    auto it = std::find(ansel_list.begin(), ansel_list.end(), new_ansel);
    if (it == ansel_list.end())
    {
        ansel_list.push_back(new_ansel);
    }
}


void ANSEL_P::put(unsigned int new_value)
{
    unsigned int i;
    unsigned int chan = first_channel;
    emplace_value_trace<trace::WriteRegisterEntry>();
    new_value &= valid_bits;
    value.put(new_value);
    cfg_mask = 0;

    for (i = 0; i < 8; i++)
    {
        if ((1 << i) & analog_pins)
        {
            if ((1 << i) & new_value)
            {
                cfg_mask |= (1 << chan);
            }

            chan++;
        }
    }

    if (!adcon1)
    {
        return;
    }

    unsigned int mask = cfg_mask;
    for (auto &an_ansel : ansel_list)
    {
        mask |= an_ansel->get_mask();
    }

    /*
    Generate ChannelConfiguration from ansel register
    */
    unsigned int cfgmax = adcon1->getMaxCfg();
    for (i = 0; i < cfgmax; i++)
    {
        adcon1->setChannelConfiguration(i, mask);
    }

    adcon1->setADCnames();
}


//------------------------------------------------------
// ADCON0_10
//
ADCON0_10::ADCON0_10(Processor *pCpu, const char *pName, const char *pDesc)
    : ADCON0(pCpu, pName, pDesc)
{
    GO_bit = GO;	//ADCON0 and ADCON0_10 have GO flag at different bit
    // It should take 13 CPU instructions to complete conversion
    // Tad of 6 completes in 15
    Tad = 6;
}


void ADCON0_10::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    /* On first call of this function old_value has already been set to
    *  it's default value, but we want to call set_channel_in. First
    *  gives us a way to do this.
    */
    static bool first = true;
    emplace_value_trace<trace::WriteRegisterEntry>();
    Dprintf(("ADCON0_10::put new_value=0x%02x old_value=0x%02x\n", new_value, old_value));

    if ((new_value ^ old_value) & ANS0 || first)
    {
        adcon1->set_channel_in(0, (new_value & ANS0) == ANS0);
    }

    if ((new_value ^ old_value) & ANS1 || first)
    {
        adcon1->set_channel_in(1, (new_value & ANS1) == ANS1);
    }

    first = false;

    // If ADON is clear GO cannot be set
    if ((new_value & ADON) != ADON)
    {
        new_value &= ~GO_bit;
    }

    // SET: Reflect it first!
    value.put(new_value);

    if (new_value & ADON)
    {
        // The A/D converter is being turned on (or maybe left on)
        if ((new_value & ~old_value) & GO_bit)
        {
            if (verbose)
            {
                printf("starting A2D conversion\n");
            }

            // The 'GO' bit is being turned on, which is request to initiate
            // and A/D conversion
            start_conversion();
        }

    }
    else
    {
        stop_conversion();
    }
}


//------------------------------------------------------
// ADCON0_12F used in 12f675. Uses ADCON1 as virtual rather than physical
// register
//
ADCON0_12F::ADCON0_12F(Processor *pCpu, const char *pName, const char *pDesc)
    : ADCON0(pCpu, pName, pDesc)
{
    GO_bit = GO;	//ADCON0 and ADCON0_10 have GO flag at different bit
    valid_bits = 0xdf;
}


void ADCON0_12F::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    new_value &= valid_bits;	// clear unused bits
    /* On first call of this function old_value has already been set to
    *  it's default value, but we want to call set_channel_in. First
    *  gives us a way to do this.
    */
    emplace_value_trace<trace::WriteRegisterEntry>();
    Dprintf(("ADCON0_12F::put new_value=0x%02x old_value=0x%02x\n", new_value, old_value));
    // tell adcon1 to use Vref
    adcon1->set_cfg_index((new_value & VCFG) ? 2 : 0);

    // If ADON is clear GO cannot be set
    if ((new_value & ADON) != ADON)
    {
        new_value &= ~GO_bit;
    }

    // SET: Reflect it first!
    value.put(new_value);

    if (new_value & ADON)
    {
        // The A/D converter is being turned on (or maybe left on)
        if ((new_value & ~old_value) & GO_bit)
        {
            if (verbose)
            {
                printf("starting A2D conversion\n");
            }

            // The 'GO' bit is being turned on, which is request to initiate
            // and A/D conversion
            start_conversion();
        }

    }
    else
    {
        stop_conversion();
    }
}


ANSEL_12F::ANSEL_12F(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
}


void ANSEL_12F::set_tad(unsigned int new_value)
{
    unsigned int Tad = 6;

    switch (new_value & (ADCS0 | ADCS1))
    {
    case 0:
        Tad = (new_value & ADCS2) ? 4 : 2;
        break;

    case ADCS0:
        Tad = (new_value & ADCS2) ? 16 : 8;
        break;

    case ADCS1:
        Tad = (new_value & ADCS2) ? 64 : 32;
        break;

    case (ADCS0|ADCS1):	// typical 4 usec, convert to osc cycles
        if (p_cpu)
        {
            Tad = (unsigned int)(4.e-6  * p_cpu->get_frequency());
            Tad = Tad < 2 ? 2 : Tad;

        }
        else
        {
            Tad = 6;
        }

        break;
    }

    Dprintf(("ANSEL_12F::set_tad %x Tad=%u\n", new_value, Tad));
    adcon0->set_Tad(Tad);
}


void ANSEL_12F::put(unsigned int new_value)
{
    unsigned int cfgmax = adcon1->getNumberOfChannels();
    unsigned int i;
    Dprintf(("ANSEL_12F::put %x cfgmax %u\n", new_value, cfgmax));
    emplace_value_trace<trace::WriteRegisterEntry>();

    /*
    Generate ChannelConfiguration from ansel register
    */
    for (i = 0; i < cfgmax; i++)
    {
        unsigned int mask = new_value & 0x0f;
        adcon1->setChannelConfiguration(i, mask);
    }

    /*
    * 	Convert A2D conversion times and set in adcon
    */
    set_tad(new_value & (ADCS2 | ADCS1 | ADCS0));
    value.put(new_value & 0x7f);
    adcon1->setADCnames();
}


//------------------------------------------------------
// ADCON0_32X used in 10f32x. Uses ADCON1 as virtual rather than physical
// register
//
ADCON0_32X::ADCON0_32X(Processor *pCpu, const char *pName, const char *pDesc)
    : ADCON0(pCpu, pName, pDesc)
{
    GO_bit = GO;	//ADCON0 and ADCON0_10 have GO flag at different bit
    valid_bits = 0xff;
}


void ADCON0_32X::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    new_value &= valid_bits;	// clear unused bits
    /* On first call of this function old_value has already been set to
    *  it's default value, but we want to call set_channel_in. First
    *  gives us a way to do this.
    */
    emplace_value_trace<trace::WriteRegisterEntry>();
    Dprintf(("ADCON0_32X::put new_value=0x%02x old_value=0x%02x\n", new_value, old_value));
    // tell adcon1 to use Vref
    //RRR adcon1->set_cfg_index((new_value & VCFG) ? 2: 0);

    switch (new_value & (ADCS0 | ADCS1 | ADCS2))
    {
    case (ADCS0|ADCS1):
    case 0:
        Tad = 2;
        break;

    case ADCS0:
        Tad = 8;
        break;

    case ADCS1:
        Tad = 32;
        break;
    }

    if (new_value & ADCS2)
    {
        Tad *= 2;
    }

    // If ADON is clear GO cannot be set
    if ((new_value & ADON) != ADON)
    {
        new_value &= ~GO_bit;
    }

    // SET: Reflect it first!
    value.put(new_value);

    if (new_value & ADON)
    {
        // The A/D converter is being turned on (or maybe left on)
        if ((new_value & ~old_value) & GO_bit)
        {
            if (verbose)
            {
                printf("starting A2D conversion\n");
            }

            // The 'GO' bit is being turned on, which is request to initiate
            // and A/D conversion
            start_conversion();
        }

    }
    else
    {
        stop_conversion();
    }
}


// catch stimulus and set channel voltage
//
a2d_stimulus::a2d_stimulus(ADCON1 * arg, int chan, const char *cPname, double _Vth, double _Zth)
    : stimulus(cPname, _Vth, _Zth)
{
    _adcon1 = arg;
    channel = chan;
}


void   a2d_stimulus::set_nodeVoltage(double v)
{
    Dprintf(("nodeVoltage =%.1f\n", v));
    nodeVoltage = v;
    _adcon1->setVoltRef(channel, v);
}


//
//--------------------------------------------------
// member functions for the FVRCON class
//--------------------------------------------------
//
FVRCON::FVRCON(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask)
    : sfr_register(pCpu, pName, pDesc), node_cvref(nullptr),
       node_adcvref(nullptr), volt_cvref(nullptr), volt_adcvref(nullptr)
{
    mask_writable = bitMask;
    node_cvref = new Stimulus_Node("n_cvref");
    volt_cvref = new stimulus("cdafvr_src", 0.0, 48000.0);
    node_cvref->attach_stimulus(volt_cvref);
    node_adcvref = new Stimulus_Node("n_adcvref");
    volt_adcvref = new stimulus("adcfvr_src", 0.0, 48000.0);
    node_adcvref->attach_stimulus(volt_adcvref);
    node_Vtref = new Stimulus_Node("n_Vtref");
    volt_Vtref = new stimulus("Vtfvr_src", 0.0, 48000.0);
    node_Vtref->attach_stimulus(volt_Vtref);
}

FVRCON::~FVRCON()
{
    node_cvref->detach_stimulus(volt_cvref);
    delete(volt_cvref);
    delete(node_cvref);
    node_adcvref->detach_stimulus(volt_adcvref);
    delete(volt_adcvref);
    delete(node_adcvref);
    node_Vtref->detach_stimulus(volt_Vtref);
    delete(volt_Vtref);
    delete(node_Vtref);
}


void FVRCON::put(unsigned int new_value)
{
    unsigned int masked_value = (new_value & mask_writable);
    emplace_value_trace<trace::WriteRegisterEntry>();
    put_value(masked_value);

    compute_VTemp(masked_value);
    compute_FVR_AD(masked_value);
    compute_FVR_CDA(masked_value);
}


void FVRCON::put_value(unsigned int new_value)
{
     unsigned int diff = value.get() ^ new_value;

  if (diff) {
    if (diff &  FVREN) {
      new_value &= ~FVRRDY;  // Clear FVRRDY regardless of ON or OFF
    }

    if (new_value & FVREN) {    // Enable ON?
      future_cycle = get_cycles().get() + 25e-6 / get_cycles().seconds_per_cycle();
      get_cycles().set_break(future_cycle, this);

    } else if (future_cycle) {
      get_cycles().clear_break(this);
      future_cycle = 0;
    }
  }
  value.put(new_value);
  compute_VTemp(new_value);
  compute_FVR_AD(new_value);
  compute_FVR_CDA(new_value);
}

// Set FVRRDY bit after timeout
void FVRCON::callback()
{
  future_cycle = 0;
  put_value(value.get() | FVRRDY);
}

void FVRCON::callback_print()
{
    std::cout <<  name() << " has callback, ID = " << CallBackID << '\n';
}


double FVRCON::compute_VTemp(unsigned int fvrcon)
{
    double ret = -1.0;

    if (!cpu14->m_cpu_temp)
	fprintf(stderr, "*** Warning m_cpu_temp not defined\n");
    if ((fvrcon&TSEN) && cpu14->m_cpu_temp)
    {
        //Transister junction threshold voltage at core temperature
        double Vt = 0.659 - (cpu14->m_cpu_temp->get() + 40.0) * 0.00132;
        ret = cpu14->get_Vdd() - ((fvrcon&TSRNG) ? 4.0 : 2.0) * Vt;

        if (ret < 0.0)
        {
            ret = -1.0;
            std::cerr << "Warning FVRCON Vdd too low for temperature range\n";
        }
    }

    if (node_Vtref)
    {
	if (ret != node_Vtref->get_nodeVoltage())
        {
            volt_Vtref->set_Vth(ret);
	    node_Vtref->set_nodeVoltage(ret);
	}
    }

    return ret;
}


double FVRCON::compute_FVR_AD(unsigned int fvrcon)
{
    double ret = -1.;
    unsigned int gain = fvrcon & (ADFVR1|ADFVR0);
    bool on = fvrcon & FVREN;

    if (on && gain)
    {
        ret = 1.024 * (1 << (gain - 1));
    }

    if (ret > p_cpu->get_Vdd())
    {
        std::cerr << "warning FVRCON FVRAD > Vdd\n";
        ret = -1.0;
    }

    if (node_adcvref)
    {
	if (ret != node_adcvref->get_nodeVoltage())
        {
            volt_adcvref->set_Vth(ret);
	    node_adcvref->set_nodeVoltage(ret);
	}
    }

    return ret;
}


double FVRCON::compute_FVR_CDA(unsigned int fvrcon)
{
    double ret = 0.0;
    unsigned int gain = (fvrcon & (CDAFVR1|CDAFVR0)) >> 2;
    bool on = fvrcon & FVREN;

    if (on && gain)
    {
        ret = 1.024 * (1 << (gain - 1));
    }

    if (node_cvref)
    {
	if (ret != node_cvref->get_nodeVoltage())
	{
            volt_cvref->set_Vth(ret);
	    node_cvref->set_nodeVoltage(ret);
	}
    }
    return ret;
}

//
//--------------------------------------------------
// member functions for the FVRCON_V2 class
// with one set of gains and FVRST set after delay
//--------------------------------------------------
//
FVRCON_V2::FVRCON_V2(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask)
   : sfr_register(pCpu, pName, pDesc)
{
    mask_writable = bitMask;
    node_cvref = new Stimulus_Node("n_cvref");
    volt_cvref = new stimulus("cdafvr_src", 0.0, 48000.0);
    node_cvref->attach_stimulus(volt_cvref);
}
FVRCON_V2::~FVRCON_V2()
{
    node_cvref->detach_stimulus(volt_cvref);
    delete(volt_cvref);
    delete(node_cvref);
}

void FVRCON_V2::put(unsigned int new_value)
{
    unsigned int masked_value = (new_value & mask_writable);
    emplace_value_trace<trace::WriteRegisterEntry>();
    put_value(masked_value);

    compute_FVR_CDA(masked_value);
}


void FVRCON_V2::put_value(unsigned int new_value)
{
     unsigned int diff = value.get() ^ new_value;

  if (diff) {
    if (diff &  FVREN) {
      new_value &= ~FVRRDY;  // Clear FVRRDY regardless of ON or OFF
    }

    if (new_value & FVREN) {    // Enable ON?
      future_cycle = get_cycles().get() + 25e-6 / get_cycles().seconds_per_cycle();
      get_cycles().set_break(future_cycle, this);

    } else if (future_cycle) {
      get_cycles().clear_break(this);
      future_cycle = 0;
    }
  }
  value.put(new_value);
  compute_FVR_CDA(new_value);
}

// Set FVRRDY bit after timeout
void FVRCON_V2::callback()
{
  future_cycle = 0;
  put_value(value.get() | FVRRDY);
}


void FVRCON_V2::callback_print()
{
    std::cout <<  name() << " has callback, ID = " << CallBackID << '\n';
}


double FVRCON_V2::compute_FVR_CDA(unsigned int fvrcon)
{
    double ret = -1.0;
    unsigned int gain = (fvrcon & (FVRS1|FVRS0)) >> 4;
    bool on = fvrcon & FVREN;

    if (on && gain)
    {
        ret = 1.024 * (1 << (gain - 1));
    }


  if (ret > p_cpu->get_Vdd()) {
    std::cerr << "warning FVRCON FVRAD(" << ret << ") > Vdd("
         << p_cpu->get_Vdd() << ")\n";
    ret = -1.0;
  }

  if (node_cvref)
  {
 	if (ret != node_cvref->get_nodeVoltage())
	{
            volt_cvref->set_Vth(ret);
            node_cvref->set_nodeVoltage(ret);
	}
  }


  return ret;
}

//
//--------------------------------------------------
// member functions for the DAC classes
//--------------------------------------------------
//
DACCON0::DACCON0(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask, unsigned int bit_res)
    : sfr_register(pCpu, pName, pDesc), FVR_ATTACH(pName),
      bit_mask(bitMask), bit_resolution(bit_res),
      name_dacout("n_")
{
    Pin_Active[0] = false;
    Pin_Active[1] = false;
    Vth[0] = 0.0;
    Vth[1] = 0.0;
    Zth[0] = 0.0;
    Zth[1] = 0.0;
    driving[0] = false;
    driving[1] = false;
    output_pin[0] = nullptr;
    output_pin[1] = nullptr;

    name_dacout += pName;
    node_dacout = new Stimulus_Node(name_dacout.c_str());
    std::string wname = pName;
    wname += "_src";
    volt_dacout = new stimulus(wname.c_str(), 0.0, 48000.0);
    node_dacout->attach_stimulus(volt_dacout);
    volt_dacout->set_Vth(-1.0);
    node_dacout->set_nodeVoltage(-1.0);
}

DACCON0::~DACCON0()
{
    // remove DAC src stimulus
    if (node_dacout)
    {
        node_dacout->detach_stimulus(volt_dacout);
        delete volt_dacout;
    }
}

void  DACCON0::put(unsigned int new_value)
{
    unsigned int masked_value = (new_value & bit_mask);
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(masked_value);

    compute_dac(masked_value);
}


void  DACCON0::put_value(unsigned int new_value)
{
    unsigned int masked_value = (new_value & bit_mask);
    value.put(masked_value);
    compute_dac(masked_value);
}


void DACCON0::set_dcaccon1_reg(unsigned int reg)
{
    daccon1_reg = reg;
    compute_dac(value.get());
}


void  DACCON0::compute_dac(unsigned int reg_value)
{
    double Vhigh = get_Vhigh(reg_value);
    double Vlow = 0.0;
    double Vout;

    if (reg_value & DACEN)  	// DAC is enabled
    {
        // Max Vout will be 1 step below Vhigh.
        Vout = (Vhigh - Vlow) * daccon1_reg / bit_resolution + Vlow;

    }
    else if (reg_value & DACLPS)
    {
        Vout = Vhigh;

    }
    else
    {
        Vout = Vlow;
    }

    set_dacoutpin(reg_value & DACOE, 0, Vout);
    set_dacoutpin(reg_value & DACOE2, 1, Vout);

    if (verbose)
    {
        printf("%s-%d Vout %.2f\n", __FUNCTION__, __LINE__, Vout);
    }

    Dprintf(("DAC DACEN %d output %.2f Vhigh %.2f bit_res %u daccon1_reg %u\n", (bool)(reg_value & DACEN), Vout, Vhigh, bit_resolution, daccon1_reg));

    if (Vout != node_dacout->get_nodeVoltage())
    {
        volt_dacout->set_Vth(Vout);
        node_dacout->set_nodeVoltage(Vout);
    }

}


void DACCON0::set_dacoutpin(bool output_enabled, int chan, double Vout)
{
    IO_bi_directional_pu *out_pin;

    if (output_pin[chan])
    {
        out_pin = (IO_bi_directional_pu *) output_pin[chan]->getPin();

    }
    else
    {
        return;
    }

    if (output_enabled)
    {
        if (!Pin_Active[chan])  	// DACOUT going to active
        {
            std::string pin_name = name().substr(0, 4);

            if (pin_name == "dacc")
            {
                pin_name = "dacout";

            }
            else if (chan == 0)
            {
                pin_name += "-1";

            }
            else
            {
                pin_name += "-2";
            }

            output_pin[chan]->AnalogReq(this, true, pin_name.c_str());
            Pin_Active[chan] = true;
            Vth[chan] = out_pin->get_VthIn();
            Zth[chan] = out_pin->get_ZthIn();
            driving[chan] = out_pin->getDriving();
            out_pin->set_ZthIn(150e3);
            out_pin->setDriving(false);
        }

        out_pin->set_VthIn(Vout);
        out_pin->updateNode();

    }
    else if (Pin_Active[chan])  	// DACOUT leaving active
    {
        output_pin[chan]->AnalogReq(this, false, output_pin[chan]->getPin()->name().c_str());
        Pin_Active[chan] = false;
        out_pin->set_VthIn(Vth[chan]);
        out_pin->set_ZthIn(Zth[chan]);
        out_pin->setDriving(driving[chan]);
        out_pin->updateNode();
    }
}


double DACCON0::get_Vhigh(unsigned int reg_value)
{
    unsigned int mode = (reg_value & (DACPSS0 | DACPSS1)) >> 2;

    switch (mode)
    {
    case 0:	// Vdd
        return p_cpu->get_Vdd();

    case 1:	// Vref+ pin, get is from A2D setup
        if (adcon1)
        {
            return (adcon1->getChannelVoltage(adcon1->getVrefHiChannel(0)));
        }

        std::cerr << "ERROR DACCON0 DACPSS=1 adcon1 not set\n";
        return 0.0;

    case 2:	// Fixed Voltage Reference
        return FVR_CDA_volt;

    case 3:	// Reserved value
        std::cerr << "ERROR DACCON0 DACPSS=3 is reserved value\n";
        return 0.0;
    }

    return 0.0;	// cant get here
}




DACCON1::DACCON1(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask, DACCON0 *_daccon0)
    : sfr_register(pCpu, pName, pDesc),
      bit_mask(bitMask), daccon0(_daccon0)
{
}


void  DACCON1::put(unsigned int new_value)
{
    emplace_value_trace<trace::WriteRegisterEntry>();
    put_value(new_value);
}


void  DACCON1::put_value(unsigned int new_value)
{
    unsigned int masked_value = (new_value & bit_mask);
    value.put(masked_value);
    Dprintf(("DAC daccon0=%p new_value 0x%x bit_mask 0x%x\n", daccon0, new_value, bit_mask));

    if (daccon0)
    {
        daccon0->set_dcaccon1_reg(masked_value);
    }
}


//------------------------------------------------------
// ADCON2_diff for A2D with differential input ie 16f178*
//
ADCON2_DIF::ADCON2_DIF(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
}


#ifdef RRR
//------------------------------------------------------
// ADCON0
//
ADCON0_32::ADCON0_32(Processor *pCpu, const char *pName, const char *pDesc)
    : ADCON0(pCpu, pName, pDesc)
{
}


void ADCON0_32::put(unsigned int new_value)
{
    emplace_value_trace<trace::WriteRegisterEntry>();
    set_Tad(new_value);
    unsigned int old_value = value.get();
    // SET: Reflect it first!
    value.put(new_value);

    if (new_value & ADON)
    {
        // The A/D converter is being turned on (or maybe left on)
        if ((new_value & ~old_value) & GO_bit)
        {
            if (verbose)
            {
                printf("starting A2D conversion\n");
            }

            Dprintf(("starting A2D conversion\n"));
            // The 'GO' bit is being turned on, which is request to initiate
            // and A/D conversion
            start_conversion();
        }

    }
    else
    {
        stop_conversion();
    }
}


void ADCON0_32::put_conversion()
{
    double dRefSep = m_dSampledVrefHi - m_dSampledVrefLo;
    double dNormalizedVoltage;
    double m_dSampledVLo;

    if (channel == 0x0e)   // shift AN21 to adcon0 channel
    {
        channel = 0x15;
    }

    if (channel == 0x0f)  	// use ADNREF for V-
    {
        m_dSampledVLo = getVrefLo();

    }
    else
    {
        m_dSampledVLo = getChannelVoltage(channel);
    }

    dNormalizedVoltage = (m_dSampledVoltage - m_dSampledVLo) / dRefSep;
    dNormalizedVoltage = dNormalizedVoltage > 1.0 ? 1.0 : dNormalizedVoltage;
    int converted = (int)(m_A2DScale * dNormalizedVoltage + 0.5);
    Dprintf(("put_conversion: V+:%g V-:%g Vrefhi:%g Vreflo:%g conversion:%d normV:%g\n",
             m_dSampledVoltage, m_dSampledVLo,
             m_dSampledVrefHi, m_dSampledVrefLo, converted, dNormalizedVoltage));

    if (verbose)
    {
        printf("result=0x%02x\n", converted);
    }

    Dprintf(("%u-bit result 0x%x ADFM %d\n", m_nBits, converted, get_ADFM()));

    if (!get_ADFM())   // signed
    {
        int sign = 0;

        if (converted < 0)
        {
            sign = 1;
            converted = -converted;
        }

        converted = ((converted << (16 - m_nBits)) % 0xffff) | sign;
    }

    adresl->put(converted & 0xff);
    adres->put((converted >> 8) & 0xff);
}


#endif //RRR


//------------------------------------------------------
// ADCON2_TRIG for A2D with start trigger ie 16f1503
//
ADCON2_TRIG::ADCON2_TRIG(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), valid_bits(0xf0)
{
    std::fill_n(CMxsync, 4, false);
}


void ADCON2_TRIG::put(unsigned int new_value)
{
    new_value &= valid_bits;
    emplace_value_trace<trace::WriteRegisterEntry>();
    put_value(new_value);
}


void ADCON2_TRIG::setCMxsync(unsigned int cm, bool output)
{
    unsigned int select = value.get() >> 4;
    printf("setCMxsync() %s cm=%u output=%d\n", name().c_str(), cm, output);

    if (select)
    {
    }

    assert(cm < 4);
    CMxsync[cm] = output;
}


void ADCON2_TRIG::t0_overflow()
{
    unsigned int select = value.get() >> 4;

    if (select == 0x02)
    {
        if (m_adcon0 && (m_adcon0->value.get() & ADCON0::ADON))
        {
            Dprintf(("trigger from timer 0\n"));
            m_adcon0->start_conversion();
        }
    }
}


void FVR_STIMULUS::set_nodeVoltage(double v)
{
    pt_base_function->set_FVR_volt(v, chan);
}

void FVR_ATTACH::attach_cda_fvr(Stimulus_Node *_fvr_node, unsigned int chan)
{
    if (!fvr_node)
    {
	fvr_name = "Cdafvr_" + reg_name;
        fvr_node = _fvr_node;
        fvr_stimulus = new FVR_STIMULUS(this, chan, fvr_name.c_str());
        fvr_node->attach_stimulus(fvr_stimulus);
	Dprintf(("FVR_ATTACH::attach_cda_fvr fvr_node=%p %s\n", fvr_node, fvr_name.c_str()));
    }
}

void FVR_ATTACH::attach_ad_fvr(Stimulus_Node *_fvr_node, unsigned int _chan)
{
    Dprintf(("FVR_ATTACH::attach_ad_fvr %s fvr_node=%p chan=%u\n", reg_name.c_str(),_fvr_node, _chan));
    if (!fvr_ad_node)
    {
	fvr_ad_name = "Adfvr_" + reg_name;
        fvr_ad_node = _fvr_node;
        fvr_ad_stimulus = new FVR_STIMULUS(this, _chan, fvr_ad_name.c_str());
        fvr_ad_node->attach_stimulus(fvr_ad_stimulus);
    }
    else
    {
	fprintf(stderr, "*** FVR_ATTACH::attach_ad_fvr reg %s already defined %p\n", reg_name.c_str(), fvr_ad_node);
    }
}

void FVR_ATTACH::attach_Vt_fvr(Stimulus_Node *_fvr_node, unsigned int chan)
{
    Dprintf(("FVR_ATTACH::attach_Vt_fvr %s fvr_node=%p chan=%d\n", reg_name.c_str(),_fvr_node, chan));
    if (!fvr_Vt_node)
    {
	fvr_Vt_name = "Vtfvr_" + reg_name;
        fvr_Vt_node = _fvr_node;
        fvr_Vt_stimulus = new FVR_STIMULUS(this, chan, fvr_Vt_name.c_str());
        fvr_Vt_node->attach_stimulus(fvr_Vt_stimulus);
    }
}

void FVR_ATTACH::detach_fvr()
{
    if (fvr_node)
    {
        fvr_node->detach_stimulus(fvr_stimulus);
        fvr_node = nullptr;
	fvr_stimulus = nullptr;
    }
    if (fvr_ad_node)
    {
        fvr_ad_node->detach_stimulus(fvr_ad_stimulus);
        fvr_ad_node = nullptr;
	fvr_ad_stimulus = nullptr;
    }
    if (fvr_Vt_node)
    {
        fvr_Vt_node->detach_stimulus(fvr_Vt_stimulus);
        fvr_Vt_node = nullptr;
	fvr_Vt_stimulus = nullptr;
    }
}

void DAC_ATTACH::attach_DAC(Stimulus_Node *_DAC_node, unsigned int chan, unsigned int n)
{
    unsigned int index = n - 1;

    assert(index < 8);
    if (!DAC_node[index])
    {
        char prefix[6];
        snprintf(prefix, sizeof(prefix), "dac%u_", n);
        DAC_name[index] = prefix + reg_name;
        DAC_node[index] = _DAC_node;
        DAC_stimulus[index] = new DAC_STIMULUS(this, chan, DAC_name[index].c_str());
        DAC_node[index]->attach_stimulus(DAC_stimulus[index]);
    }
    else
    {
        fprintf(stderr, "***DAC_ATTACH::attach_DAC %s n=%u chan=%u already defined***\n", DAC_name[index].c_str(), n, chan);
    }
}

void DAC_ATTACH::detach_DAC()
{
    for(int i = 0; i < 8; i++)
    {
        if (DAC_node[i])
        {
            DAC_node[i]->detach_stimulus(DAC_stimulus[i]);
    	    delete DAC_stimulus[i];
    	    DAC_stimulus[i] = nullptr;
    	    DAC_node[i] = nullptr;
        }
    }
}

void DAC_STIMULUS::set_nodeVoltage(double v)
{
    pt_base_function->set_DAC_volt(v, chan);
}
