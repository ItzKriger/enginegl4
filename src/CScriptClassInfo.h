#pragma once
#include <memory>
#include <map>

#include "CScriptingEngine.h"

template<typename IdType>
class CScriptClassInfo
{
public:
    CScriptClassInfo(const IdType& _LuaKey, std::shared_ptr<sol::state> _State, const std::map<std::string, sol::function>& _Functions, CScriptingEngine* engine) : 
        LuaKey(_LuaKey), State(_State), Functions(_Functions), Engine(engine) {}

    IdType LuaKey;
	std::shared_ptr<sol::state> State;
	std::map<std::string, sol::function> Functions;
	CScriptingEngine* Engine = nullptr;
};
