#pragma once
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>
#include <unordered_map>

#include "CFunctionBase.h"
#include "CSwitchable.h"
#include "CFunction.h"
#include "CLuaFunction.h"
#include "CScriptObjectBase.h"
#include "U_SafeLuaCall.h"

class CSmartCallback;
using CSmartCallbackPtr = std::shared_ptr<CSmartCallback>;

class CCallbackHandlerBase : public virtual CSwitchable, public CScriptObjectBase
{
public:
	virtual ~CCallbackHandlerBase();

    void RegisterCallback(CSmartCallback* callback);
    void UnregisterCallback(CSmartCallback* callback);

	virtual sol::object CallFromLua(sol::variadic_args va, sol::state_view s) = 0;
	virtual std::shared_ptr<CSmartCallback> GetLastFunctionHandleShared() = 0;
	virtual CSmartCallbackPtr AddLuaFunction(sol::function func, sol::object name) = 0;
	virtual sol::object CallLua(sol::variadic_args va, sol::this_state s) = 0;

	DEFINE_SOL_USERTYPE();

	virtual void m_deleteFunction(void* ptr) = 0;
	virtual void DeleteFunctionAtIndex(size_t index) = 0;
	virtual void DeleteFunctionsByName(const std::string& name) = 0;
private:
	std::vector<CSmartCallback*> callbacks;
};

class CSmartCallback : public CScriptObjectBase, public std::enable_shared_from_this<CSmartCallback>
{
public:
	CSmartCallback() = delete;
	CSmartCallback(CCallbackHandlerBase& _handler, void* _func);
    ~CSmartCallback();

	void Invalidate();
	void ForceDelete();

	bool IsValid() const;
	bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

	CCallbackHandlerBase& Handler;
	void* FunctionPointer = nullptr;
private:
	void m_InitiateDeletion();
};

template <typename Ret, typename... Args>
class CCallbackHandler : public CCallbackHandlerBase //TODO add CallByName
{
public:
	//out - CPP to LUA
	//in  - LUA TO CPP

	using CLuaArgsOutTransformer = CLuaFunction<Ret, Args...>::CustomOutCallerFunction;
	using CLuaArgsInTransformer = CFunctionBase<Ret, Args...>::CustomInCallerFunction;

	CCallbackHandler() = default;

	CCallbackHandler(CCallbackHandler&&) noexcept = default;
    CCallbackHandler& operator=(CCallbackHandler&&) noexcept = default;

    CCallbackHandler(const CCallbackHandler&) = delete;
    CCallbackHandler& operator=(const CCallbackHandler&) = delete;

	~CCallbackHandler()
	{
		for (auto& func : Functions)
		{
			CLuaFunction<Ret, Args...>* luafunc = dynamic_cast<CLuaFunction<Ret, Args...>*>(func.get());
			if(luafunc) { std::wcout << L"detected lua func at deleting CCallbackHandler\n"; continue; }
		}
	}

	using FuncType = CFunctionBase<Ret, Args...>;
	using FuncTypeUniquePtr = std::unique_ptr<FuncType>;
	using FuncTypeRawPtr = FuncType*;
	using ReturnType = Ret;

	using ReturnsVector = typename std::conditional<std::is_void<Ret>::value, std::vector<int>, std::vector<Ret>>::type;

	CSmartCallbackPtr AddLuaFunction(sol::function func, sol::object name) override
	{
		Add(std::make_unique<CLuaFunction<Ret, Args...>>(func));

		std::string nm;
		if(name.valid() && name.is<std::string>())
		{
			nm = name.as<std::string>();
			if(!nm.empty())
			{
				Functions.back()->SetName(nm);
			}
		}
		return GetLastFunctionHandleShared();
	}

	sol::object CallLua(sol::variadic_args va, sol::this_state s) override
	{
		return CallFromLua(va, s);
	}

