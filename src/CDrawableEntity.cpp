#include "CDrawableEntity.h"
#include "CEngine.h"
#include "CTransformable.h"
#include "CDrawable3D.h"
#include "CDrawableModel.h"
#include "CScenesManager.h"
#include "U_ShortAPI.h"

void CDrawableEntity::V_Init()
{
    auto entity = GetEntity();
    auto transformable = entity->Components.GetComponentTyped<CTransformable>();
    if(!transformable)
    {
        entity->Components.CreateComponent<CTransformable>();
        transformable = entity->Components.GetComponentTyped<CTransformable>();
    }

    Log::Instance() << "CDrawableEntity::V_Init()\n";
    transformable->Transform.OnTransformChanged += [this](CTransformBase::CMeasurePack oldprs, CTransformBase* _this)
    {
        auto drawable3d = std::dynamic_pointer_cast<CDrawable3D>(Drawable);
        //Log::Instance() << "transformable->Transform.OnTransformChanged\n";
        if(drawable3d)
        {
            //Log::Instance() << "drawable3d->Transform.SetPRS(_this->GetPRS());\n";
            drawable3d->Transform.SetPRS(_this->GetPRS());
        }
    };
}

void CDrawableEntity::FullPack(CBufferWrapper& packet) //let client to decide in what space to put this drawable
{
    auto drmodel = dynamic_cast<CDrawableModel*>(Drawable.get());
    packet.WriteLenString<std::uint8_t, std::string>(drmodel->GetModel()->Name);
    //packet.Write<std::uint16_t>(SpaceID);
}

void CDrawableEntity::FullUnpack(CBufferWrapper& packet)
{
    std::string mdlName = packet.ReadLenString<std::uint8_t, std::string>();
    SetEntityDrawable(GetEntity(), mdlName + ".emdl"); //TODO good workaround with adding an extension. Try to avoid using it

    AddDrawableToSpace(Drawable, 0); //TODO find an actual space (or multiple) for drawable
}

LINK_ENTITY_COMPONENT_TO_CLASS(CDrawableEntity, drawable);