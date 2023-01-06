/*
   Copyright (C) 2016	Roy R Rankin

This file is part of the libgpsim_extras library of gpsim

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



/* IN_MODULE should be defined for modules */
#define IN_MODULE


#include <iostream>
#include <math.h>


#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include <solar.h>
#include <src/stimuli.h>
#include "src/gpsim_time.h"
#include <src/ioports.h>
#include <src/symbol.h>
#include <src/value.h>
#include <src/gpsim_interface.h>

//----------------------------------------

class VSscaleAttribute : public Float {
public:
  SolarModule *pur;

  explicit VSscaleAttribute(SolarModule *ppur)
    : Float("VSscale", 0.0, "Solar Panel Voltage scale factor"),
      pur(ppur)
  {
    if (pur) {
      Float::set(pur->VSscale);
    }
  }

  virtual void set(double r)
  {
    Float::set(r);

    if (pur) {
      pur->VSscale = r;
    }
  };
};


//----------------------------------------

class inductorAttribute : public Float {
public:
  SolarModule *pur;

  explicit inductorAttribute(SolarModule *ppur)
    : Float("inductor", 0.0, "Buck converter inductor"),
      pur(ppur)
  {
    if (pur) {
      Float::set(pur->inductor);
    }
  }

  virtual void set(double r)
  {
    Float::set(r);

    if (pur) {
      pur->inductor = r;
    }
  };
};


//----------------------------------------

class VBscaleAttribute : public Float {
public:
  SolarModule *pur;

  explicit VBscaleAttribute(SolarModule *ppur)
    : Float("VBscale", 0.0, "Battery Voltage scale factor"),
      pur(ppur)
  {
    if (pur) {
      Float::set(pur->VBscale);
    }
  }

  virtual void set(double r)
  {
    Float::set(r);

    if (pur) {
      pur->VBscale = r;
    }
  }
};


//----------------------------------------

class AscaleAttribute : public Float {
public:
  SolarModule *pur;

  explicit AscaleAttribute(SolarModule *ppur)
    : Float("Ascale", 0.0, "panel current scale factor"),
      pur(ppur)
  {
    if (pur) {
      Float::set(pur->Ascale);
    }
  }

  virtual void set(double r)
  {
    Float::set(r);

    if (pur) {
      pur->Ascale = r;
    }
  }
};


//----------------------------------------

class AoffAttribute : public Float {
public:
  SolarModule *pur;

  explicit AoffAttribute(SolarModule *ppur)
    : Float("Aoffset", 0.0, "panel zero current Voltage"),
      pur(ppur)
  {
    if (pur) {
      Float::set(pur->Aoffset);
    }
  }

  virtual void set(double r)
  {
    Float::set(r);

    if (pur) {
      pur->Aoffset = r;
    }
  }
};


//----------------------------------------

class DOCAttribute : public Float {
public:
  SolarModule *pur;

  explicit DOCAttribute(SolarModule *ppur)
    : Float("BDOC", 0.0, "Battery degree of charge"),
      pur(ppur)
  {
    if (pur) {
      Float::set(pur->BDOC);
    }
  }

  virtual void set(double r)
  {
    Float::set(r);

    if (pur) {
      pur->set_BDOC(r);
    }
  }
};


//--------------------------------------------------------------
// SolarModule::create_iopin_map
//

class PCM : public IOPIN {
public:
  PCM(const char *name, SolarModule *parent);
  virtual void setDrivenState(bool);

private:
  SolarModule *m_Parent;
};


PCM::PCM(const char *name, SolarModule *Parent)
  : IOPIN(name), m_Parent(Parent)
{
}


void PCM::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);

  if (m_Parent) {
    m_Parent->setPcm(bNewState);
  }
}


class PCM_ENABLE : public IOPIN {
public:
  PCM_ENABLE(const char *name, SolarModule *parent);
  virtual void setDrivenState(bool);

private:
  SolarModule *m_Parent;
};


PCM_ENABLE::PCM_ENABLE(const char *name, SolarModule *Parent)
  : IOPIN(name), m_Parent(Parent)
{
}


void PCM_ENABLE::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);

  if (m_Parent) {
    m_Parent->setPcmEnable(bNewState);
  }
}


void SolarModule::setPcmEnable(bool bNewState)
{
  enabled = bNewState;

  if (!enabled) {
    active = false;
    Solar_panel(0.0);
  }
}


