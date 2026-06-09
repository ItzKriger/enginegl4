#pragma once
#include "CComponent.h"
#include "CScriptingEngine.h"
#include "CScriptClassesManager.h"
#include "CObjectFactory.h"

#include <unordered_map>
#include <functional>
#include <string>
#include <stack>

class CScriptingManager : public CComponent
{
public:
    CScriptingManager();

    template<typename... Args>
    void CallHook(const std::string& name, Args... args)
    {
        for(auto& kv : ScriptingEngines)
        {
            PushCurrentEngine(kv.second.get());
            kv.second->CallHook<Args...>(name, args...);
            PopCurrentEngine();
        }
    }

    template<typename... Args>
    void CallSingleHook(const std::string& name, const std::string& hookName, Args... args)
    {
        for(auto& kv : ScriptingEngines)
        {
            PushCurrentEngine(kv.second.get());
            kv.second->CallSingleHook<Args...>(name, hookName, args...);
            PopCurrentEngine();
        }
    }

    void AddScriptEngine(const std::string& name, std::unique_ptr<CScriptingEngine>&& eng);
    void DeleteScriptEngine(const std::string& name);
    bool IsScriptEngineExist(const std::string& name);

    bool IsScriptComponentRegistered(const std::string& type);
    bool RemoveRegisteredComponent(const std::string& type); //true - removed, false - not
    CGenericComponent* CreateScriptComponent(const std::string& type);

    void V_DeInit() override;
    void Trace(const std::function<void(CScriptingEngine*)>& func);

    sol::state& GetStateReference(lua_State* state);
    std::shared_ptr<sol::state> GetStateSharedPtr(lua_State* state);
    CScriptingEngine* GetEngine(sol::state_view state);
    CScriptingEngine* GetEngine(const std::string& name);

    CScriptingEngine* GetCurrentEngine();

    void PushCurrentEngine(CScriptingEngine* eng);
    void PopCurrentEngine();

    void ResetEngineClasses(CScriptingEngine* engine);

    CScriptClassesManager<std::string> ScriptClassesFactory;

    std::unordered_map<std::string, std::unique_ptr<CScriptingEngine>> ScriptingEngines;
    DEFINE_COMPONENT();
private:
    std::stack<CScriptingEngine*> m_processEngines;
};
