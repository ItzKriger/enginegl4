#pragma once
#include "CEntityComponent.h"
#include "CPhysicsEngine.h"

class CPhysicalEntity : public CEntityComponent
{
public:
    void V_Init() override;
    CWorldUnit* WorldUnit = nullptr;

    void FullPack(CBufferWrapper& packet) override;
	void FullUnpack(CBufferWrapper& packet) override;

    DEFINE_SOL_USERTYPE();
    DEFINE_ENTITY_COMPONENT();
};
