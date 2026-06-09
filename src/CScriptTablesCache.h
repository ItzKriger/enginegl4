#pragma once
#include "sol/sol.hpp"
#include "U_Scripting.h"

#include <unordered_map>

#define TABLE_CACHE_INLINE(_class) _class(std::shared_ptr<sol::state>& s) : CScriptTablesCache(s) {}

#define TABLE_CACHE_DECLARE(_class) _class(std::shared_ptr<sol::state>& s);

#define TABLE_CACHE_DEFINE(_class) _class::_class(std::shared_ptr<sol::state>& s) : CScriptTablesCache(s) {}

template<typename Key, typename Value = sol::table>
class CScriptTablesCache
{
public:
    CScriptTablesCache(std::shared_ptr<sol::state>& s) : State(s) {}

    virtual ~CScriptTablesCache() {}

    Value Get(const Key& key)
    {
        auto it = Items.find(key);

        if(it == Items.end())
        {
            Value tabl = State->create_table();
            bool succ = m_Create(key, tabl);

            if(succ)
            {
                Items[key] = tabl;
            }
        }

        auto ret = Items.find(key);
        return (ret != Items.end()) ? ret->second : sol::lua_nil;
    }

    void Delete(const Key& key)
    {
        Items.erase(key);
    }

    bool IsCached(const Key& key)
    {
        return Items.find(key) != Items.end();
    }

    sol::object GatedDeleteOrGet(const Key& key, bool gates_opened)
    {
        if (gates_opened)
		{
            return Get(key);
        }
        else if(!gates_opened && IsCached(key))
        {
            Delete(key);
        }
        return sol::lua_nil;
    }

    virtual bool m_Create(const Key& key, Value tabl) = 0;

    std::unordered_map<Key, Value> Items;
    std::shared_ptr<sol::state>& State;
};
