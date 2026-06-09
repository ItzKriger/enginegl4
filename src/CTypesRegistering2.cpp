#include "CTypesRegistering2.h"
#include "CTypesRegistering.h"

#include "CScriptingEngine.h"

#include "CCommand.h"
#include "CCommandProcessor.h"

void CScriptingEngine::RegisterCommandsTypes()
{
    State->new_usertype<CCommandSender>
    (
        "CCommandSender",
        "GetID", &CCommandSender::GetID,
        "Self", sol::readonly_property([]() -> const CCommandSender& { return CCommandSender::Self; }),
        "SelfId", sol::readonly_property([]() { return CCommandSender::SelfId; })
    );

    State->new_usertype<CCommandArgsWrapper>
    (
        "CCommandArgsWrapper",
        sol::meta_function::index,
        [](CCommandArgsWrapper& wrapper, sol::object _key, sol::this_state ts) -> sol::object
        {
            if(_key.is<size_t>())
            {
                return sol::make_object(ts, wrapper.GetArgument(_key.as<size_t>()));
            }
            return sol::lua_nil;
        },
        "GetArgument", &CCommandArgsWrapper::GetArgument,
        "IsEmpty", &CCommandArgsWrapper::IsEmpty,
        "GetSize", &CCommandArgsWrapper::GetSize,
        "IsArgumentAvailable", &CCommandArgsWrapper::IsArgumentAvailable,
        "IsArgumentEmpty", &CCommandArgsWrapper::IsArgumentEmpty,
        "Merge", &CCommandArgsWrapper::Merge
    );

    State->new_enum<CCommand::CmdStatus>
    (
        "CmdStatus",
        {
            { "OK", CCommand::CmdStatus::OK },
            { "Incorrect", CCommand::CmdStatus::Incorrect },
            { "Error", CCommand::CmdStatus::Error },
            { "Server", CCommand::CmdStatus::Server }
        }
    );

    State->new_enum<CCommand::CType>
    (
        "CmdType",
        {
            { "Client", CCommand::CType::Client },
            { "ServerPreProcess", CCommand::CType::ServerPreProcess },
            { "Server", CCommand::CType::Server }
        }
    );

    State->new_enum<CCommand::CmdRight>
    (
        "CmdRight",
        {
            { "Client", CCommand::CmdRight::Client },
            { "ClientOnly", CCommand::CmdRight::ClientOnly },
            { "Admin", CCommand::CmdRight::Admin },
            { "AdminOnly", CCommand::CmdRight::AdminOnly },
            { "Console", CCommand::CmdRight::Console }
        }
    );

    /*
	CCallbackHandler<CmdStatus, const CCommandArgsWrapper&, const CCommandSender&> Handlers; //ret:cmdstatus, args, sender
    */

    State->new_usertype<CCommand>
    (
        "CCommand",
        "AddAlias", &CCommand::AddAlias,
        "RemoveAlias", &CCommand::RemoveAlias,
        "IsAliasExist", &CCommand::IsAliasExist,
        "IsNamedLike", &CCommand::IsNamedLike,
        "ProcessRights", &CCommand::ProcessRights,
        "Type", &CCommand::Type,
        "Name", &CCommand::Name,
        "Description", &CCommand::Description
    );

    State->new_usertype<CCommandsManager>
    (
        "CCommandsManager",
        sol::no_constructor,
        "IsCommandPresent", [](CCommandsManager& man, const std::string& name) { return man.IsCommandPresent(name); },
        "GetCommand", [](CCommandsManager& man, const std::string& name) { return man.GetCommand(name).get(); },
        "Register", [](CCommandsManager& man, const std::string& cmdname, int cmdtype, int cmdrights, sol::function cmdfunc) -> int
        {
            if(cmdname.empty() || man.IsCommandPresent(cmdname)) { return 1; }
            if(!cmdfunc.valid()) { return 2; }
            man.CreateLuaCommand(cmdname, (CCommand::CType)cmdtype, (CCommand::CmdRight)cmdrights, cmdfunc);
            return 0;
        }
    );

    State->new_usertype<CCommandProcessor::CCommandsHistory>
    (
        "CCommandsHistory",
        "CurrentInput", &CCommandProcessor::CCommandsHistory::CurrentInput,
        "Commands", &CCommandProcessor::CCommandsHistory::Commands,
        "Index", &CCommandProcessor::CCommandsHistory::Index,
        "Add", &CCommandProcessor::CCommandsHistory::Add,
        "GetNext", &CCommandProcessor::CCommandsHistory::GetNext,
        "GetPrev", &CCommandProcessor::CCommandsHistory::GetPrev,
        "SentCommand", &CCommandProcessor::CCommandsHistory::SentCommand,
        "EditedCommand", &CCommandProcessor::CCommandsHistory::EditedCommand,
        "Reset", &CCommandProcessor::CCommandsHistory::Reset,
        "Clear", &CCommandProcessor::CCommandsHistory::Clear
    );

    State->new_usertype<CCommandProcessor>
    (
        "CCommandProcessor",
        sol::no_constructor,
        SOL_COMPONENT_BASE,
        "History", &CCommandProcessor::History,
        "ProcessCommandLine", &CCommandProcessor::ProcessCommandLine,
        "ProcessSingleCommand", &CCommandProcessor::ProcessSingleCommand,
        "PreProcessString", &CCommandProcessor::PreProcessString,
        "ProcessSpecialChars", &CCommandProcessor::ProcessSpecialChars,
        "Commands", sol::property([](CCommandProcessor& proc) -> CCommandsManager&
        {
            return proc.Commands;
        }),
        "Manager", sol::property([](CCommandProcessor& proc) -> CCommandsManager&
        {
            return proc.Commands;
        }),
        "AddCommandToQueue", &CCommandProcessor::AddCommandToQueue,
        "ProcessQueue", &CCommandProcessor::ProcessQueue,
        "AddAlias", &CCommandProcessor::AddAlias,
        "GetAlias", &CCommandProcessor::GetAlias,
        "AliasExists", &CCommandProcessor::AliasExists,
        "RemoveAlias", &CCommandProcessor::RemoveAlias
    );
}
