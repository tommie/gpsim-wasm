/*
   Copyright (C) 1998,1999 T. Scott Dattalo
		 2006,2015 Roy R Rankin

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


#ifndef SRC_SSP_H_
#define SRC_SSP_H_

#include <glib.h>

#include "pic-processor.h"
#include "ioports.h"
#include "pir.h"
#include "registers.h"
#include "trigger.h"

class PicTrisRegister;
class Processor;
class SDI_SignalSink;
class SCL_SignalSink;
class SS_SignalSink;
class SDO_SignalSource;
class SDI_SignalSource;
class SCK_SignalSource;

enum SSP_TYPE
{
    SSP_TYPE_BSSP = 1,
    SSP_TYPE_SSP,
    SSP_TYPE_MSSP,
    SSP_TYPE_MSSP1
};

class SSP_MODULE;
class SSP1_MODULE;

class _SSPCON : public sfr_register, public TriggerObject
{
public:
    _SSPCON(Processor *pCpu, SSP_MODULE *);

    enum
    {
        SSPM_SPImaster4 = 0x0,      // SPI master mode, clock = FOSC/4
        SSPM_SPImaster16 = 0x1,     // SPI master mode, clock = FOSC/16
        SSPM_SPImaster64 = 0x2,     // SPI master mode, clock = FOSC/64
        SSPM_SPImasterTMR2 = 0x3,   // SPI master mode, clock = TMR2/2
        SSPM_SPIslaveSS = 0x4,      // SPI slave mode, clock = SCK, /SS controlled
        SSPM_SPIslave = 0x5,        // SPI slave mode, clock = SCK, not /SS controlled
        SSPM_SPImasterAdd = 0xa,	// SPI master mode, clock = FOSC/4*(sspadd+1)

        SSPM_I2Cslave_7bitaddr = 0x6,
        SSPM_I2Cslave_10bitaddr = 0x7,
        SSPM_MSSPI2Cmaster = 0x8,
        SSPM_LoadMaskFunction = 0x9,
        SSPM_I2Cfirmwaremaster = 0xb,
        SSPM_I2Cslave_7bitaddr_ints = 0xe,
        SSPM_I2Cslave_10bitaddr_ints = 0xf,

        /* None of the documentation I have seen show these, but Scott? thought
        they were the good name RRR
        SSPM_I2Cfirmwaremultimaster_7bitaddr_ints = 0xe,
        SSPM_I2Cfirmwaremaster_10bitaddr_ints = 0xf,
        */
    };

    // Register bit definitions

    enum
    {
        SSPM0  = 1 << 0,
        SSPM1  = 1 << 1,
        SSPM2  = 1 << 2,
        SSPM3  = 1 << 3,
        CKP    = 1 << 4,
        SSPEN  = 1 << 5,
        SSPOV  = 1 << 6,
        WCOL   = 1 << 7
    };

    static const unsigned int SSPM_mask = (SSPM0 | SSPM1 | SSPM2 | SSPM3);

    void put(unsigned int) override;
    void put_value(unsigned int) override;

    bool isSSPEnabled() { return (value.get() & SSPEN) == SSPEN; }
    bool isI2CActive(unsigned int);
    bool isI2CSlave(unsigned int);
    bool isI2CMaster(unsigned int);
    bool isSPIActive(unsigned int);

    bool isSPIMaster();
    void setWCOL();
    void setSSPOV() { put_value(value.get() | SSPOV); }
    void setSSPMODULE(SSP_MODULE *);

private:
    SSP_MODULE *m_sspmod;
};


class _SSPCON2 : public sfr_register
{
public:
    _SSPCON2(Processor *pCpu, SSP_MODULE *);

    enum
    {
        SEN  = 1 << 0,	// Start or Stretch enable
        RSEN  = 1 << 1,	// Repeated Start
        PEN  = 1 << 2,	// Stop condition enable
        RCEN = 1 << 3,	// Receive enable bit
        ACKEN = 1 << 4,	// Acknowledge Sequence enable bit
        ACKDT = 1 << 5,	// Acknowledge Data bit
        ACKSTAT = 1 << 6,	// Acknowledge status bit
        GCEN = 1 << 7		// General call enable
    };

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;

private:
    SSP_MODULE   *m_sspmod;
};


class _SSP1CON3 : public sfr_register
{
public:
    _SSP1CON3(Processor *pCpu, SSP1_MODULE *);

    enum
    {
        DHEN  = 1 << 0,	// Data hold enable
        AHEN  = 1 << 1,	// Address hold enable
        SBCDE = 1 << 2,	// Slave Mode Bus Collision Detect Enable bit
        SDAHT = 1 << 3,	// SDA Hold Time Selection bit
        BOEN  = 1 << 4,	// Buffer Overwrite Enable bit
        SCIE  = 1 << 5,	// Start Condition Interrupt Enable bit
        PCIE  = 1 << 6,	// Stop Condition Interrupt Enable bit
        ACKTIM = 1 << 7	// Acknowledge Time Status bit
    };

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;

private:
    SSP1_MODULE   *m_sspmod;
};