void SolarModule::setPcm(bool bNewState)
{
  uint64_t now = get_cycles().get();

  if (!enabled) {
    start_cycle = now;
    return;
  }

  if (bNewState) {
    if (active) {
      static int duty_last = -1;
      int duty;
      double freq = 0.0;
      Tperiod = now - start_cycle;

      if (Tperiod > 0) {
        freq = 1.0 / (Tperiod * get_cycles().seconds_per_cycle());
        duty = (100 * Thigh) / Tperiod;
        delta_mah += Isp * 1000.0 * Tperiod * get_cycles().seconds_per_cycle() / 3600.0;

        if (delta_mah > 0.1) {
          cur_mah += delta_mah;
          set_BDOC(100.0 * cur_mah / cap_mah);
        }

      } else {
        freq = 0.0;
        duty = 0;
      }

      start_cycle = now;

      if (duty != duty_last) {
        printf("%" PRINTF_GINT64_MODIFIER "d cycles  %2" PRINTF_GINT64_MODIFIER "d/%2" PRINTF_GINT64_MODIFIER "d Duty %2d F=%.2fkHz Vsp %.2f Isp %.2f Pout %5.2f Vbat %.2f\n", now, Thigh, Tperiod, duty, freq / 1000., Vsp, Isp, Vsp * Isp, VbatOC + Isp * Rbat);
        duty_last = duty;
      }

    } else {
      active = true;
      start_cycle = now;
    }

  } else if (active) {	// signal has gone low
    Thigh = now - start_cycle;
    Solar_panel(Thigh * get_cycles().seconds_per_cycle());
  }
}


void SolarModule::create_iopin_map()
{
  //   The solar controller has 4 pins.
  //   1 Solar panel voltage readout
  //   2 solar panel current readout
  //   3 battery volgage readout
  //   4 PWM input
  create_pkg(6);
  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created.The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.
  assign_pin(1, Vsol);
  assign_pin(2, Asol);
  assign_pin(3, Vbat);
  assign_pin(4, pwm);
  assign_pin(5, pwm_enable);
}


//--------------------------------------------------------------

Module * SolarModule::construct(const char *_new_name)
{
  SolarModule *pur = new SolarModule(_new_name, "Solar Module");
  return pur;
}


//--------------------------------------------------------------
SolarModule::SolarModule(const char *init_name, const char * desc)
  : Module(init_name, desc), Voc(21.60), Isc(1.27), Vmp(17.3), Imp(1.16),
    Rbat(1.5), cap_mah(20.0), enabled(false)
{
  Vsol = new IO_bi_directional_pu("Vsol");
  addSymbol(Vsol);
  Asol = new IO_bi_directional_pu("Asol");
  addSymbol(Asol);
  Vbat = new IO_bi_directional_pu("Vbat");
  addSymbol(Vbat);
  pwm = new PCM("PWM", this);
  addSymbol(pwm);
  pwm_enable = new PCM_ENABLE("OK", this);
  addSymbol(pwm_enable);
  //res->set_Vpullup(vinit);
  create_iopin_map();
  // Default module attributes.
  //initializeAttributes();
  set_description("\
pullup resistor or generic voltage source\n\
 Attributes:\n\
 .resistance - pullup resistance\n\
 .voltage - pullup or drive voltage\n\
 .capacitance - pin capacitance\n\
");

  if (verbose) {
    std::cout << description() << '\n';
  }

  // Note ResistanceAttribute is designed to give access
  // to res.Zth with a symbol name of "modulename + '.resistance'".
  VSattr = new VSscaleAttribute(this);
  VBattr = new VBscaleAttribute(this);
  AVattr = new AscaleAttribute(this);
  Aoffattr = new AoffAttribute(this);
  indattr = new inductorAttribute(this);
  docattr = new DOCAttribute(this);
  addSymbol(VSattr);
  addSymbol(VBattr);
  addSymbol(AVattr);
  addSymbol(Aoffattr);
  addSymbol(indattr);
  addSymbol(docattr);
  VSattr->set(0.1667);
  VBattr->set(0.1667);
  AVattr->set(0.5);
  Aoffattr->set(2.5);
  indattr->set(47e-6);
  docattr->set(45.0);
  pvi_init();
  start_cycle = 0;
  future_cycle = 0;
  Vsol->set_Vth(Voc * VSscale);
  Vsol->set_Vpullup(Voc * VSscale);
  Vsol->setDriving(false);
  Vsol->update_pullup('1', true);
  Vsol->updateNode();
  Asol->set_Vth(Aoffset);
  Asol->set_Vpullup(Aoffset);
  Asol->setDriving(false);
  Asol->update_pullup('1', true);
  Asol->updateNode();
  Vbat->set_Vth(VbatOC * VBscale);
  Vbat->set_Vpullup(VbatOC * VBscale);
  Vbat->setDriving(false);
  Vbat->update_pullup('1', true);
  Vbat->updateNode();
#ifdef MANAGING_GUI
  pu_window = nullptr;

  if (get_interface().bUsingGUI()) {
    build_window();
  }

#endif
}


