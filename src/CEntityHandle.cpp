#include "CEntityHandle.h"
#include "CGame.h"
#include "CEngine.h"
#include "CEntity.h"

CEntityHandle::CEntityHandle(const CEntity* ent)
{
    if(!ent) { return; }

    KnownType = ent->GetType();
    KnownSerial = ent->GetSerialNumber();
    KnownUUID = ent->GetUUID();
    KnownWorldID = ent->GetWorldID();

    m_isInitted = true;
}

bool CEntityHandle::operator==(CEntity* ent) const
{
    if(!m_isInitted) { return false; }
    return (ent->GetSerialNumber() == KnownSerial && ent->GetUUID() == KnownUUID && ent->GetWorldID() == KnownWorldID && ent->GetType() == KnownType);
}

bool CEntityHandle::operator==(CEntityHandle hndl) const
{
    if(!m_isInitted) { return false; }
    return (hndl.KnownSerial == KnownSerial && hndl.KnownUUID == KnownUUID && hndl.KnownWorldID == KnownWorldID && hndl.KnownType == KnownType);
}

bool CEntityHandle::IsValid() const
{
    if(!m_isInitted) { return false; }
    return m_GetEntity();
}

CEntity* CEntityHandle::m_GetEntity() const
{
    if(!m_isInitted) { return nullptr; }

    bool game_exist = CEngine::GetInstance()->Components.IsComponentPresent<CGame>();
    if(!game_exist) { return nullptr; }

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    size_t worlds_count = game->Worlds.Count();

    if(!worlds_count) { return nullptr; }
    auto world = game->Worlds.GetWorld(KnownWorldID);

    if(!world) { return nullptr; }

    CEntity* foundEntity = nullptr;
    world->Entities.Trace([this, &foundEntity](CEntity* ent)
    {
        if((*this) == ent)
        {
            foundEntity = ent;
        }
    });

    return foundEntity;
}

CEntity* CEntityHandle::Get() const
{
    return m_GetEntity();
}
