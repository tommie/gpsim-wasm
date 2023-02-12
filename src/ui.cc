#include "ui.h"

#include <stdarg.h>

#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "interface.h"
#include "registers.h"
#include "value.h"

const char * s_psEnglishMessages[] = {
  "",                                                     // Place holder so we don't have a zero
  "break reading register %s\n",                          // IDS_BREAK_READING_REG
  "break reading register %s with value %u\n",            // IDS_BREAK_READING_REG_VALUE
  "break reading register %s %s %u\n",                    // IDS_BREAK_READING_REG_OP_VALUE
  "break writing register %s\n",                          // IDS_BREAK_WRITING_REG
  "break writing register %s with value %u\n",            // IDS_BREAK_WRITING_REG_VALUE
  "break writing register %s %s %u\n",                    // IDS_BREAK_WRITING_REG_OP_VALUE
  "execution break at address %s\n",                      // IDS_BREAK_ON_EXEC_ADDRESS
  "unrecognized processor in the program file\n",         // IDS_PROGRAM_FILE_PROCESSOR_NOT_KNOWN
  "file name '%s' is too long\n",                         // IDS_FILE_NAME_TOO_LONG
  "file %s not found\n",                                  // IDS_FILE_NOT_FOUND
  "file %s is not formatted properly\n",                  // IDS_FILE_BAD_FORMAT
  "no processor has been specified\n",                    // IDS_NO_PROCESSOR_SPECIFIED
  "processor %s initialization failed\n",                 // IDS_PROCESSOR_INIT_FAILED
  "the program file type does not contain processor\n"    // first part of IDS_FILE_NEED_PROCESSOR_SPECIFIED
  "you need to specify processor with the processor command\n", // IDS_FILE_NEED_PROCESSOR_SPECIFIED
  "an appropriate list file for %s was not found\n",      // IDS_LIST_FILE_NOT_FOUND
  "Hit breakpoint %d\n",                                  // IDS_HIT_BREAK
  nullptr,     // IDS_
};

class CGpsimUserInterface : public IUserInterface {
public:
  explicit CGpsimUserInterface(const char *paStrings[]);
  virtual ~CGpsimUserInterface() {}

  void         SetStreams(FILE *in, FILE *out);
  ISimConsole &GetConsole() override;
  void SetConsole(ISimConsole *pConsole) override;
  void DisplayMessage(unsigned int uStringID, ...) override;
  void DisplayMessage(FILE * pOut, unsigned int uStringID, ...) override;
  void DisplayMessage(const char *fmt, ...) override;
  void DisplayMessage(FILE * pOut, const char *fmt, ...) override;

  const char * FormatProgramAddress(unsigned int uAddress,
    unsigned int uMask) override;
  const char * FormatProgramAddress(unsigned int uAddress,
    unsigned int uMask, int iRadix) override;
  const char * FormatRegisterAddress(Register *) override;
  const char * FormatRegisterAddress(unsigned int uAddress,
    unsigned int uMask) override;
  const char * FormatLabeledValue(const char * pLabel,
    unsigned int uValue) override;
  virtual const char * FormatLabeledValue(const char * pLabel,
    unsigned int uValue, unsigned int uMask, int iRadix,
    std::string_view pHexPrefix);
  const char * FormatValue(unsigned int uValue) override;
  const char * FormatValue(int64_t uValue) override;
  const char * FormatValue(int64_t uValue, uint64_t uMask) override;
  const char * FormatValue(int64_t uValue, uint64_t uMask,
    int iRadix) override;
  virtual const char * FormatValue(int64_t uValue,
    uint64_t uMask, int iRadix, std::string_view pHexPrefix);

  const char * FormatValue(char *str, int len,
    int iRegisterSize, RegisterValue value) override;
//  virtual char *       FormatValueAsBinary(char *str, int len,
//    int iRegisterSize, RegisterValue value);

  void SetProgramAddressRadix(int iRadix) override;
  void SetRegisterAddressRadix(int iRadix) override;
  void SetValueRadix(int iRadix) override;

  void SetProgramAddressMask(unsigned int uMask) override;
  void SetRegisterAddressMask(unsigned int uMask) override;
  void SetValueMask(unsigned int uMask) override;

  void SetExitOnBreak(FNNOTIFYEXITONBREAK) override;
  void NotifyExitOnBreak(int iExitCode) override;

