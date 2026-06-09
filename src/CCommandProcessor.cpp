#include "CCommandProcessor.h"
#include "CConVarManager.h"
#include "CEngine.h"
#include "U_String.h"
#include "CLogger.h"
#include "CWrapable.h"

#include <sstream>
#include <chrono>
#include <thread>
#include <unordered_map>

std::unordered_map<std::string, CCommand::CType> CmdStringTypes = 
{
	{ "client", CCommand::CType::Client },
	{ "serverpreprocess", CCommand::CType::ServerPreProcess},
	{ "server", CCommand::CType::Server }
};

std::unordered_map<std::string, CCommand::CmdRight> CmdStringRights = 
{
	{ "client", CCommand::CmdRight::Client },
	{ "clientonly", CCommand::CmdRight::ClientOnly },
	{ "admin", CCommand::CmdRight::Admin },
	{ "adminonly", CCommand::CmdRight::AdminOnly },
	{ "console", CCommand::CmdRight::Console }
};

std::unordered_map<std::string, CCommand::CmdStatus> CmdStringStatuses = 
{
	{ "ok", CCommand::CmdStatus::OK },
	{ "incorrect", CCommand::CmdStatus::Incorrect },
	{ "error", CCommand::CmdStatus::Error },
	{ "server", CCommand::CmdStatus::Server }
};

bool CCommandProcessor::V_ScriptInit(std::shared_ptr<sol::state> _state, sol::table table)
{
	if(_state->get<sol::object>("command") == sol::lua_nil)
	{
		sol::table command = _state->create_named_table("command");

		sol::table type = command.create_named("type");
		sol::table rights = command.create_named("rights");
		sol::table status = command.create_named("status");
		
		for(auto& kv : CmdStringTypes) { type.set(kv.first, int(kv.second)); }
		for(auto& kv : CmdStringRights) { rights.set(kv.first, int(kv.second)); }
		for(auto& kv : CmdStringStatuses) { status.set(kv.first, int(kv.second)); }
	}

	table.set_function("register", [this](const std::string& cmdname, int cmdtype, int cmdrights, sol::function cmdfunc) -> int
	{
		if(cmdname.empty() || Commands.IsCommandPresent(cmdname)) { return 1; }
		if(!cmdfunc.valid()) { return 2; }
		Commands.CreateLuaCommand(cmdname, (CCommand::CType)cmdtype, (CCommand::CmdRight)cmdrights, cmdfunc);
		return 0;
	});

	table.set_function("process", [this](const std::wstring& cmdline) -> void //TODO maybe return statuses + ability to specify sender
	{
		ProcessCommandLine(cmdline, CCommandSender::Self);
	});

	return true;
}

void CCommandProcessor::AddCommandToQueue(const std::wstring& cmd)
{
	std::lock_guard lock(Mutex);
	Queue.push_back(cmd);

	History.SentCommand(cmd);
}

void CCommandProcessor::ProcessQueue()
{
	std::vector<std::wstring> queue_copy;
	{
		std::lock_guard lock(Mutex);

		if(Queue.empty()) { return; }
		queue_copy.swap(Queue);
	}

	bool echo = true;
	COMPONENT_CALL_GET(echo, CConVarManager, GetConVarValue<bool>("con.echo"));

	for (auto& cmd : queue_copy)
	{
		if(echo) { COMPONENT_CALL(CLogger, Outln(L"> " + cmd)); }
		ProcessCommandLine(cmd, CCommandSender::Self);
	}
}

void CCommandProcessor::V_Update()
{
	ProcessQueue();
}

std::wstring CCommandProcessor::ProcessSpecialChars(const std::wstring& cmdline)
{
	std::wstring line = cmdline;

	StringUtils::replace<std::wstring>(line, L"\\", L"\\\\");
	StringUtils::replace<std::wstring>(line, L"\n", L"\\n");
	StringUtils::replace<std::wstring>(line, L"\r", L"\\r");
	StringUtils::replace<std::wstring>(line, L"\"", L"\\\"");
	StringUtils::replace<std::wstring>(line, L";", L"\\;");
	return line;
}

