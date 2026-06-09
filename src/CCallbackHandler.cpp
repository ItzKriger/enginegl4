#include "CCallbackHandler.h"

CCallbackHandlerBase::~CCallbackHandlerBase()
{
    for (auto callback : callbacks)
    {
        callback->Invalidate();
    }
}

void CCallbackHandlerBase::RegisterCallback(CSmartCallback* callback) { callbacks.push_back(callback); }
void CCallbackHandlerBase::UnregisterCallback(CSmartCallback* callback) { std::erase(callbacks, callback); }

CSmartCallback::CSmartCallback(CCallbackHandlerBase& _handler, void* _func)  : Handler(_handler), FunctionPointer(_func)
{
    Handler.RegisterCallback(this);
}

void CSmartCallback::m_InitiateDeletion()
{
    if (FunctionPointer)
    {
        Handler.m_deleteFunction(FunctionPointer);
        Handler.UnregisterCallback(this);
    }
}

CSmartCallback::~CSmartCallback()
{
    m_InitiateDeletion();
}

void CSmartCallback::ForceDelete()
{
    m_InitiateDeletion();
}

void CSmartCallback::Invalidate() { FunctionPointer = nullptr; }
bool CSmartCallback::IsValid() const { return FunctionPointer; }

bool CSmartCallback::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    table.set_function("isValid", [this]() -> bool { return IsValid(); });
    table.set_function("invalidate", [this]() -> void { return Invalidate(); });
    table.set_function("forceDelete", [this]() -> void { return ForceDelete(); });

    return true;
}

LINK_SOL_USERTYPE(CCallbackHandlerBase);