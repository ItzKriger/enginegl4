#include "CGenericComponent.h"
#include "CEngine.h"
#include "CScriptingManager.h"

#include "U_Log.h"

//GetCurrentTable
//GetScriptTable

CGenericComponent::~CGenericComponent()
{
}

bool CGenericComponent::V_Init()
{
    sol::object ret = CallScriptedFunction("init", GetCurrentUserType());
    return (ret.valid() && ret.is<bool>()) ? ret.as<bool>() : true;
}

void CGenericComponent::V_PostInit()
{
    CallScriptedFunction("postinit", GetCurrentUserType());
}

void CGenericComponent::V_DeInit()
{
    CallScriptedFunction("deinit", GetCurrentUserType());
}

void CGenericComponent::V_Update()
{
    CallScriptedFunction("update", GetCurrentUserType());
}

sol::table CGenericComponent::GetCurrentTable()
{
    if(!State) { return sol::lua_nil; }
    return GetScriptTable(State);
}

sol::object CGenericComponent::GetCurrentUserType()
{
    if(!State) { return sol::lua_nil; }
    return GetScriptUserType(*State);
}

std::string CGenericComponent::GetType() const
{
    return GetLuaKey();
}

LINK_SOL_USERTYPE(CGenericComponent);