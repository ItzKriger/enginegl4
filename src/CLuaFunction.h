#pragma once
#include "CFunctionBase.h"
#include "U_ScriptingStates.h"
#include "U_SafeLuaCall.h"

#include "sol/sol.hpp"
#include <functional>
#include <iostream>

template <typename ReturnValue, typename... Args>
class CLuaFunction : public virtual CFunctionBase<ReturnValue, Args...>
{
public:
	//out - CPP to LUA
	//in  - LUA TO CPP

	using CustomOutCallerFunction = std::function<ReturnValue(const sol::function&, Args...)>;

	CLuaFunction(const sol::function& _func) : State(ScriptStates::GetStateSharedPtr(_func.lua_state()))
	{
		Function = std::make_shared<sol::function>(_func);
	}

	CLuaFunction(const sol::function& _func, const CustomOutCallerFunction& _outcaller) : State(ScriptStates::GetStateSharedPtr(_func.lua_state()))
	{
		Function = std::make_shared<sol::function>(_func);
		MakeCustomOutCaller(_outcaller);
	}

	~CLuaFunction()
	{
		Function.reset();
	}

	ReturnValue Call(Args... args) const override
	{
		if (CustomOutCaller && Function && Function->valid())
		{
			return (*CustomOutCaller)(*Function, args...);
		}
		
		sol::object ret;

		if(Function && Function->valid())
		{
			ret = SafeCallLuaReturn((*Function), args...);
			//ret = (*Function)(args...);
		}

		if constexpr (!std::is_void_v<ReturnValue>)
		{
			return ret.valid() ? ret.as<ReturnValue>() : ReturnValue{};
		}
	}

	bool IsValid() const override
	{
		return Function && Function->valid();
	}

	void MakeCustomOutCaller(const CustomOutCallerFunction& outcaller)
	{
		CustomOutCaller.reset();
		CustomOutCaller = std::make_unique<CustomOutCallerFunction>(outcaller);
	}

	std::unique_ptr<CustomOutCallerFunction> CustomOutCaller;

	std::shared_ptr<sol::state> State;
	std::shared_ptr<sol::function> Function;
};
