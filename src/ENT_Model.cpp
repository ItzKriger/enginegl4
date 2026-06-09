#include "ENT_Model.h"
#include "CEngine.h"
#include "CTransformable.h"
#include "CDrawableAnimatable.h"
#include "CDrawableEntity.h"
#include "CResourcesManager.h"
#include "CWindowManager.h"
#include "CScenesManager.h"
#include "U_ShortAPI.h"

void ENT_Model::V_Init()
{
    SetEntityModel(this);
}

void ENT_Model::FullPack(CBufferWrapper& packet)
{
    
}

void ENT_Model::FullUnpack(CBufferWrapper& packet)
{
    
}

LINK_ENTITY_TO_CLASS(ENT_Model, model);