SolarModule::~SolarModule()
{
  removeSymbol(VSattr);
  removeSymbol(VBattr);
  removeSymbol(AVattr);
  removeSymbol(Aoffattr);
  removeSymbol(indattr);
  removeSymbol(docattr);
  removeSymbol(Vsol);
  removeSymbol(Asol);
  removeSymbol(Vbat);
  removeSymbol(pwm);
  removeSymbol(pwm_enable);
  delete VSattr;
  delete VBattr;
  delete AVattr;
  delete Aoffattr;
  delete indattr;
  delete docattr;
}


void SolarModule::pvi_init()
{
  // the following is N*f*k*T/q where
  //   N Number of in series solar cells 36 for 12 volt panel
  //   f is a device fudge factor 1.5 typical but 2 gives better results
  //   k boltzmann constant 1.3806488E-23
  //   T temp in kelvin 27C = 300 K
  //   q electron charge 1.60217657E-19
  Nvt = 36 * (2.0 * 1.3806488E-23 * 300.0) / 1.60217657E-19;
  Isat = Isc / (exp(Voc / Nvt) - 1.0);
  v3 = 1.10 * Vmp;
  i3 = Isc - Isat * (exp(v3 / Nvt) - 1.0);
  v2 = Vmp;
  i2 = Imp;
  v1 = 0.95 * Vmp;
  i1 = Isc - Isat * (exp(v1 / Nvt) - 1.0);
  r0 = v1 / (Isc - i1);
  r1 = (v2 - v1) / (Isc - i2 - (v2) / r0);
  r2 = (v3 - v2) / (Isc - i3 - (v3 - v1) / r1 - v3 / r0);
  r3 = (Voc - v3) / (Isc - (Voc - v2) / r2 - (Voc - v1) / r1 - Voc / r0);
}


// Model Solar panel VI characteristics, Return estimate of current for
// given voltage.
// pvi_init() must first be call to compute r0-r3 and v0-v3
// this model follows the work of Mohamed Azab in "Improved Circuit Model
// of Photovoltaic Array" in International Journal of Electrical Power
// and Energy Systems Engineering 2:3 2009
double SolarModule::pvi(double volts)
{
  double di0, di1, di2, di3;

  if (volts >= Voc) {
    return 0.0;
  }

  di1 = di2 = di3 = 0.0;
  di0 = volts / r0;

  if (volts > v1) {
    di1 = (volts - v1) / r1;
  }

  if (volts > v2) {
    di2 = (volts - v2) / r2;
  }

  if (volts > v3) {
    di3 = (volts - v3) / r3;
  }

  return (Isc - di0 - di1 - di2 - di3);
}


// Given the solar panel current, return the voltage
double SolarModule::piv(double I)
{
  double num, denom;

  if (I >= Isc) {
    return 0;
  }

  num = Isc - I;
  denom = 1.0 / r0;

  if (I < i3) {
    num += v3 / r3;
    denom += 1.0 / r3;
  }

  if (I < i2) {
    num += v2 / r2;
    denom += 1.0 / r2;
  }

  if (I < i1) {
    num += v1 / r1;
    denom += 1.0 / r1;
  }

  return num / denom;
}


void SolarModule::set_BDOC(double soc)
{
  BDOC = soc;
  cur_mah = cap_mah * soc / 100.0;
  delta_mah = 0.0;
  VbatOC = battery_voltage(soc);
  Vbat->set_Vth(VbatOC * VBscale);
  Vbat->set_Vpullup(VbatOC * VBscale);
  Vbat->updateNode();
}


double SolarModule::battery_voltage(double soc)
{
  static double bat_soc[] = { 6.0, 11.51, 11.66, 11.81, 11.96, 12.10, 12.24,
                              12.37, 12.50, 12.62, 12.7, 13.5
                            };
  int i1, i2;
  double V;

  if (soc > 110.0) {
    soc = 110.0;
  }

  i1 = soc / 10.0;     // index 0-11
  i2 = i1 + 1;

  if (i1 >= 11) {
    i2 = 11;
    i1 = 10;
  }

  V = (bat_soc[i2] - bat_soc[i1]) * (soc - i1 * 10.0) / (10.0) + bat_soc[i1];
  return V;
}


/*
 * 	Given the time in seconds that the drive pulse is high (in seconds),
 * 	compute the solar panel current voltage operating point.
 *
 *	This function views a solar panel as having 3 regions
 *	Region 1 Voltage between 0 and 0.95 * Vmppt where panel acts as
 *	         a current source. (voltage changes faster than current)
 *	         In this region we hunt for the voltage where the panel
 *	         current and inductor current match.
 *	Region 2 Voltage is between Voc and 1.1 * Vmppt and panel acts
 *	         like a Voltage source (current changes faster than voltage)
 *	         In this region we hunt for the current which matches the
 *	         panel and inductor voltages
 *	Region 3 is the transition between the two regions and is where the
 *	         maximum power point lies.
 *	         This region is a hystorises region of the computation.
 *	         If the initial voltage is less than Vmppt we use the region
 *	         1 algorithm unless we go into region 2 in which case we
 *	         change to that algorithm. Comversly if the inital voltage
 *	         for the interations is greater than Vmppt we use the region 2
 *	         algorithm unless we go into region 1 in which case we change
 *	         to that algorithm.
 */
