'use strict';

import gpsimLoad_ from './gpsim_wasm.mjs';

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

gpsimLoad().then(async module => {
    const gpsim = {
        gpsimInterface: {
            eAdvancementModes: module.gpsimInterface_eAdvancementModes,
        },
        Interface: module.Interface,
        ProcessorConstructor: module.ProcessorConstructor,
        SignalSink: module.SignalSink,
        get_interface: module.get_interface,
    };

    let InterfaceImpl = gpsim.Interface.extend("InterfaceImpl", {
        UpdateObject(newValue) {
            console.log("updateObject", newValue);
        },

        SimulationHasStopped() {
            console.log("simulationHasStopped");
        },

        NewProcessor(p) {
            console.log("newProcessor", p);
        },

        Update() {
            console.log("update");
        },
    });

    let SignalSinkImpl = gpsim.SignalSink.extend('SignalSinkImpl', {
        __construct(pinNumber) {
            this.__parent.__construct.call(this);
            this.pinNumber = pinNumber;
        },

        setSinkState(v) {
            console.log("sink", this.pinNumber, String.fromCharCode(v));
        },

        release() {
            this.delete();
        },
    });

    module.initialize_gpsim_core();

    if (false) {
        const procList = gpsim.ProcessorConstructor.GetList();
        const procs = new Array(procList.size());
        for (let i = 0; i < procs.length; ++i) {
            procs[i] = procList.get(i);
        }
        console.log(procs);

        const procCons = gpsim.ProcessorConstructor.findByType('p16f887');
        const proc = procCons.ConstructProcessor('aproc');
    }

    const sim = gpsim.get_interface();
    const ctx = sim.simulation_context();

    try {
        const iface = new InterfaceImpl();
        try {
            sim.add_interface(iface);  // Takes ownership.
        } catch (ex) {
            iface.delete();
            throw ex;
        }

        try {
            const proc = ctx.add_processor_by_type('p16f887', 'aproc');
            proc.init_program_memory_at_index(0, new Uint8Array([0x83, 0x16, 0x03, 0x13, 0x85, 0x01, 0x83, 0x12, 0x03, 0x13, 0xFF, 0x30, 0x85, 0x00, 0x63, 0x00, 0x00, 0x28]));

            const pinCount = proc.get_pin_count();
            for (let i = 1; i <= pinCount; ++i) {
                const pin = proc.get_pin(i);
                if (pin) {
                    console.log("pin", i, pin.name());
                    pin.getMonitor().addSignalSink(new SignalSinkImpl(i));
                }
            }

            proc.step(20);

            const trace = ctx.GetTraceReader();
            console.log('Trace:', trace.empty, trace.size, trace.discarded);
            while (!trace.empty) {
                console.log('  ', trace.front());
                trace.pop();
            }
        } finally {
            sim.remove_interface(iface.get_id());
        }
    } finally {
        ctx.Clear();
    }
})