  static Integer  s_iValueRadix;
  static String   s_sValueHexPrefix;
  static Integer  s_iProgAddrRadix;
  static String   s_sProgAddrHexPrefix;
  static Integer  s_iRAMAddrRadix;
  static String   s_sRAMAddrHexPrefix;

  static Integer  s_iValueMask;
  static Integer  s_iProgAddrMask;
  static Integer  s_iRAMAddrMask;

protected:
  std::string        m_sLabeledAddr;
  std::string        m_sFormatValueInt64_T;

  const char ** m_paStrings;
//  CGpsimConsole m_Console;
};


Integer CGpsimUserInterface::s_iValueRadix(       "UIValueRadix",             IUserInterface::eHex);
String  CGpsimUserInterface::s_sValueHexPrefix(   "UIValueHexPrefix",         "$");
Integer CGpsimUserInterface::s_iProgAddrRadix(    "UIProgamAddressRadix",     IUserInterface::eHex);
String  CGpsimUserInterface::s_sProgAddrHexPrefix("UIProgamAddressHexPrefix", "$");
Integer CGpsimUserInterface::s_iRAMAddrRadix(     "UIRAMAddressRadix",        IUserInterface::eHex);
String  CGpsimUserInterface::s_sRAMAddrHexPrefix( "UIRAMAddressHexPrefix",    "$");

Integer CGpsimUserInterface::s_iValueMask(        "UIValueMask",             0xff);
Integer CGpsimUserInterface::s_iProgAddrMask(     "UIProgamAddressMask",     0xff);
Integer CGpsimUserInterface::s_iRAMAddrMask(      "UIRAMAddressMask",        0xff);


class NullConsole : public ISimConsole {
public:
  void Printf(const char * /* fmt */, ...) override {}
  void VPrintf(const char * /* fmt */, va_list /* argptr */) override {}
  void Puts(const char*) override {}
  void Putc(const char) override {}
  const char * Gets(char *, int) override { return "";}
};


NullConsole g_NullConsole;
static ISimConsole    *g_pConsole = &g_NullConsole;

class NullUserInterface : public IUserInterface {
//  NullConsole m_Console;
public:
  ISimConsole &GetConsole() override { return *g_pConsole; }
  void SetConsole(ISimConsole *pConsole) override { g_pConsole = pConsole; }
  void DisplayMessage(unsigned int /* uStringID */, ...) override {}
  void DisplayMessage(FILE * /* pOut */, unsigned int /* uStringID */, ...) override {}
  void DisplayMessage(const char * /* fmt */, ...) override {}
  void DisplayMessage(FILE * /* pOut */, const char * /* fmt */, ...) override {}

  const char * FormatProgramAddress(unsigned int /* uAddress */,
    unsigned int /* uMask */) override {return "";}
  const char * FormatProgramAddress(unsigned int /* uAddress */,
    unsigned int /* uMask */, int /* iRadix */) override {return "";}
  const char * FormatRegisterAddress(Register *) override
  { return ""; }
  const char * FormatRegisterAddress(unsigned int /* uAddress */,
    unsigned int /* uMask */) override {return "";}
  const char * FormatLabeledValue(const char * /* pLabel */,
    unsigned int /* uValue */) override {return "";}
  const char * FormatValue(unsigned int /* uValue */) override {return "";}
  const char * FormatValue(int64_t /* uValue */) override {return "";}
  const char * FormatValue(int64_t /* uValue */, uint64_t /* uMask */) override {return "";}
  const char * FormatValue(int64_t /* uValue */, uint64_t /* uMask */,
    int /* iRadix */) override {return "";}

  const char * FormatValue(char * /* str */, int /* len */,
    int /* iRegisterSize */, RegisterValue /* value */) override {return "";}

  void SetProgramAddressRadix(int /* iRadix */) override {}
  void SetRegisterAddressRadix(int /* iRadix */) override {}
  void SetValueRadix(int /* iRadix */) override {}

  void SetProgramAddressMask(unsigned int /* uMask */) override {}
  void SetRegisterAddressMask(unsigned int /* uMask */) override {}
  void SetValueMask(unsigned int /* uMask */) override {}

  void SetExitOnBreak(FNNOTIFYEXITONBREAK) override {}
  void NotifyExitOnBreak(int /* iExitCode */) override {}
};


CGpsimUserInterface g_DefaultUI(s_psEnglishMessages);
static IUserInterface *g_GpsimUI = &g_DefaultUI;

