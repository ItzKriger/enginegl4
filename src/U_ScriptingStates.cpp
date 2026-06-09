#include "U_ScriptingStates.h"
#include "CEngine.h"
#include "CScriptingManager.h"

sol::state& ScriptStates::GetStateReference(lua_State* state)
{
    return CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetStateReference(state);
}

std::shared_ptr<sol::state> ScriptStates::GetStateSharedPtr(lua_State* state)
{
    return CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetStateSharedPtr(state);
}

bool ScriptStates::IsTypeRegistered(lua_State* state, size_t hash)
{
    return CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetEngine(state)->IsTypeRegistered(hash);
}

bool ScriptStates::IsTypeRegistered(lua_State* state, const std::string& _name)
{
    return CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetEngine(state)->IsTypeRegistered(_name);
}

void ScriptStates::RegisterType(lua_State* state, const std::string& _name, size_t hash)
{
    Log::Instance() << "Registered lua usertype " << _name << Log::Endl;
    return CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetEngine(state)->RegisterType(_name, hash);
}
