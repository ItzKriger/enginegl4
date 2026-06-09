#pragma once
#include "ILuaWrapableBase.h"
#include "CTypedWrapableBase.h"

#include "U_Scripting.h"

template<typename T>
class ILuaWrapable : public virtual ILuaWrapableBase, public virtual CTypedWrapableBase<T>
{
public:
    void SetFromLua(sol::object obj) override
    {
        CTypedWrapableBase<T>::SetValue(ScriptUtils::FromObject<T>(obj));
    }

    sol::object ConvertToLua(sol::state_view state) override
    {
        return ScriptUtils::ToObject(CTypedWrapableBase<T>::GetValue(), state);
    }
};
