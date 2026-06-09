#pragma once
#include <string>
#include <iostream>
#include "sol/sol.hpp"

#include "CCallbackHandler.h"
#include "CScriptObject.h"

#include "U_String.h"
#include "U_SafeLuaCall.h"

class CWrapableBase : public CScriptObject
{
public:
	CWrapableBase()
	{
		OnStrValueChanged.SetupLuaTransformers([](const sol::function& func, CWrapableBase* self, std::wstring old, std::wstring newval) -> void
		{
			SafeCallLua(func, self->GetScriptTable(func.lua_state()), old, newval);
			//func(old, newval);
		},
		[this](auto func, sol::variadic_args va)
		{
			std::wstring args[2];
			for(size_t i = 0; i < va.size() && i < sizeof(args) / sizeof(args[0]); i++)
			{
				sol::object obj = va.get<sol::object>();
				if(obj.valid() && obj.is<std::wstring>())
				{
					args[i] = obj.as<std::wstring>();
				}
			}

			func->Call(this, args[0], args[1]);
			return sol::make_object<int>(va.lua_state(), 0);
		});

		OnValueChangedShared.SetupLuaTransformers([](const sol::function& func, CWrapableBase* self) -> void
		{
			SafeCallLua(func, self->GetScriptTable(func.lua_state()));
			//func(old, newval);
		},
		[this](auto func, sol::variadic_args va)
		{
			func->Call(this);
			return sol::make_object<int>(va.lua_state(), 0);
		});

		OnScriptInit += [this](std::shared_ptr<sol::state> _state, sol::table _table) -> bool
		{
			sol::table onstrvaluechanged = OnStrValueChanged.GetScriptTable(_state);
			sol::table onvaluechangedshared = OnValueChangedShared.GetScriptTable(_state);

			_table["onstrvaluechanged"] = onstrvaluechanged;
			_table["onvaluechangedshared"] = onvaluechangedshared;

			return true;
		};
	}

	virtual ~CWrapableBase();

	void SetValueStrANSI(const std::string& str);
	std::string GetValueStrANSI() const;

	template<typename T>
	void SetWrapValue(T val)
	{
		SetValueStr(ToWstr<T>(val));
	}

	template<typename T>
	T GetWrapValue() const
	{
		return StringUtils::FromStr<T>(GetValueStr());
	}

	template <typename U>
	U GetCastValue() const
	{
		void* casted = m_castToAny(typeid(U));
		if (!casted) { return U{}; }

		U ret = *(reinterpret_cast<U*>(casted));

		delete casted;
		return ret;
	}

	template <typename U>
	void SetCastValue(U val)
	{
		return m_setByPtr(&val, typeid(U));
	}

	virtual void m_setByPtr(void* ptr, const std::type_info& targetType) = 0;
	virtual void* m_castToAny(const std::type_info& targetType) const = 0;

	CCallbackHandler<void, CWrapableBase*, std::wstring, std::wstring> OnStrValueChanged;
	CCallbackHandler<void, CWrapableBase*> OnValueChangedShared;

	virtual void SetValueStr(const std::wstring& str) = 0;
	virtual std::wstring GetValueStr() const = 0;

	virtual const std::string GetType() const = 0;
	virtual const std::type_info& GetTypeInfo() const = 0;
	virtual size_t GetSize() const = 0;
};
