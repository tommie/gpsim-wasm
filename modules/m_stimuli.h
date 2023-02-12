/*
   Copyright (C) 2006 Scott Dattalo

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


#ifndef MODULES_MOD_STIMULI_H_
#define MODULES_MOD_STIMULI_H_

#include "../src/modules.h"
#include "../src/trigger.h"
#include "../src/value.h"

#include <iosfwd>
#include <list>
#include <string>

class IO_bi_directional;
class PicPortRegister;
class PicTrisRegister;
class PicLatchRegister;

namespace ExtendedStimuli {

class PulseAttribute;
class PulseInitial;
class PulsePeriodAttribute;


class ValueStimulusData {
public:
  int64_t time;
  Float  *v;

  inline bool operator < (ValueStimulusData &rValue)
  {
    return time < rValue.time;
  }
  inline bool operator == (ValueStimulusData rValue)
  {
    return time == rValue.time;
  }
};


class StimulusBase : public Module, public TriggerObject {
public:
  StimulusBase(const char *_name, const char *_desc);
  ~StimulusBase();

  virtual void callback_print();
  void create_iopin_map();
  void putState(double new_Vth);

protected:
  IO_bi_directional *m_pin;
};


//----------------------------------------------------------------------
class PulseGen : public StimulusBase {
public:
  explicit PulseGen(const char *_name = nullptr);
  ~PulseGen();

  static Module *construct(const char *new_name);

  virtual void callback();
  virtual void put_data(ValueStimulusData &data_point);
  virtual std::string toString();

  void update();
  void update_period();

private:
  PulseAttribute *m_set;
  PulseAttribute *m_clear;
  PulseInitial   *m_init;
  PulsePeriodAttribute *m_period;
  uint64_t m_future_cycle;
  uint64_t m_start_cycle;

  std::list<ValueStimulusData> samples;
  std::list<ValueStimulusData>::iterator sample_iterator;

  void setBreak(uint64_t next_cycle, std::list<ValueStimulusData>::iterator);
};


//----------------------------------------------------------------------
// File Stimulus and Recorder use this attribute
//----------------------------------------------------------------------
template<typename T>
class FileNameAttribute : public String {
public:
  explicit FileNameAttribute(T *parent);

  virtual void update();

private:
  T *m_Parent;
};


//----------------------------------------------------------------------
class FileStimulus : public StimulusBase {
public:
  static Module *construct(const char *new_name);
  explicit FileStimulus(const char *_name);

  void newFile();
  void parseLine(bool first = false);
  virtual void callback();

private:
  FileNameAttribute<FileStimulus> *m_filename;
  std::ifstream *m_fp = nullptr;
  uint64_t m_future_cycle = 0;
  double m_future_value;
};


//----------------------------------------------------------------------
class Recorder_Input;

class FileRecorder : public Module {
public:
  explicit FileRecorder(const char *_name);
  ~FileRecorder();

  static Module *construct(const char *new_name);

  void newFile();
  virtual void record(bool NewVal);
  virtual void record(double NewVal);

private:
  FileNameAttribute<FileRecorder> *m_filename;
  Recorder_Input *m_pin;
  std::ofstream *m_fp;
  double m_lastval;
  unsigned int	 int_lastval;
};


//----------------------------------------------------------------------
class RegisterAddressAttribute; // used for mapping registers into a processor memory
class PortPullupRegister;


class PortStimulus : public Module, public TriggerObject {
public:
  PortStimulus(const char *_name, int nPins);
  ~PortStimulus();

  static Module *construct8(const char *new_name);
  static Module *construct16(const char *new_name);
  static Module *construct32(const char *new_name);

  virtual void callback_print();
  void create_iopin_map();

protected:
  int m_nPins;
  PicPortRegister  *mPort;
  PicTrisRegister  *mTris;
  PicLatchRegister *mLatch;
  PortPullupRegister *mPullup;
  RegisterAddressAttribute *mPortAddress;
  RegisterAddressAttribute *mTrisAddress;
  RegisterAddressAttribute *mLatchAddress;
  RegisterAddressAttribute *mPullupAddress;
};


}

#endif // MODULES_MOD_STIMULI_H_
