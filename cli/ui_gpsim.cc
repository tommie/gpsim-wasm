#include "ui_gpsim.h"

///
///  CGpsimConsole
///  Connector between the gpsim console and the
///  console handler for the loaded modules.
//////////////////////////////////////////////////

CGpsimConsole::CGpsimConsole()
    : m_pfOut(nullptr), m_pfIn(nullptr)
{
}


void CGpsimConsole::Printf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(m_pfOut, fmt, ap);
  va_end(ap);
}


void CGpsimConsole::VPrintf(const char *fmt, va_list argptr)
{
  vfprintf(m_pfOut, fmt, argptr);
}


void CGpsimConsole::Puts(const char*s)
{
  fputs(s, m_pfOut);
}


void CGpsimConsole::Putc(const char c)
{
  fputc(c, m_pfOut);
}


const char *CGpsimConsole::Gets(char *s, int size)
{
  return fgets(s, size, m_pfIn);
}


void CGpsimConsole::SetOut(FILE *pOut)
{
  m_pfOut = pOut;
}


void CGpsimConsole::SetIn(FILE *pIn)
{
  m_pfIn = pIn;
}


CGpsimConsole g_Console;

// From input.cc
class Macro;
void add_string_to_input_buffer(const char *s, Macro *m = nullptr);

void NotifyExitOnBreak(int /* iExitCode */ )
{
  add_string_to_input_buffer("abort_gpsim_now\n");
}


void initialize_ConsoleUI()
{
  g_Console.SetOut(stdout);
  g_Console.SetIn(stdin);
  GetUserInterface().SetConsole(&g_Console);
  GetUserInterface().SetExitOnBreak(NotifyExitOnBreak);
}

