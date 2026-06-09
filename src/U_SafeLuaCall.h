#pragma once
#include "sol/sol.hpp"
#include <sstream>

void OutError(const std::wstring& err);
void OutError(const std::string& err);

template <typename... Args>
bool SafeCallLua(const sol::function& func, Args&&... args)
{
    sol::protected_function pf(func.lua_state(), func);
    sol::protected_function_result res = pf(std::forward<Args>(args)...);
    if (!res.valid())
    {
        sol::error err = res;
        std::string err_msg = err.what();
        
        std::stringstream ss;
        ss << "Lua error: " << err_msg << '\n';
        OutError(ss.str());

        return false;
    }
    return true;
}

template <typename... Args>
sol::object SafeCallLuaReturn(const sol::function& func, Args&&... args)
{
    sol::protected_function pf(func.lua_state(), func);
    sol::protected_function_result res = pf(std::forward<Args>(args)...);

    if (!res.valid())
    {
        sol::error err = res;
        std::string err_msg = err.what();
        
        std::stringstream ss;
        ss << "Lua error: " << err_msg << '\n';
        OutError(ss.str());

        return sol::make_object(func.lua_state(), sol::nil);
    }

    return sol::object(res);
}
