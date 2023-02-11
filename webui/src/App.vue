<script setup lang="ts">
import {
  onMounted,
  reactive,
  ref,
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
    UpdateObject(newValue) {
      console.log('updateObject', newValue);
    },

    RemoveObject() {
      console.log('removeObject');
    },

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
const pins = reactive(new Map<number, Pin>());
watch([gpsim, ihexFirmware, procTypeName], ([gpsim, ihexFirmware, procTypeName]) => {
  if (!gpsim || !ihexFirmware || !procTypeName) {
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
        state: 'w',
      });
      pin.getMonitor().addSignalSink(new SignalSinkImpl(i));
    }
  }
});

const pc = ref(0);
const executedInsns = reactive<string[]>([]);
watch(proc, proc => {
  if (proc) {
    // The PC xref does not issue callbacks for simple instruction
    // stepping.
    proc.GetProgramCounter().add_xref();
    pc.value = proc.GetProgramCounter().get_PC();
  } else {
    executedInsns.clear();
    pc.value = 0;
  }
});

function resetSimulation() {
  if (!proc.value) return;

  proc.value.reset(gpsim.value.RESET_TYPE.MCLR_RESET);
  pc.value = proc.value.GetProgramCounter().get_PC();
  executedInsns.splice(0, executedInsns.length);
}

function stepSimulation(nSteps = 1) {
  if (!gpsim.value || !proc.value) return;

  executedInsns.push(`${zeroPaddedHex(pc.value, 4)}  ${proc.value.disasm(pc.value)}`);
  gpsim.value.get_interface().step_simulation(nSteps);
  pc.value = proc.value.GetProgramCounter().get_PC();
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
    <br/>
    <label for="pc">Program Counter:</label>
    <input name="pc" :value="zeroPaddedHex(pc, 4)" readonly>

    <h2>Executed Instructions</h2>
    <ol>
      <li v-for="(insn, index) in executedInsns" :key="index">{{insn}}</li>
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
