#include "cmd_manager.h"

#include <string.h>

#include <algorithm>

//
//  CCommandManager
//////////////////////////////////////////////////

CCommandManager::CCommandManager()
{
}

int CCommandManager::Execute(std::string &sName, const char *cmdline)
{
  ICommandHandler *handler = find(sName.c_str());
  if (handler) {
    return handler->Execute(cmdline, &GetConsole());
  }
  return CMD_ERR_PROCESSORNOTDEFINED;
}

int CCommandManager::Register(ICommandHandler * ch)
{
  List::iterator it = std::lower_bound(m_HandlerList.begin(), m_HandlerList.end(),
    ch, lessThan());
  if (it != m_HandlerList.end() && strcmp((*it)->GetName(), ch->GetName()) == 0) {
    return CMD_ERR_PROCESSORDEFINED;
  }
  m_HandlerList.insert(it, ch);
  return CMD_ERR_OK;
}

ICommandHandler * CCommandManager::find(const char *name)
{
  CommandHandlerKey key(name);
  List::iterator it = std::lower_bound(m_HandlerList.begin(), m_HandlerList.end(),
    (ICommandHandler*)&key, lessThan());
  if (it != m_HandlerList.end() && strcmp((*it)->GetName(), name) == 0) {
    return *it;
  }
  return nullptr;
}

CCommandManager CCommandManager::m_CommandManger;

CCommandManager &CCommandManager::GetManager()
{
  return m_CommandManger;
}

void CCommandManager::ListToConsole()
{
  ISimConsole &console = GetConsole();

  for (const auto &it : m_HandlerList) {
    console.Printf("%s\n", (*it).GetName());
  }
}
