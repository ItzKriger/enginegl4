#pragma once
#include "CEntityComponent.h"
#include "CWrapableBase.h"

#include <map>
#include <unordered_map>
#include <string>
#include <memory>

class CEntityTagsManager : public CEntityComponent
{
public:
    std::unordered_map<std::string, std::unique_ptr<CWrapableBase>> Tags;

    bool IsTagExist(const std::string& name);
    CWrapableBase* GetTag(const std::string& name);

    std::unique_ptr<CWrapableBase>& AddTag(const std::string& name, CWrapableBase* ptr); //ptr then will be handled by unique_ptr inside CEntityTagsManager::Tags

    template<typename T>
    T GetTagValue(const std::string& name, T defval = T{})
    {
        CWrapableBase* tag = GetTag(name);
        if(!tag) { return defval; }

        return tag->GetCastValue<T>();
    }

    DEFINE_ENTITY_COMPONENT();
};
