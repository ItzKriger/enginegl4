#pragma once
#include "CEntityComponent.h"
#include "CNetMessage.h"
#include "CTransform.h"
#include "CEntityHandle.h"

class CNetTransformChanged : public CNetMessage
{
public:
    CTransformBase::CMeasurePack Pack;

    CEntityHandle Entity;
    bool ChangePosition = true;
    bool ChangeRotation = true;
    bool ChangeScale = true;

    CTimePoint RemoteChanged;

    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;
    void Process() override;

    DEFINE_NET_MESSAGE();
};

class CTransformable : public CEntityComponent
{
public:
    void V_Init() override;
    void V_Update() override;

    void FullPack(CBufferWrapper& packet) override;
	void FullUnpack(CBufferWrapper& packet) override;

    CTransform Transform;
    std::vector<std::pair<CTimePoint, CTransformBase::CMeasurePack>> InterpQueue;
    std::vector<std::pair<CTimePoint, CTransformBase::CMeasurePack>> DumpQueue;

    CTimePoint LastDump;

    DEFINE_SOL_USERTYPE();
    bool m_remoteTransformable = false;

    DEFINE_ENTITY_COMPONENT();
};
