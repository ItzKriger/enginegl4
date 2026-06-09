#pragma once
#include "CEntity.h"

class ENT_Prop : public CEntity
{
public:
    void V_Init() override;

    void FullPack(CBufferWrapper& packet) override;
	void FullUnpack(CBufferWrapper& packet) override;

    //std::string KnownModel, KnownCollision;
    //std::uint16_t SceneID = 0, SpaceID = 0, WorldID = 0;

    DEFINE_ENTITY();
};
