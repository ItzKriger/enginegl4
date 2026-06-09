#pragma once
#include "sol/sol.hpp"
#include <memory>
#include <unordered_map>

#define DEFINE_SOL_USERTYPE() sol::object V_GetScriptUserType(sol::state_view state) override;
#define LINK_SOL_USERTYPE(_class) sol::object _class::V_GetScriptUserType(sol::state_view state) { return sol::make_object(state, this); }

class CScriptObjectBase
{
public:
    CScriptObjectBase() = default;
    CScriptObjectBase(std::shared_ptr<sol::state> state);

    virtual ~CScriptObjectBase() = default;

    sol::table ScriptInit(std::shared_ptr<sol::state> state);
    sol::table ScriptInit(sol::state_view state);

    virtual bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table);
    virtual bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table);

    void ScriptTypeInit(std::shared_ptr<sol::state> state);
    void ScriptTypeInit(sol::state_view state);

    virtual void V_BaseScriptTypeInit(sol::state_view state);
    virtual void V_ScriptTypeInit(sol::state_view state);

    sol::object GetScriptUserType(sol::state_view state);
    virtual sol::object V_GetScriptUserType(sol::state_view state);

    sol::table GetScriptTable(sol::state_view _state);
    sol::table GetScriptTable(std::shared_ptr<sol::state> _state_ptr);

    bool IsScriptTableExist(sol::state_view _state) const;
    bool IsScriptTableExist(std::shared_ptr<sol::state> _state_ptr) const;

    bool DeleteScriptTable(sol::state_view _state);
    bool DeleteScriptTable(std::shared_ptr<sol::state> _state_ptr);

    std::unordered_map<std::shared_ptr<sol::state>, sol::table> Bindings;
    virtual bool m_internalInit(std::shared_ptr<sol::state> state, sol::table table);
};
