#pragma once
#include "CWrapableBase.h"

#include "sol/sol.hpp"

class ILuaWrapableBase : public virtual CWrapableBase
{
public:
    virtual void SetFromLua(sol::object obj) = 0;
    virtual sol::object ConvertToLua(sol::state_view state) = 0;
};
