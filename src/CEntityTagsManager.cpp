#include "CEntityTagsManager.h"
#include "CEngine.h"

#include <stdexcept>

bool CEntityTagsManager::IsTagExist(const std::string& name)
{
    return Tags.find(name) != Tags.end();
}

CWrapableBase* CEntityTagsManager::GetTag(const std::string& name)
{
    auto it = Tags.find(name);
    return (it != Tags.end()) ? (*it).second.get() : nullptr;
}

std::unique_ptr<CWrapableBase>& CEntityTagsManager::AddTag(const std::string& name, CWrapableBase* ptr)
{
    if(!ptr || IsTagExist(name)) { throw std::runtime_error("Trying to add invalid or existing tag"); }
    Tags.insert({ name, std::unique_ptr<CWrapableBase>(ptr) });
    return Tags[name];
}

LINK_ENTITY_COMPONENT_TO_CLASS(CEntityTagsManager, tagsmanager);