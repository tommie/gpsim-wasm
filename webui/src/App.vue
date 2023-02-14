<script setup lang="ts">
import {
  onMounted,
  markRaw,
  reactive,
  ref,
  shallowReactive,
  shallowRef,
  watch,
} from 'vue';

import gpsimLoad_ from '../../wasm/.build/wasm/gpsim_wasm.mjs';

function zeroPaddedHex(i: number, w: number) {
  return i.toString(0x10).padStart(w, '0');
}

function base64Encode(v: Uint8Array) {
  const CHUNK_SZ = 0x8000;

  let c = [];
  for (let i = 0; i < v.length; i += CHUNK_SZ) {
    c.push(String.fromCharCode.apply(null, v.subarray(i, i + CHUNK_SZ)));
  }
  return btoa(c.join(''));
}

function base64Decode(s: string) {
  return new Uint8Array(atob(s.replace(/[\n]/g, '')).
                                split('').
                                map(function(c) {
                                  return c.charCodeAt(0);
                                }));
}

interface CPPVector<T> {
  size(): number;
  get(i: number): T;
  set(i: number, v: T);
}

function vectorToArray<T>(v: CPPVector<T>, mapper = (e: T) => e) {
  const n = v.size();
  const a = new Array<T>(n);
  for (let i = 0; i < n; ++i) {
    a[i] = mapper(v.get(i));
  }
  return a;
}

async function gpsimLoad(timeoutMS) {
  // WASM library initialization isn't keeping Node.js busy.
  //
  // https://github.com/nodejs/node/issues/22088
  let timeout = setTimeout(
    () => reject(new Error('timeout while loading gpsim library')),
    timeoutMS || 10000);

  try {
    return await gpsimLoad_({
      print: console.log,
      printErr: console.error,
      noInitialRun: true,
    });
  } finally {
    clearTimeout(timeout);
    timeout = -1;
  }
}

interface Processor {
}

const gpsim = ref();
const gpsimErr = ref<Error>();

watch(gpsim, gpsim => {
  if (!gpsim) return;
  console.log('gpsim WebAssembly library initialized.');
});

watch(gpsim, gpsim => {
  if (!gpsim) return;

  const InterfaceImpl = gpsim.Interface.extend('InterfaceImpl', {
    SimulationHasStopped() {},

    NewProcessor(p) {
      console.log('newProcessor', p);
    },

    NewModule(m) {
      console.log('newModule', m);
    },

    Update() {
      console.log('update');
    },
  });

  const sim = gpsim.get_interface();
  const iface = new InterfaceImpl();
  try {
    sim.add_interface(iface);  // Takes ownership.
  } catch (ex) {
    iface.delete();
    throw ex;
  }
});

interface ProcessorType {
  names: string[];
}

const procTypes = ref<ProcessorType[]>();
watch(gpsim, gpsim => {
  if (!gpsim) return;

  // We only store the names for now, so the bound
  // ProcessorConstructor object can be GCd.
  procTypes.value = vectorToArray(
    gpsim.ProcessorConstructor.GetList(),
    proc => ({ names: vectorToArray(proc.names) }));
});

interface gpsimObject {
  description(): string;
  name(): string;
}

interface Value extends gpsimObject {}

interface Register extends Value {
  address: number;
  isa: number;
  name: string;
  value: number;
}

interface Pin {
  name: string;
  state: string;
}

const procTypeName = ref('p16f887');

const proc = shallowRef<Processor>();
const pins = shallowReactive(new Map<number, Pin>());
const registers = reactive(new Map<number, Register>());
watch([gpsim, procTypeName], ([gpsim, procTypeName]) => {
  if (!gpsim || !procTypeName) {
    registers.clear();
    pins.clear();
    proc.value = undefined;
    return;
  }

  const SignalSinkImpl = gpsim.SignalSink.extend('SignalSinkImpl', {
    __construct(pinNumber) {
      this.__parent.__construct.call(this);
      this.pinNumber = pinNumber;
    },

    setSinkState(v) {
      pins.get(this.pinNumber).state = String.fromCharCode(v);
    },

    release() {
      this.delete();
    },
  });

  const sim = gpsim.get_interface();
  const ctx = sim.simulation_context();

  ctx.Clear();

  proc.value = ctx.add_processor_by_type(procTypeName, 'Main');

  pins.clear();

  const pinCount = proc.value.get_pin_count();
  for (let i = 1; i <= pinCount; ++i) {
    const pin = proc.value.get_pin(i);
    if (pin) {
      pins.set(i, {
        name: pin.name(),
        state: String.fromCharCode(pin.getBitChar()),
      });
      pin.getMonitor().addSignalSink(new SignalSinkImpl(i));
    }
  }

  registers.clear();

  const regCount = proc.value.get_register_count();
  for (let i = 0; i < regCount; ++i) {
    const reg = proc.value.get_register(i);
    if (reg) {
      registers.set(reg.address, {
        address: reg.address,
        name: reg.name(),
        isa: reg.isa,
        value: reg.get_value(),
        impl: markRaw(reg),
      });
    }
  }
});