CCommand::CmdStatus CCommandProcessor::ProcessSingleCommand(const std::string& cmd, const CCommandArgsWrapper& args, const CCommandSender& sender)
{
	if(!m_customProcs.empty()) { m_customProcs.top()->ProcessSingleCommand(cmd, args, sender); return CCommand::CmdStatus::OK; }

	CConVarManager::CVarNode* cvar = nullptr;
	COMPONENT_CALL_GET(cvar, CConVarManager, Root.GetNode(cmd));

	auto italias = Aliases.find(cmd);

	if(italias != Aliases.end()) //TODO aliases only for self sender (console)
	{
		std::wstring cmdline = italias->second;
		for(size_t i = 0; i < args.size(); i++)
		{
			std::wstring thisArg = L"%ARG" + std::to_wstring(i) + L"%";
			StringUtils::replace(cmdline, thisArg, args.at(i));
		}
		ProcessCommandLine(cmdline, sender);
		return CCommand::CmdStatus::OK;
	}

	bool exist = Commands.IsCommandPresent(cmd);

	if(cvar && !exist)
	{
		if(args.empty())
		{
			std::wstringstream ws;
			ws << StringUtils::StrToWstr(cmd) << L" is a node " << (cvar->Value.Wrapable ? (L"with value \"" + cvar->Value.Wrapable->GetValueStr() + L"\"") : L"without value") << '\n';

			COMPONENT_CALL(CLogger, Out(ws.str()));
		}
		else
		{
			std::wstring val = args.Merge(0, args.size() - 1);

			if(cvar->Value.Wrapable)
			{
				cvar->Value.Wrapable->SetValueStr(val);
			}
		}
		return CCommand::CmdStatus::OK;
	}

	if (!exist) { return CCommand::CmdStatus::Incorrect; }

	auto& command = Commands.GetCommand(cmd);	

	//TODO command/user rights check

	CCommand::CmdStatus status = command->CallHandler(args, sender);
	return status;
}

