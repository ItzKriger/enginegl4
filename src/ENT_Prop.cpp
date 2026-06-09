#include "ENT_Prop.h"
#include "CEngine.h"
#include "CPhysicalEntity.h"
#include "U_ShortAPI.h"
#include "CDrawableEntity.h"
#include "CScenesManager.h"

void ENT_Prop::V_Init()
{
    SetEntityModel(this);
    if(!Components.IsComponentPresent<CPhysicalEntity>())
    {
        Components.CreateComponent<CPhysicalEntity>(false);
    }

    auto drawable = Components.GetComponentTyped<CDrawableEntity>();
    if(drawable)
    {
        drawable->Init();
    }

    auto physical = Components.GetComponentTyped<CPhysicalEntity>();
    if(physical)
    {
        physical->Init();
    }
}

void ENT_Prop::FullPack(CBufferWrapper& packet)
{

}

void ENT_Prop::FullUnpack(CBufferWrapper& packet)
{

}

LINK_ENTITY_TO_CLASS(ENT_Prop, prop);