interface TraceEntry {
  type: string;
  index?: number;
}

interface EmptyEntry extends TraceEntry {
  type: 'empty';
}

interface CycleCounterEntry extends TraceEntry {
  type: 'cycleCounter';
  cycle: number;
}

interface RegisterEntry extends TraceEntry {
  type: 'readRegister' | 'writeRegister';
  address: number;
  value: number;
}

interface WEntry extends TraceEntry {
  type: 'readW' | 'writeW';
  address: number;
  value: number;
}

interface PCEntry extends TraceEntry {
  type: 'setPC' | 'incrementPC' | 'skipPC' | 'branchPC';
  address: number;
  target?: number;
}

const pc = ref(0);
const wreg = ref();
const traceLog = shallowReactive<TraceEntry[]>([]);
const tracesDiscarded = ref(0);
watch(proc, proc => {
  if (proc) {
    pc.value = proc.GetProgramCounter().get_PC();

    // TODO: Downcasting to pin_processor does not work unless the
    // concrete processor class is registered.
    //
    // https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html#automatic-downcasting
    if (proc.Wget) {
      wreg.value = proc.Wget();
    }
  } else {
    traceLog.splice(0, traceLog.length);
    tracesDiscarded.value = 0;
    wreg.value = undefined;
    pc.value = 0;
  }
});

function discardTraceLog(ctx) {
  const traceReader = ctx.GetTraceReader();

  while (!traceReader.empty)
    traceReader.pop();
}

function readTraceLog(ctx) {
  const traceReader = ctx.GetTraceReader();

  tracesDiscarded.value += traceReader.discarded;

  // Discard entries that are too old.
  while (traceReader.size > 100) {
    traceReader.pop();
  }

  while (!traceReader.empty) {
    const e = traceReader.front();

    switch (e.type) {
      case 'empty':
        continue;

      case 'setPC':
      case 'incrementPC':
      case 'skipPC':
      case 'branchPC':
        e.insn = proc.value.disasm(e.address);
        break;

      case 'writeRegister':
        const reg = registers.get(e.address);
        if (reg) {
          reg.value = reg.impl.get_value();
        }
        break;

      case 'writeW':
        if (proc.value.Wget) {
          wreg.value = proc.value.Wget();
        }
        break;
    }

    e.index = ++traceEntryIndex;
    traceLog.push(e);
    traceReader.pop();
  }

  if (traceLog.length > 100) {
    traceLog.splice(0, traceLog.length - 100);
  }
}

function resetSimulation() {
  if (!proc.value) return;

  proc.value.reset(gpsim.value.RESET_TYPE.EXIT_RESET);
  pc.value = proc.value.GetProgramCounter().get_PC();
  readTraceLog(gpsim.value.get_interface().simulation_context());
}

let traceEntryIndex = 0;
function stepSimulation(nSteps = 1) {
  if (!gpsim.value || !proc.value) return;

  gpsim.value.get_interface().step_simulation(nSteps);
  pc.value = proc.value.GetProgramCounter().get_PC();
  readTraceLog(gpsim.value.get_interface().simulation_context());
}

interface Program {
  targetProcessorType: string;
  upload(p: Processor);
}

const program = shallowRef<Program>();
watch(program, (program, oldProgram) => {
  if (oldProgram) oldProgram.delete();
});
watch([proc, program], ([proc, program]) => {
  if (!gpsim.value || !proc || !program) return;

  const procType = gpsim.value.ProcessorConstructor.findByType(program.targetProcessorType);

  if (!vectorToArray(procType.names).some(name => name === procTypeName.value))
    return;

  program.upload(proc);
  proc.reset(gpsim.value.RESET_TYPE.POR_RESET);

  discardTraceLog(gpsim.value.get_interface().simulation_context());

  console.log('Uploaded to processor type', program.targetProcessorType);
  console.debug('  Code ranges:', vectorToArray(program.code).map(r => ({ addr: r.address, size: r.code.length })));
  console.debug('  Directives:', vectorToArray(program.directives).map(dir => ({ addr: dir.address, type: dir.type, text: dir.text })));
  console.debug('  Line refs:', vectorToArray(program.lineRefs).map(ref => ({ addr: ref.address, file: ref.file, line: ref.line })));
  console.debug('  Symbols:', vectorToArray(program.symbols).map(sym => ({ type: sym.type, name: sym.name, value: sym.value })));
});

function loadFirmware(firmware: Uint8Array) {
  const prog = new gpsim.value.Program(firmware);
  try {
    program.value = prog;
  } catch (ex) {
    prog.delete();
    throw ex;
  }

  localStorage.setItem('prevFirmwareFile', base64Encode(firmware));
}

watch(gpsim, gpsim => {
  if (!gpsim) return;

  const firmware = base64Decode(localStorage.getItem('prevFirmwareFile') ?? '');

  if (!firmware.length)
    return;

  loadFirmware(firmware);

  console.log('Loaded previous firmware');
});

