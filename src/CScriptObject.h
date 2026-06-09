#pragma once
#include "CScriptObjectBase.h"
#include "CCallbackHandler.h"

//bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override
//bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override

class CScriptObject : public CScriptObjectBase
{
public:
    bool m_internalInit(std::shared_ptr<sol::state> state, sol::table table) override;
    CCallbackHandler<bool, std::shared_ptr<sol::state>, sol::table> OnScriptInit;
};