void SolarModule::Solar_panel(double Ton)
{
  double Vfirst = Vsp;
  double vbat = VbatOC;
  uint64_t cycle_off;
  Rbat = 0.0005 * exp(0.11 * BDOC);
  // The following is an estimate of the average current
  // through an inductor when a constant voltage is applied

  if (Ton) {
    double dv = 1.0;
    int i;

    for (i = 0; i < 50 && fabs(dv) > 0.005; i++) {
      double Isol;

      if (Vsp < vbat) {
        Isp = pvi(vbat);
        Vsp = vbat + Isp * Rbat;

      } else if (Vsp > Voc) {
        Vsp = Voc;
        Isp = pvi(vbat);
      }

      if (Vfirst < v2 && Vsp < v3) {	// Solar panel acting as current source
        Isol = pvi(Vsp);
        dv  = Isp * (2 * inductor / Ton + Rbat) + vbat - Vsp;

        if (dv > Voc - Vsp) {
          dv = Voc - Vsp;

        } else if (dv < vbat - Vsp) {
          dv = vbat - Vsp;
        }

        Vsp += dv / 3;
        Isp = pvi(Vsp);

        if (Vsp > v3) {
          //		fprintf(stderr, "Switch R1 to R2 i = %d\n", i);
          Vfirst = Vsp;
        }
      }

      //  else if (Vsp >= v3)	// Solar panel more like voltage source
      else {
        double Iind;

        if (Isp > Isc) {
          Isp = Isc;

        } else if (Isp < 0.0) {
          Isp = 0.1;
        }

        Vsp = piv(Isp);

        if (Vsp < vbat + Isp * Rbat) {
          Vsp = vbat + Isp * Rbat;
        }

        Iind = (Vsp - vbat) / (2 * inductor / Ton + Rbat);
        Isol = pvi(Vsp);
        dv = Iind - Isol;
        Isp += dv / 6.0;
        Vsp = piv(Isp);

        if (Vsp < v1) {
          //		fprintf(stderr, "Switch R2 to R1 i=%d\n", i);
          Vfirst = Vsp;
        }
      }

      if (Vsp > Voc) {
        Vsp = Voc;

      } else if (Vsp < 0.0) {
        Vsp = vbat;
      }
    }

    if (i > 49) {
      fprintf(stderr, "%s did not converge Vsp %.2f Isp %.2f\n", __FUNCTION__, Vsp, Isp);
    }

  } else {	//Ton == 0
    Vsp = Voc;
    Isp = 0.0;
  }

  if (Ton) {
    cycle_off = 2200;

  } else {
    cycle_off = 0;
  }

  Vsol->set_Vth(Vsp * VSscale);
  Vsol->set_Vpullup(Vsp * VSscale);
  Vsol->updateNode();
  Vbat->set_Vth((vbat + Rbat * Isp)*VBscale);
  Vbat->set_Vpullup((vbat + Rbat * Isp)*VBscale);
  Vbat->updateNode();
  Asol->set_Vth(Aoffset + Isp * Ascale);
  Asol->set_Vpullup(Aoffset + Isp * Ascale);
  Asol->updateNode();

  if (future_cycle) {
    if (cycle_off) {
      uint64_t fc = cycle_off + get_cycles().get();
      get_cycles().reassign_break(future_cycle, fc, this);
      future_cycle = fc;

    } else {
      get_cycles().clear_break(this);
      future_cycle = 0;
    }

  } else if (cycle_off) {
    future_cycle = cycle_off + get_cycles().get();
    get_cycles().set_break(future_cycle, this);
  }
}


void SolarModule::callback()
{
  get_cycles().clear_break(this);
  future_cycle = 0;
  Solar_panel(0.0);
}


#ifdef MANAGING_GUI

static void pu_cb(GtkWidget *button, SolarModule * pur_class)
{
  // SolarModule *pur = pur_class;      ??? What should this do
}


void SolarModule::build_window()
{
  GtkWidget *buttonbox;
  GtkWidget *button;
  pu_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  buttonbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(pu_window), buttonbox);
  gtk_container_set_border_width(GTK_CONTAINER(buttonbox), 1);
  button = gtk_button_new_with_label(name().c_str());
  g_signal_connect(button, "clicked",
                   G_CALLBACK(pu_cb), (gpointer)this);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0);
  gtk_widget_show_all(pu_window);
  set_widget(pu_window);
}


#endif
