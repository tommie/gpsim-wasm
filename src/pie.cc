#include <assert.h>

#include "trace.h"
#include "pie.h"
#include "pir.h"
#include "processor.h"

PIE::PIE(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc), pir(nullptr)
{
}

void PIE::setPir(PIR *pPir)
{
  pir = pPir;
}

void PIE::put(unsigned int new_value)
{
  assert(pir);
  trace.raw(write_trace.get() | value.get());
  value.put(new_value & pir->valid_bits);

  if (pir->interrupt_status())
    pir->setPeripheralInterrupt();
}

