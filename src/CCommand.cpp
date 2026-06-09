#include "CCommand.h"

void CCommand::AddAlias(const std::string& alias)
{
	if (IsAliasExist(alias) || alias == Name) { return; }
	Aliases.insert(alias);
}

void CCommand::RemoveAlias(const std::string& alias)
{
	if (!IsAliasExist(alias)) { return; }
	Aliases.erase(alias);
}

bool CCommand::IsAliasExist(const std::string& alias) const
{
	return Aliases.find(alias) != Aliases.end();
}

CCommand::CmdStatus CCommand::CallHandler(const CCommandArgsWrapper& args, const CCommandSender& sender) const
{
	auto ret = Handlers.CallWithReturns(args, sender);
	if (ret.empty()) { return CCommand::CmdStatus::Error; }
	return ret.front();
}

CCommand::CmdStatus CCommand::operator()(const CCommandArgsWrapper& args, const CCommandSender& sender) const
{
	return CallHandler(args, sender);
}

bool CCommand::IsNamedLike(const std::string& name) const
{
	if (Name == name) { return true; }
	auto it = Aliases.find(name);
	return it != Aliases.end();

	//return (Name == name) ? true : (Aliases.find(name) != Aliases.end());
}

std::make_signed_t<size_t> CCommandSender::GetID() const
{
	return m_senderId;
}

const CCommandSender CCommandSender::Self;
const std::make_signed_t<size_t> CCommandSender::SelfId = -1;
