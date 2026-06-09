#pragma once
#include "CEntityComponent.h"
#include "CDrawable.h"

class CDrawableEntity : public CEntityComponent
{
public:
    std::shared_ptr<CDrawable> Drawable;
    void V_Init() override;

    void FullPack(CBufferWrapper& packet) override;
	void FullUnpack(CBufferWrapper& packet) override;
    
    DEFINE_ENTITY_COMPONENT();
};
