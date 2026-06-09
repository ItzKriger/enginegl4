#pragma once
#include <set>
#include <string>
#include <vector>
#include "CCallbackHandler.h"

#include "CCommandArgsWrapper.h"

#define COMMAND_ARGS const CCommandArgsWrapper& args, const CCommandSender& sender
#define COMMAND_ARGS_TEMPLATE const CCommandArgsWrapper&, const CCommandSender&
#define COMMAND_LAMBDA (const CCommandArgsWrapper& args, const CCommandSender& sender) -> CCommand::CmdStatus
#define COMMAND_RETURN CCommand::CmdStatus

#define CMD_OK CCommand::CmdStatus::OK
#define CMD_INC CCommand::CmdStatus::Incorrect
#define CMD_ERR CCommand::CmdStatus::Error
#define CMD_SRV CCommand::CmdStatus::Server

class CCommandSender //TODO handle with serial number etc instead of ID
{
public:
	std::make_signed_t<size_t> GetID() const;

	//static CCommandSender FromClient(cl);
	//static CCommandSender Console();
	static const CCommandSender Self; //console, or client processing it's own commands
	static const std::make_signed_t<size_t> SelfId; //console, or client processing it's own commands
private:
	std::make_signed_t<size_t> m_senderId = SelfId;
};

class CCommand
{
public:
	enum class CmdStatus
	{
		OK,
		Incorrect,
		Error,
		Server
	};

	enum class CType
	{
		Client,
		ServerPreProcess,
		Server
	};

	enum class CmdRight
	{
		Client,
		ClientOnly,
		Admin,
		AdminOnly,
		Console
	};

	void AddAlias(const std::string& alias);
	void RemoveAlias(const std::string& alias);
	bool IsAliasExist(const std::string& alias) const;

	bool IsNamedLike(const std::string& name) const;

	CmdStatus CallHandler(const CCommandArgsWrapper& args, const CCommandSender& sender) const;
	CmdStatus operator()(const CCommandArgsWrapper& args, const CCommandSender& sender) const;

	CmdRight ProcessRights = CmdRight::Client;
	CType Type = CType::Client;

	std::string Name, Description;
	std::set<std::string> Aliases;

	CCallbackHandler<CmdStatus, const CCommandArgsWrapper&, const CCommandSender&> Handlers; //ret:cmdstatus, args, sender
};
