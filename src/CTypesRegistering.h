#pragma once
#include "sol/sol.hpp"

#define SOL_COMPONENT_BASE sol::base_classes, sol::bases<CComponent>(), sol::meta_function::index, &userdata_index_func<CComponent>, sol::meta_function::new_index, &userdata_new_index_func<CComponent>
#define SOL_ENTITY_BASE sol::base_classes, sol::bases<CEntity>(), sol::meta_function::index, &entity_index_func, sol::meta_function::new_index, &userdata_new_index_func<CEntity>
#define SOL_ENTITY_COMPONENT_BASE sol::base_classes, sol::bases<CEntityComponent>()
#define SOL_RESOURCE_BASE sol::base_classes, sol::bases<CResource>()
#define HANDLER CCallbackHandlerBase&
#define HANDLER_CAST(_this) static_cast<CCallbackHandlerBase&>(_this)

template<typename T>
sol::object userdata_index_func(T& _obj, sol::object _key, sol::this_state ts)
{
    if(_key.valid() && _key.is<std::string>() && _key.as<std::string>() == "__meta")
    {
        if(!_obj.UserData.valid())
        {
            _obj.UserData = sol::state_view(ts).create_table();
        }

        sol::table __meta = _obj.UserData[sol::metatable_key];
        if(!__meta.valid())
        {
            __meta = sol::state_view(ts).create_table();
            _obj.UserData[sol::metatable_key] = __meta;
        }

        return __meta;
    }

    if(!_obj.UserData.valid())
    {
        return sol::lua_nil;
    }

    sol::object obj = _obj.UserData[_key];
    if(obj.valid())
    {
        return obj;
    }
    return sol::lua_nil;
}

template<typename T>
void userdata_new_index_func(T& _obj, sol::object _key, sol::object _value, sol::this_state ts)
{
    if(!_key.valid()) { return; }

    if(!_obj.UserData.valid())
    {
        _obj.UserData = sol::state_view(ts).create_table();
    }

    if(_key.is<std::string>() && _key.as<std::string>() == "__meta")
    {
        _obj.UserData[sol::metatable_key] = _value;
        return;
    }

    _obj.UserData[_key] = _value;
}

class CEntity;
sol::object entity_index_func(CEntity& _obj, sol::object _key, sol::this_state ts);