#include "CScriptObjectBase.h"
#include "CEngine.h"
#include "U_Scripting.h"
#include "CScriptingManager.h"

CScriptObjectBase::CScriptObjectBase(std::shared_ptr<sol::state> state) { ScriptInit(state); }
bool CScriptObjectBase::m_internalInit(std::shared_ptr<sol::state> state, sol::table table) { return true; }
bool CScriptObjectBase::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) { return true; }

bool CScriptObjectBase::V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    return true;
}

sol::table CScriptObjectBase::ScriptInit(sol::state_view state)
{
    return ScriptInit(ScriptUtils::GetStateSharedPtr(state));
}

sol::table CScriptObjectBase::ScriptInit(std::shared_ptr<sol::state> state)
{
    sol::table Table = state->create_table();

    bool succ_base = V_BaseScriptInit(state, Table);
    if(!succ_base) { return sol::lua_nil; }

    bool succ = V_ScriptInit(state, Table);
    if(!succ) { return sol::lua_nil; }

    bool internal_succ = m_internalInit(state, Table);
    if(!internal_succ) { return sol::lua_nil; }

    Bindings.insert({ state, Table });
    return Table;
}

bool CScriptObjectBase::IsScriptTableExist(sol::state_view _state) const
{
    return std::find_if(Bindings.begin(), Bindings.end(), [&_state](auto& kv) -> bool { return (*kv.first) == _state; }) != Bindings.end();
}

bool CScriptObjectBase::IsScriptTableExist(std::shared_ptr<sol::state> _state_ptr) const
{
    return std::find_if(Bindings.begin(), Bindings.end(), [&_state_ptr](auto& kv) -> bool { return kv.first == _state_ptr; }) != Bindings.end();
}

sol::table CScriptObjectBase::GetScriptTable(sol::state_view _state)
{
    auto it = std::find_if(Bindings.begin(), Bindings.end(), [&_state](auto& kv) -> bool { return (*kv.first) == _state; });
    if(it == Bindings.end()) { return ScriptInit(ScriptUtils::GetStateSharedPtr(_state)); }
    return it->second;
}

sol::table CScriptObjectBase::GetScriptTable(std::shared_ptr<sol::state> _state_ptr)
{
    auto it = std::find_if(Bindings.begin(), Bindings.end(), [&_state_ptr](auto& kv) -> bool { return kv.first == _state_ptr; });
    if(it == Bindings.end()) { return ScriptInit(_state_ptr); }
    return it->second;
}

bool CScriptObjectBase::DeleteScriptTable(sol::state_view _state)
{
    auto it = std::find_if(Bindings.begin(), Bindings.end(), [&_state](auto& kv) -> bool { return (*kv.first) == _state; });
    if(it != Bindings.end()) { Bindings.erase(it); return true; }
    return false;
}

bool CScriptObjectBase::DeleteScriptTable(std::shared_ptr<sol::state> _state_ptr)
{
    auto it = std::find_if(Bindings.begin(), Bindings.end(), [&_state_ptr](auto& kv) -> bool { return kv.first == _state_ptr; });
    if(it != Bindings.end()) { Bindings.erase(it); return true; }
    return false;
}

void CScriptObjectBase::ScriptTypeInit(std::shared_ptr<sol::state> state)
{
    return ScriptTypeInit(*state);
}

void CScriptObjectBase::ScriptTypeInit(sol::state_view state)
{
    V_BaseScriptTypeInit(state);
    V_ScriptTypeInit(state);
}

sol::object CScriptObjectBase::GetScriptUserType(sol::state_view state)
{
    return V_GetScriptUserType(state);
}

sol::object CScriptObjectBase::V_GetScriptUserType(sol::state_view state)
{
    return sol::make_object(state, this);
}

void CScriptObjectBase::V_BaseScriptTypeInit(sol::state_view state) {}
void CScriptObjectBase::V_ScriptTypeInit(sol::state_view state) {}