	bool V_ScriptInit(std::shared_ptr<sol::state> _state, sol::table table) override //TODO __call
	{
		table.set_function("add", [this](sol::function func, sol::object name) -> sol::table
		{
			Add(std::make_unique<CLuaFunction<Ret, Args...>>(func));

			std::string nm;
			if(name.valid() && name.is<std::string>())
			{
				nm = name.as<std::string>();
				if(!nm.empty())
				{
					Functions.back()->SetName(nm);
				}
			}

			sol::table tabl = sol::state_view(func.lua_state()).create_table();
			FuncTypeRawPtr ptr = Functions.back().get();

			tabl.set_function("delete", [this, ptr]() mutable
			{
				if(!ptr) { return; }
				DeleteFunction(ptr);
				ptr = nullptr;
			});

			tabl.set_function("getSmartDeleter", [this, ptr](sol::this_state ts) mutable
			{
				auto obj = GetFunctionHandleShared(ptr);
				auto ret = obj->GetScriptTable(ts);

				ret["__obj"] = obj;
				return ret;
			});

			tabl.set("name", nm);
			tabl.set("__ptr", ptr);
			return tabl;
		});

		table.set_function("call", [this](sol::variadic_args va, sol::this_state s) -> sol::object
		{
			return CallFromLua(va, s);
		});

		table.set_function("isEnabled", [this](sol::this_state s) -> bool
		{
			return IsEnabled();
		});

		table.set_function("setState", [this](bool state, sol::this_state s) -> void
		{
			return SetEnabledState(state);
		});

		table.set_function("delete", [this](sol::object obj, sol::this_state s) -> void
		{
			if(!obj.valid()) { return; }
			if(obj.is<sol::table>())
			{
				sol::table tabl = obj.as<sol::table>();
				sol::object del_obj = tabl["delete"];

				if(del_obj.valid() && del_obj.is<sol::function>())
				{
					sol::function del_func = del_obj.as<sol::function>();

					SafeCallLua(del_func);
					//del_func();
				}
			}
			else if(obj.is<std::string>())
			{
				std::string nm = obj.as<std::string>();
				DeleteFunctionsByName(nm);
			}
		});

		table.set_function("getLastFunctionSmartDeleter", [this](sol::this_state ts) -> sol::object
		{
			auto obj = GetLastFunctionHandleShared();
			
			auto ret = obj->GetScriptTable(ts);
			ret["__obj"] = obj;

			return ret;
		});
		return true;
	}

	CCallbackHandler& operator+=(FuncTypeUniquePtr&& func)
	{
		Add(std::move(func));
		return *this;
	}

	FuncTypeRawPtr Add(FuncTypeUniquePtr&& func)
	{
		Functions.push_back(std::move(func));
		SetupSingleFunctionTransformers(Functions.back());

		return Functions.back().get();
	}

	bool IsEmpty() const
	{
		return Functions.empty();
	}

	operator bool() const
	{
		return !Functions.empty();
	}

	void operator()(Args... args) const
	{
		return Call(args...);
	}

	void Call(Args... args) const
	{
		if (!IsEnabled()) { return; }
		for (auto& func : Functions)
		{
			(*func)(args...);
		}
	}

	sol::object CallFromLua(sol::variadic_args va, sol::state_view s) override
	{
		if (!IsEnabled()) { return sol::lua_nil; }

		if constexpr (std::is_same_v<Ret, void>)
		{
			for (auto& func : Functions)
			{
				func->CallFromLua(va, s);
			}
			return sol::lua_nil;
		}
		else
		{
			size_t index = 1;
			sol::table ret = s.create_table();

			for (auto& func : Functions)
			{
				ret[index] = func->CallFromLua(va, s);
				index++;
			}
			return ret;
		}
	}

	ReturnsVector CallWithReturns(Args... args) const
	{
		if (!IsEnabled()) { return ReturnsVector(); }

		ReturnsVector ret;
		if constexpr (std::is_void_v<Ret>)
		{
			ret.resize(Functions.size());

			std::fill(ret.begin(), ret.end(), int(0));

			for (auto& func : Functions)
			{
				(*func)(args...);
			}
			return ret;
		}
		else
		{
			ret.reserve(Functions.size());

			for (auto& func : Functions)
			{
				Ret result = (*func)(args...);
				ret.push_back(result);
			}
			return ret;
		}
	}

	CFunction<Ret, Args...>* AddFunction(const std::function<Ret(Args...)>& func)
	{
		Functions.push_back(std::make_unique<CFunction<Ret, Args...>>(func));
		SetupSingleFunctionTransformers(Functions.back());

		return dynamic_cast<CFunction<Ret, Args...>*>(Functions.back().get());
	}

	CLuaFunction<Ret, Args...>* AddLuaFunction(const sol::protected_function& func)
	{
		Add(std::make_unique<CLuaFunction<Ret, Args...>>(func));
		return dynamic_cast<CLuaFunction<Ret, Args...>*>(Functions.back().get());
	}

	CLuaFunction<Ret, Args...>* AddLuaFunction(std::unique_ptr<CLuaFunction<Ret, Args...>>&& func)
	{
		return dynamic_cast<CLuaFunction<Ret, Args...>*>(Add(func));
	}

	CCallbackHandler& operator+=(const std::function<Ret(Args...)>& func)
	{
		AddFunction(func);
		return *this;
	}

	CCallbackHandler& operator+=(const sol::protected_function& func)
	{
		AddLuaFunction(func);
		return *this;
	}

	bool IsEmpty()
	{
		return Functions.empty();
	}

