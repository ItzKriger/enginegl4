#include "CWorld.h"

CWorld::CWorld(worldid_t id) : UUID(id), Entities(id) {}

worldid_t CWorld::GetUUID() const
{
    return UUID;
}

void CWorld::Update()
{
    Entities.Trace([](CEntity* ent) { ent->Update(); });
}
