
#ifndef SRC_UI_H_
#define SRC_UI_H_

#include <stdarg.h>
#include <stdio.h>
#include <iosfwd>

#include "exports.h"
#include "glib.h"

class Register;
class RegisterValue;

class ISimConsole {
public:
  virtual ~ISimConsole()
  {
  }

  virtual void Printf(const char *fmt, ...) = 0;
  virtual void VPrintf(const char *fmt, va_list argptr) = 0;
  virtual void Puts(const char*) = 0;
  virtual void Putc(const char) = 0;
  virtual const char* Gets(char *, int) = 0;
};


class IUserInterface {
public:
  enum {
    eHex,
    eDec,
    eOct,
  };

  IUserInterface() : m_uVerbose(0) {}

  virtual ~IUserInterface()
  {
  }

  virtual ISimConsole &GetConsole() = 0;
  virtual void SetConsole(ISimConsole *pConsole) = 0;
  virtual void DisplayMessage(unsigned int uStringID, ...) = 0;
  virtual void DisplayMessage(FILE * pOut, unsigned int uStringID, ...) = 0;
  virtual void DisplayMessage(const char *fmt, ...) = 0;
  virtual void DisplayMessage(FILE * pOut, const char *fmt, ...) = 0;

  // To be implemented to test on GetVerbosity()
  // virtual void TraceMessage(unsigned int uStringID, ...) = 0;
  // virtual void TraceMessage(const char *fmt, ...) = 0;

  virtual const char * FormatProgramAddress(unsigned int uAddress,
    unsigned int uMask) = 0;
  virtual const char * FormatProgramAddress(unsigned int uAddress,
    unsigned int uMask, int iRadix) = 0;
  virtual const char * FormatRegisterAddress(unsigned int uAddress,
    unsigned int uMask) = 0;
  virtual const char * FormatRegisterAddress(Register *) = 0;
  virtual const char * FormatLabeledValue(const char * pLabel,
    unsigned int uValue) = 0;
  virtual const char * FormatValue(unsigned int uValue) = 0;
  virtual const char * FormatValue(gint64 uValue) = 0;
  virtual const char * FormatValue(gint64 uValue, guint64 uMask) = 0;
  virtual const char * FormatValue(gint64 uValue, guint64 uMask,
    int iRadix) = 0;

  virtual const char * FormatValue(char *str, int len,
    int iRegisterSize, RegisterValue value) = 0;

  virtual void SetProgramAddressRadix(int iRadix) = 0;
  virtual void SetRegisterAddressRadix(int iRadix) = 0;
  virtual void SetValueRadix(int iRadix) = 0;

  virtual void SetProgramAddressMask(unsigned int uMask) = 0;
  virtual void SetRegisterAddressMask(unsigned int uMask) = 0;
  virtual void SetValueMask(unsigned int uMask) = 0;

  typedef void (*FNNOTIFYEXITONBREAK)(int);
  virtual void SetExitOnBreak(FNNOTIFYEXITONBREAK) = 0;
  virtual void NotifyExitOnBreak(int iExitCode) = 0;
  FNNOTIFYEXITONBREAK m_pfnNotifyOnExit = nullptr;

  inline void SetVerbosity(unsigned int uVerbose)
  {
    m_uVerbose = uVerbose;
  }

  inline unsigned int GetVerbosity()
  {
    return m_uVerbose;
  }

  inline unsigned int &GetVerbosityReference()
  {
    return m_uVerbose;
  }

private:
  unsigned int m_uVerbose;
};


// extern "C" IUserInterface & GetUserInterface(void);
LIBGPSIM_EXPORT IUserInterface & GetUserInterface();
LIBGPSIM_EXPORT void             SetUserInterface(IUserInterface * rGpsimUI);
LIBGPSIM_EXPORT void             SetUserInterface(std::streambuf* pOutStreamBuf);

class GlobalVerbosityAccessor {
public:
  GlobalVerbosityAccessor()
  {
  }

  inline operator unsigned int() {
    return GetUserInterface().GetVerbosity();
  }
  // This gives the statements if(verbose) access
  inline operator bool()
  {
    return GetUserInterface().GetVerbosity() != 0;
  }

  friend unsigned int operator&(const GlobalVerbosityAccessor& rVerbosity, int iValue);
};


// This gives the statements if(verbose & 4) access
inline unsigned int operator&(const GlobalVerbosityAccessor& /* rVerbosity */, int iValue)
{
  return (GetUserInterface().GetVerbosity() & iValue) != 0;
}


extern GlobalVerbosityAccessor verbose;

///
///   Gpsim string IDs
#define IDS_BREAK_READING_REG                 1
#define IDS_BREAK_READING_REG_VALUE           2
#define IDS_BREAK_READING_REG_OP_VALUE        3
#define IDS_BREAK_WRITING_REG                 4
#define IDS_BREAK_WRITING_REG_VALUE           5
#define IDS_BREAK_WRITING_REG_OP_VALUE        6
#define IDS_BREAK_ON_EXEC_ADDRESS             7
#define IDS_PROGRAM_FILE_PROCESSOR_NOT_KNOWN  8
#define IDS_FILE_NAME_TOO_LONG                9
#define IDS_FILE_NOT_FOUND                    10
#define IDS_FILE_BAD_FORMAT                   11
#define IDS_NO_PROCESSOR_SPECIFIED            12
#define IDS_PROCESSOR_INIT_FAILED             13
#define IDS_FILE_NEED_PROCESSOR_SPECIFIED     14
#define IDS_LIST_FILE_NOT_FOUND               15
#define IDS_HIT_BREAK                         16
#define IDS_LAST_VALID_GPSIM_ID               17

#endif
