#pragma once
#include <map>
#include <string>
#include <memory>
#include "sol/sol.hpp"

#include "U_Log.h"
#include "U_SafeLuaCall.h"

class CScriptClassBase //TODO multiple LUA states
{
public:
	CScriptClassBase(const std::map<std::string, sol::function>& _Functions, std::shared_ptr<sol::state> _State);
	
	template<typename... Args>
	sol::object CallScriptedFunction(const std::string& name, Args... args)
	{
		if(!State) { return sol::lua_nil; }

		auto it = Functions.find(name);
		if(it == Functions.end()) { return sol::lua_nil; }

		if(!it->second.valid() || !it->second.is<sol::function>()) { return sol::lua_nil; }

		return SafeCallLuaReturn(it->second, args...);
		//return (it->second)(args...);
	}
	
	std::shared_ptr<sol::state> State;
	std::map<std::string, sol::function> Functions;
};
