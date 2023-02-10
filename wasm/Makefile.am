bin_PROGRAMS = gpsim_wasm.js
bin_SCRIPTS = gpsim_wasm.wasm gpsim_wasm.wasm.map
CLEANFILES = $(bin_SCRIPTS)

gpsim_wasm_js_LDADD = ../src/libgpsim.la -lembind
gpsim_wasm_js_LDFLAGS = \
	-sINCOMING_MODULE_JS_API=noExitRuntime,noInitialRun,onAbort,onRuntimeInitialized,print,printErr \
	-sMODULARIZE=1 \
	-sALLOW_TABLE_GROWTH=1
gpsim_wasm_js_SOURCES = wasm.cc

# node.js doesn't play well with the same package being split across
# multiple directories.
$(builddir)/gpsim_test.mjs: $(srcdir)/gpsim_test.mjs
	cp "$^" "$@"

check: $(builddir)/gpsim_test.mjs
	node ./gpsim_test.mjs
