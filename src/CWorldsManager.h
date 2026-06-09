#pragma once
#include "CWorld.h"
#include "CScriptObject.h"

#include <map>
#include <memory>
#include <unordered_map>

class CWorldsManager : public CScriptObject
{
public:
    worldid_t CreateWorld();
    CWorld* GetWorld(worldid_t id = 0);

    bool IsWorldExist(worldid_t id) const;
    size_t Count() const;

    void DeleteWorld(worldid_t id);
    worldid_t GetNextFreeID() const;

    void Update();

    std::unordered_map<worldid_t, std::unique_ptr<CWorld>> Items;
};
