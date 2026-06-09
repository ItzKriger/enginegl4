#include "ENT_Generic.h"

std::string ENT_Generic::GetType() const { return GetLuaKey(); }
void ENT_Generic::V_Init() { CallScriptedFunction("init", GetCurrentTable()); }
void ENT_Generic::V_PostInit() { CallScriptedFunction("postinit", GetCurrentTable()); }
void ENT_Generic::V_Update() { CallScriptedFunction("update", GetCurrentTable()); }

void ENT_Generic::FullPack(CBufferWrapper& packet) { CallScriptedFunction("fullpack", GetCurrentTable()); } //TODO table for CBufferWrapper
void ENT_Generic::FullUnpack(CBufferWrapper& packet) { CallScriptedFunction("fullunpack", GetCurrentTable()); } //TODO table for CBufferWrapper

sol::table ENT_Generic::GetCurrentTable()
{
    if(!State) { return sol::lua_nil; }
    return GetScriptTable(State);
}

LINK_SOL_USERTYPE(ENT_Generic);