	void SetupLuaTransformers(const CLuaArgsOutTransformer& outtransform, const CLuaArgsInTransformer& intransform)
	{
		LuaArgsOutTransformer = std::make_unique<CLuaArgsOutTransformer>(outtransform);
		LuaArgsInTransformer = std::make_unique<CLuaArgsInTransformer>(intransform);

		for(auto& func : Functions)
		{
			SetupSingleFunctionTransformers(func);
		}
	}

	size_t GetCount()
	{
		return Functions.size();
	}

	size_t GetLastIndex()
	{
		if(IsEmpty()) { return 0; }
		return GetCount() - 1;
	}

	CSmartCallback GetLastFunctionHandle()
	{
		return CSmartCallback(*this, Functions.empty() ? nullptr : Functions.back().get());
	}

	std::shared_ptr<CSmartCallback> GetLastFunctionHandleShared() override
	{
		return std::make_shared<CSmartCallback>(*this, Functions.empty() ? nullptr : Functions.back().get());
	}

	std::unique_ptr<CSmartCallback> GetLastFunctionHandleUnique()
	{
		return std::make_unique<CSmartCallback>(*this, Functions.empty() ? nullptr : Functions.back().get());
	}

	CSmartCallback GetFunctionHandle(FuncTypeRawPtr rawptr)
	{
		auto it = std::find_if(Functions.begin(), Functions.end(), [rawptr](auto& func) -> bool { return func.get() == rawptr; });
		return CSmartCallback(*this, (it == Functions.end()) ? nullptr : (*it).get());
	}

	std::shared_ptr<CSmartCallback> GetFunctionHandleShared(FuncTypeRawPtr rawptr)
	{
		auto it = std::find_if(Functions.begin(), Functions.end(), [rawptr](auto& func) -> bool { return func.get() == rawptr; });
		return std::make_shared<CSmartCallback>(*this, (it == Functions.end()) ? nullptr : (*it).get());
	}

	std::unique_ptr<CSmartCallback> GetFunctionHandleUnique(FuncTypeRawPtr rawptr)
	{
		auto it = std::find_if(Functions.begin(), Functions.end(), [rawptr](auto& func) -> bool { return func.get() == rawptr; });
		return std::make_unique<CSmartCallback>(*this, (it == Functions.end()) ? nullptr : (*it).get());
	}

	FuncTypeUniquePtr& GetLastFunction()
	{
		if(IsEmpty()) { throw std::runtime_error("No functions"); }
		return Functions.back();
	}

	FuncTypeRawPtr GetLastFunctionRaw()
	{
		if(IsEmpty()) { return nullptr; }
		return Functions.back().get();
	}

	void DeleteFunctionAtIndex(size_t index) override
	{
		if(index >= Functions.size()) { return; }
		Functions.erase(Functions.begin() + index);
	}

	void DeleteFunction(FuncTypeUniquePtr& smrptr)
	{
		auto it = std::find_if(Functions.begin(), Functions.end(), [&smrptr](auto& myptr) -> bool
		{
			return smrptr == myptr;
		});

		if(it == Functions.end()) { return; }
		Functions.erase(it);
	}

	void DeleteFunction(FuncTypeRawPtr ptr)
	{
		auto it = std::find_if(Functions.begin(), Functions.end(), [ptr](auto& myptr) -> bool
		{
			return ptr == myptr.get();
		});

		if(it == Functions.end()) { return; }
		Functions.erase(it);
	}

	void m_deleteFunction(void* ptr) override
	{
		auto it = std::find_if(Functions.begin(), Functions.end(), [ptr](auto& myptr) -> bool
		{
			return ptr == static_cast<void*>(myptr.get());
		});

		if(it == Functions.end()) { return; }
		Functions.erase(it);
	}

	void DeleteFunctionsByName(const std::string& name) override
	{
		if(name.empty()) { return; }
		
		std::erase_if(Functions, [&name](auto& ptr) -> bool
		{
			return ptr->HasName() && ptr->GetName() == name;
		});
	}

	std::vector<FuncTypeUniquePtr> Functions;
private:
	void SetupSingleFunctionTransformers(FuncTypeUniquePtr& func)
	{
		CLuaFunction<Ret, Args...>* luafunc = dynamic_cast<CLuaFunction<Ret, Args...>*>(func.get());

		if(luafunc && LuaArgsOutTransformer)
		{
			luafunc->MakeCustomOutCaller(*LuaArgsOutTransformer);
		}

		if(LuaArgsInTransformer)
		{
			func->MakeCustomInCaller(*LuaArgsInTransformer);
		}
	}

	std::unique_ptr<CLuaArgsOutTransformer> LuaArgsOutTransformer;
	std::unique_ptr<CLuaArgsInTransformer> LuaArgsInTransformer;
};