const firmwareFile = shallowRef<HTMLInputElement>();
async function onLoadProgram(e: UIEvent) {
  e.preventDefault();

  if (!gpsim.value || !firmwareFile.value) return;
  if (firmwareFile.value.files.length < 1) return;

  const file = firmwareFile.value.files[0];

  loadFirmware(new Uint8Array(await file.arrayBuffer()));

  console.log('Loaded firmware', file.name, ', type', file.type ?? 'unknown');
}

function registerName(addr) {
  const reg = registers.get(addr);
  return (reg ? reg.name : `R${zeroPaddedHex(addr, 4)}`);
}

function sourceLineRefByAddr(addr) {
  if (!program.value) return undefined;

  const refs = vectorToArray(program.value.findLinesByAddr(addr));

  if (!refs.length) return undefined;

  const ref = refs[0];

  return `@${ref.file}:${ref.line}`;
}

function pinSymbol(pin: Pin) {
  switch (pin.state) {
    case '0': return '○';
    case '1': return '●';
    case 'w': return '↓';
    case 'W': return '↑';
    default: return '–';
  }
}

onMounted(() => {
  gpsimLoad(10000).then(module => {
    module.initialize_gpsim_core();
    gpsim.value = module;
    gpsimErr.value = undefined;
    return module;
  }, ex => {
    gpsimErr.value = ex;
    gpsim.value = undefined;
  });
});
</script>

<template>
  <div>
    <h2>Settings</h2>
    <form>
      <label for="processor">Processor:</label>
      <select id="processor" v-model="procTypeName">
        <option v-for="procType in procTypes" :key="procType" :value="procType.names[2]">{{procType.names[1]}}</option>
      </select>
    </form>
    <form @submit="onLoadProgram">
      <label for="firmwarefile">Firmware File</label>
      <input type="file" accept=".cod" id="firmwarefile" ref="firmwareFile" required>
      <button type="submit">Load program</button>
    </form>
  </div>

  <div>
    <h2>Device Summary</h2>
    <button @click="stepSimulation(1)">Step Instruction</button>
    <button @click="resetSimulation()">Reset</button>
    <div>
      <label for="pc">Program Counter:</label>
      <input name="pc" :value="zeroPaddedHex(pc, 4)" readonly>
    </div>
    <div v-if="wreg !== undefined">
      <label for="wreg">W:</label>
      <input name="wreg" :value="zeroPaddedHex(wreg, 2)" readonly>
    </div>
    <div>
      <span>Pins:</span>
      <span v-for="entry in pins" :key="entry[0]" :title="entry[1].name" :class="[`pin-state-${entry[1].state}`]">
        {{pinSymbol(entry[1])}}
      </span>
    </div>
  </div>

  <div>
    <h2>Registers</h2>
    <ol>
      <li v-for="entry in registers" :key="entry[0]">{{entry[1].name}}: {{zeroPaddedHex(entry[1].value, 2)}}</li>
    </ol>
  </div>

  <div>
    <h2>Execution Trace</h2>
    <div v-if="tracesDiscarded > 0">
      {{tracesDiscarded}} discarded traces.
    </div>
    <ol>
      <template v-for="entry in traceLog" :key="entry.index">
        <li v-if="entry.type === 'interrupt'">
          Interrupt {{entry.address}}
        </li>
        <li v-else-if="entry.type === 'readRegister'">
          &larr; {{registerName(entry.address)}} ({{zeroPaddedHex(entry.value, 2)}})
        </li>
        <li v-else-if="entry.type === 'writeRegister'">
          &rarr; {{registerName(entry.address)}} (was {{zeroPaddedHex(entry.value, 2)}})
        </li>
        <li v-else-if="entry.type === 'readW'">
          &larr; W ({{zeroPaddedHex(entry.value, 2)}})
        </li>
        <li v-else-if="entry.type === 'writeW'">
          &rarr; W (was {{zeroPaddedHex(entry.value, 2)}})
        </li>
        <li v-else-if="entry.type === 'setPC'">
          PC &larr; {{zeroPaddedHex(entry.address, 4)}}
        </li>
        <li v-else-if="entry.insn">
          {{zeroPaddedHex(entry.address, 4)}} {{entry.insn}}
          <span v-if="entry.type !== 'incrementPC'"> ({{entry.type}})</span>
          <span>{{sourceLineRefByAddr(entry.address)}}</span>
        </li>
        <li v-else-if="entry.type === 'reset'">
          Reset {{entry.reset}}
        </li>
      </template>
    </ol>
  </div>

  <div>
    <h2>Device Pins</h2>
    <ul>
      <li v-for="entry in pins" :key="entry[0]">{{entry[1].name}}: {{entry[1].state}}</li>
    </ul>
  </div>
</template>

<style scoped>
.pin-state-Z,
.pin-state-W {
  color: gray;
}
</style>
