/*
   Copyright (C) 1998 T. Scott Dattalo

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


#ifndef SRC_STIMULI_H_
#define SRC_STIMULI_H_

#include <list>
#include <string>

#include <glib.h>

#include "breakpoints.h"
#include "gpsim_object.h"
#include "trigger.h"
#include "value.h"

/* forward references: */
class stimulus;

/* typedefs */
typedef std::list<Value*> SymbolList_t;
typedef std::list<std::string> StringList_t;
typedef std::list<stimulus *> StimulusList_t;
typedef std::list<gpsimObject *> gpsimObjectList_t;

/* Support functions */
extern void dump_stimulus_list();

/****************************************************************************
 *
 * Include file support stimuli.
 *
 *       stimulus             TriggerObject
 *       |  \                      /
 *       |   -----------------+----
 *       |                    |
 *       |- IOPIN             |- source_stimulus
 *          |                              |
 *          |- IO_input                    |- square_wave
 *             |                           |- triangle_wave
 *             |- IO_open_collector        |- asynchronous_stimulus
 *             |- IO_bi_directional        |- dc_supply
 *                |                        |- open_collector
 *                |- IO_bi_directional_pu
 *
 *  A stimulus is used to stimulate stimuli. What's that mean? Well,
 * in gpsim, the pic I/O pins are derived from the stimulus base class
 * (as can be seen from above). The I/O pins are what interface to the
 * 'external' world. In some cases, I/O pins are inputs and others they're
 * outputs. The stimulus base class defines the basic functionality of
 * a stimulus and how this interface to the outside world is to occur.
 *
 */

#define MAX_DRIVE        0x100000
#define MAX_ANALOG_DRIVE 0x1000

class Stimulus_Node : public gpsimObject, public TriggerObject
{
public:
    bool warned;         // keeps track of node warnings (e.g. floating node, contention)
    double voltage;      // The most recent target voltage of this node
    double Cth;          // The most recent capacitance (to ground) measured on this node.
    double Zth;          // The most recent thevenin resistance computed on this node.

    double current_time_constant; // The most recent time constant for the attached stimuli.
    double delta_voltage;     // Amplitude of initial change
    double  minThreshold;     // Use DC value when voltage this close
    guint64 cap_start_cycle;  // cycles when RC value last calculated
    guint64 future_cycle;     // cycles when next callback expected

    double initial_voltage;   // node voltage at the instant of change
    double DCVoltage;      // Target voltage when settling

    bool bSettling;           // true when the voltage is settling
    stimulus *stimuli;        // Pointer to the first stimulus connected to this node.
    int nStimuli;             // number of stimuli attached to this node.

    explicit Stimulus_Node(const char *n = nullptr);
    virtual ~Stimulus_Node();

    void   set_nodeVoltage(double v);
    double get_nodeVoltage();
    double get_nodeZth() { return Zth;}
    double get_nodeCth() { return Cth; }

    void update();

    void attach_stimulus(stimulus *);
    void detach_stimulus(stimulus *);

    // When a node is given a name, it is also added to the symbol
    // table. If bClearableSymbol is true, then the symbol can be
    // automatically removed when the symbol table is cleared.
    virtual void new_name(const char *, bool bClearableSymbol = false);
    virtual void new_name(std::string &, bool bClearableSymbol = false);

    // When the node is settling (due to RC charging/discharging)
    // it's voltage is periodically updated by invoking callback()
    void callback() override;
    void callback_print() override;

    // factory function
    static Stimulus_Node * construct(const char * psName);
    std::string toString() override;

protected:
    void update(guint64 current_time); // deprecated
    void refresh();
    void updateStimuli();
    guint64 calc_settlingTimeStep();

    guint64 settlingTimeStep;
};


//========================================================================
//
// stimulus
//
// The stimulus class is the base class for all of the analog interfaces
// between modules. A stimulus is a 1-node device that has a characteristic
// impedance and voltage. If you're familiar with circuit analysis, these
// are the Thevenin voltage and impedance.
//
// gpsim is not a spice simulator. So complex devices like transistors or
// opamps are not modeled. In fact, even simple devices like capacitors and
// inductors are not modeled.
//
class stimulus : public Value
{
public:
    Stimulus_Node *snode;      // Node to which this stimulus is attached
    stimulus *next;            // next stimulus that's on the snode

