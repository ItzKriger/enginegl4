#include "CScriptConvarTablesCache.h"
#include "CConVarManager.h"
#include "CEngine.h"

bool CScriptConvarTablesCache::m_Create(const std::string& key, sol::table tabl)
{
    if(!CEngine::GetInstance()->Components.IsComponentPresent<CConVarManager>()) { return false; }

    auto man = CEngine::GetInstance()->Components.GetComponentTyped<CConVarManager>();
    if(man->IsConVarExist(key))
    {
        //man->GetRealConVar(key).ScriptInit(tabl); TODO unused
        return true;
    }
    return false;
}

TABLE_CACHE_DEFINE(CScriptConvarTablesCache);