LIBGPSIM_EXPORT  IUserInterface & GetUserInterface()
{
  return *g_GpsimUI;
}


LIBGPSIM_EXPORT  void SetUserInterface(IUserInterface * pGpsimUI)
{
  if (pGpsimUI) {
    g_GpsimUI = pGpsimUI;
  }
  else {
    g_GpsimUI = &g_DefaultUI;
  }
}


static std::streambuf *s_pSavedCout;

LIBGPSIM_EXPORT void SetUserInterface(std::streambuf * pOutStreamBuf)
{
  if (!pOutStreamBuf && s_pSavedCout) {
    std::cout.rdbuf(s_pSavedCout);
    s_pSavedCout = nullptr;
  }
  else {
    s_pSavedCout = std::cout.rdbuf(pOutStreamBuf);
  }
}


///
///   CGpsimUserInterface
///
CGpsimUserInterface::CGpsimUserInterface(const char *paStrings[])
{
  m_paStrings = paStrings;
  m_pfnNotifyOnExit = nullptr;
}


void CGpsimUserInterface::SetStreams(FILE * /* in */, FILE * /* out */)
{
//  m_Console.SetOut(out);
//  m_Console.SetIn(in);
}


ISimConsole &CGpsimUserInterface::GetConsole()
{
  return *g_pConsole;
}


void CGpsimUserInterface::SetConsole(ISimConsole *pConsole)
{
  g_pConsole = pConsole;
}


void CGpsimUserInterface::DisplayMessage(unsigned int uStringID, ...)
{
  va_list ap;
  va_start(ap, uStringID);
  g_pConsole->VPrintf(m_paStrings[uStringID], ap);
  va_end(ap);
}


void CGpsimUserInterface::DisplayMessage(FILE * pOut, unsigned int uStringID, ...)
{
  va_list ap;
  va_start(ap, uStringID);
  if (!pOut || pOut == stdout) {
    g_pConsole->VPrintf(m_paStrings[uStringID], ap);
  }
  else {
    vfprintf(pOut, m_paStrings[uStringID], ap);
  }
  va_end(ap);
}


void CGpsimUserInterface::DisplayMessage(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  g_pConsole->VPrintf(fmt, ap);
  va_end(ap);
}


void CGpsimUserInterface::DisplayMessage(FILE * pOut, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if (!pOut || pOut == stdout) {
    g_pConsole->VPrintf(fmt, ap);
  }
  else {
    vfprintf(pOut, fmt, ap);
  }
  va_end(ap);
}


void CGpsimUserInterface::SetProgramAddressRadix(int iRadix)
{
  s_iProgAddrRadix = iRadix;
}


void CGpsimUserInterface::SetRegisterAddressRadix(int iRadix)
{
  s_iRAMAddrRadix = iRadix;
}


void CGpsimUserInterface::SetValueRadix(int iRadix)
{
  s_iValueRadix = iRadix;
}


void CGpsimUserInterface::SetProgramAddressMask(unsigned int uMask)
{
  s_iProgAddrMask = uMask;
}


void CGpsimUserInterface::SetRegisterAddressMask(unsigned int uMask)
{
  s_iRAMAddrMask = uMask;
}


void CGpsimUserInterface::SetValueMask(unsigned int uMask)
{
  s_iValueMask = uMask;
}


void CGpsimUserInterface::SetExitOnBreak(FNNOTIFYEXITONBREAK pFunc)
{
  m_pfnNotifyOnExit = pFunc;
}


void CGpsimUserInterface::NotifyExitOnBreak(int iExitCode)
{
  if (m_pfnNotifyOnExit) {
    m_pfnNotifyOnExit(iExitCode);
  }
}


const char * CGpsimUserInterface::FormatProgramAddress(unsigned int uAddress,
    unsigned int uMask)
{
  //const char * pLabel = get_symbol_table().findProgramAddressLabel(uAddress);
  const char *pLabel = "FIXME-ui.cc";
  return FormatLabeledValue(pLabel, uAddress, uMask,
    s_iProgAddrRadix, s_sProgAddrHexPrefix);
}


const char * CGpsimUserInterface::FormatProgramAddress(unsigned int uAddress,
    unsigned int uMask, int iRadix)
{
  return FormatValue((int64_t)uAddress, uMask, iRadix, s_sProgAddrHexPrefix);
}


