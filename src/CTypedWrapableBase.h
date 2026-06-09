#pragma once
#include "CWrapableBase.h"
#include "CCallbackHandler.h"
#include "CConversionRegistry.h"
#include "ILuaWrapableBase.h"
#include "CScriptFieldsManager.h"

#include "U_String.h"
#include "U_Types.h"
#include "U_Scripting.h"
#include "U_SafeLuaCall.h"

template<typename T>
class CTypedWrapableBase : public virtual CWrapableBase
{
public:
	using ValueType = T;

	CScriptFieldsManager ScriptFields;

	CTypedWrapableBase()
	{
		OnValueChanged.SetupLuaTransformers([](const sol::function& func, CTypedWrapableBase<T>* self, T old, T newval) -> void
		{
			SafeCallLua(func, self->GetScriptTable(func.lua_state()), old, newval);
			//func(old, newval);
		},
		[this](auto func, sol::variadic_args va)
		{
			T args[2];
			for(size_t i = 0; i < va.size() && i < sizeof(args) / sizeof(args[0]); i++)
			{
				sol::object obj = va.get<sol::object>();
				if(obj.valid() && obj.is<T>())
				{
					args[i] = obj.as<T>();
				}
			}

			func->Call(this, args[0], args[1]);
			return sol::make_object<int>(va.lua_state(), 0);
		});

		OnScriptInit += [this](std::shared_ptr<sol::state> _state, sol::table _table) -> bool
		{
			ScriptFields.AddField("value", std::make_unique<CFunctionalScriptField>(
			[this](sol::state_view sv) -> sol::object
			{
				return ScriptUtils::ToObject<T>(GetValue(), sv);
			},
			[this](sol::object obj) -> void
			{
				SetValue(ScriptUtils::FromObject<T>(obj));
			}));

			sol::table onvaluechanged = OnValueChanged.GetScriptTable(_state);
			_table["onvaluechanged"] = onvaluechanged;

			ScriptFields.CreateMetaTable(_table);
			return true;
		};
	}

	CTypedWrapableBase(const T& val) : CTypedWrapableBase()
	{
		SetValue(val);
	}

	const std::string GetType() const override
	{
		return GetTypeName<T>();
	}

	const std::type_info& GetTypeInfo() const override
	{
		return typeid(T);
	}

	size_t GetSize() const override
	{
		return sizeof(T);
	}

	void SetValue(const T& value)
	{
		T oldValue = m_getValue();
		m_setValue(value);

		if (!OnValueChanged.IsEmpty())
		{
			OnValueChanged(this, oldValue, value);
		}

		if (!OnStrValueChanged.IsEmpty())
		{
			constexpr bool is_string = std::is_same_v<T, std::string>;
			constexpr bool is_wstring = std::is_same_v<T, std::wstring>;

			if constexpr (is_string)
			{
				OnStrValueChanged(this, StringUtils::StrToWstr(oldValue), StringUtils::StrToWstr(value));
			}
			else if constexpr (is_wstring)
			{
				OnStrValueChanged(this, oldValue, value);
			}
			else
			{
				OnStrValueChanged(this, StringUtils::ToStr<std::wstring>(oldValue), StringUtils::ToStr<std::wstring>(value));
			}
		}

		if(!OnValueChangedShared.IsEmpty())
		{
			OnValueChangedShared(this);
		}
	}

	T GetValue() const
	{
		return m_getValue();
	}

	void SetValueStr(const std::wstring& str) override
	{
		constexpr bool is_string = std::is_same_v<T, std::string>;
		constexpr bool is_wstring = std::is_same_v<T, std::wstring>;

		if constexpr (is_string)
		{
			SetValue(StringUtils::WstrToStr(str));
		}
		else if constexpr (is_wstring)
		{
			SetValue(str);
		}
		else
		{
			SetValue(StringUtils::FromStr<T, std::wstring>(str));
		}
	}

	std::wstring GetValueStr() const override
	{
		constexpr bool is_string = std::is_same_v<T, std::string>;
		constexpr bool is_wstring = std::is_same_v<T, std::wstring>;

		if constexpr (is_string)
		{
			return StringUtils::StrToWstr(m_getValue());
		}
		else if constexpr (is_wstring)
		{
			return m_getValue();
		}
		else
		{
			return StringUtils::ToStr<std::wstring, T>(m_getValue());
		}
	}

	void* m_castToAny(const std::type_info& targetType) const override
	{
		T arg = m_getValue();
		return CConversionRegistry::GetInstance()->convert(&arg, GetTypeInfo(), targetType);
	}

	void m_setByPtr(void* ptr, const std::type_info& targetType) override
	{
		void* var = CConversionRegistry::GetInstance()->convert(ptr, targetType, GetTypeInfo());
		if (!var) { return; }

		T toset = *reinterpret_cast<T*>(var);
		SetValue(toset);

		delete var;
	}

	virtual void m_setValue(const T& val) = 0;
	virtual T m_getValue() const = 0;

	CCallbackHandler<void, CTypedWrapableBase<T>*, T, T> OnValueChanged;
};
