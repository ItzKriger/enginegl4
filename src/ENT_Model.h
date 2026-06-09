#pragma once
#include "CEntity.h"

class ENT_Model : public CEntity
{
public:
    void V_Init() override;

    void FullPack(CBufferWrapper& packet) override;
	void FullUnpack(CBufferWrapper& packet) override;

    DEFINE_ENTITY();
};
