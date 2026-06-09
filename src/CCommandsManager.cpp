#include "CCommandsManager.h"

bool CCommandsManager::IsCommandPresent(const std::string& name) const
{
    return std::find_if(Items.begin(), Items.end(), [&name](auto& ptr) -> bool
    {
        return ptr->IsNamedLike(name);
    }) != Items.end();
}

std::unique_ptr<CCommand>& CCommandsManager::GetCommand(const std::string& name)
{
    auto it = std::find_if(Items.begin(), Items.end(), [&name](auto& ptr) -> bool
    {
        return ptr->IsNamedLike(name);
    });

    if (it == Items.end())
    {
        //return nullptr;
        throw std::runtime_error("No such command");
    }
    return *it;
}

bool CCommandsManager::IsCommandPresent(const std::unique_ptr<CCommand>& cmd) const
{
    return std::find_if(Items.begin(), Items.end(), [&cmd](auto& ptr) -> bool
    {
        return ptr == cmd;
    }) != Items.end();
}

bool CCommandsManager::CanAdd(const std::unique_ptr<CCommand>& item) const
{
    bool NameMatch = IsCommandPresent(item->Name);
    bool AliasMatch = false;
    
    for (const auto& s : item->Aliases)
    {
        bool mtch = IsCommandPresent(s);
        if (mtch)
        {
            AliasMatch = true;
            break;
        }
    }
    
    return !IsCommandPresent(item) && (!NameMatch && !AliasMatch);
}

void CCommandsManager::Add(std::unique_ptr<CCommand>&& cmd)
{
    Items.push_back(std::move(cmd));
}

std::unique_ptr<CCommand>& CCommandsManager::CreateCommand(const std::string& name, CCommand::CType type, CCommand::CmdRight rights, const std::function<COMMAND_RETURN(COMMAND_ARGS)>& hndlr)
{
    std::unique_ptr<CCommand> cmd = std::make_unique<CCommand>();

    cmd->Name = name;

    cmd->Type = type;
    cmd->ProcessRights = rights;

    cmd->Handlers += hndlr;

    Add(std::move(cmd));
    return Items.back();
}

std::unique_ptr<CCommand>& CCommandsManager::CreateLuaCommand(const std::string& name, CCommand::CType type, CCommand::CmdRight rights, const sol::function& func)
{
    std::unique_ptr<CCommand> cmd = std::make_unique<CCommand>();

    cmd->Name = name;

    cmd->Type = type;
    cmd->ProcessRights = rights;

    std::unique_ptr<CLuaFunction<CCommand::CmdStatus, const CCommandArgsWrapper&, const CCommandSender&>> _func = std::make_unique<CLuaFunction<CCommand::CmdStatus, const CCommandArgsWrapper&, const CCommandSender&>>(func);
    _func->MakeCustomOutCaller([](const sol::function& luafunc, COMMAND_ARGS) -> COMMAND_RETURN
        {
            sol::protected_function safe_luafunc(luafunc.lua_state(), luafunc);

            sol::object ret_obj = safe_luafunc(args.ToLua(), sender.GetID()); //TODO fix it
            CCommand::CmdStatus ret = CCommand::CmdStatus::OK;

            if(ret_obj.valid() && ret_obj.is<int>())
            {
                ret = (CCommand::CmdStatus)ret_obj.as<int>();
            }
            return ret;
        });

    _func->MakeCustomInCaller([](CFunctionBase<COMMAND_RETURN, COMMAND_ARGS_TEMPLATE>* rawfunc, sol::variadic_args va) -> sol::object
    {
        if(va.size() == 0)
        {
            return sol::make_object<int>(va.lua_state(), (int)rawfunc->Call({}, CCommandSender::Self));
        }

        if(va.size() == 1)
        {
            //return sol::make_object<int>(va.lua_state(), (int)rawfunc->Call({}, CCommandSender::FromID())); TODO sender
            return sol::make_object<int>(va.lua_state(), (int)rawfunc->Call({}, CCommandSender::Self)); //TEMPORARY
        }

        std::vector<std::wstring> args;
        for(size_t i = 0; i < va.size() - 1; i++)
        {
            args.push_back(va.get<std::wstring>(i));
        }
        return sol::make_object<int>(va.lua_state(), (int)rawfunc->Call(args, CCommandSender::Self)); //TODO sender
    });

    cmd->Handlers.Add(std::move(_func));
    Add(std::move(cmd));
    return Items.back();
}
