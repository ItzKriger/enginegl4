#pragma once
#include <map>
#include <string>
#include <memory>
#include <stdexcept>
#include <algorithm>

#include "CCommand.h"

class CCommandsManager
{
public:
    bool IsCommandPresent(const std::string& name) const;
    std::unique_ptr<CCommand>& GetCommand(const std::string& name);

    bool IsCommandPresent(const std::unique_ptr<CCommand>& cmd) const;
    bool CanAdd(const std::unique_ptr<CCommand>& item) const;
    void Add(std::unique_ptr<CCommand>&& cmd);

    std::unique_ptr<CCommand>& CreateCommand(const std::string& name, CCommand::CType type, CCommand::CmdRight rights, const std::function<COMMAND_RETURN(COMMAND_ARGS)>& hndlr);
    std::unique_ptr<CCommand>& CreateLuaCommand(const std::string& name, CCommand::CType type, CCommand::CmdRight rights, const sol::function& func);

    std::vector<std::unique_ptr<CCommand>> Items;
};
