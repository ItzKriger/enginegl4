#include "CScriptingManager.h"
#include "CEngine.h"

#include "CGenericComponent.h"
#include "CGenericConsole.h"
#include "CGenericResource.h"
#include "ENT_Generic.h"
#include "CLogger.h"

CScriptingManager::CScriptingManager()
{
    ScriptClassesFactory.SetFunction<CGenericComponent>(CEngine::GetInstance()->ComponentsFactory);
    ScriptClassesFactory.SetFunction<CGenericConsole>(CEngine::GetInstance()->ConsoleFactory);
    ScriptClassesFactory.SetFunction<ENT_Generic>(CEngine::GetInstance()->EntitiesFactory);
    ScriptClassesFactory.SetFunction<CGenericResource>(CEngine::GetInstance()->ResourcesFactory);
}

void CScriptingManager::AddScriptEngine(const std::string& name, std::unique_ptr<CScriptingEngine>&& eng)
{
    if(name.empty() || ScriptingEngines.find(name) != ScriptingEngines.end()) { throw std::runtime_error("Trying to add existing script engine or with empty name"); }
    ScriptingEngines.insert({ name, std::move(eng) });

    PushCurrentEngine(ScriptingEngines[name].get());

    ScriptingEngines[name]->Name = name;
    ScriptingEngines[name]->Init();

    PopCurrentEngine();
}

void CScriptingManager::DeleteScriptEngine(const std::string& name)
{
    auto it = ScriptingEngines.find(name);
    if(it == ScriptingEngines.end()) { return; }

    ScriptingEngines.erase(it);
}

bool CScriptingManager::IsScriptEngineExist(const std::string& name)
{
    return ScriptingEngines.find(name) != ScriptingEngines.end();
}

CScriptingEngine* CScriptingManager::GetEngine(const std::string& name)
{
    auto it = ScriptingEngines.find(name);
    return (it != ScriptingEngines.end()) ? it->second.get() : nullptr;
}

void CScriptingManager::Trace(const std::function<void(CScriptingEngine*)>& func)
{
    for(auto& kv : ScriptingEngines)
    {
        func(kv.second.get());
    }
}

sol::state& CScriptingManager::GetStateReference(lua_State* state)
{
    for(auto& kv : ScriptingEngines)
    {
        if(kv.second->State->lua_state() == state)
        {
            return *kv.second->State;
        }
    }
    throw std::runtime_error("No such state");
}

std::shared_ptr<sol::state> CScriptingManager::GetStateSharedPtr(lua_State* state)
{
    for(auto& kv : ScriptingEngines)
    {
        if(kv.second->State->lua_state() == state)
        {
            return kv.second->State;
        }
    }
    return nullptr;
    //throw std::runtime_error("No such state");
}

CScriptingEngine* CScriptingManager::GetEngine(sol::state_view state)
{
    for(auto& kv : ScriptingEngines)
    {
        if(kv.second->State->lua_state() == state.lua_state())
        {
            return kv.second.get();
        }
    }
    throw std::runtime_error("No such state");
}

CScriptingEngine* CScriptingManager::GetCurrentEngine()
{
    return m_processEngines.empty() ? nullptr : m_processEngines.top();
}

void CScriptingManager::PushCurrentEngine(CScriptingEngine* eng)
{
    if(!eng) { return; }
    m_processEngines.push(eng);
}

void CScriptingManager::PopCurrentEngine()
{
    if(m_processEngines.empty()) { return; }
    m_processEngines.pop();
}

bool CScriptingManager::IsScriptComponentRegistered(const std::string& type)
{
    for(auto& kv : ScriptingEngines)
    {
        if(kv.second->ComponentsManager.IsPresent(type))
        {
            return true;
        }
    }
    return false;
}

CGenericComponent* CScriptingManager::CreateScriptComponent(const std::string& type)
{
    for(auto& kv : ScriptingEngines)
    {
        if(kv.second->ComponentsManager.IsPresent(type))
        {
            return kv.second->ComponentsManager.Create(type);
        }
    }
    return nullptr;
}

bool CScriptingManager::RemoveRegisteredComponent(const std::string& type)
{
    for(auto& kv : ScriptingEngines)
    {
        if(kv.second->ComponentsManager.IsPresent(type))
        {
            kv.second->ComponentsManager.Remove(type);
            return true;
        }
    }
    return false;
}

void CScriptingManager::ResetEngineClasses(CScriptingEngine* engine)
{
    ScriptClassesFactory.ResetEngineClassesEverywhere(engine);
}

void CScriptingManager::V_DeInit()
{
    ScriptingEngines.clear();
}

LINK_COMPONENT_TO_CLASS(CScriptingManager, scripting);