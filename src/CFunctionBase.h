#pragma once
#include "CIteratableBase.h"
#include "U_ScriptingArgs.h"

#include "sol/sol.hpp"

template <typename ReturnValue, typename... Args>
class CFunctionBase : public virtual CIteratableBase<CFunctionBase<ReturnValue, Args...>>
{
public:
	//out - CPP to LUA
	//in  - LUA TO CPP

	using CustomInCallerFunction = std::function<sol::object(CFunctionBase*, sol::variadic_args)>;

	virtual ~CFunctionBase() {}

	sol::object CallFromLua(sol::variadic_args va, sol::state_view s)
	{
		if (CustomInCaller)
		{
			return (*CustomInCaller)(this, va);

			/*if constexpr (std::is_same_v<ReturnValue, void>)
			{
				(*CustomInCaller)(this, va);
				return sol::lua_nil;
			}
			else
			{
				return sol::make_object(s, (*CustomInCaller)(this, va));
			}*/
		}

		auto lua_state = va.lua_state();
		
		auto extracted_args = ScriptingArgs::extract_args<Args...>(va);
		return std::apply([this, lua_state](Args... args) -> sol::object
		{
			if constexpr (!std::is_void_v<ReturnValue>)
			{
				return sol::make_object<ReturnValue>(lua_state, this->Call(args...));
			}
			this->Call(args...);
			return sol::lua_nil;
		}, extracted_args);

		/*if constexpr (std::is_same_v<ReturnValue, void>)
		{
			std::apply([this](Args... args) -> ReturnValue { this->Call(args...); }, extracted_args);
			return sol::lua_nil;
		}
		else
		{
    		return sol::make_object(s, std::apply([this](Args... args) -> ReturnValue { return this->Call(args...); }, extracted_args));
		}*/
	}

	virtual ReturnValue Call(Args... args) const = 0;
	ReturnValue operator()(Args... args) const
	{
		return Call(args...);
	}

	virtual bool IsValid() const = 0;

	operator bool() const
	{
		return (bool)IsValid();
	}

	void MakeCustomInCaller(const CustomInCallerFunction& incaller)
	{
		CustomInCaller.reset();
		CustomInCaller = std::make_unique<CustomInCallerFunction>(incaller);
	}

	std::unique_ptr<CustomInCallerFunction> CustomInCaller;

	bool HasName() const { return !m_Name.empty(); }
	std::string GetName() const { return m_Name; }
	void SetName(const std::string& _Name) { m_Name = _Name; }
private:
	std::string m_Name;
};
