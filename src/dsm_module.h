#ifndef SRC_DSM_MODULE_h_
#define SRC_DSM_MODULE_h_

#include "registers.h"

class DSM_MODULE;
class IOPIN;
class MDoutSignalSource;
class PinModule;
class Processor;
class SSP1_MODULE;
class USART_MODULE;
class minSink;
class carhSink;
class carlSink;


//_MDCON: MODULATION CONTROL REGISTER
class _MDCON : public sfr_register
{
public:
  _MDCON(Processor *pCpu, const char *pName, const char *pDesc, DSM_MODULE *);
  void put(unsigned int) override;
  void put_value(unsigned int) override;

  unsigned int mask;

private:
    DSM_MODULE *mDSM;
};


// MODULATION SOURCE CONTROL REGISTER
class _MDSRC : public sfr_register
{
public:
  _MDSRC(Processor *pCpu, const char *pName, const char *pDesc, DSM_MODULE *);
  void put(unsigned int) override;
  void put_value(unsigned int) override;

  unsigned int mask;

private:
    DSM_MODULE *mDSM;
};

// _MDCARH: MODULATION HIGH CARRIER CONTROL REGISTER
class _MDCARH : public sfr_register
{
public:
  _MDCARH(Processor *pCpu, const char *pName, const char *pDesc, DSM_MODULE *);
  void put(unsigned int) override;
  void put_value(unsigned int) override;

  unsigned int mask;

private:

    DSM_MODULE *mDSM;
};

// _MDCARL: MODULATION LOW CARRIER CONTROL REGISTER
class _MDCARL : public sfr_register
{
public:
  _MDCARL(Processor *pCpu, const char *pName, const char *pDesc, DSM_MODULE *);
  void put(unsigned int) override;
  void put_value(unsigned int) override;

  unsigned int mask;

private:
    DSM_MODULE *mDSM;
};

class DSM_MODULE
{
public:
  enum {
    MDBIT  = 1<<0, // Allows software to manually set modulation source input to module
    MDOUT  = 1<<3, // Modulator Output bit (read only)
    MDOPOL = 1<<4, // Modulator Output Polarity Select bit
    MDSLR  = 1<<5, // MDOUT Pin Slew Rate Limiting bit
    MDOE   = 1<<6, // Modulator Module Pin Output Enable bit
    MDEN   = 1<<7,  // Modulator Module Enable bit

    MDCHSYNC = 1<<5,  // Modulator High Carrier Synchronization Enable bit
    MDCHPOL = 1<<6, // Modulator High Carrier Polarity Select bit
    MDCHODIS = 1<<7, //Modulator High Carrier Output Disable bit
    MDCLSYNC = 1<<5,  // Modulator Low Carrier Synchronization Enable bit
    MDCLPOL = 1<<6, // Modulator Low Carrier Polarity Select bit
    MDCLODIS = 1<<7, //Modulator Low Carrier Output Disable bit

    MDMSODIS = 1<<7, // Modulation Source Output Disable bit
  };
     explicit DSM_MODULE(Processor *pCpu);
     ~DSM_MODULE();
     _MDCON  mdcon;
     _MDSRC  mdsrc;
     _MDCARH mdcarh;
     _MDCARL mdcarl;

     virtual void setOUTpin(PinModule *pm) {m_mdout = pm;}
     virtual void setMINpin(PinModule *pm) {m_mdmin = pm;}
     virtual void setCIN1pin(PinModule *pm) {m_mdcin1 = pm;}
     virtual void setCIN2pin(PinModule *pm) {m_mdcin2 = pm;}
     virtual void rmModSrc(unsigned int);
     virtual void setModSrc(unsigned int, unsigned int);
     virtual void minEdge(char new3State);
     virtual void carhEdge(char new3State);
     virtual void carlEdge(char new3State);
     virtual void releaseMDout();
     void new_mdcon(unsigned int, unsigned int);
     void new_mdsrc(unsigned int, unsigned int);
     void new_mdcarh(unsigned int, unsigned int);
     void new_mdcarl(unsigned int, unsigned int);
     void dsm_logic(bool carl_neg_edge, bool carh_new_edge);
     void putMDout(bool);
     
     PinModule *m_mdout;
     PinModule *m_mdmin;
     minSink   *m_minSink;
     PinModule *m_mdcin1;
     int       cin1Sink_cnt;
     carlSink  *m_carlSink;
     PinModule *m_mdcin2;
     carhSink  *m_carhSink;
     MDoutSignalSource *out_source;
     char	mdout;
     USART_MODULE *usart_mod;
     SSP1_MODULE  *ssp_mod1;
     SSP1_MODULE  *ssp_mod2;

private:

    bool	mdmin_state;	//value of min 
    bool	mdcarl_state;	//value of carl
    bool	mdcarh_state;	//value of carh

    bool	dflipflopH;
    bool	dflipflopL;
    PinModule   *dsmSrc_pin;
    IOPIN	*monitor_pin;
    PinModule   *monitor_mod;
};

#endif
