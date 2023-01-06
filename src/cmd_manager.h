#ifndef SRC_CMD_MANAGER_H_
#define SRC_CMD_MANAGER_H_

#include "gpsim_interface.h"
#include "ui.h"

#include <string.h>
#include <algorithm>
#include <list>
#include <vector>
#include <string>
#include <functional>

class CommandHandlerKey : public ICommandHandler {
public:
  explicit CommandHandlerKey(const char *name)
    : m_name(name)
  {
  }
  const char *GetName() override { return m_name; }
  int Execute(const char * /* commandline */ ,  ISimConsole * /* out */) override
  {
    return CMD_ERR_COMMANDNOTDEFINED;
  }
  int ExecuteScript(const std::list<std::string> & /* script */ , ISimConsole * /* out */ ) override
  {
    return CMD_ERR_ERROR;
  }
  const char * m_name;
};

class CCommandManager {
public:
  CCommandManager();
  int   Register(ICommandHandler * ch);
  int   Execute(std::string &sName, const char *cmdline);

  static CCommandManager m_CommandManger;
  static CCommandManager &GetManager();
  ICommandHandler * find(const char *name);
  ISimConsole &GetConsole() {
    return GetUserInterface().GetConsole();
  }
  void ListToConsole();

private:
  struct lessThan : std::binary_function<ICommandHandler*, ICommandHandler*, bool> {
    bool operator()(const ICommandHandler* left, const ICommandHandler* right) const {
      return strcmp(((ICommandHandler*)left)->GetName(),
        ((ICommandHandler*)right)->GetName()) < 0;
    }
  };

  typedef std::vector<ICommandHandler*> List;

  List m_HandlerList;
};

#endif