class _SSPSTAT : public sfr_register
{
public:
    _SSPSTAT(Processor *pCpu, SSP_MODULE *);

    // Register bit definitions

    enum
    {
        BF  = 1 << 0, // Buffer Full
        UA  = 1 << 1, // Update Address
        RW  = 1 << 2, // Read/Write info
        S   = 1 << 3, // Start bit (I2C mode)
        P   = 1 << 4, // Stop bit (I2C mode)
        DA  = 1 << 5, // Data/Address bit (I2C mode)

        // below are SSP and MSSP only. This class will force them to
        // always be 0 if ssptype == SSP_TYPE_BSSP. This will give the
        // corrent behavior.
        CKE = 1 << 6, // SPI clock edge select
        SMP = 1 << 7 // SPI data input sample phase
    };

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;

private:
    SSP_MODULE   *m_sspmod;
};


class _SSPBUF : public sfr_register
{
public:
    _SSPBUF(Processor *pCpu, SSP_MODULE *);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;

    bool isFull() { return m_bIsFull; }
    void setFullFlag(bool bNewFull) { m_bIsFull = bNewFull; }

    SSP_TYPE ssptype;

private:
    SSP_MODULE   *m_sspmod;
    bool m_bIsFull;
};


class _SSPMSK : public sfr_register
{
public:
    _SSPMSK(Processor *pCpu, const char *_name);

    void put(unsigned int new_value) override;
};


class _SSPADD : public sfr_register
{
public:
    _SSPADD(Processor *pCpu, SSP_MODULE *);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;

private:
    SSP_MODULE   *m_sspmod;
};


class SPI: public  TriggerObject
{
public:
    SPI(SSP_MODULE *, _SSPCON *, _SSPSTAT *, _SSPBUF *);

    bool isIdle() { return m_state == eIDLE; }
    virtual void clock(bool);
    virtual void start_transfer();
    virtual void stop_transfer();
    virtual void set_halfclock_break();
    void callback() override;
    void newSSPBUF(unsigned int);
    virtual void startSPI();

    SSP_MODULE *m_sspmod;
    _SSPBUF   *m_sspbuf;
    _SSPCON   *m_sspcon;
    _SSPSTAT  *m_sspstat;

protected:
    unsigned int m_SSPsr;  // internal Shift Register
    enum SSPStateMachine
    {
        eIDLE,
        eACTIVE,
        eWAITING_FOR_LAST_SMP
    } m_state;

    int bits_transfered;
    Processor *cpu;
};


class SPI_1: public  SPI
{
public:
    SPI_1(SSP1_MODULE *, _SSPCON *, _SSPSTAT *, _SSPBUF *, _SSP1CON3 *, _SSPADD *);

    void stop_transfer() override;
    void set_halfclock_break() override;

    _SSP1CON3 *m_ssp1con3;
    _SSPADD	*m_sspadd;
};


class I2C: public  TriggerObject
{
public:
    SSP_MODULE *m_sspmod;
    _SSPBUF   *m_sspbuf;
    _SSPCON   *m_sspcon;
    _SSPSTAT  *m_sspstat;
    _SSPCON2  *m_sspcon2;
    _SSPADD   *m_sspadd;

    I2C(SSP_MODULE *, _SSPCON *, _SSPSTAT *, _SSPBUF *, _SSPCON2 *, _SSPADD *);
    virtual void clock(bool);
    virtual void sda(bool);
    void callback() override;
    virtual void set_idle();
    virtual void newSSPBUF(unsigned int value);
    virtual void newSSPADD(unsigned int value);
    virtual void start_bit();
    virtual void rstart_bit();
    virtual void stop_bit();
    virtual void master_rx();
    virtual void ack_bit();
    virtual bool isIdle();
    virtual void setBRG();
    virtual void clrBRG();
    virtual bool rx_byte();
    virtual void bus_collide();
    virtual void slave_command();
    virtual bool end_ack();
    virtual bool match_address(unsigned int sspsr);
    virtual bool do_stop_sspif();
    bool		scl_clock_high();
    bool		scl_neg_tran();
    bool		scl_pos_tran();
    bool		scl_clock_low();

protected:
    unsigned int m_SSPsr;  // internal Shift Register

    enum I2CStateMachine
    {
        eIDLE,
        RX_CMD,
        RX_CMD2,
        RX_DATA,
        TX_DATA,
        CLK_TX_BYTE,
        CLK_RX_BYTE,
        CLK_ACKEN,
        CLK_RSTART,
        CLK_STOP,
        CLK_START,
        CLK_RX_ACK
    } i2c_state;

    int 	bits_transfered;
    int   phase;
    guint64 future_cycle;
    Processor *cpu;
};


class I2C_1: public  I2C
{
public:
    I2C_1(SSP_MODULE *, _SSPCON *, _SSPSTAT *, _SSPBUF *, _SSPCON2 *, _SSPADD *, _SSP1CON3 *);

    void clock(bool) override;
    void bus_collide() override;
    bool do_stop_sspif() override;

    SSP_MODULE *m_sspmod;
    _SSP1CON3  *m_sspcon3;
};


