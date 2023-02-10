/*
   Copyright (C) 1998,1999,2000,2001 T. Scott Dattalo

This file is part of the libgpsim_modules library of gpsim

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


#ifndef MODULES_SOLAR_SOLAR_H_
#define MODULES_SOLAR_SOLAR_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include <config.h>

#include "../../src/modules.h"
#include "../../src/stimuli.h"

class  VSscaleAttribute;
class  VBscaleAttribute;
class  AscaleAttribute;
class  AoffAttribute;
class  inductorAttribute;
class  DOCAttribute;
class  PCM;
class  PCM_ENABLE;

class SolarModule : public Module , public TriggerObject {
public:
  IO_bi_directional_pu *Vsol;
  IO_bi_directional_pu *Vbat;
  IO_bi_directional_pu *Asol;
  PCM *pwm;
  PCM_ENABLE *pwm_enable;

#ifdef MANAGING_GUI

  GtkWidget *pu_window;

#endif

  SolarModule(const char *init_name = nullptr, const char *desc = nullptr);
  ~SolarModule();

  // Inheritances from the Package class
  virtual void create_iopin_map();
  void setPcm(bool);
  void setPcmEnable(bool);
  double battery_voltage(double soc);
  void Solar_panel(double Ton);
  void callback();
  void set_BDOC(double);
  double pvi(double);
  double piv(double);
  void pvi_init();

  static Module *construct(const char *new_name = nullptr);

  double Voc;   	// Solar panel open circuit voltage
  double Isc;   	// Solar panel short circuit current
  double Vmp;		// Solar panel Voltage of max power point
  double Imp;		// Solar panel Current at max power point
  double VbatOC;	// OC Battery voltage
  double Rbat;		// Internal battery resistance
  double Vscale;	// Voltage scale factor
  double VSscale;	// Panel Voltage scale factor
  double VBscale;	// Battery Voltage scale factor
  double Ascale;	// Current scale factor V/A
  double Aoffset;	// Zero current offset voltage
  double BcapMax;	// Max battery capacity amp-hours
  double BDOC;		// Battery Degree of charge
  double cap_mah;	// Battery capacity mAH
  double cur_mah;	// Battery current charge
  double delta_mah;	// accumulate charge added to battery
  bool   enabled;
  bool	 active;
  double Nvt;
  double Isat;
  double Vsp;		// Present solar panel output voltage
  double Isp;		// Present solar panel output current
  double inductor;	// controler inductor

#ifdef MANAGING_GUI
  void build_window();
#endif

private:
  VSscaleAttribute *VSattr;
  VBscaleAttribute *VBattr;
  AscaleAttribute *AVattr;
  AoffAttribute *Aoffattr;
  inductorAttribute *indattr;
  DOCAttribute *docattr;
  uint64_t future_cycle;

  double r0, r1, r2, r3;
  double v1, v2, v3;
  double i1, i2, i3;
  // Variables
  int64_t  Tperiod;	// cycles PWM period
  int64_t  Thigh;	// cycles PWM high
  uint64_t start_cycle;
};


#endif // MODULES_SOLAR_SOLAR_H_
