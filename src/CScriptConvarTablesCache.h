#pragma once
#include "CScriptTablesCache.h"

class CScriptConvarTablesCache : public CScriptTablesCache<std::string>
{
public:
    TABLE_CACHE_DECLARE(CScriptConvarTablesCache);
    bool m_Create(const std::string& key, sol::table tabl) override;
};