class SSP_MODULE
{
public:
    _SSPBUF   sspbuf;
    _SSPCON   sspcon;
    _SSPSTAT  sspstat;
    _SSPCON2  sspcon2;	// MSSP

    // set to NULL for BSSP (It doesn't have this register)
    _SSPADD   sspadd;
    _SSPMSK   *sspmsk;

    explicit SSP_MODULE(Processor *);
    virtual ~SSP_MODULE();

    virtual void initialize(PIR_SET *ps,
                            PinModule *_SckPin,
                            PinModule *_SdiPin,
                            PinModule *_SdoPin,
                            PinModule *_SsPin,
                            PicTrisRegister *_i2ctris,
                            SSP_TYPE ssptype = SSP_TYPE_BSSP);


    virtual void SDI_SinkState(char);
    virtual void SS_SinkState(char);
    virtual void SCL_SinkState(char);
    virtual bool get_SDI_State() { return m_SDI_State; }
    virtual bool get_SCL_State() { return m_SCL_State; }
    virtual bool get_SS_State() { return m_SS_State; }
    virtual void Sck_toggle();
    virtual void putStateSDO(char _state);
    virtual void putStateSCK(char _state);
    virtual void mk_ssp_int(PIR *reg, unsigned int bit)
    {
        m_ssp_if = new InterruptSource(reg, bit);
    }
    virtual void mk_bcl_int(PIR *reg, unsigned int bit)
    {
        m_bcl_if = new InterruptSource(reg, bit);
    }
    virtual void set_sspif();
    virtual void set_bclif();
    virtual void startSSP(unsigned int value);
    virtual void stopSSP(unsigned int value);
    virtual void changeSSP(unsigned int new_value, unsigned int old_value);
    virtual void ckpSPI(unsigned int value);
    virtual void newSSPBUF(unsigned int value);
    virtual void newSSPADD(unsigned int value);
    virtual void newSSPCON2(unsigned int value);
    virtual void rdSSPBUF();
    virtual void tmr2_clock();
    virtual SSP_TYPE ssp_type() { return m_ssptype; }
    virtual void setSCL(bool);
    virtual void setSDA(bool);
    virtual bool SaveSSPsr(unsigned int value);
    virtual bool isI2CIdle() { return m_i2c->isIdle(); }
    virtual bool isI2CMaster() { return sspcon.isI2CMaster(sspcon.value.get()); }
    virtual bool isI2CSlave() { return sspcon.isI2CSlave(sspcon.value.get()); }
    virtual void releaseSDIpin();
    virtual void releaseSDOpin();
    virtual void releaseSCLpin();
    virtual void releaseSSpin();
    virtual void releaseSCKpin();

    Processor *cpu;

protected:
    InterruptSource *m_ssp_if;
    InterruptSource *m_bcl_if;
    PIR_SET   *m_pirset;
    SPI 	    *m_spi;
    I2C 	    *m_i2c;
    PinModule *m_sck_in;	// Should be same as m_sck, but required by pps maping to work
    PinModule *m_sck;
    PinModule *m_ss;
    PinModule *m_sdo;
    PinModule *m_sdi;
    PicTrisRegister *m_i2c_tris;
    SSP_TYPE  m_ssptype;

    bool m_SDI_State;
    bool m_SCL_State;
    bool m_SS_State;

    SCK_SignalSource 	*m_SckSource;
    SDO_SignalSource 	*m_SdoSource;
    SDI_SignalSource 	*m_SdiSource;
    SDI_SignalSink 	*m_SDI_Sink;
    SCL_SignalSink 	*m_SCL_Sink;
    SS_SignalSink 	*m_SS_Sink;
    bool 			m_sink_set;
    bool			m_sdo_active;
    bool			m_sdi_active;
    bool			m_sck_active;
};


class SSP1_MODULE : public SSP_MODULE, public apfpin
{
    //MSSP1
public:
    explicit SSP1_MODULE(Processor *);
    ~SSP1_MODULE();

    enum
    {
        SCK_PIN = 0,
        SDI_PIN,
        SDO_PIN,
        SS_PIN,
        SCK_IN_PIN
    };

    _SSP1CON3  	ssp1con3;

    void initialize(PIR_SET *ps,
                    PinModule *_SckPin,
                    PinModule *_SdiPin,
                    PinModule *_SdoPin,
                    PinModule *_SsPin,
                    PicTrisRegister *_i2ctris,
                    SSP_TYPE ssptype = SSP_TYPE_MSSP1) override;

    void setIOpin(PinModule *pin, int data) override;
    void set_sckInPin(PinModule *_sckPin_in);
    void set_sckOutPin(PinModule *_sckPin);
    void set_sckPin(PinModule *_sckPin);
    void set_sdiPin(PinModule *_sdiPin);
    void set_sdoPin(PinModule *_sdoPin);
    PinModule *get_sdoPin() { return m_sdo; }
    void set_ssPin(PinModule *_ssPin);
    void set_tris(PicTrisRegister *_i2ctris) { m_i2c_tris = _i2ctris; }
    void changeSSP(unsigned int new_value, unsigned int old_value) override;
    bool SaveSSPsr(unsigned int value) override;
};

#endif  // SRC_SSP_H_