    stimulus(const char *n = nullptr,
             double _Vth = 5.0,
             double _Zth = 1e3
            );
    virtual ~stimulus();

    // When a stimulus is given a name, it is also added to the symbol
    // table. If bClearableSymbol is true, then the symbol can be
    // automatically removed when the symbol table is cleared.
    virtual void new_name(const char *, bool bClearableSymbol = true);
    virtual void new_name(std::string &, bool bClearableSymbol = true);

    // Functions for accessing/manipulating the thevenin voltage and impedance.
    virtual void   getThevenin(double &v, double &z, double &c);
    virtual double get_Vth() { return Vth; }
    virtual void   set_Vth(double v) { Vth = v; }
    virtual double get_Zth() { return Zth; }
    virtual void   set_Zth(double z) { Zth = z; }
    virtual double get_Cth() { return Cth; }
    virtual void   set_Cth(double c) { Cth = c; }

    virtual double get_nodeVoltage() { return nodeVoltage; }
    virtual void   set_nodeVoltage(double v) { nodeVoltage = v; }

    virtual bool getDriving() { return bDriving; }
    virtual void setDriving(bool bNewDriving) { bDriving=bNewDriving; }

    // Functions for accessing/manipulating the stimulus state

    // Control the driving state, i.e. the state this stimulus wishes to drive
    virtual bool getDrivingState() {return bDrivingState;}
    virtual void setDrivingState(bool new_dstate) { bDrivingState = new_dstate;}
    virtual void setDrivingState(char new3State)
    { bDrivingState = new3State == '1';}

    // Control the driven state, i.e. the state some external node wishes to
    // drive this stimulus.
    virtual bool getDrivenState() { return getDrivingState(); }
    virtual void setDrivenState(bool new_dstate) { setDrivingState(new_dstate);}

    // Control the 'state' of the node.
    virtual bool getState() { return getDrivingState(); }
    virtual void putState(bool new_dstate) { setDrivingState(new_dstate);}

    // getBitChar - this complements the Register class' getBitStr function
    virtual char getBitChar() { return getState() ? '1' : '0'; }
    virtual void attach(Stimulus_Node *s);
    virtual void detach(Stimulus_Node *s);

    // If a stimulus changes its state, it can signal this change to
    // any other stimuli that are connected to it.
    virtual void updateNode() { if(snode) snode->update();}

    // Display info about the stimulus.
    virtual void show();
    std::string toString() override;

protected:
    bool bDrivingState;        // 0/1 digitization of the analog state we're driving
    bool bDriving;             // True if this stimulus is a driver


    double Vth;                // Open-circuit or Thevenin voltage
    double Zth;                // Input or Thevenin resistance
    double Cth;                // Stimulus capacitance.

    double nodeVoltage;        // The voltage driven on to this stimulus by the snode

    // These are only here because they're pure virtual functions in the parent class.
    unsigned int get_value() override { return 0; }
    virtual void put_value(unsigned int) {}

    // factory function
    static stimulus * construct(const char * psName);
};

class source_stimulus : public stimulus, public TriggerObject
{
public:
    enum SOURCE_TYPE
    {
        DC,
        SQW,
        ASY,
        TRI,
        RESISTOR,
        OPEN_COLLECTOR,
        EVENT
    };

    source_stimulus()
      : digital(true), start_cycle(0), time(0), period(0),
        duty(0), phase(0),
        initial_state(0.0)
    {
    }

    void callback() override;
    void callback_print() override;

    virtual void put_period(Value *);
    virtual void put_duty(Value *);
    virtual void put_phase(Value *);
    virtual void put_initial_state(Value *);
    virtual void put_start_cycle(Value *);
    virtual void set_digital() { digital = true; }
    virtual void set_analog() { digital = false; }
    virtual void start() {}

    void show() override;

protected:
    bool digital;

    guint64
    start_cycle,
    time,
    period,
    duty,
    phase;

    double initial_state;
};


