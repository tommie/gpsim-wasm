#include <emscripten/bind.h>

#include "../src/gpsim_interface.h"
#include "../src/processor.h"
#include "../src/stimuli.h"

using namespace emscripten;

namespace {

  class InterfaceWrapper : public wrapper<Interface> {
  public:
    EMSCRIPTEN_WRAPPER(InterfaceWrapper);

    void UpdateObject(void *obj, int new_value) override {
      call<void>("UpdateObject", new_value);
    }

    void SimulationHasStopped(void *obj) override {
      call<void>("SimulationHasStopped");
    }

    void NewProcessor(Processor *p) override {
      // Passing a raw pointer makes embind use val::take_ownership,
      // deleting the object when returning.
      call<void>("NewProcessor", val(p));
    }

    void Update(void *obj) override {
      call<void>("Update");
    }
  };

  class SignalSinkWrapper : public wrapper<SignalSink> {
  public:
    EMSCRIPTEN_WRAPPER(SignalSinkWrapper);

    void setSinkState(char v) {
      call<void>("setSinkState", v);
    }

    void release() {
      call<void>("release");
    }
  };

  std::string Processor_disasm(const Processor &p, unsigned int address) {
    if (!p.pma) return "";

    auto *insn = p.pma->getFromAddress(address);
    if (!insn) return p.bad_instruction.name();

    // Note that name() and name(buf, n) don't return the same thing.
    char buf[1024];
    return insn->name(buf, sizeof(buf));
  }

  unsigned int Processor_get_PC(const Processor &p) {
    return p.pc->get_PC();
  }

  void Processor_init_program_memory_at_index(Processor *p, unsigned int address, const std::string &data) {
    p->init_program_memory_at_index(address, reinterpret_cast<const uint8_t*>(data.data()), data.size());
  }

  std::vector<std::string> ProcessorConstructor_names(const ProcessorConstructor &self) {
    std::vector<std::string> names;

    for (int i = 0; i < nProcessorNames; ++i) {
      if (self.names[i]) names.emplace_back(self.names[i]);
    }

    return names;
  }

  ProcessorConstructor * ProcessorConstructor_findByType(const std::string &type) {
    return ProcessorConstructor::findByType(type.c_str());
  }

  std::vector<ProcessorConstructor *> ProcessorConstructor_GetList() {
    const auto &list = *ProcessorConstructor::GetList();

    return std::vector<ProcessorConstructor *>(std::begin(list), std::end(list));
  }

  Processor * ProcessorConstructor_ConstructProcessor(ProcessorConstructor *cons, const std::string &name) {
    return cons->ConstructProcessor(name.c_str());
  }

  Processor * CSimulationContext_add_processor_by_type(CSimulationContext *ctx, const std::string &type, const std::string &name) {
    return ctx->add_processor(type.c_str(), name.c_str());
  }

  CSimulationContext * gpsimInterface_simulation_context(gpsimInterface *iface) {
    return &iface->simulation_context();
  }

  gpsimInterface * get_interface_wrapper() {
    return &get_interface();
  }

  EMSCRIPTEN_BINDINGS(libgpsim) {
    enum_<gpsimInterface::eAdvancementModes>("gpsimInterface_eAdvancementModes")
      .value("eAdvanceNextInstruction", gpsimInterface::eAdvanceNextInstruction)
      .value("eAdvanceNextCycle", gpsimInterface::eAdvanceNextCycle)
      .value("eAdvanceNextCall", gpsimInterface::eAdvanceNextCall)
      .value("eAdvanceNextReturn", gpsimInterface::eAdvanceNextReturn);

    enum_<RESET_TYPE>("RESET_TYPE")
      .value("MCLR_RESET", RESET_TYPE::MCLR_RESET)
      .value("SIM_RESET", RESET_TYPE::SIM_RESET);

    class_<Interface>("Interface")
      .allow_subclass<InterfaceWrapper>("InterfaceWrapper", constructor<>())
      .function("UpdateObject", optional_override([](Interface &self, void *obj, int new_value) { return self.Interface::UpdateObject(obj, new_value); }), allow_raw_pointers())
      .function("SimulationHasStopped", optional_override([](Interface &self, void *obj) { return self.Interface::SimulationHasStopped(obj); }), allow_raw_pointers())
      .function("NewProcessor", optional_override([](Interface &self, Processor *p) { return self.Interface::NewProcessor(p); }), allow_raw_pointers())
      .function("Update", optional_override([](Interface &self, void *obj) { return self.Interface::Update(obj); }), allow_raw_pointers())
      .function("get_id", &Interface::get_id)
      .function("set_id", &Interface::set_id);

    class_<SignalSink>("SignalSink")
      .allow_subclass<SignalSinkWrapper>("SignalSinkWrapper", constructor<>())
      .function("setSinkState", &SignalSink::setSinkState)
      .function("release", &SignalSink::release);

    class_<PinMonitor>("PinMonitor")
      .function("addSignalSink", select_overload<void(SignalSink*)>(&PinMonitor::addSink), allow_raw_pointers());

    class_<gpsimObject>("gpsimObject")
      .function("description", &gpsimObject::description)
      .function("name", select_overload<std::string&() const>(&gpsimObject::name));

    class_<Value, base<gpsimObject>>("Value");

    class_<stimulus, base<Value>>("stimulus");

    class_<IOPIN, base<stimulus>>("IOPIN")
      .function("getMonitor", &IOPIN::getMonitor, allow_raw_pointers());

    class_<Module>("Module")
      .function("get_pin_count", &Module::get_pin_count)
      .function("get_pin", &Module::get_pin, allow_raw_pointers());

    class_<Processor, base<Module>>("Processor")
      .function("disasm", &Processor_disasm)
      .function("get_PC", &Processor_get_PC)
      .function("init_program_memory_at_index", Processor_init_program_memory_at_index, allow_raw_pointers())
      .function("reset", &Processor::reset)
      .function("step", &Processor::step);

    class_<ProcessorConstructor>("ProcessorConstructor")
      .function("ConstructProcessor", &ProcessorConstructor_ConstructProcessor, allow_raw_pointers())
      .property("names", &ProcessorConstructor_names)
      .class_function("findByType", &ProcessorConstructor_findByType, allow_raw_pointers())
      .class_function("GetList", ProcessorConstructor_GetList);

    class_<CSimulationContext>("CSimulationContext")
      .function("add_processor", select_overload<Processor*(Processor*)>(&CSimulationContext::add_processor), allow_raw_pointers())
      .function("add_processor_by_type", CSimulationContext_add_processor_by_type, allow_raw_pointers())
      .function("Clear", &CSimulationContext::Clear);

    class_<gpsimInterface>("gpsimInterface")
      .constructor()
      // The reset function is a no-op with a FIX ME...
      .function("step_simulation", &gpsimInterface::step_simulation)
      .function("add_interface", &gpsimInterface::add_interface, allow_raw_pointers())
      .function("remove_interface", &gpsimInterface::remove_interface)
      .function("simulation_context", gpsimInterface_simulation_context, allow_raw_pointers());

    register_vector<ProcessorConstructor *>("ProcessorConstructorList");
    register_vector<std::string>("StringVector");

    function("initialize_gpsim_core", initialize_gpsim_core);
    function("get_interface", get_interface_wrapper, allow_raw_pointers());
  }

}
