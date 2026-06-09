#pragma once
#include "sol/sol.hpp"
#include "CGenericComponent.h"

#include <map>
#include <unordered_map>

class CScriptComponent
{
public:
    CScriptComponent(const std::string& _TypeName, sol::function _Constructor, sol::function _Destructor,
        sol::function _LuaInit, sol::function _LuaPostInit, sol::function _LuaUpdate);

    sol::function Constructor, Destructor, Init, PostInit, Update;
    std::string TypeName;
};

class CScriptComponentsManager
{
public:
    CScriptComponentsManager() = delete;
    CScriptComponentsManager(std::shared_ptr<sol::state>& _State);

    CGenericComponent* Create(const std::string& typenm);
    bool IsPresent(const std::string& typenm) const;

    void Register(const std::string& typenm, sol::function _Constructor, sol::function _Destructor,
        sol::function _LuaInit, sol::function _LuaPostInit, sol::function _LuaUpdate);

    void Remove(const std::string& typenm);

    std::shared_ptr<sol::state>& State;
    std::unordered_map<std::string, CScriptComponent> Components;
};
