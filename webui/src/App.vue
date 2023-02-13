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

interface Processor {
  init_program_memory_at_index(addr: number, data: Uint8Array);
}

function loadIntelHex(proc: Processor, s: string): Uint8Array {
  let highAddr = 0;
  const missingEOF = s.split(/$/gm).every(line => {
    line = line.trim();
    if (line.indexOf(':') !== 0) return true;

    let data = new Array(Math.floor((line.length - 1) / 2));
    for (let i = 1, j = 0; i < line.length; i += 2, j++) {
      data[j] = parseInt(line.substring(i, i + 2), 0x10);
    }
    if (data.length < 1 + 2 + 1 + 1) {
      throw new Error(`invalid IHEX line length: ${line.length}`);
    }
    const count = data[0];
    if (data.length !== 1 + 2 + 1 + count + 1) {
      throw new Error(`invalid IHEX count: ${count}, line length ${data.length}`);
    }

    const addr = (data[1] << 8) | data[2];
    const rtype = data[3];
    const csum = data.reduce((a, b) => a + b, 0) % 256;
    if (csum !== 0) {
      throw new Error(`invalid checksum (should be zero): ${csum}`);
    }
    data = data.slice(4, data.length - 1);

    switch (rtype) {
      case 0:
        proc.init_program_memory_at_index(highAddr + addr, new Uint8Array(data));
        break;

      case 1:
        return false;

      case 4:
        highAddr = (data[0] << 24) | (data[1] << 16);
        break;

      default:
        throw new Error(`unhandled IHEX record type: ${rtype}`);
    }

    return true;
  });

  if (missingEOF) {
    throw new Error('missing EOF record in IHEX');
  }
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
const ihexFirmware = ref(`:020000040000FA
:10000000FE30831603138500831203130608831240
:06001000031385006300EC
:00000001FF
`);

const proc = shallowRef<Processor>();
const pins = shallowReactive(new Map<number, Pin>());
const registers = reactive(new Map<number, Register>());
watch([gpsim, ihexFirmware, procTypeName], ([gpsim, ihexFirmware, procTypeName]) => {
  if (!gpsim || !ihexFirmware || !procTypeName) {
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
  loadIntelHex(proc.value, ihexFirmware);

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

  proc.value.reset(gpsim.value.RESET_TYPE.MCLR_RESET);
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

function registerName(addr) {
  const reg = registers.get(addr);
  return (reg ? reg.name : `R${zeroPaddedHex(addr, 4)}`);
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
    <label for="processor">Processor:</label>
    <select id="processor" v-model="procTypeName">
      <option v-for="procType in procTypes" :key="procType" :value="procType.names[2]">{{procType.names[1]}}</option>
    </select>
    <br/>
    <label for="ihexfirmware">Intel HEX Firmware</label>
    <textarea id="ihexfirmware" v-model="ihexFirmware"></textarea>
  </div>

  <div>
    <h2>Device</h2>
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
          {{zeroPaddedHex(entry.address, 4)}} {{entry.insn}}<span v-if="entry.type !== 'incrementPC'"> ({{entry.type}})</span>
        </li>
        <li v-else-if="entry.type === 'reset'">
          Reset {{entry.reset}}
        </li>
      </template>
    </ol>

    <h2>Registers</h2>
    <ol>
      <li v-for="entry in registers" :key="entry[0]">{{entry[1].name}}: {{zeroPaddedHex(entry[1].value, 2)}}</li>
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
</style>