const char * CGpsimUserInterface::FormatRegisterAddress(Register *pReg)
{
  if (!pReg)
    return "";

  return FormatLabeledValue(pReg->name().c_str(),
                            pReg->address,
                            static_cast<uint64_t>(s_iRAMAddrMask),
                            static_cast<uint64_t>(s_iRAMAddrRadix), s_sRAMAddrHexPrefix);
}


const char * CGpsimUserInterface::FormatRegisterAddress(unsigned int uAddress,
                                                        unsigned int /* uMask */)
{
  //register_symbol * pRegSym = get_symbol_table().findRegisterSymbol(uAddress, uMask);
  //const char * pLabel = pRegSym == NULL ? "" : pRegSym->name().c_str();
  const char *pLabel = "FIXME-ui.cc";
  return FormatLabeledValue(pLabel, uAddress,
                            static_cast<uint64_t>(s_iRAMAddrMask),
                            static_cast<uint64_t>(s_iRAMAddrRadix), s_sRAMAddrHexPrefix);
}


const char * CGpsimUserInterface::FormatLabeledValue(const char * pLabel,
                                                     unsigned int uValue)
{
  return FormatLabeledValue(pLabel, uValue, static_cast<uint64_t>(s_iValueMask),
                            static_cast<uint64_t>(s_iValueRadix), s_sValueHexPrefix);
}


const char * CGpsimUserInterface::FormatLabeledValue(const char * pLabel,
                                                     unsigned int uValue,
                                                     unsigned int uMask,
                                                     int          iRadix,
                                                     std::string_view pHexPrefix)
{
  m_sLabeledAddr.clear();
  const char *pValue = FormatValue(uValue, uMask, iRadix, pHexPrefix);
  if (pLabel && *pLabel != 0) {
    m_sLabeledAddr.append(pLabel);
    m_sLabeledAddr.append("(");
    m_sLabeledAddr.append(pValue);
    m_sLabeledAddr.append(")");
  }
  else {
    m_sLabeledAddr = pValue;
  }
  return m_sLabeledAddr.c_str();
}


const char * CGpsimUserInterface::FormatValue(unsigned int uValue)
{
  return FormatLabeledValue(nullptr, uValue, static_cast<uint64_t>(s_iValueMask),
                            static_cast<uint64_t>(s_iValueRadix), s_sValueHexPrefix);
}


const char * CGpsimUserInterface::FormatValue(int64_t uValue)
{
  return FormatValue(uValue, s_iValueMask, s_iValueRadix);
}


const char * CGpsimUserInterface::FormatValue(int64_t uValue, uint64_t uMask)
{
  return FormatValue(uValue, uMask, s_iValueRadix, s_sValueHexPrefix);
}


const char * CGpsimUserInterface::FormatValue(int64_t uValue,
    uint64_t uMask, int iRadix)
{
  return FormatValue(uValue, uMask, iRadix, s_sValueHexPrefix);
}


const char * CGpsimUserInterface::FormatValue(int64_t uValue,
    uint64_t uMask, int iRadix, std::string_view pHexPrefix)
{
  std::ostringstream osValue;
  int iBytes = 0;
  uint64_t l_uMask = uMask;
  int iDigits;
  while (l_uMask) {
    iBytes++;
    l_uMask >>= 8;
  }

  switch (iRadix) {
  case eHex:
    iDigits = iBytes * 2;
    osValue << pHexPrefix;
    osValue << std::hex << std::setw(iDigits) << std::setfill('0');
    break;

  case eDec:
    osValue << std::dec;
    break;

  case eOct:
    iDigits = iBytes * 3;
    osValue << '0';
    osValue << std::oct << std::setw(iDigits) << std::setfill('0');
    break;
  }
  osValue << (uValue & uMask);
  m_sFormatValueInt64_T = osValue.str();
  return m_sFormatValueInt64_T.c_str();
}


const char * CGpsimUserInterface::FormatValue(char *str, int len,
                                        int iRegisterSize,
                                        RegisterValue value)
{
  if (!str || !len)
    return 0;

  char hex2ascii[] = "0123456789ABCDEF";
  int min = (len < iRegisterSize * 2) ? len : iRegisterSize * 2;

  if (value.data == INVALID_VALUE)
    value.init = 0xfffffff;

  for (int i = 0; i < min; i++) {
    if (value.init & 0x0f)
      str[min-i-1] = '?';
    else
      str[min-i-1] = hex2ascii[value.data & 0x0f];
    value >>= 4;
  }
  str[min] = 0;

  return str;
}
