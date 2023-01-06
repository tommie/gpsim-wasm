/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian
		 2013 Roy R. Rankin

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

#ifndef SRC_PIR_H_
#define SRC_PIR_H_

#include <assert.h>
#include <stdio.h>
#include "registers.h"
#include "pie.h"

class INTCON;
class PIE;
class Processor;

//---------------------------------------------------------
// PIR Peripheral Interrupt register base class for PIR1 & PIR2

class PIR : public sfr_register
{
protected:
  INTCON  *intcon;
  PIE     *pie;
  sfr_register *ipr;
public:
  int valid_bits;
  int writable_bits;

  PIR(Processor *pCpu, const char *pName, const char *pDesc,
      INTCON *, PIE *, int _valid_bits);
  // The PIR base class supports no PIR bits directly
  virtual void clear_sspif(){}
  virtual void clear_sppif(){}
  virtual void clear_rcif(){}
  virtual void clear_txif(){}
  virtual void set_adif(){}
  virtual void set_bclif(){}
  virtual void set_ccpif(){}
  virtual void set_cmif(){}
  virtual void set_c1if(){fprintf(stderr, "RRR set_c1if FIX\n");}
  virtual void set_c2if(){fprintf(stderr, "RRR set_c2if FIX\n");}
  virtual void set_c3if(){fprintf(stderr, "RRR set_c3if FIX\n");}
  virtual void set_c4if(){fprintf(stderr, "RRR set_c4if FIX\n");}
  virtual void set_c5if(){fprintf(stderr, "RRR set_c5if FIX\n");}
  virtual void set_eccp1if(){}
  virtual void set_eeif(){}
  virtual void set_errif(){}
  virtual void set_irxif(){}
  virtual void set_lvdif(){}
  virtual void set_pspif(){}
  virtual void set_rcif(){}
  virtual void set_rxb0if(){}
  virtual void set_rxb1if(){}
  virtual void set_sspif(){}
  virtual void set_sppif(){fprintf(stderr, "set_sppif FIX\n");}
  virtual void set_tmr1if(){}
  virtual void set_tmr1gif(){}
  virtual void set_tmr2if(){}
  virtual void set_tmr3if(){ fprintf(stderr, "set_tmr3if FIX\n");}
  virtual void set_txb0if(){}
  virtual void set_txb1if(){}
  virtual void set_txb2if(){}
  virtual void set_txif(){}
  virtual void set_wakif(){}
  virtual void set_usbif(){}
  virtual void set_nco1if() { fprintf(stderr, "set_nco1if() FIX\n");}

  virtual unsigned int get_txif() { return 0;}
  virtual unsigned int get_rcif() { return 0;}
  virtual unsigned int get_sspif() { return 0;}
  virtual unsigned int get_sppif() { return 0;}

  /// A generic method to set an interrupt bit by mask
  virtual void set(int mask)
  {
    put(get() | mask);
  }

  /// Obtain interrupt request state, as a mask of priorities if relevant
  virtual int interrupt_status();

  void put(unsigned int new_value) override;

  virtual void setInterrupt(unsigned int bitMask);
  virtual void setPeripheralInterrupt();

  void set_intcon(INTCON *);
  void set_pie(PIE *);
  void set_ipr(sfr_register *);
};

/*---------------------------------------------------------
// InterruptSource

  this is an alternative to the set_xxif method which
  a> is becoming unwieldly
  b> removes hard coding to allow multiple instantiation of classes
     with different interrupts

*/

class InterruptSource
{
public:
  InterruptSource(PIR *_pir, unsigned int bitMask);
  void Trigger();
  void Clear();
  unsigned int Get() { return m_pir->value.get() & m_bitMask;}
  void release(); // called when source is no longer needed.
private:
  PIR *m_pir;
  unsigned int m_bitMask;
};

//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 1 of the PIR1 register - as seen on the 16f62x

class PIR1v1 : public PIR
{
public:

  enum {
    TMR1IF  = 1 << 0,
    TMR2IF  = 1 << 1,
    CCP1IF  = 1 << 2,
    SSPIF   = 1 << 3,
    TXIF    = 1 << 4,
    RCIF    = 1 << 5,
    CMIF    = 1 << 6,     // 16f62x
    EEIF    = 1 << 7      // 16f62x
  };