///------------------------------------------------------------
///
/// SignalSink - A pure virtual class that allows signals driven by external
/// stimuli to be routed to one or more objects monitoring them (e.g. one
/// sink may be a bit in a port register while another may be a peripheral)

class SignalSink
{
public:
    virtual ~SignalSink()
    {
    }

    virtual void setSinkState(char) = 0;
    virtual void release() = 0;
};

///-------------------------------------------------------------
///
/// AnalogSink - An analog sink is similar to a digital sink. The primary
/// difference is that an analog sink redirects an analog signal to one
/// or more objects. A signal sink only redirects digital signals.

class AnalogSink
{
public:
    virtual ~AnalogSink()
    {
    }

    virtual void setSinkState(double) = 0;
    virtual void release() = 0;
};


///------------------------------------------------------------
/// The PinMonitor class allows other objects to be notified whenever
/// a Pin changes states.
/// (Note: In older versions of gpsim, iopins notified the Port registers
/// in which they were contained by direcly calling the register setbit()
/// method. This is deprecated - and eventually will cause compile time errors.)
class PinMonitor
{
public:
    PinMonitor();
    virtual ~PinMonitor();

    void addSink(SignalSink *);
    void removeSink(SignalSink *);
    void addSink(AnalogSink *);
    void removeSink(AnalogSink *);

    virtual void setDrivenState(char) = 0;
    virtual void setDrivingState(char) = 0;
    virtual void set_nodeVoltage(double) = 0;
    virtual void putState(char) = 0;
    virtual void setDirection() = 0;
    virtual void updateUI() {}  // FIXME  - make this pure virtual too.

protected:
    /// The SignalSink list is a list of all sinks that can receive digital data
    std::list<SignalSink *> sinks;

    /// The AnalogSink list is a list of all sinks that can receive analog data
    std::list<AnalogSink *> analogSinks;
};

class IOPIN : public stimulus
{
public:
    enum IOPIN_TYPE
    {
        INPUT_ONLY,          // e.g. MCLR
        BI_DIRECTIONAL,      // most iopins
        BI_DIRECTIONAL_PU,   // same as bi_directional, but with pullup resistor. e.g. portb
        OPEN_COLLECTOR       // bit4 in porta on the 18 pin midrange devices.
    };

    enum IOPIN_DIRECTION
    {
        DIR_INPUT,
        DIR_OUTPUT
    };

    IOPIN(const char *n = nullptr,
          double _Vth = 5.0,
          double _Zth = 1e8,
          double _ZthWeak = 1e6,
          double _ZthFloating = 1e7
         );

    ~IOPIN();

    virtual void setMonitor(PinMonitor *);
    virtual PinMonitor *getMonitor() { return m_monitor; }
    void set_nodeVoltage(double v) override;
    bool getDrivingState() override;
    void setDrivingState(bool new_dstate) override;
    void setDrivingState(char) override;
    bool getDrivenState() override;
    void setDrivenState(bool new_dstate) override;
    virtual void forceDrivenState(char);
    virtual char getForcedDrivenState();
    bool getState() override;
    void putState(bool new_dstate) override;
    virtual void putState(double new_Vth);
    virtual void set_digital_threshold(double vdd);
    void get(char *return_str, int len) override;

    virtual void set_ZthWeak(double Z) { ZthWeak = Z;}
    virtual double get_ZthWeak() { return ZthWeak;}
    virtual void set_ZthFloating(double Z) { ZthFloating = Z;}
    virtual double get_ZthFloating() { return ZthFloating;}

    virtual void set_l2h_threshold(double V) {l2h_threshold = V;}
    virtual double get_l2h_threshold() { return l2h_threshold;}
    virtual void set_h2l_threshold(double V) {h2l_threshold = V;}
    virtual double get_h2l_threshold() { return h2l_threshold;}

    virtual void toggle();
    void attach(Stimulus_Node *s) override;

