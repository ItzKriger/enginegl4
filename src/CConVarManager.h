#pragma once
#include "CComponent.h"
#include "CNode.h"
#include "CWrapableBase.h"
#include "CConVar.h"
#include <memory>
#include <shared_mutex>

class CConVarManager : public CComponent //TODO cvar descriptions TODO const
{
public:
    using CVar = CWrapableBase;
    using CVarRawPtr = CVar*;
    using CVarPtr = std::unique_ptr<CVar>;

    using RealCVar = CConVar;
    using RealCVarRawPtr = RealCVar*;
    using RealCVarPtr = std::unique_ptr<RealCVar>;

    using CVarNode = CNode<RealCVar>;

    bool V_Init() override;

    bool IsConVarExist(const std::string& path);
    CVarRawPtr GetConVar(const std::string& path);
    bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    CVarNode* AddConVar(const std::string& path, CVarRawPtr ptr); //ptr then will be handled by unique_ptr inside CConVarManager::Root
    bool DeleteConVar(const std::string& path); //true - deleted, false - error occured

    CVarNode* GetRealConVarNode(const std::string& path);
    RealCVar& GetRealConVar(const std::string& path);

    template<typename T>
    T GetConVarValue(const std::string& path, T defval = T{})
    {
        //std::shared_lock guard(m_Mutex); //TODO is required?

        CVarRawPtr cvar = GetConVar(path);
        if(!cvar) { return defval; }

        return cvar->GetCastValue<T>();
    }

    template<typename T>
    bool SetConVarValue(const std::string& path, const T& _val)
    {
        //std::shared_lock guard(m_Mutex); //TODO is required?

        CVarRawPtr cvar = GetConVar(path);
        if(!cvar) { return false; }

        cvar->SetCastValue<T>(_val);
        return true;
    }

    CVarNode Root;

    DEFINE_SOL_USERTYPE();
    DEFINE_COMPONENT();
private:
    std::shared_mutex m_Mutex;
};