  void set_tmr1if() override
  {
    put(get() | TMR1IF);
  }

  void set_tmr2if() override
  {
    put(get() | TMR2IF);
  }

  void set_ccpif() override
  {
    put(get() | CCP1IF);
  }

  void set_sspif() override
  {
    put(get() | SSPIF);
  }

  void set_txif() override;
  void set_rcif() override;

  void set_cmif() override;
  void set_c1if() override { set_cmif(); }
  void set_c2if() override { set_cmif(); }


  void set_eeif() override;

  unsigned int get_sspif() override
  {
    return value.get() & SSPIF;
  }
  void clear_sspif() override;

  unsigned int get_txif() override
  {
    return value.get() & TXIF;
  }
  void clear_txif() override;

  unsigned int get_rcif() override
  {
    return value.get() & RCIF;
  }
  void clear_rcif() override;

  PIR1v1(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};


//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 2 of the PIR1 register - as seen on the 18xxxx devices
// and devices like the 16c63a.

class PIR1v2 : public PIR
{
public:

  enum {
    TMR1IF  = 1 << 0,
    TMR2IF  = 1 << 1,
    CCP1IF  = 1 << 2,
    SSPIF   = 1 << 3,
    TXIF    = 1 << 4,
    RCIF    = 1 << 5,
    ADIF    = 1 << 6,     // 18cxxx
    PSPIF   = 1 << 7,
    SPPIF   = 1 << 7
  };

  void set_tmr1if() override
  {
    put(get() | TMR1IF);
  }

  void set_tmr2if() override
  {
    put(get() | TMR2IF);
  }

  void set_ccpif() override
  {
    put(get() | CCP1IF);
  }

  void set_sspif() override;

  unsigned int get_sspif() override
  {
    return value.get() & SSPIF;
  }
  void clear_sspif() override;

  void set_txif() override;
  void set_rcif() override;

  void set_adif() override
  {
    put(get() | ADIF);
  }

  unsigned int get_sppif() override
  {
    return value.get() & SPPIF;
  }
  void set_sppif() override;

  void set_pspif() override;


  unsigned int get_txif() override
  {
    return value.get() & TXIF;
  }
  void clear_txif() override;
  unsigned int get_rcif() override
  {
    return value.get() & RCIF;
  }
  void clear_rcif() override;

