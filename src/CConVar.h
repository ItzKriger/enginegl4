#pragma once
#include "CWrapableBase.h"
#include "CScriptFieldsManager.h"
#include "CScriptObject.h"

#include <string>

class CConVar : public CScriptObject
{
public:
    CConVar() = default;
    CConVar(std::unique_ptr<CWrapableBase> _wrapable);

    std::unique_ptr<CWrapableBase> Wrapable;
    std::wstring Description;

    bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;
    CScriptFieldsManager ScriptFields;
};
