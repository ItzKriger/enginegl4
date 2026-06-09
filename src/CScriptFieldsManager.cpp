#include "CScriptFieldsManager.h"

CFunctionalScriptField::CFunctionalScriptField(GetterFunc getter, SetterFunc setter, bool readonly) : CScriptFieldBase(readonly)
{
    Getter = std::make_unique<GetterFunc>(getter);
    Setter = std::make_unique<SetterFunc>(setter);
}

CFunctionalScriptField::CFunctionalScriptField(GetterFunc getter) : CScriptFieldBase(true)
{
    Getter = std::make_unique<GetterFunc>(getter);
}

void CFunctionalScriptField::SetObject(sol::object obj)
{
    if(Setter)
    {
        (*Setter)(obj);
    }
};

sol::object CFunctionalScriptField::GetObject(sol::state_view s)
{
    if(Getter)
    {
        return (*Getter)(s);
    }
    return sol::lua_nil;
}
