#include "CWorldsManager.h"
#include <limits>

worldid_t CWorldsManager::CreateWorld()
{
    worldid_t id = GetNextFreeID();
    Items.insert({ id, std::make_unique<CWorld>(id) });

    return id; //TODO maybe net event?
}

CWorld* CWorldsManager::GetWorld(worldid_t id)
{
    auto it = Items.find(id);
    return (Items.find(id) != Items.end() ? (*it).second.get() : nullptr);
}

void CWorldsManager::DeleteWorld(worldid_t id)
{
    auto it = Items.find(id);
    if(it != Items.end())
    {
        Items.erase(it);
    }
}

bool CWorldsManager::IsWorldExist(worldid_t id) const
{
    return Items.find(id) != Items.end();
}

size_t CWorldsManager::Count() const
{
    return Items.size();
}

worldid_t CWorldsManager::GetNextFreeID() const
{
    for(worldid_t i = 0; i < std::numeric_limits<worldid_t>::max(); i++)
    {
        if (Items.find(i) == Items.end())
        {
            return i;
        }
    }
    return std::numeric_limits<worldid_t>::max();
}

void CWorldsManager::Update()
{
    for(auto& kv : Items)
    {
        kv.second->Update();
    }
}
