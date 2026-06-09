#pragma once
#include "CScriptClassBase.h"

template<typename Key = std::string>
class CScriptClass : public CScriptClassBase
{
public:
	CScriptClass(const Key& _LuaKey, const std::map<std::string, sol::function>& _Functions, std::shared_ptr<sol::state> _State) : m_LuaKey(_LuaKey), CScriptClassBase(_Functions, _State) {}
	Key GetLuaKey() const { return m_LuaKey; }
private:
	Key m_LuaKey;
};
