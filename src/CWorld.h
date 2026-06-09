#pragma once
#include "CEntitiesManager.h"
#include "U_Typedefs.h"

class CWorld
{
public:
    CWorld() = delete;
    CWorld(worldid_t id);

    worldid_t GetUUID() const;
    void Update();

    CEntitiesManager Entities;
private:
    worldid_t UUID = 0;
};
