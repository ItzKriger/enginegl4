#pragma once
#include "CScriptTablesCache.h"

class CScriptComponentTablesCache : public CScriptTablesCache<std::string>
{
public:
    TABLE_CACHE_DECLARE(CScriptComponentTablesCache);
    bool m_Create(const std::string& key, sol::table tabl) override;
};
