#include <emscripten/bind.h>

#include <sstream>

#include "../src/gpsim_interface.h"
#include "../src/pic-processor.h"
#include "../src/processor.h"
#include "../src/stimuli.h"
#include "../src/trace.h"
#include "../src/trace_registry.h"
#include "../src/util/cod.h"
#include "../src/util/program.h"

using namespace emscripten;

namespace {

  class InterfaceWrapper : public wrapper<Interface> {
  public:
    EMSCRIPTEN_WRAPPER(InterfaceWrapper);

    void SimulationHasStopped(void *obj) override {
      call<void>("SimulationHasStopped");
    }

    void NewProcessor(Processor *p) override {
      // Passing a raw pointer makes embind use val::take_ownership,
      // deleting the object when returning.
      call<void>("NewProcessor", val(p));
    }

    void NewModule(Module *m) override {
      // Passing a raw pointer makes embind use val::take_ownership,
      // deleting the object when returning.
      call<void>("NewModule", val(m));
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

  Program_Counter* Processor_GetProgramCounter(Processor &p) {
    return p.pc;
  }

  unsigned int Processor_get_register_count(Processor &p) {
    return p.rma.get_size();
  }

  Register* Processor_get_register(Processor &p, unsigned int addr) {
    return p.rma.get_register(addr);
  }

  void Processor_init_program_memory_at_index(Processor &p, unsigned int address, const std::string &data) {
    p.init_program_memory_at_index(address, reinterpret_cast<const uint8_t*>(data.data()), data.size());
  }

  void Processor_step(Processor &p, val cond) {
    if (cond.instanceof(val::global("Function"))) {
      p.step([&cond](unsigned int step) { return cond(step).as<bool>(); });
      return;
    }

    unsigned int nsteps = 1;
    if (cond.isNumber()) {
      nsteps = cond.as<unsigned int>();
    } else if (cond.hasOwnProperty("numSteps")) {
      nsteps = cond["numSteps"].as<unsigned int>();
    }
    p.step([nsteps](unsigned int step) { return step < nsteps; });
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

  std::vector<std::string> SymbolTable_t_symbols(const SymbolTable_t &t) {
    std::vector<std::string> names;
    const_cast<SymbolTable_t&>(t).ForEachSymbol([&names](const SymbolEntry_t &entry) { names.push_back(entry.first); });
    return names;
  }

  std::vector<std::string> SymbolTable_modules(const SymbolTable &t) {
    std::vector<std::string> names;
    const_cast<SymbolTable&>(t).ForEachModule([&names](const SymbolTableEntry_t &entry) { names.push_back(entry.first); });
    return names;
  }

  Processor * CSimulationContext_add_processor_by_type(CSimulationContext *ctx, const std::string &type, const std::string &name) {
    return ctx->add_processor(type.c_str(), name.c_str());
  }

  void gpsimInterface_step_simulation(gpsimInterface &iface, val cond) {
    if (cond.instanceof(val::global("Function"))) {
      iface.step_simulation([&cond](unsigned int step) { return cond(step).as<bool>(); });
      return;
    }

    unsigned int nsteps = 1;
    if (cond.isNumber()) {
      nsteps = cond.as<unsigned int>();
    } else if (cond.hasOwnProperty("numSteps")) {
      nsteps = cond["numSteps"].as<unsigned int>();
    }
    iface.step_simulation([nsteps](unsigned int step) { return step < nsteps; });
  }

  CSimulationContext * gpsimInterface_simulation_context(gpsimInterface &iface) {
    return &iface.simulation_context();
  }

  gpsimInterface * get_interface_wrapper() {
    return &get_interface();
  }

  val TraceReader_front(const trace::TraceReader &reader) {
    if (reader.empty()) return val::undefined();

    auto ref = reader.front();
    auto e = trace::EntryRegistry::parse(ref);

    val o = val::object();

    std::visit([&o](auto &&e) {
      using T = std::decay_t<decltype(e)>;

      if constexpr (std::is_same_v<T, trace::EmptyEntry>) {
        o.set("type", "empty");
      } else if constexpr (std::is_same_v<T, trace::CycleCounterEntry>) {
        o.set("type", "cycleCounter");
        o.set("cycle", static_cast<int>(e.cycle()));
      } else if constexpr (std::is_same_v<T, trace::ReadRegisterEntry>) {
        if (e.address() == 0xFFFF) {
          o.set("type", "readW");
        } else {
          o.set("type", "readRegister");
          o.set("address", e.address());
        }
        o.set("value", e.value());
        if (e.mask() != 0xFF) o.set("mask", e.mask());
      } else if constexpr (std::is_same_v<T, trace::WriteRegisterEntry>) {
        if (e.address() == 0xFFFF) {
          o.set("type", "writeW");
        } else {
          o.set("type", "writeRegister");
          o.set("address", e.address());
        }
        o.set("value", e.value());
        if (e.mask() != 0xFF) o.set("mask", e.mask());
      } else if constexpr (std::is_same_v<T, trace::SetPCEntry>) {
        o.set("type", "setPC");
        o.set("address", e.address());
        o.set("target", e.target());
      } else if constexpr (std::is_same_v<T, trace::IncrementPCEntry>) {
        o.set("type", "incrementPC");
        o.set("address", e.address());
      } else if constexpr (std::is_same_v<T, trace::SkipPCEntry>) {
        o.set("type", "skipPC");
        o.set("address", e.address());
      } else if constexpr (std::is_same_v<T, trace::BranchPCEntry>) {
        o.set("type", "branchPC");
        o.set("address", e.address());
      } else if constexpr (std::is_same_v<T, trace::InterruptEntry>) {
        o.set("type", "interrupt");
      } else if constexpr (std::is_same_v<T, trace::ResetEntry>) {
        o.set("type", "reset");
        o.set("reset", e.type());
      } else {
        o.set("type", static_cast<int>(e.type()));
      }
    }, e);

    return o;
  }

  std::unique_ptr<util::Program> Program_constructor(const std::string &data) {
    auto prog = std::make_unique<util::Program>();
    std::istringstream is(data);

    if (int err = util::CODFileReader::read_program(prog.get(), &is); err) {
      std::ostringstream os;
      os << "Program failed to load: " << err;
      val::global("Error").new_(os.str()).throw_();
      return {};
    }

    if (int err = prog->build_indices(); err) {
      std::ostringstream os;
      os << "Program failed to load (building indices): " << err;
      val::global("Error").new_(os.str()).throw_();
      return {};
    }

    return prog;
  }

  void Program_upload(const util::Program &prog, Processor *p) {
    if (int err = util::upload(p, prog); err) {
      std::ostringstream os;
      os << "Programming failed: " << err;
      val::global("Error").new_(os.str()).throw_();
      return;
    }
  }

  std::vector<util::SourceLineRef> Program_find_lines_by_addr(const util::Program &prog, unsigned int addr) {
    auto refPtrs = prog.find_lines(addr);

    std::vector<util::SourceLineRef> refs;
    refs.reserve(refPtrs.size());
    for (const auto &refPtr : refPtrs) {
      refs.emplace_back(*refPtr);
    }

    return refs;
  }

  EMSCRIPTEN_BINDINGS(libgpsim) {
    enum_<RESET_TYPE>("RESET_TYPE")
      .value("EXIT_RESET", RESET_TYPE::EXIT_RESET)
      .value("MCLR_RESET", RESET_TYPE::MCLR_RESET)
      .value("POR_RESET", RESET_TYPE::POR_RESET)
      .value("SIM_RESET", RESET_TYPE::SIM_RESET);

    enum_<Register::REGISTER_TYPES>("REGISTER_TYPES")
      .value("INVALID_REGISTER", Register::INVALID_REGISTER)
      .value("GENERIC_REGISTER", Register::GENERIC_REGISTER)
      .value("FILE_REGISTER", Register::FILE_REGISTER)
      .value("SFR_REGISTER", Register::SFR_REGISTER)
      .value("BP_REGISTER", Register::BP_REGISTER);

    class_<Interface>("Interface")
      .allow_subclass<InterfaceWrapper>("InterfaceWrapper", constructor<>())
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

    class_<Program_Counter, base<Value>>("Program_Counter")
      .function("get_PC", &Program_Counter::get_PC);

    class_<stimulus, base<Value>>("stimulus");

    class_<IOPIN, base<stimulus>>("IOPIN")
      .function("getBitChar", &IOPIN::getBitChar)
      .function("getMonitor", &IOPIN::getMonitor, allow_raw_pointers());

    class_<Register, base<Value>>("Register")
      .property("address", &Register::address)
      .function("get_value", &Register::get_value)
      .property("isa", &Register::isa);

    class_<Module, base<gpsimObject>>("Module")
      .function("get_pin_count", &Module::get_pin_count)
      .function("get_pin", &Module::get_pin, allow_raw_pointers());

    class_<Processor, base<Module>>("Processor")
      .function("GetProgramCounter", &Processor_GetProgramCounter, allow_raw_pointers())
      .function("disasm", &Processor_disasm)
      .function("get_register_count", &Processor_get_register_count)
      .function("get_register", &Processor_get_register, allow_raw_pointers())
      .function("init_program_memory_at_index", Processor_init_program_memory_at_index)
      .function("reset", &Processor::reset)
      .function("step", &Processor_step);

    class_<pic_processor, base<Processor>>("pic_processor")
      .function("Wget", &pic_processor::Wget);

    class_<ProcessorConstructor>("ProcessorConstructor")
      .function("ConstructProcessor", &ProcessorConstructor_ConstructProcessor, allow_raw_pointers())
      .property("names", &ProcessorConstructor_names)
      .class_function("findByType", &ProcessorConstructor_findByType, allow_raw_pointers())
      .class_function("GetList", ProcessorConstructor_GetList);

    class_<SymbolTable_t>("SymbolTable_t")
      .function("findSymbol", &SymbolTable_t::findSymbol, allow_raw_pointers())
      .property("symbols", &SymbolTable_t_symbols);

    class_<SymbolTable>("SymbolTable")
      .function("find", &SymbolTable::find, allow_raw_pointers())
      .function("findSymbolTable", &SymbolTable::findSymbolTable, allow_raw_pointers())
      .property("modules", &SymbolTable_modules);

    class_<CSimulationContext>("CSimulationContext")
      .function("add_processor", select_overload<Processor*(Processor*)>(&CSimulationContext::add_processor), allow_raw_pointers())
      .function("add_processor_by_type", CSimulationContext_add_processor_by_type, allow_raw_pointers())
      .function("Clear", &CSimulationContext::Clear)
      .function("GetSymbolTable", &CSimulationContext::GetSymbolTable)
      .function("GetTraceReader", &CSimulationContext::GetTraceReader);

    class_<gpsimInterface>("gpsimInterface")
      .constructor()
      .function("add_interface", &gpsimInterface::add_interface, allow_raw_pointers())
      .function("remove_interface", &gpsimInterface::remove_interface)
      .function("simulation_context", gpsimInterface_simulation_context, allow_raw_pointers())
      // The reset function is a no-op with a FIX ME...
      .function("step_simulation", &gpsimInterface_step_simulation);

    class_<trace::TraceReader>("TraceReader")
      .property("discarded", &trace::TraceReader::discarded)
      .property("empty", &trace::TraceReader::empty)
      .property("size", &trace::TraceReader::size)
      .function("front", &TraceReader_front)
      .function("pop", &trace::TraceReader::pop);

    class_<util::CodeRange>("CodeRange")
      .property("address", std::function([](const util::CodeRange &r) {
        return static_cast<unsigned int>(r.addr);
      }))
      .property("code", &util::CodeRange::code);

    class_<util::SourceDirective>("SourceDirective")
      .property("address", std::function([](const util::SourceDirective &dir) {
        return static_cast<unsigned int>(dir.addr);
      }))
      .property("type", std::function([](const util::SourceDirective &dir) {
        return std::string(dir.type);
      }))
      .property("text", std::function([](const util::SourceDirective &dir) {
        return std::string(dir.text);
      }));

    class_<util::SourceLineRef>("SourceLineRef")
      .property("address", std::function([](const util::SourceLineRef &ref) {
        return static_cast<unsigned int>(ref.addr);
      }))
      .property("file", std::function([](const util::SourceLineRef &ref) {
        return std::string(ref.file);
      }))
      .property("line", &util::SourceLineRef::line);

    class_<util::SourceSymbol>("SourceSymbol")
      .property("type", std::function([](const util::SourceSymbol &sym) -> std::string {
        switch (sym.type) {
        case util::SourceSymbolType::DATA: return "data";
        case util::SourceSymbolType::PROGRAM: return "program";
        case util::SourceSymbolType::CONSTANT: return "constant";
        default: return "unknown";
        }
      }))
      .property("name", &util::SourceSymbol::name)
      .property("value", &util::SourceSymbol::value);

    class_<util::Program>("Program")
      .constructor(&Program_constructor)
      .property("code", &util::Program::code)
      .property("directives", &util::Program::directives)
      .property("lineRefs", &util::Program::line_refs)
      .property("symbols", &util::Program::symbols)
      .function("upload", &Program_upload, allow_raw_pointers())
      .function("findLinesByAddr", &Program_find_lines_by_addr)
      .property("targetProcessorType", std::function([](const util::Program &p) {
        return std::string(p.target_processor_type());
      }));

    register_vector<ProcessorConstructor *>("ProcessorConstructorList");
    register_vector<std::string>("StringVector");
    register_vector<util::CodeRange>("CodeRangeVector");
    register_vector<util::SourceDirective>("SourceDirectiveVector");
    register_vector<util::SourceLineRef>("SourceLineRefVector");
    register_vector<util::SourceSymbol>("SourceSymbolVector");

    function("initialize_gpsim_core", initialize_gpsim_core);
    function("get_interface", get_interface_wrapper, allow_raw_pointers());
  }

}
