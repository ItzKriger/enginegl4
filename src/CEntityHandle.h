#pragma once
#include "U_Typedefs.h"

#include <string>

class CEntity;
class CEntityHandle
{
public:
    CEntityHandle() = default;
    CEntityHandle(const CEntity* ent);

    CEntity* Get() const;
    bool IsValid() const;

    bool operator==(CEntity* ent) const;
    bool operator==(CEntityHandle hndl) const;
private:
    std::string KnownType;
    serial_t KnownSerial = 0;
    entityid_t KnownUUID = 0;
    worldid_t KnownWorldID = 0;

    bool m_isInitted = false;
    CEntity* m_GetEntity() const;
};
