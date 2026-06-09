#pragma once
#include "sol/sol.hpp"

namespace ScriptStates
{
    sol::state& GetStateReference(lua_State* state);
    std::shared_ptr<sol::state> GetStateSharedPtr(lua_State* state);

    bool IsTypeRegistered(lua_State* state, size_t hash);
    bool IsTypeRegistered(lua_State* state, const std::string& _name);

    void RegisterType(lua_State* state, const std::string& _name, size_t hash);
}
