#include "CScriptComponentsManager.h"

CScriptComponent::CScriptComponent(const std::string& _TypeName, sol::function _Constructor,
    sol::function _Destructor, sol::function _LuaInit, sol::function _LuaPostInit, sol::function _LuaUpdate) : 
    TypeName(_TypeName), Constructor(_Constructor), Destructor(_Destructor), Init(_LuaInit), PostInit(_LuaPostInit), Update(_LuaUpdate)
{
    
}

CGenericComponent* CScriptComponentsManager::Create(const std::string& typenm)
{
    if(!IsPresent(typenm)) { return nullptr; }

    auto& compInfo = Components.find(typenm)->second;

    std::map<std::string, sol::function> _functions = 
    {
        { "constructor", compInfo.Constructor },
        { "destructor", compInfo.Destructor },
        { "init", compInfo.Init },
        { "postinit", compInfo.PostInit },
        { "update", compInfo.Update },
    };

    return new CGenericComponent(typenm, _functions, State);
}

bool CScriptComponentsManager::IsPresent(const std::string& typenm) const
{
    return Components.find(typenm) != Components.end();
}

void CScriptComponentsManager::Register(const std::string& typenm, sol::function _Constructor, sol::function _Destructor,
                                        sol::function _LuaInit, sol::function _LuaPostInit, sol::function _LuaUpdate)
{
    if(typenm.empty() || IsPresent(typenm)) { return; }
    Components.insert({ typenm, { typenm, _Constructor, _Destructor, _LuaInit, _LuaPostInit, _LuaUpdate } });
}

void CScriptComponentsManager::Remove(const std::string& typenm)
{
    auto it = Components.find(typenm);
    if(it != Components.end())
    {
        Components.erase(it);
    }
}

CScriptComponentsManager::CScriptComponentsManager(std::shared_ptr<sol::state>& _State) : State(_State) {}