bin_PROGRAMS = gpsim_wasm.mjs
bin_SCRIPTS = gpsim_wasm.wasm gpsim_wasm.wasm.map gpsim_wasm.d.ts
CLEANFILES = $(bin_SCRIPTS)

gpsim_wasm_mjs_LDADD = ../src/libgpsim.la -lembind
gpsim_wasm_mjs_LDFLAGS = \
	-sINCOMING_MODULE_JS_API=noExitRuntime,noInitialRun,onAbort,onRuntimeInitialized,print,printErr \
	-sMODULARIZE=1 \
	-sALLOW_MEMORY_GROWTH=1 \
	-sALLOW_TABLE_GROWTH=1
gpsim_wasm_mjs_SOURCES = wasm.cc

# https://www.gnu.org/software//automake/manual/html_node/Multiple-Outputs.html
gpsim_wasm.wasm: gpsim_wasm.mjs
gpsim_wasm.wasm.map: gpsim_wasm.mjs

# node.js doesn't play well with the same package being split across
# multiple directories. This is for the dev server.
$(builddir)/gpsim_test.mjs: $(srcdir)/gpsim_test.mjs
	cp "$^" "$@"

check: $(builddir)/gpsim_test.mjs
	node ./gpsim_test.mjs