std::vector<std::vector<std::wstring>> CCommandProcessor::PreProcessString(const std::wstring& cmdline) //TODO rewrite it
{
	if (StringUtils::IsEmpty(cmdline)) { return {}; }

	bool inString = false;
	std::vector<std::wstring> cmdlines;
	std::wstring procString = cmdline;
	std::vector<std::vector<std::wstring>> ret;

	wchar_t reat = procString.front();
	while (reat == L' ')
	{
		procString.erase(procString.begin());
		reat = procString.front();
	}

	reat = procString.back();
	while (reat == L' ')
	{
		procString.erase(procString.begin() + (procString.size() - 1));
		reat = procString.back();
	}

	for (size_t i = 0; i < procString.size(); ++i)
	{
		if ((i > 0 && procString.at(i) == L'\"' && procString.at(i - 1) != L'\\') || (i == 0 && procString.at(0) == L'\"'))
		{
			inString = !inString;
			continue;
		}

		if (i > 0 && !inString && (procString.at(i) == L' ' || procString.at(i) == L'\t') && (procString.at(i - 1) == L' ' || procString.at(i - 1) == L'\t'))
		{
			procString.erase(procString.begin() + (i - 1));
			i--;
		}
	}

	inString = false;
	for (size_t i = 0; i < procString.size(); ++i)
	{
		if ((i > 0 && procString.at(i) == L'\"' && procString.at(i - 1) != L'\\') || (i == 0 && procString.at(0) == L'\"'))
		{
			inString = !inString;
			continue;
		}

		if ((i + 1) < procString.size() && (procString.at(i) == L' ' || procString.at(i) == L'\t') && procString.at(i + 1) == L';')
		{
			procString.erase(procString.begin() + i);
			i--;
		}
	}

	inString = false;
	std::wstring curCmd;

	for (size_t i = 0; i < procString.size(); ++i)
	{
		if ((i > 0 && procString.at(i) == L'\"' && procString.at(i - 1) != L'\\') || (i == 0 && procString.at(0) == L'\"'))
		{
			inString = !inString;
		}

		if (procString.at(i) == L';' && ((i == 0) || (procString.at(i - 1) != L'\\')) && !inString)
		{
			procString.erase(procString.begin() + i);
			if (!StringUtils::IsEmpty(curCmd))
			{
				cmdlines.push_back(curCmd);
				//Log::Instance() << "cmdlines.push_back(\"" << curCmd << "\")\n";
				curCmd.clear();
			}
			//continue; TODO hack (when commented)
		}

		curCmd += procString.at(i);

		if ((i + 1) >= procString.size())
		{
			if (!StringUtils::IsEmpty(curCmd))
			{
				cmdlines.push_back(curCmd);
				//Log::Instance() << "cmdlines.push_back2(\"" << curCmd << "\")\n";
				curCmd.clear();
			}
		}
	}

	for (std::wstring& s : cmdlines)
	{
		bool inStr = false;
		std::wstring curline;
		std::vector<std::wstring> toadd;

		for (size_t i = 0; i < s.size(); ++i)
		{
			if ((i > 0 && s.at(i) == '\"' && s.at(i - 1) != '\\') || (i == 0 && s.at(0) == '\"'))
			{
				inStr = !inStr;

				if ((i + 1) >= s.size())
				{
					toadd.push_back(curline);
					curline.clear();
				}
				continue;
			}

			if ((s.at(i) == ' ' || s.at(i) == '\t') && !inStr)
			{
				if (!StringUtils::IsEmpty(curline))
				{
					toadd.push_back(curline);
					curline.clear();
				}
				continue;
			}

			curline += s.at(i);

			if ((i + 1) >= s.size())
			{
				if (!StringUtils::IsEmpty(curline))
				{
					toadd.push_back(curline);
					curline.clear();
				}
			}
		}

		for (std::wstring& ins : toadd)
		{
			StringUtils::replace<std::wstring>(ins, L"\\\\", L"\\");
			StringUtils::replace<std::wstring>(ins, L"\\n", L"\n");
			StringUtils::replace<std::wstring>(ins, L"\\r", L"\r");
			StringUtils::replace<std::wstring>(ins, L"\\\"", L"\"");
			StringUtils::replace<std::wstring>(ins, L"\\;", L";");
		}

		if (!toadd.empty())
		{
			ret.push_back(toadd);
		}
	}

	return ret;
}

void CCommandProcessor::ProcessCommandLine(const std::wstring& cmdline, const CCommandSender& sender) //TODO maybe return vector<CmdStatus>
{
	std::vector<std::vector<std::wstring>> commands = PreProcessString(cmdline);

	for (std::vector<std::wstring>& args : commands)
	{
		if (args.empty()) { continue; }

		std::wstring wcommand = args.front();
		std::string command = StringUtils::WstrToStr(args.front());
		args.erase(args.begin());

		CCommand::CmdStatus status = ProcessSingleCommand(command, args, sender);

		if (status == CCommand::CmdStatus::Incorrect)
		{
			//TODO status notification
		}
	}
}

CCommandProcessor::CCommandsHistory::CCommandsHistory()
{
	Commands.reserve(64);

	COMPONENT_CALL(CConVarManager, AddConVar("con.history.max", new CWrapable<unsigned int>(64)));
	COMPONENT_CALL(CConVarManager, AddConVar("con.history.edit.keepindex", new CWrapable<unsigned int>(1)));
	COMPONENT_CALL(CConVarManager, AddConVar("con.history.edit.replace", new CWrapable<unsigned int>(0)));
}