    // These functions don't apply to Inputs, but provide an
    // interface for the derived classes.
    virtual void update_direction(unsigned int /* x */, bool /* refresh */ ) {}
    virtual IOPIN_DIRECTION  get_direction() {return DIR_INPUT; }
    virtual void update_pullup(char /* new_state */, bool /* refresh */ ) {}
    virtual void open_drain(bool) {}
    virtual void input_level(bool /* _level */ ) {}
    virtual void set_is_analog(bool /* flag */ ) {}

    void   getThevenin(double &v, double &z, double &c) override;
    double get_Vth() override;

    char getBitChar() override;
    void show() override;
/// Change object name without affecting stimulus
    virtual void newGUIname(const char *);
    virtual std::string &GUIname() const;
    virtual bool is_newGUIname() { return gui_name_updated; }
    virtual void clr_is_newGUIname() { gui_name_updated = false; }
    virtual void set_schmitt_level(bool _schmitt, double vdd);

protected:
    bool is_analog;  		  // Pin is in analog mode
    bool gui_name_updated;       // True if object name has changed
    std::string gui_name;

    bool bDrivenState;       // binary state we're being driven to
    char cForcedDrivenState; // forced state when no snode is attached.

    PinMonitor *m_monitor;

    // When connected to a node, these are thresholds used to determine whether
    // we're being driven by a weak driver or not.
    double ZthWeak;
    double ZthFloating;

    // These are the low to high and high to low input thresholds. The
    // units are volts.
    double l2h_threshold;
    double h2l_threshold;
    double Vdrive_high;
    double Vdrive_low;
    bool	 schmitt_level;
};

class IO_bi_directional : public IOPIN
{
public:

    IO_bi_directional(const char *n = nullptr,
                      double _Vth = 5.0,
                      double _Zth = 150.0,
                      double _ZthWeak = 1e6,
                      double _ZthFloating = 1e7,
                      double _VthIn = 0.3,
                      double _ZthIn = 1e10);
    double get_Zth() override;
    double get_Vth() override;
    virtual double get_VthIn() { return VthIn;}
    virtual double get_ZthIn() { return ZthIn;}
    virtual void set_VthIn(double _VthIn) { VthIn = _VthIn;}
    virtual void set_ZthIn(double _ZthIn) { ZthIn = _ZthIn;}
    char getBitChar() override;

    void set_nodeVoltage(double new_nodeVoltage) override;
    void putState(bool new_state) override;
    void putState(double new_Vth) override;

    void update_direction(unsigned int,bool refresh) override;
    IOPIN_DIRECTION  get_direction() override
    {return getDriving() ? DIR_OUTPUT : DIR_INPUT;}
    void   getThevenin(double &v, double &z, double &c) override;
    virtual bool getPullupStatus() { return false;}

protected:
    /// Impedance of the IOPIN when it's not driving.
    double ZthIn;

    /// Voltage of the IOPIN when it's not driving
    /// (this is the voltage the I/O pin floats to when there's
    /// nothing connected to it)
    double VthIn;
};


class IO_bi_directional_pu : public IO_bi_directional
{
public:
    IO_bi_directional_pu(const char *n = nullptr,
                         double _Vth = 5.0,
                         double _Zth = 150.0,
                         double _ZthWeak = 1e6,
                         double _ZthFloating = 1e7,
                         double _VthIn = 0.3,
                         double _ZthIn = 1e8,
                         double _Zpullup = 20e3
                        );

    ~IO_bi_directional_pu();

    double get_Vth() override;
    double get_Zth() override;

    virtual void set_Zpullup(double Z) { Zpullup = Z; }
    virtual double get_Zpullup() { return Zpullup; }
    virtual void set_Vpullup(double V) { Vpullup = V; }
    virtual double get_Vpullup() { return Vpullup; }

    //RRRbool getPullupStatus() override { return bPullUp;}
    char getBitChar() override;
    void update_pullup(char new3State, bool refresh) override;
    void set_is_analog(bool flag) override;
    void getThevenin(double &v, double &z, double &c) override;
    bool getPullupStatus() override { return bPullUp;}

protected:
    bool   bPullUp;  // True when pullup is enabled
    double Zpullup;  // resistance of the pullup
    double Vpullup;  // Voltage the pullup resistor is tied to.
};


