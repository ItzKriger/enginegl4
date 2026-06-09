#include "CGenericConsole.h"
#include "U_Scripting.h"

void CGenericConsole::V_Init()
{
    CallScriptedFunction("init", GetCurrentTable());
}

void CGenericConsole::V_Update()
{
    CallScriptedFunction("update", GetCurrentTable());
}

void CGenericConsole::Print(const std::wstring& text)
{
    CallScriptedFunction("print", GetCurrentTable(), text);
}

void CGenericConsole::Error(const std::wstring& err)
{
    CallScriptedFunction("error", GetCurrentTable(), err);
}

void CGenericConsole::SetColor(const CColor& text, const CColor& bg)
{
    //TODO implement
}

std::pair<CColor, CColor> CGenericConsole::GetColor() const
{
    //TODO implement
    return { CColor::Black, CColor::White };
}

sol::table CGenericConsole::GetCurrentTable()
{
    if(!State) { return sol::lua_nil; }
    return GetScriptTable(State);
}

std::string CGenericConsole::GetType() const
{
    return GetLuaKey();
}
