#pragma once
#include "sol/sol.hpp"
#include "CCallbackHandler.h"
#include "CScriptComponentTablesCache.h"
#include "CScriptComponentsManager.h"
#include "CScriptConvarTablesCache.h"
#include "U_SafeLuaCall.h"

#include <filesystem>
#include <string>
#include <map>
#include <tuple>
#include <unordered_map>

class CScriptingEngine
{
public:
    CScriptingEngine();
    virtual ~CScriptingEngine();

    void Init();
    virtual void V_Init();

    void AddHook(const std::string& hooksGroup, sol::function func, const std::string& hookName = "");
    void DeleteHook(const std::string& hooksGroup, sol::function func);
    void DeleteHook(const std::string& hooksGroup, const std::string& hookName);

    void ScriptFile(const std::filesystem::path& path);
    void ScriptString(const std::wstring& code);
    void ScriptDirectory(const std::filesystem::path& dir);

    void RegisterGeneralTypes();
    void RegisterOtherTypes();

    void RegisterDrawableTypes();
    void RegisterResourceTypes();
    void RegisterPhysicsTypes();
    void RegisterEntityTypes();
    void RegisterGameTypes();
    void RegisterTransformTypes();
    void RegisterMath();
    void RegisterConVarTypes();
    void RegisterGenericTypes();
    void RegisterEngineType();
    void RegisterComponentType();
    void RegisterCallbackHandlerType();
    void RegisterCommandsTypes();

    std::shared_ptr<sol::state> State;
    std::multimap<std::string, std::pair<std::string, sol::function>> Hooks;

    template<typename... Args>
    void CallHook(const std::string& hooksGroup, Args... args)
    {
        auto range = Hooks.equal_range(hooksGroup);

        for (auto it = range.first; it != range.second; ++it)
        {
            SafeCallLua(it->second.second, args...);
            //it->second.second(args...);
        }
    }

    template<typename... Args>
    void CallSingleHook(const std::string& hooksGroup, const std::string& hookName, Args... args)
    {
        auto it = std::find_if(Hooks.begin(), Hooks.end(), [&hooksGroup, &hookName](const auto& kv) -> bool
        {
            return kv.first == hooksGroup && kv.second.first == hookName;
        });

        if(it != Hooks.end())
        {
            SafeCallLua(it->second.second, args...);
            //it->second.second(args...);
        }
    }

    bool IsTypeRegistered(size_t hash);
    bool IsTypeRegistered(const std::string& _name);

    void RegisterType(const std::string& _name, size_t hash);

    CScriptComponentsManager ComponentsManager;

    std::string Name;
    std::unordered_map<std::string, size_t> RegisteredTypes;

    CCallbackHandler<void, CScriptingEngine*> OnInit;
};