  PIR1v2(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 3 of the PIR1 register - as seen on the p16f630 devices

class PIR1v3 : public PIR
{
public:

  enum {
    TMR1IF  = 1 << 0,
    TMR2IF  = 1 << 1,	//16f684
    OSFIF   = 1 << 2,	//16f684
    CMIF    = 1 << 3,
    C1IF    = 1 << 3,	//16f684
    C2IF    = 1 << 4,	//16f684
    CCP1IF  = 1 << 5,	//16f684
    ADIF    = 1 << 6,
    EEIF    = 1 << 7
  };

  void set_tmr1if() override;
  void set_tmr2if() override;
  void set_cmif() override;

  void set_adif() override;

  void set_eeif() override;
  void set_c1if() override;
  void set_c2if() override;
  void set_ccpif() override { put(get() | CCP1IF); }

  PIR1v3(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 4 of the PIR1 register - as seen on the p16f91x devices

class PIR1v4 : public PIR
{
public:

  enum {
    TMR1IF  = 1 << 0,
    TMR2IF  = 1 << 1,
    CCP1IF  = 1 << 2,
    SSPIF   = 1 << 3,
    TXIF    = 1 << 4,
    RCIF    = 1 << 5,
    ADIF    = 1 << 6,
    EEIF    = 1 << 7
  };

  void set_tmr1if() override { put(get() | TMR1IF); }
  void set_tmr2if() override { put(get() | TMR2IF); }
  void set_ccpif() override { put(get() | CCP1IF); }
  void set_sspif() override { put(get() | SSPIF); }
  void set_txif() override;
  void set_rcif() override;
  void clear_txif() override;
  void clear_rcif() override;
  unsigned int get_txif() override { return value.get() & TXIF; }
  unsigned int get_rcif() override { return value.get() & RCIF; }
  void set_adif() override { put(get() | ADIF); }
  void set_eeif() override { put(get() | EEIF); }

  PIR1v4(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};


//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 2
//
// This is version 1 of the PIR1 register - as seen on the 16f62x

class PIR2v1 : public PIR
{
public:
    enum
    {
      CCP2IF = 1 << 0
    };

    void set_ccpif() override
    {
        put(get() | CCP2IF);
    }

    PIR2v1(Processor *pCpu, const char *pName, const char *pDesc,
           INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 2
//
// This is version 2 of the PIR2 register - as seen on the 18xxxx devices
// and devices like the 16c63a.

class PIR2v2 : public PIR
{
public:
    enum
    {
        CCP2IF  = 1 << 0,
        ECCP1IF = 1 << 0,             /* only on the PIC18F4xx devices */
        TMR3IF  = 1 << 1,
        LVDIF   = 1 << 2,
        HLVDIF  = 1 << 2,		/* 18f26k22 */
        BCLIF   = 1 << 3,
        EEIF    = 1 << 4,
        C2IF    = 1 << 5,		/* 18f14k22 */
        C1IF    = 1 << 6,		/* 18f14k22 */
        CMIF    = 1 << 6,              /* PIC16F87xA, PIC18F4xx devices */
        OSCFIF  = 1 << 7		/* 18f14k22 */
    };

    void set_eccp1if() override
    {
        put(get() | ECCP1IF);
    }

    void set_ccpif() override    /* RP - needs to define set_ccpif too! */
    {
        put(get() | CCP2IF);
    }

    void set_tmr3if() override
    {
        put(get() | TMR3IF);
    }

    void set_lvdif() override
    {
        put(get() | LVDIF);
    }

    void set_bclif() override;
    void set_eeif() override;
    void set_cmif() override;
    void set_c1if() override { set_cmif(); }
    void set_c2if() override { set_cmif(); }

    PIR2v2(Processor *pCpu, const char *pName, const char *pDesc,
           INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 3
//
// This is version 2 of the PIR2 register - as seen on the 16f88x devices

class PIR2v3 : public PIR
{
public:
    enum
    {
        CCP2IF  = 1 << 0,
        ULPWUIF = 1 << 2,
        BCLIF   = 1 << 3,
        EEIF    = 1 << 4,
        C1IF    = 1 << 5,
        C2IF    = 1 << 6,
        OSFIF   = 1 << 7
    };

    void set_ccpif() override
    {
        put(get() | CCP2IF);
    }

    void set_bclif() override;
    void set_eeif() override;
    void set_c1if() override;
    void set_c2if() override;

    PIR2v3(Processor *pCpu, const char *pName, const char *pDesc,
           INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 3
//
// This is version 4 of the PIR2 register - as seen on the 18F4455 devices

class PIR2v4 : public PIR
{
public:
    enum
    {
        CCP2IF  = 1 << 0,
        TMR3IF  = 1 << 1,
        HLVDIF  = 1 << 2,
        BCLIF   = 1 << 3,
        EEIF    = 1 << 4,
        USBIF   = 1 << 5,
        CMIF    = 1 << 6,
        OSCFIF  = 1 << 7
    };

    void set_ccpif() override
    {
      put(get() | CCP2IF);
    }
    void set_tmr3if() override
    {
      put(get() | TMR3IF);
    }
    virtual void set_hlvdif()
    {
      put(get() | HLVDIF);
    }
    virtual void set_oscfif()
    {
      put(get() | OSCFIF);
    }

    void set_bclif() override;
    void set_eeif() override;
    void set_usbif() override;
    void set_cmif() override;
    void set_c1if() override { set_cmif(); }
    void set_c2if() override { set_cmif(); }

    PIR2v4(Processor *pCpu, const char *pName, const char *pDesc,
           INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR2 Peripheral Interrupt register
//
// This is version 5 of the PIR2 register - as seen on the p16f91x devices

class PIR2v5 : public PIR
{
public:
  enum {
    CCP2IF  = 1 << 0,
    LVDIF   = 1 << 2,
    LCDIF   = 1 << 4,
    C1IF    = 1 << 5,
    C2IF    = 1 << 6,
    OSFIF   = 1 << 7
  };

  virtual void set_ccp2if() { put(get() | CCP2IF); }
  void set_lvdif() override { put(get() | LVDIF); }
  virtual void set_lcdif() { put(get() | LCDIF); }
  void set_c1if() override { put(get() | C1IF); }
  void set_c2if() override { put(get() | C2IF); }
  virtual void set_osfif() { put(get() | OSFIF); }

  PIR2v5(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR3 Peripheral Interrupt register # 3
//
// This is version 1 of the PIR3 register - as seen on the 18F6520 devices

class PIR3v1 : public PIR
{
public:
    enum
    {
        CCP3IF = 1 << 0,
        CCP4IF = 1 << 1,
        CCP5IF = 1 << 2,
        TMR4IF = 1 << 3,
        TXIF   = 1 << 4,
        RCIF   = 1 << 5
    };

    void set_ccpif() override    /* RP - needs to define set_ccpif too! */
    {
        put(get() | CCP3IF);
    }

    void set_tmr2if() override
    {
        put(get() | TMR4IF);
    }
    void set_txif() override;
    void set_rcif() override;

    unsigned int get_txif() override
    {
        return value.get() & TXIF;
    }
    void clear_txif() override;
    unsigned int get_rcif() override
    {
        return value.get() & RCIF;
    }
    void clear_rcif() override;

    PIR3v1(Processor *pCpu, const char *pName, const char *pDesc,
           INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR3 Peripheral Interrupt register # 3
//
// This is version 2 of the PIR3 register - as seen on the 18F248 devices
// Perhaps other devices too - it contains bits for the CAN device

class PIR3v2 : public PIR
{
public:
    enum
    {
        RXB0IF  = 1 << 0,
        RXB1IF  = 1 << 1,
        TXB0IF  = 1 << 2,
        TXB1IF  = 1 << 3,
        TXB2IF  = 1 << 4,
        ERRIF   = 1 << 5,
        WAKIF   = 1 << 6,
        IRXIF   = 1 << 7
    };

    void set_rxb0if() override
    {
        put(get() | RXB0IF);
    }

    void set_rxb1if() override
    {
        put(get() | RXB1IF);
    }

    void set_txb0if() override
    {
        put(get() | TXB0IF);
    }

    void set_txb1if() override
    {
        put(get() | TXB1IF);
    }

    void set_txb2if() override
    {
        put(get() | TXB2IF);
    }

    void set_errif() override
    {
        put(get() | ERRIF);
    }

    void set_wakif() override
    {
        put(get() | WAKIF);
    }

    void set_irxif() override
    {
        put(get() | IRXIF);
    }

    PIR3v2(Processor *pCpu, const char *pName, const char *pDesc,
           INTCON *, PIE *);
};

class PIR3v3 : public PIR
{
public:
    enum
    {
        TMR1GIF  = 1 << 0,
        TMR3GIF  = 1 << 1,
        TMR5GIF  = 1 << 2,
        CTMUIF   = 1 << 3,
        TX2IF    = 1 << 4,
        RC2IF    = 1 << 5,
        BCL2IF   = 1 << 6,
        SSP2IF   = 1 << 7
    };
    PIR3v3(Processor *pCpu, const char *pName, const char *pDesc,
           INTCON *, PIE *);
};


/*---------------------------------------------------------
 PIR2 Peripheral Interrupt register # 2

 This is version 1 of the PIR4 register - as seen on the 18f26k22
*/

class PIR4v1: public PIR
{
public:
enum
{
    CCP3IF  = 1 << 0,
    CCP4IF  = 1 << 1,
    CCP5IF  = 1 << 2,
};
  virtual void set_ccp3if()
    {
      put(get() | CCP3IF);
    }
  virtual void set_ccp4if()
    {
      put(get() | CCP4IF);
    }
  virtual void set_ccp5if()
    {
      put(get() | CCP5IF);
    }
  PIR4v1(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};
class PIR5v1: public PIR
{
public:

enum
{
    TMR4IF  = 1 << 0,
    TMR5IF  = 1 << 1,
    TMR6IF  = 1 << 2,
};
  virtual void set_tmr4if()
    {
      put(get() | TMR4IF);
    }
  virtual void set_tmr5if()
    {
      put(get() | TMR5IF);
    }
  virtual void set_tmr6if()
    {
      put(get() | TMR6IF);
    }
  PIR5v1(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};
/*
 * PIR_SET defines an interface to some common interrupt capabilities.
 * PIR_SET is a pure virtual class - you must instantiate a more specific
 * version of PIR_SET.
 *
 * The idea behind PIR_SET is to hide the location of the interrupt bits.
 * in some cases, a bit might be in PIR1, in others it might be in PIR2.
 * Instead of accessing the register directly, you go through PIR_SET
 * and it will find the proper PIR register.
 */

class PIR_SET
{
public:
  virtual ~PIR_SET()
  {
  }

  virtual int interrupt_status()
  {
    return 0;
  }

  // uart stuff
  virtual bool get_txif()
  {
    return false;
  }
  virtual void set_txif() {}
  virtual void clear_txif() {}
  virtual bool get_rcif()
  {
    return false;
  }
  virtual void set_rcif() {}
  virtual void clear_rcif() {}

  // ssp stuff
  virtual bool get_sspif()
  {
    return false;
  }
  virtual void clear_sspif() {}
  virtual void set_sspif() {}
  virtual void set_bclif() {}


  virtual void set_sppif() {}
  virtual void set_pspif() {}
  virtual void set_cmif() {}
  virtual void set_c1if() {}
  virtual void set_c2if() {}
  virtual void set_c3if() {}
  virtual void set_c4if() {}

  // eeprom stuff
  virtual void set_eeif() {}

  // CCP stuff
  virtual void set_ccpif() {}

  // Timer stuff
  virtual void set_tmr1if() {}
  virtual void set_tmr1gif() {}
  virtual void set_tmr2if() {}
  virtual void set_adif() {}

  virtual void set_nco1if() { printf("RRR bad set_nco1if() \n");}
};


//----------------------------------------
// Supports 1 or 2 Pir version 1 registers

class PIR_SET_1 : public PIR_SET
{
public:
    PIR_SET_1()
        : pir1(nullptr), pir2(nullptr)
    {}

    virtual ~PIR_SET_1()
    {
    }

    void set_pir1(PIR *p1) { pir1 = p1; }
    void set_pir2(PIR *p2) { pir2 = p2; }

    int interrupt_status() override
    {
        assert(pir1 != 0);
        if (pir2)
            return pir1->interrupt_status() | pir2->interrupt_status();
        else
            return pir1->interrupt_status();
    }

  // uart stuff
    bool get_txif() override
    {
        assert(pir1 != 0);
        return pir1->get_txif() != 0;
    }
    void set_txif() override
    {
        assert(pir1 != 0);
        pir1->set_txif();
    }
    void clear_txif() override
    {
        assert(pir1 != 0);
        pir1->clear_txif();
    }
    bool get_rcif() override
    {
        assert(pir1 != 0);
        return pir1->get_rcif() != 0;
    }
    void set_rcif() override
    {
        assert(pir1 != 0);
        pir1->set_rcif();
    }
    void clear_rcif() override
    {
        assert(pir1 != 0);
        pir1->clear_rcif();
    }

  // ssp stuff
    bool get_sspif() override
    {
        assert(pir1 != 0);
        return pir1->get_sspif() != 0;
    }
    void set_sspif() override
    {
        assert(pir1 != 0);
        pir1->set_sspif();
    }
    void clear_sspif() override
    {
        assert(pir1 != 0);
        pir1->clear_sspif();
    }

  // eeprom stuff
    void set_eeif() override
    {
        assert(pir1 != 0);
        pir1->set_eeif();
    }

    // CCP stuff
    void set_ccpif() override
    {
        assert(pir1 != 0);
        pir1->set_ccpif();
    }

    // Timer stuff
    void set_tmr1if() override
    {
        assert(pir1 != 0);
        pir1->set_tmr1if();
    }
    void set_tmr2if() override
    {
        assert(pir1 != 0);
        pir1->set_tmr2if();
    }

    // A/D stuff - not part of base PIR_SET class
    void set_adif() override
    {
        assert(pir1 != 0);
        pir1->set_adif();
    }
    // Comparator
    void set_cmif() override
    {
        assert(pir1 != 0);
        pir1->set_cmif();
    }

    void set_c1if() override
    {
        assert(pir1 != 0);
        pir1->set_c1if();
    }
    void set_c2if() override
    {
        assert(pir1 != 0);
        pir1->set_c2if();
    }

    void set_c3if() override
    {
        assert(pir1 != 0);
        pir1->set_c3if();
    }
    void set_c4if() override
    {
        assert(pir1 != 0);
        pir1->set_c4if();
    }

private:
    PIR *pir1;
    PIR *pir2;
};


// Supports 1, 2 ,3, 4 or 5 Pir registers

class PIR_SET_2 : public PIR_SET
{
public:
    PIR_SET_2()
        : pir1(nullptr), pir2(nullptr), pir3(nullptr), pir4(nullptr), pir5(nullptr)
    {}

    virtual ~PIR_SET_2()
    {
    }

    void set_pir1(PIR *p1) { pir1 = p1; }
    void set_pir2(PIR *p2) { pir2 = p2; }
    void set_pir3(PIR *p3) { pir3 = p3; }
    void set_pir4(PIR *p4) { pir4 = p4; }
    void set_pir5(PIR *p5) { pir5 = p5; }

    int interrupt_status() override
    {
        assert(pir1 != 0);
        int result = pir1->interrupt_status();
        if (pir2)
            result |= pir2->interrupt_status();
        if (pir3)
            result |= pir3->interrupt_status();
        if (pir4)
            result |= pir4->interrupt_status();
        if (pir5)
            result |= pir5->interrupt_status();
        return result;
    }

    // uart stuff
    bool get_txif() override
    {
        assert(pir1 != 0);
        return pir1->get_txif() != 0;
    }
    void set_txif() override
    {
        assert(pir1 != 0);
        pir1->set_txif();
    }
    void clear_txif() override
    {
        assert(pir1 != 0);
        pir1->clear_txif();
    }
    bool get_rcif() override
    {
        assert(pir1 != 0);
        return pir1->get_rcif() != 0;
    }
    void set_rcif() override
    {
        assert(pir1 != 0);
        pir1->set_rcif();
    }
    void clear_rcif() override
    {
        assert(pir1 != 0);
        pir1->clear_rcif();
    }

    // ssp stuff
    bool get_sspif() override
    {
        assert(pir1 != 0);
        return pir1->get_sspif() != 0;
    }
    void set_sspif() override
    {
        assert(pir1 != 0);
        pir1->set_sspif();
    }
    void clear_sspif() override
    {
        assert(pir1 != 0);
        pir1->clear_sspif();
    }

    // spp stuff
    virtual bool get_sppif()
    {
        assert(pir1 != 0);
        return pir1->get_sppif() != 0;
    }
    void set_sppif() override
    {
        assert(pir1 != 0);
        pir1->set_sppif();
    }
    virtual void clear_sppif()
    {
        assert(pir1 != 0);
        pir1->clear_sppif();
    }

    // eeprom stuff
    void set_eeif() override
    {
        assert(pir2 != 0);
        pir2->set_eeif();
    }

    // CCP stuff
    void set_ccpif() override
    {
        assert(pir1 != 0);
        pir1->set_ccpif();
    }

    // Timer stuff
    void set_tmr1if() override
    {
        assert(pir1 != 0);
        pir1->set_tmr1if();
    }
    void set_tmr1gif() override
    {
        assert(pir1 != 0);
        pir1->set_tmr1gif();
    }
    void set_tmr2if() override
    {
        assert(pir1 != 0);
        pir1->set_tmr2if();
    }

    // A/D stuff - not part of base PIR_SET class
    void set_adif() override
    {
        assert(pir1 != 0);
        pir1->set_adif();
    }
    // Comparator
    void set_cmif() override
    {
        assert(pir2 != 0);
        pir2->set_cmif();
    }

    void set_c1if() override
    {
        assert(pir2 != 0);
        pir2->set_c1if();
    }
    void set_c2if() override
    {
        assert(pir2 != 0);
        pir2->set_c2if();
    }

    void set_c3if() override
    {
        assert(pir2 != 0);
        pir2->set_c3if();
    }

    void set_c4if() override
    {
        assert(pir2 != 0);
        pir2->set_c4if();
    }

    // I2C master
    void set_bclif() override
    {
        assert(pir2 != 0);
        pir2->set_bclif();
    }

    // Parallel Slave Port
    void set_pspif() override
    {
        assert(pir1 != 0);
        pir1->set_pspif();
    }

private:
    PIR *pir1;
    PIR *pir2;
    PIR *pir3;
    PIR *pir4;
    PIR *pir5;
};

#endif // SRC_PIR_H_
