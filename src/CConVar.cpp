#include "CConVar.h"
#include "U_Scripting.h"
#include "ILuaWrapableBase.h"

CConVar::CConVar(std::unique_ptr<CWrapableBase> _wrapable) : Wrapable(std::move(_wrapable)) {}

bool CConVar::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    sol::state_view sv = table.lua_state();
    
    ScriptFields.AddField("description", std::make_unique<CProxyScriptField<std::wstring>>(Description));
    
    ScriptFields.AddField("value", std::make_unique<CFunctionalScriptField>(
    [this](sol::state_view sv) -> sol::object
    {
        ILuaWrapableBase* lua = dynamic_cast<ILuaWrapableBase*>(Wrapable.get());
        if(!lua) { return sol::lua_nil; }

        return lua->ConvertToLua(sv);
    },
    [this](sol::object obj) -> void
    {
        ILuaWrapableBase* lua = dynamic_cast<ILuaWrapableBase*>(Wrapable.get());
        if(!lua || !obj.valid()) { return; }

        lua->SetFromLua(obj);
    }));

    sol::table wrapable = Wrapable->GetScriptTable(state);
    table["wrapable"] = wrapable;

    sol::table meta = sv.create_table();

    ScriptFields.SetMetaFunctions(meta);
    table[sol::metatable_key] = meta;

    return true;
}