void CCommandProcessor::CCommandsHistory::Add(const std::wstring& cmd)
{
	if (cmd.empty()) { return; }

	unsigned int historyMax = 64;
	COMPONENT_CALL_GET(historyMax, CConVarManager, GetConVarValue<unsigned int>("con.history.max", 64));

	if (Commands.size() + 1 >= historyMax)
	{
		Commands.erase(Commands.begin());
	}
	Commands.push_back(cmd);
}

std::wstring CCommandProcessor::CCommandsHistory::GetNext()
{
	if (Commands.empty()) { return CurrentInput; }
	if (Index + 1 < Commands.size()) { Index++; }
	return *(Commands.rbegin() + Index);
}

std::wstring CCommandProcessor::CCommandsHistory::GetPrev()
{
	if (Commands.empty()) { return CurrentInput; }
	if (Index - 1 >= -1) { Index--; }
	if (Index < 0) { return CurrentInput; }
	return *(Commands.rbegin() + Index);
}

void CCommandProcessor::CCommandsHistory::SentCommand(const std::wstring& cmd, bool add)
{
	Index = -1;
	CurrentInput.clear();

	if (add)
	{
		Add(cmd);
	}
}

void CCommandProcessor::CCommandsHistory::EditedCommandAsync(const std::wstring& cmd)
{
	std::lock_guard lock(Mutex);
	EditedCommand(cmd);
}

void CCommandProcessor::CCommandsHistory::EditedCommand(const std::wstring& cmd)
{
	bool keepindex = true, editreplace = false;

	COMPONENT_CALL_GET(keepindex, CConVarManager, GetConVarValue<unsigned int>("con.history.edit.keepindex", 1));
	COMPONENT_CALL_GET(editreplace, CConVarManager, GetConVarValue<unsigned int>("con.history.edit.replace", 0));

	if (keepindex && editreplace && !cmd.empty())
	{
		Commands.at(Index) = cmd;
	}
	else if (!keepindex)
	{
		CurrentInput = cmd;
		Index = -1;
	}
}

void CCommandProcessor::CCommandsHistory::Clear()
{
	Index = -1;
	Commands.clear();
}

void CCommandProcessor::CCommandsHistory::Reset()
{
	Index = -1;
	CurrentInput.clear();
}

void CCommandProcessor::AddCustomProcessor(std::unique_ptr<CCustomCommandProcessor>&& ptr)
{
	m_customProcs.push(std::move(ptr));
}

void CCommandProcessor::QuitCustomProcessor()
{
	if(m_customProcs.empty()) { return; }

	m_customProcs.top()->OnQuit(m_customProcs.top().get());
	m_customProcs.pop();
}

void CCustomCommandProcessor::Quit()
{
	COMPONENT_CALL(CCommandProcessor, QuitCustomProcessor());
}

void CCustomCommandProcessor::ProcessSingleCommand(const std::string& cmd, const CCommandArgsWrapper& args, const CCommandSender& sender)
{
	if(cmd == "quit" || cmd == "exit") { return Quit(); }
	return m_process(cmd, args, sender);
}

void CCommandProcessor::AddAlias(const std::string& _cmd1, const std::wstring& _cmd2)
{
	auto it = Aliases.find(_cmd1);
	
	if(it != Aliases.end())
	{
		Aliases.erase(it);
	}
	Aliases.emplace(_cmd1, _cmd2);
}

std::wstring CCommandProcessor::GetAlias(const std::string& _alias) const
{
	auto it = Aliases.find(_alias);
	return it != Aliases.end() ? it->second : std::wstring{};
}

bool CCommandProcessor::AliasExists(const std::string& _alias) const
{
	return Aliases.find(_alias) != Aliases.end();
}

void CCommandProcessor::RemoveAlias(const std::string& _alias)
{
	auto it = Aliases.find(_alias);
	if(it != Aliases.end())
	{
		Aliases.erase(it);
	}
}

sol::object CCommandProcessor::V_GetScriptUserType(sol::state_view state)
{
	return sol::make_object(state, this);
}

LINK_COMPONENT_TO_CLASS(CCommandProcessor, commandprocessor);