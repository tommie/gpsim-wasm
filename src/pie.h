#ifndef SRC_PIE_H_
#define SRC_PIE_H_

class PIR;
class Processor;

#include "registers.h"

//---------------------------------------------------------
// PIE Peripheral Interrupt Enable register base class
// for PIE1 & PIE2

class PIE : public sfr_register
{
public:
  PIE(Processor *pCpu, const char *pName, const char *pDesc);

  void put(unsigned int new_value) override;
  void setPir(PIR *pPir);
protected:
  PIR *pir;
};

#endif /* SRC_PIE_H_ */
