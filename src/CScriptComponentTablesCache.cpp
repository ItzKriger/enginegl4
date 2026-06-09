#include "CScriptComponentTablesCache.h"
#include "CEngine.h"

bool CScriptComponentTablesCache::m_Create(const std::string& key, sol::table tabl)
{
    if(CEngine::GetInstance()->Components.IsComponentPresent(key))
    {
        auto& comp = CEngine::GetInstance()->Components.GetComponent(key);
        //comp->ScriptInit(tabl, *State); TODO unused
        return true;
    }
    return false;
}

TABLE_CACHE_DEFINE(CScriptComponentTablesCache);