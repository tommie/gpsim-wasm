/*
   Copyright (C) 2006 Scott Dattalo
   Copyright (C) 2020 Roy R. Rankin

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


/*
  stimuli.cc

  This module provides extended stimuli for gpsim.


  pulsegen - This a stimulus designed to synthesize square waves.
    It has the following attributes:

    .set
    .clear
    .delete
    .period

    The set and clear attributes generate the edge states.
    For example if the module is instantiated as PG, then

    gpsim> PG.set = 0x1000
    gpsim> PG.clear = 0x2000

    generate a rising edge at cycle 0x1000 and a falling edge at
    cycle 0x2000. Any number of edges can be specified and they
    may be in any order. The delete attribute removes an edge:

    gpsim> PG.delete = 0x2000  # remove the edge at cycle 0x2000

    The period attribute tells how many cycles there are in a
    rollover. If period is 0, then the pulsegen is not periodic.
    If there are edges beyond the period time, then those will be
    ignored.

    To be added:

    .start - specify a cycle to start
    .vhi - voltage for high drive
    .vlo - voltage for low drive
    .rth - output resitance
    .cth - output capacitance.


  pwlgen - piecewise linear generator.
    <not implemented>

  FileStimulus - time and values are taken from a file.
    .file - Name of input file.
    .pin  - Output pin
    .xpos - module x position on breadboard
    .ypos - module y position on breadboard

    The input file contains two columns of data seperated by
    one or more tabs and/or spaces.

    The first column is the cycle in base 10.
    The second column is the pin drive voltage.


  FileRecorder - time and values are written to a file.
    .file    - name of file or pipe to write data to
    .digital - is the signal digital (true) or analog (false)
    .pin     - Input pin
    .xpos    - module x position on breadboard
    .ypos    - module y position on breadboard

    The first column is the cycle that pin state changes.
    If .digital=true, second column is pin state - high(1) or low(0)
    If .digital=false (default), second column is the input pin voltage.
        In this mode the output is suitable for input for FileStimulus.

  PortStimulus - module with 8 IO pins
    .portAdr - Output port register address
    .trisAdr - Tristate register address
    .latAdr  - Output latch address
    .pullupAdr - Pullup register address
    .p1-.p8  - IO pins
    .xpos    - module x position on breadboard
    .ypos    - module y position on breadboard

    This module acts like a processor port module.
    Assuming the module is loaded with name P1,
    first declare data memory resgisters as follows:
            UDATA 0x10
	P1_Port RES 1
	P1_Tris RES 1
	P1_Lat  RES 1
	P1_Pullup RES 1
           GLOBAL P1_Port, P1_Tris, P1_Lat, P1_Pullup
	...

        ;# specify the PortStimulus addresses.
        ;  These are the PIC addresses to where the PortStimulus
        ;  registers are mapped.
          .sim "P1.portAdr = &P1_Port"
          .sim "P1.trisAdr = &P1_Tris"
          .sim "P1.latAdr  = &P1_Lat"
          .sim "P1.pullupAdr  = &P1_Pullup"

    Then to access the module register just do operations on the relevant
    on the data registers. i.e.  movwf P1_Lat

*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

#include "m_stimuli.h"
#include "module_attribute.h"

#include "../src/gpsim_time.h"
#include "../src/pic-ioports.h"
#include "../src/registers.h"
#include "../src/stimuli.h"
#include "../src/trace.h"
#include "../src/processor.h"
#include "../src/packages.h"
#include "../src/ui.h"

namespace ExtendedStimuli {


//----------------------------------------------------------------------
// Attributes


//
class PulseAttribute : public Integer {
public:
  PulseAttribute(PulseGen *pParent, const char *_name, const char * desc, double voltage);
  virtual void set(int64_t);

private:
  PulseGen *m_pParent;
  double    m_voltage;
};


class PulsePeriodAttribute : public Integer {
public:
  PulsePeriodAttribute(PulseGen *pParent, const char *_name, const char * desc);
  virtual void set(int64_t);

private:
  PulseGen *m_pParent;
};


class PulseInitial : public Float {
public:
  PulseInitial(PulseGen *pParent, const char *_name, const char * desc, double voltage);
  virtual void set(double);

private:
  PulseGen *m_pParent;
//  double    m_voltage;
};


//----------------------------------------------------------------------
//----------------------------------------------------------------------
PulseAttribute::PulseAttribute(PulseGen *pParent,
                               const char *_name, const char * desc,
                               double voltage)
  : Integer(_name, 0, desc), m_pParent(pParent), m_voltage(voltage)
{
}


void PulseAttribute::set(int64_t i)
{
  Integer::set(i);
  ValueStimulusData vsd;
  vsd.time = i;
  vsd.v = new Float(m_voltage);
  m_pParent->put_data(vsd);
}


//----------------------------------------------------------------------
PulsePeriodAttribute::PulsePeriodAttribute(PulseGen *pParent,
    const char *_name, const char * desc)
  : Integer(_name, 0, desc), m_pParent(pParent)
{
}


void PulsePeriodAttribute::set(int64_t i)
{
  Integer::set(i);
  m_pParent->update_period();
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
PulseInitial::PulseInitial(PulseGen *pParent,
                           const char *_name, const char * desc,
                           double voltage)
  : Float(_name, 0, desc), m_pParent(pParent) //, m_voltage(voltage)
{
}


void PulseInitial::set(double d)
{
  Float::set(d);
  m_pParent->update();
}


//----------------------------------------------------------------------
// StimulusBase
//----------------------------------------------------------------------
StimulusBase::StimulusBase(const char *_name, const char *_desc)
  : Module(_name, _desc)
{
  // The I/O pin
  m_pin = new IO_bi_directional("pin");
  m_pin->set_is_analog(true);
  m_pin->set_Zth(0.01);
  m_pin->update_direction(IOPIN::DIR_OUTPUT, true);
  addSymbol(m_pin);
}


StimulusBase::~StimulusBase()
{
  removeSymbol(m_pin);
}


void StimulusBase::putState(double new_Vth)
{
  m_pin->putState(new_Vth);
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------

void StimulusBase::callback_print()
{
  printf("ExtendedStimulus:%s CallBack ID %d\n", name().c_str(), CallBackID);
}


void StimulusBase::create_iopin_map()
{
  create_pkg(1);
  assign_pin(1, m_pin);
}


//----------------------------------------------------------------------
// FileGen Module
//----------------------------------------------------------------------
//FileGen::FileGen


//----------------------------------------------------------------------
// PulseGen Module
//----------------------------------------------------------------------

Module *PulseGen::construct(const char *new_name)
{
  return new PulseGen(new_name);
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------

PulseGen::PulseGen(const char *_name)
  : StimulusBase(_name,
      "Pulse Generator\n"
      " Attributes:\n"
      " .set - time when the pulse will drive high\n"
      " .clear - time when the pulse will drive low\n"
      " .period - time the pulse stream is repeated\n"
      " .initial - initial pin voltage\n"),
    m_future_cycle(0), m_start_cycle(0)
{
  // Attributes for the pulse generator.
  m_set = new PulseAttribute(this, "set", "r/w cycle time when ouput will be driven high", 5.0);
  m_clear = new PulseAttribute(this, "clear", "r/w cycle time when ouput will be driven low", 0.0);
  m_period = new PulsePeriodAttribute(this,
                                      "period", "r/w cycle time to specify pulse stream repeat rate");
  m_init = new PulseInitial(this, "initial", "initial I/O pin voltage", 5.0);
  addSymbol(m_set);
  addSymbol(m_clear);
  addSymbol(m_period);
  addSymbol(m_init);
  sample_iterator = samples.end();
  create_iopin_map();
}


PulseGen::~PulseGen()
{
  removeSymbol(m_set);
  removeSymbol(m_clear);
  removeSymbol(m_period);
  removeSymbol(m_init);
  delete m_set;
  delete m_clear;
  delete m_period;
  delete m_init;
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
void PulseGen::callback()
{
  //uint64_t currCycle = get_cycles().get();
  if (sample_iterator != samples.end()) {
    m_future_cycle = 0;
    double d = (*sample_iterator).v->get();
    m_pin->putState(d > 2.5);
    ++sample_iterator;

    // If we reached the end of a non-periodic waveform
    // then we're done.

    if (sample_iterator == samples.end()  &&
        m_period->get() == 0) {
      return;
    }

    // If this is a periodic pulse stream and either
    //     a) we reached the end of the sequence
    //     b) we have more data but it exceeds the period
    // then
    //   start the stream over.
    if (m_period->get() && ((sample_iterator == samples.end()  ||
                                (*sample_iterator).time > m_period->get()))) {
      sample_iterator = samples.begin();
      m_start_cycle += m_period->get();
    }

    m_future_cycle = m_start_cycle + (*sample_iterator).time;
    get_cycles().set_break(m_future_cycle, this);
  }
}


//------------------------------------------------------------
// Set a callback break point at the cycle specified
// Point the sample_iterator to the appropiate sample

void  PulseGen::setBreak(uint64_t next_cycle, std::list<ValueStimulusData>::iterator si)
{
  if (m_future_cycle) {
    get_cycles().clear_break(this);
    m_future_cycle = 0;
    sample_iterator = samples.end();
  }

  if (next_cycle > get_cycles().get()) {
    get_cycles().set_break(next_cycle, this);
    m_future_cycle = next_cycle;
    sample_iterator = si;
  }
}


//------------------------------------------------------------
// cycleIsInFuture - find_if helper predicate.
//
// The stl find_if() algorithm takes a pointer to a function whose
// job is to compare a list element to a reference value. Stroustrop
// calls this function a predicate.

static int64_t current_cycle = 0;


static bool cycleIsInFuture(ValueStimulusData &data_point)
{
  return data_point.time > current_cycle;
}


void PulseGen::update_period()
{
  // If the period is 0 then force the start to 0.
  if (m_period->get() == 0) {
    m_start_cycle = 0;
  }

  // Find the next sample that will generate an edge.
  std::list<ValueStimulusData>::iterator si;
  current_cycle = get_cycles().get() -  m_start_cycle;
  si = find_if(samples.begin(), samples.end(), cycleIsInFuture);

  if (si == samples.end() && m_period->get()) {
    setBreak(m_start_cycle + m_period->get(), samples.begin());
  }
}


void PulseGen::update()
{
  if (samples.empty()) {
    double d = m_init->get();
    m_pin->putState(d > 2.5);
    return;  // There are no samples
  }

  current_cycle = get_cycles().get();
  std::list<ValueStimulusData>::iterator si;

  if (current_cycle == 0) {
    // The simulation hasn't started yet.
    // Point to the second sample
    si = samples.begin();
    ++si;

    // If the next sample *is* the second one then we've been here before
    // (and that means we've already handled the first sample)
    if (sample_iterator == si) {
      return;
    }

    if (si == samples.end()) {
      si = samples.begin();
      sample_iterator = si;
      double d = (*si).v->get();
      m_pin->putState(d > 2.5);
    }

    sample_iterator = si;
    --si;
    double d = (*si).v->get();
    m_pin->putState(d > 2.5);
    setBreak((*sample_iterator).time, sample_iterator);
    return;
  }

  current_cycle -= m_start_cycle;
  si = find_if(samples.begin(), samples.end(), cycleIsInFuture);

  if (si == sample_iterator) {
    return;
  }

  setBreak(m_start_cycle + (*si).time, si);
}


static bool compare_data_point(const ValueStimulusData &data_point1,
                               const ValueStimulusData &data_point2)
{
  return data_point1.time < data_point2.time;
}


void PulseGen::put_data(ValueStimulusData &data_point)
{
  std::list<ValueStimulusData>::iterator si;
  si = find(samples.begin(), samples.end(), data_point);

  if (si == samples.end()) {
    samples.push_back(data_point);
    samples.sort(compare_data_point);

  } else {
    delete (*si).v;
    (*si).v = data_point.v;
  }

  update();
}


std::string PulseGen::toString()
{
  std::ostringstream sOut;
  sOut << "pulsegen toString method" << std::hex;
  std::list<ValueStimulusData>::iterator si;

  if (m_period->get()) {
    sOut << "\nperiod 0x" << m_period->get();
  }

  if (m_start_cycle) {
    sOut << "\nstart  0x" << m_start_cycle;
  }

  si = samples.begin();

  while (si != samples.end()) {
    sOut << '\n';
    double d = (*si).v->get();
    sOut << "  {0x" << (*si).time << ',' << d << '}';

    if (si == sample_iterator) {
      sOut << " <-- Next at cycle 0x" << (m_start_cycle + (*si).time);
    }

    ++si;
  }

  return sOut.str();
}


//----------------------------------------------------------------------
// File Stimulus and Recorder use this attribute
//----------------------------------------------------------------------
template<typename T>
FileNameAttribute<T>::FileNameAttribute(T *parent)
  : String("file", "", "Name of a file or pipe"), m_Parent(parent)
{
  parent->addSymbol(this);
}


template<typename T>
void FileNameAttribute<T>::update()
{
  m_Parent->newFile();
}


//----------------------------------------------------------------------
// File Stimulus
//----------------------------------------------------------------------
Module *FileStimulus::construct(const char *new_name)
{
  return new FileStimulus(new_name);
}


FileStimulus::FileStimulus(const char *_name)
  : StimulusBase(_name,
      "File Stimulus\n"
      " Attributes:\n"
      " .file - name of file or pipe supplying data\n"),
      m_future_value(0.0)
{
  // Attributes for the pulse generator.
  m_filename = new FileNameAttribute<FileStimulus>(this);
  create_iopin_map();

  if (verbose) {
    std::cout << description() << '\n';
  }
}


void FileStimulus::newFile()
{
  if (m_future_cycle) {
    get_cycles().clear_break(this);
    m_future_cycle = 0;
  }

  delete m_fp;
  m_fp = nullptr;

  if (!m_filename->get().empty())
  {
    m_fp = new std::ifstream(m_filename->get());
    if (m_fp->fail())
    {
	std::cerr << "Warning " << name() << " cannot open " << m_filename->get() << std::endl;
	delete m_fp;
	m_fp = nullptr;
	return;
    }
  }

  // Set the first breakpoint
  parseLine(true);
}


void FileStimulus::parseLine(bool first)
{
  if (!m_fp || m_fp->eof()) {
    return;
  }

  m_fp->precision(16);
  *m_fp >> std::dec >> m_future_cycle >> m_future_value;

  if (m_fp->eof()) {
    return;
  }

  if (m_fp->fail())
  {
     std::cerr << "File Error " << name() << " " << m_filename->get() << std::endl;
     return;
  }
  if (verbose)
  {
    std::cout << this->name() << " read " << std::dec << m_future_value << " @ 0x"
         << std::hex << m_future_cycle << '\n';
  }

  if (m_future_cycle > get_cycles().get()) {
    get_cycles().set_break(m_future_cycle, this);

  } else if (first) {
    // Always set the reset value
    putState(m_future_value);
    // Set the next breakpoint
    parseLine();

  } else {
    if (verbose)
      std::cout << this->name() << " WARNING: Ignoring past stimulus " << std::dec
           << m_future_value << " @ 0x" << std::hex << m_future_cycle << '\n';

    // Do not set this value, it could have a good or a bad side effect.
    // Set the next breakpoint
    parseLine();
  }
}


void FileStimulus::callback()
{
  // Remove this breakpoint
  get_cycles().clear_break(this);
  m_future_cycle = 0;
  // Set the new value
  putState(m_future_value);
  // Set the next breakpoint
  parseLine();
}


//----------------------------------------------------------------------
// File Recorder
//----------------------------------------------------------------------
class Recorder_Input : public IOPIN {
public:
  Recorder_Input(const char *n, FileRecorder *pParent);
  virtual void setDrivenState(bool);
  virtual void set_nodeVoltage(double);

private:
  bool is_digital();

  FileRecorder *m_pParent;
  BooleanAttribute *m_digitalattribute;
};


Recorder_Input::Recorder_Input(const char *n, FileRecorder *pParent)
  : IOPIN(n), m_pParent(pParent)
{
  m_digitalattribute = new BooleanAttribute("digital", false,
      "Is the signal digital (true) or analog (false)");
  pParent->addSymbol(m_digitalattribute);
}


void Recorder_Input::setDrivenState(bool new_dstate)
{
  IOPIN::setDrivenState(new_dstate);

  if (is_digital()) {
    m_pParent->record(new_dstate);
  }
}


void Recorder_Input::set_nodeVoltage(double v)
{
  IOPIN::set_nodeVoltage(v);

  if (!is_digital()) {
    m_pParent->record(v);
  }
}


bool Recorder_Input::is_digital()
{
  return m_digitalattribute->get();
}


//------------------------------------------------------------------------
Module *FileRecorder::construct(const char *new_name)
{
  FileRecorder *pFileRecorder = new FileRecorder(new_name);
  return pFileRecorder;
}


FileRecorder::FileRecorder(const char *_name)
  : Module(_name,
      "File Recorder\n"
      " Attributes:\n"
      " .file - name of file or pipe to write data to\n"
      " .digital - is the signal digital (true) or analog (false)\n"),
    m_fp(nullptr), m_lastval(99.0), int_lastval(99)
{
  create_pkg(1);
  m_pin = new Recorder_Input("pin", this);
  assign_pin(1, m_pin);
  addSymbol(m_pin);
  // Attribute for the recorder.
  m_filename = new FileNameAttribute<FileRecorder>(this);

  if (verbose) {
    std::cout << description() << '\n';
  }
}


FileRecorder::~FileRecorder()
{
  removeSymbol(m_pin);

  delete m_fp;
}


// Invoked when the FileNameAttribute is updated.
void FileRecorder::newFile()
{
  delete m_fp;

  m_fp = nullptr;

  if (!m_filename->get().empty())
  {
    m_fp = new std::ofstream(m_filename->get());
    if (m_fp->rdstate())
    {
	std::cerr << "Warning " << name() << " cannot open " << m_filename->get() << std::endl;
	delete m_fp;
	m_fp = nullptr;
    }
  }
}


void FileRecorder::record(bool NewVal)
{
  unsigned newvalue = (NewVal ? 1 : 0);


  if (newvalue == int_lastval) {
    return;
  }

  if (m_fp) {
    int64_t cur_cycle = get_cycles().get();
    *m_fp << std::dec << cur_cycle << ' ' << newvalue << std::endl;

    if (verbose) {
      std::cout << this->name() << " recording " << newvalue << " @ 0x" << std::hex
           << cur_cycle << '\n';
    }

    int_lastval = newvalue;
  }
}


void FileRecorder::record(double NewVal)
{

  if (NewVal == m_lastval) {
    return;
  }

  if (m_fp) {
    int64_t cur_cycle = get_cycles().get();
    m_fp->precision(16);
    *m_fp << std::dec << cur_cycle << ' ' << NewVal << std::endl;

    if (verbose) {
      std::cout << this->name() << " recording " << NewVal << " @ 0x" << std::hex
           << cur_cycle << '\n';
    }

    m_lastval = NewVal;
  }
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------

class RegisterAddressAttribute : public Integer {
public:
  RegisterAddressAttribute(Register *pReg, const char *_name, const char * desc);
  virtual void set(int64_t);

private:
  Register *m_replaced;
  const unsigned int InvalidAddress;
};


RegisterAddressAttribute::RegisterAddressAttribute(Register *pReg, const char *_name, const char * desc)
  : Integer(_name, 0xffffffff, desc),
    m_replaced(pReg),
    InvalidAddress(0xffffffff)
{
  m_replaced->address = InvalidAddress;
}


void RegisterAddressAttribute::set(int64_t i)
{
  if (!m_replaced) return;

  Processor *pcpu = dynamic_cast<Processor*>(m_replaced->get_module());

  if (pcpu) {
    if (m_replaced->address != InvalidAddress) {
      pcpu->rma.removeRegister(m_replaced->address, m_replaced);
    }

    m_replaced->address = i & 0xffffffff;

    if (!pcpu->rma.insertRegister(m_replaced->address, m_replaced)) {
      m_replaced->address = InvalidAddress;
    }

    int64_t insertAddress = m_replaced->address;
    Integer::set(insertAddress);
  }
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
class PortPullupRegister : public sfr_register {
public:
  PortPullupRegister(Module *mod, const char *_name,
                     PicPortRegister *_port,
                     unsigned int enableMask);
  ~PortPullupRegister() {}

  virtual void put(unsigned int new_value);

private:
  PicPortRegister *m_port;
  unsigned int m_EnableMask;
};


PortPullupRegister::PortPullupRegister(Module *mod, const char *_name,
                                       PicPortRegister *_port,
                                       unsigned int enableMask)
  : sfr_register(mod, _name, "Port Pullup"), m_port(_port), m_EnableMask(enableMask)
{
  new_name(_name);
  value = RegisterValue(0, ~enableMask);
}


void PortPullupRegister::put(unsigned int new_value)
{
  emplace_value_trace<trace::WriteRegisterEntry>();
  unsigned int diff = (value.data ^ new_value) & m_EnableMask;
  value.data = new_value;

  if (diff && m_port) {
    for (unsigned int i = 1, j = 0; diff && i; i <<= 1, j++)
      if (diff & i) {
        m_port->getPin(j)->update_pullup(((value.data & i) != 0) ? '1' : '0', true);
      }

    m_port->updatePort();
  }
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------

Module *PortStimulus::construct8(const char *new_name)
{
  return new PortStimulus(new_name, 8);
}


Module *PortStimulus::construct16(const char *new_name)
{
  return new PortStimulus(new_name, 16);
}


Module *PortStimulus::construct32(const char *new_name)
{
  return new PortStimulus(new_name, 32);
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------

PortStimulus::PortStimulus(const char *_name, int nPins)
  : Module(_name,
      "Port Stimulus\n"
      " Attributes:\n"
      " .port - port name\n"
      " .tris - tris name\n"
      " .lat  - latch name\n"
      " .pullup  - pullup name\n"),
    m_nPins(nPins)
{
  mPort   = new PicPortRegister((Processor *)this, "port", "", m_nPins, (1 << m_nPins) - 1);
  mTris   = new PicTrisRegister((Processor *)this, "tris", "", mPort, true, (1 << m_nPins) - 1);
  mLatch  = new PicLatchRegister((Processor *)this, "lat", "", mPort, (1 << m_nPins) - 1);
  mPullup = new PortPullupRegister(this, "pullup", mPort, (1 << m_nPins) - 1);
  mPortAddress = new RegisterAddressAttribute(mPort, "portAdr", "Port register address");
  mTrisAddress = new RegisterAddressAttribute(mTris, "trisAdr", "Tris register address");
  mLatchAddress = new RegisterAddressAttribute(mLatch, "latAdr", "Latch register address");
  mPullupAddress = new RegisterAddressAttribute(mPullup, "pullupAdr", "Pullup register address");
  addSymbol(mPort);
  addSymbol(mTris);
  addSymbol(mLatch);
  addSymbol(mPullup);
  addSymbol(mPortAddress);
  addSymbol(mTrisAddress);
  addSymbol(mLatchAddress);
  addSymbol(mPullupAddress);
  create_iopin_map();
}


PortStimulus::~PortStimulus()
{
  printf("~PortStimulus\n");
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------

void PortStimulus::callback_print()
{
  printf("PortStimulus:%s CallBack ID %d\n", name().c_str(), CallBackID);
}


void PortStimulus::create_iopin_map()
{
  create_pkg(m_nPins);
  char pinNumber[12];

  for (int i = 0; i < m_nPins; i++) {
    snprintf(pinNumber, sizeof(pinNumber), "p%d", i+1);

    auto ppin = new IO_bi_directional_pu(pinNumber);
    ppin->update_direction(IOPIN::DIR_OUTPUT, true);
    assign_pin(i + 1, mPort->addPin(this, ppin, i));
  }
}


} // end of namespace ExtendedStimuli
