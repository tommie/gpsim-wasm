
#ifndef CLI_UI_GPSIM_H_
#define CLI_UI_GPSIM_H_

#include <stdarg.h>
#include <stdio.h>
#include "../src/ui.h"

class CGpsimConsole : public ISimConsole {
public:
  CGpsimConsole();
  void Printf(const char *fmt, ...) override;
  void VPrintf(const char *fmt, va_list argptr) override;
  void Puts(const char*) override;
  void Putc(const char) override;
  const char *Gets(char *, int) override;

  void SetOut(FILE *pOut);
  void SetIn(FILE *pIn);

protected:
  FILE *m_pfOut;
  FILE *m_pfIn;
};

#endif