class IO_open_collector : public IO_bi_directional_pu
{
public:
    explicit IO_open_collector(const char *n = nullptr);
    double get_Zth() override;
    double get_Vth() override;
    char   getBitChar() override;
    void   getThevenin(double &v, double &z, double &c) override;
    void   open_drain(bool _OpenDrain) override { OpenDrain = _OpenDrain;}
protected:
    bool	OpenDrain;
};

class square_wave : public source_stimulus
{
public:
    square_wave(unsigned int _period, unsigned int _duty, unsigned int _phase, const char *n = nullptr);

    double get_Vth() override;
};

class triangle_wave : public source_stimulus
{
public:

    double m1, b1, m2, b2;

    triangle_wave(unsigned int _period, unsigned int _duty, unsigned int _phase, const char *n = nullptr);

    double get_Vth() override;
};

class StimulusData
{
public:
    guint64 time;
    double value;
};

class ValueStimulusData
{
public:
    guint64 time;
    Value  *v;
};

/// ValueStimulus
///

class ValueStimulus : public source_stimulus
{
protected:
    ValueStimulusData  initial;
    Value             *current;
    guint64            future_cycle;
    ValueStimulusData  next_sample;

    std::list<ValueStimulusData> samples;
    std::list<ValueStimulusData>::iterator sample_iterator;

public:
    void callback() override;
    virtual void put_data(ValueStimulusData &data_point);
    void put_initial_state(Value *) override;
    void get(char *return_str, int len) override;

    double get_Vth() override;
    void start() override;

    explicit ValueStimulus(const char *n = nullptr);
    virtual ~ValueStimulus();
    void show() override;

protected:
    ValueStimulusData *getNextSample();
};

class AttributeStimulus : public ValueStimulus
{
    Value    *attr;
public:
    explicit AttributeStimulus(const char *n = nullptr);
    // virtual ~AttributeStimulus();

    void callback() override;
    void setClientAttribute(Value *);
    void show() override;
};

/*
 * An "Event" is a special stimulus that will assert for a single clock
 * cycle.
 *
 * Since Events are derived from the source_stimulus class, they can
 * be either single shot or repetitive.
 *
 */

class Event : public source_stimulus
{
public:
    unsigned int current_state;
    void callback() override;
    Event();
};

class DATA_RECEIVER;

class DATA_SERVER
{
public:
     DATA_SERVER(unsigned int _source_code){ source_code = _source_code;}
     DATA_RECEIVER *data_rcv = nullptr;

     // Allow data_receiver to attach to multiple servers.
     // by taging v2 with server source
     enum
     {
	SERV_MASK = 0xf000,
	CLC       = 0x1000,	//Configurable Logic Cell
	ZCD       = 0x2000,	// Zero Crossing Detector
	CM        = 0x3000,	// Comparator Module
        TMR2      = 0x4000,  	// TMR2(4,6) postscale or match with PR246
	AT1       = 0x5000, 	//AT1 Module
	CCP	  = 0x6000,	// Capture/Compare/PWM module
	PWM	  = 0x7000,	//PWM module
	TMR1	  = 0x8000,	// TMR1(3,5) overflow
	TMR0	  = 0x9000,	// TMR0 overflos
	STM       = 0xa000,     // STM
     };
     const char *mod_name[9] = {"", "CLC", "ZCD", "CM", "TMR2", "AT1", 
				"CCP", "PWN", "TMR1"};

     void send_data(int v1, int v2 = 0);
     void attach_data(DATA_RECEIVER *);
     void detach_data(DATA_RECEIVER *);
private:
    unsigned int source_code;
};

// Data from the data_server is pushed to modules which include this class
// through the rcv_data function
class DATA_RECEIVER
{
public:
    DATA_RECEIVER(const char *_name): next(nullptr),  rcv_name(_name){}
    ~DATA_RECEIVER(){}

    DATA_RECEIVER *next;
    virtual void rcv_data(int v1, int v2){}
    std::string name() {return rcv_name;}
    std::string rcv_name;
     const char *mod_name[8] = {"", "CLC", "ZCD", "CM", "TMR2", "AT1", 
				"CCP", "PWN"};
};

#endif  // SRC_STIMULI_H_
