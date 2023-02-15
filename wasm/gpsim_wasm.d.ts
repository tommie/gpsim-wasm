export default function(opts: {
  print: typeof console.log,
  printErr: typeof console.error,
  noInitialRun: boolean,
}): Promise<GPSIMModule>;

interface GPSIMModule {
  RESET_TYPE: typeof RESET_TYPE;
  REGISTER_TYPES: REGISTER_TYPES;

  Interface: EmConstructor<Interface>;
  SignalSink: EmConstructor<SignalSink>;
  ProcessorConstructor: typeof ProcessorConstructor;
  Program: typeof Program;

  get_interface(): gpsimInterface;
  initialize_gpsim_core(): void;
}

declare enum RESET_TYPE {
  EXIT_RESET,
  MCLR_RESET,
  POR_RESET,
  SIM_RESET,
}

declare enum REGISTER_TYPES {
  INVALID_REGISTER,
  GENERIC_REGISTER,
  FILE_REGISTER,
  SFR_REGISTER,
  BP_REGISTER,
}

declare abstract class Interface extends EmObject {
  SimulationHasStopped(): void;
  NewProcessor(p: Processor): void;
  NewModule(m: Module): void;
  Update(): void;
  get_id(): number;
  set_id(id: number): void;
}

declare abstract class SignalSink extends EmObject {
  setSinkState(v: number): void;
  release(): void;
}

declare class PinMonitor extends EmObject {
  addSignalSink(s: SignalSink): void;
}

declare class gpsimObject extends EmObject {
  description(): string;
  name(): string;
}

declare class Value extends gpsimObject {}

declare class Program_Counter extends EmObject {
  get_PC(): number;
}

declare class stimulus extends Value {
}

declare class IOPIN extends stimulus {
  getBitChar(): number;
  getMonitor(): PinMonitor;
}

declare class Register extends Value {
  address: number;
  get_value(): number;
  isa: number;
}

declare class Module extends gpsimObject {
  get_pin_count(): number;
  get_pin(num: number): IOPIN | null;
}

declare class Processor extends Module {
  GetProgramCounter(): Program_Counter;
  disasm(addr: number): string;
  get_register_count(): number;
  get_register(addr: number): Register | null;
  init_program_memory_at_index(addr: number, data: Uint8Array): void;
  reset(type: RESET_TYPE): void;
  step(cond: StepCondition): void;
}

type StepCondition = ((step: number) => boolean) | number | { numSteps?: number };

declare class pic_processor extends Processor {
  Wget(): number;
}

declare class ProcessorConstructor extends EmObject {
  ConstructProcessor(name: string): Processor;
  names: EmVector<string>;
  static findByType(type: string): ProcessorConstructor | null;
  static GetList(): EmVector<ProcessorConstructor>;
}

declare class SymbolTable_t extends EmObject {
  findSymbol(name: string): gpsimObject | null;
  symbols: string[];
}

declare class SymbolTable extends EmObject {
  find(name: string): gpsimObject | null;
  findSymbolTable(name: string): SymbolTable_t | null;
  modules: string[];
}

declare class CSimulationContext extends EmObject {
  add_processor(p: Processor): Processor;
  add_processor_by_type(type: string, name: string): Processor;
  Clear(): void;
  GetSymbolTable(): SymbolTable;
  GetTraceReader(): TraceReader;
}

declare class gpsimInterface extends EmObject {
  add_interface(iface: Interface): void;
  remove_interface(id: number): void;
  simulation_context(): CSimulationContext;
  step_simulation(cond: StepCondition): void;
}

declare class TraceReader extends EmObject {
  discarded: number;
  empty: boolean;
  size: number;
  front(): TraceEntry | undefined;
  pop(): void;
}

interface EmptyEntry {
  type: 'empty';
}

interface CycleCounterEntry {
  type: 'cycleCounter';
  cycle: number;
}

interface RegisterEntry {
  type: 'readRegister' | 'writeRegister';
  address: number;
  value: number;
}

interface WEntry {
  type: 'readW' | 'writeW';
  address: number;
  value: number;
}

interface PCEntry {
  type: 'setPC' | 'incrementPC' | 'skipPC' | 'branchPC';
  address: number;
  target?: number;
  insn?: string;
}

interface InterruptEntry {
  type: 'interrupt';
}

interface ResetEntry {
  type: 'reset';
  reset: string;
}

type TraceEntry = EmptyEntry | CycleCounterEntry | RegisterEntry | WEntry | PCEntry | InterruptEntry | ResetEntry;

declare class CodeRange extends EmObject {
  address: number;
  code: Uint8Array;
}

declare class SourceDirective extends EmObject {
  address: number;
  type: string;
  text: string;
}

declare class SourceLineRef extends EmObject {
  address: number;
  file: string;
  line: number;
}

declare class SourceSymbol extends EmObject {
  type: string;
  name: string;
  value: number;
}

declare class Program extends EmObject {
  constructor(firmware: Uint8Array);
  targetProcessorType: string;
  code: EmVector<CodeRange>;
  directives: EmVector<SourceDirective>;
  lineRefs: EmVector<SourceLineRef>;
  symbols: EmVector<SourceSymbol>;

  findLinesByAddr(addr: number): EmVector<SourceLineRef>;
  upload(p: Processor): void;
}

//
// EmBind common types
//

interface EmVector<T> extends EmObject {
  size(): number;
  get(i: number): T;
  set(i: number, v: T): void;
}

// TODO: Can this be made into an EmSubclassable<T> so we can use
// "typeof EmSubclassable<T>" above? Or at least infer E's constructor
// parameters from __construct?
interface EmConstructor<T extends Object> {
  new(...vs: any[]): T;
  extend<E extends T>(name: string, props: Partial<EmConstructorProps<T, E>> & ThisType<E>): EmConstructor<E>;
}

type EmConstructorProps<T extends Object, E extends T> = Partial<E> & {
  __construct(this: EmConstructorPropsThis<T, E>, ...vs: any[]): void;
};

type EmConstructorPropsThis<T extends Object, E extends T> = EmConstructorProps<T, E> & {
  __parent: T extends never ? never : EmConstructorPropsThis<Object, T>;
};

declare class EmObject {
  delete(): void;
}
