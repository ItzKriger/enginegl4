#pragma once
#include "CComponent.h"
#include "CCommandsManager.h"
#include <vector>
#include <shared_mutex>
#include <stack>

class CCustomCommandProcessor
{
public:
	virtual ~CCustomCommandProcessor() = default;
	void ProcessSingleCommand(const std::string& cmd, const CCommandArgsWrapper& args, const CCommandSender& sender);

	virtual void m_process(const std::string& cmd, const CCommandArgsWrapper& args, const CCommandSender& sender) = 0;

	void Quit();
	CCallbackHandler<void, CCustomCommandProcessor*> OnQuit;
};

class CCommandProcessor : public CComponent
{
public:
	class CCommandsHistory
	{
	public:
		CCommandsHistory();

		std::wstring CurrentInput;
		std::vector<std::wstring> Commands;

		int Index = -1;

		void Add(const std::wstring& cmd);

		std::wstring GetNext();
		std::wstring GetPrev();

		void SentCommand(const std::wstring& cmd = L"", bool add = true);
		void EditedCommand(const std::wstring& cmd);
		void EditedCommandAsync(const std::wstring& cmd);

		void Reset();
		void Clear();

		std::mutex Mutex;
	} History;

	void ProcessCommandLine(const std::wstring& cmdline, const CCommandSender& sender);
	CCommand::CmdStatus ProcessSingleCommand(const std::string& cmd, const CCommandArgsWrapper& args, const CCommandSender& sender);

	static std::vector<std::vector<std::wstring>> PreProcessString(const std::wstring& cmdline);
	static std::wstring ProcessSpecialChars(const std::wstring& cmdline);

	sol::object V_GetScriptUserType(sol::state_view state) override;
	bool V_ScriptInit(std::shared_ptr<sol::state> _state, sol::table table) override;

	CCommandsManager Commands;
	DEFINE_COMPONENT();

	void V_Update() override;
	void AddCommandToQueue(const std::wstring& cmd);
	void ProcessQueue();

	void AddCustomProcessor(std::unique_ptr<CCustomCommandProcessor>&& ptr);
	void QuitCustomProcessor();

	void AddAlias(const std::string& _cmd1, const std::wstring& _cmd2);
	std::wstring GetAlias(const std::string& _alias) const;
	bool AliasExists(const std::string& _alias) const;
	void RemoveAlias(const std::string& _alias);

	std::shared_mutex Mutex;
	std::vector<std::wstring> Queue;

	std::unordered_map<std::string, std::wstring> Aliases;
private:
	std::stack<std::unique_ptr<CCustomCommandProcessor>> m_customProcs;
};
