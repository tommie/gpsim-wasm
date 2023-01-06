/*
   Copyright (C) 2014 Roy R Rankin

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

#ifndef SRC_SPP_H_
#define SRC_SPP_H_

#include "registers.h"
#include "trigger.h"

class PIR_SET;
class PinModule;
class Processor;
class SPP;
class SppSignalSource;
class PicPSP_PortRegister;
class PicTrisRegister;

class SPPCON : public sfr_register, public TriggerObject {
public:
  SPPCON(Processor *pCpu, const char *pName, const char *pDesc);
  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  void set_spp(SPP *_spp)
  {
    spp = _spp;
  }

private:
  SPP	*spp = nullptr;
};


class SPPCFG : public sfr_register, public TriggerObject {
public:
  SPPCFG(Processor *pCpu, const char *pName, const char *pDesc);
  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  void set_spp(SPP *_spp)
  {
    spp = _spp;
  }

private:
  SPP	*spp = nullptr;
};


class SPPEPS : public sfr_register, public TriggerObject {
public:
  SPPEPS(Processor *pCpu, const char *pName, const char *pDesc);
  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  void set_spp(SPP *_spp)
  {
    spp = _spp;
  }

private:
  SPP	*spp = nullptr;
};


class SPPDATA : public sfr_register, public TriggerObject {
public:
  SPPDATA(Processor *pCpu, const char *pName, const char *pDesc);
  void put(unsigned int new_value) override;
  unsigned int get() override;
  void put_value(unsigned int new_value) override;
  void set_spp(SPP *_spp)
  {
    spp = _spp;
  }

private:
  SPP	*spp;
};


class SPP : public TriggerObject {
public:
  SPP()
  {
  }
  ~SPP();

  void initialize(PIR_SET *pir_set, PicPSP_PortRegister *port_set,
                  PicTrisRegister *port_tris,
                  SPPCON *_sppcon, SPPCFG *_sppcfg, SPPEPS *_sppeps,
                  SPPDATA *_sppdata, PinModule  *pin_clk1spp, PinModule *pin_clk2spp,
                  PinModule  *pin_oespp, PinModule *pin_csspp
                 );
  void data_write(unsigned int data);
  unsigned int data_read();
  void eps_write(unsigned int data);
  void cfg_write(unsigned int data);
  void enabled(bool state);
  void callback() override;

  enum {
    SPPEN	= 1 << 0,		// SPPCON
    SPPOWN	= 1 << 1,
    WS0	= 1 << 0,		// SPPCFG
    WS1	= 1 << 1,
    WS2	= 1 << 2,
    WS3	= 1 << 3,
    CLK1EN	= 1 << 4,
    CSEN	= 1 << 5,
    CLKCFG0 = 1 << 6,
    CLKCFG1 = 1 << 7,
    ADDR0	= 1 << 0,		//SPPEPS
    ADDR1	= 1 << 1,
    ADDR2	= 1 << 2,
    ADDR3	= 1 << 3,
    SPPBUSY = 1 << 4,
    WRSPP	= 1 << 6,
    RDSPP	= 1 << 7
  };

protected:
  // cycle States
  enum {
    ST_IDLE = 0,
    ST_CYCLE1,
    ST_CYCLE2,
  };
  // I/O operatiom
  enum {
    ADDR_WRITE = 1,
    DATA_WRITE,
    DATA_READ
  };

  SPPCON	*sppcon = nullptr;
  SPPCFG	*sppcfg = nullptr;
  SPPEPS	*sppeps = nullptr;
  SPPDATA	*sppdata = nullptr;
  bool		state_enabled = false;
  unsigned int  cfg_value = 0;
  unsigned int  eps_value = 0;
  unsigned int  data_value = 0;
  PinModule 	*pin_clk1spp = nullptr;
  PinModule 	*pin_clk2spp = nullptr;
  PinModule 	*pin_oespp = nullptr;
  PinModule 	*pin_csspp = nullptr;
  int		cycle_state = 0;
  unsigned int  io_operation = 0;
  SppSignalSource *sig_oespp = nullptr;
  SppSignalSource *sig_csspp = nullptr;
  SppSignalSource *sig_clk1spp = nullptr;
  SppSignalSource *sig_clk2spp = nullptr;
  bool		active_sig_oe = false;
  bool		active_sig_cs = false;
  bool		active_sig_clk1 = false;
  bool		active_sig_clk2 = false;
  PIR_SET 	*pir_set = nullptr;
  PicPSP_PortRegister *parallel_port = nullptr;
  PicTrisRegister *parallel_tris = nullptr;
};

#endif	// SRC_SPP_H_
