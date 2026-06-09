#include "CRendererBase.h"
#include "CDrawableAnimatable.h"

std::string CRendererBase::GetType() const { return ""; }
std::string CRendererBase::GetModelType() const { return "model"; }
std::string CRendererBase::GetMaterialType() const { return "material"; }
std::shared_ptr<CDrawable> CRendererBase::CreateAnimatedDrawable() const { return std::make_shared<CDrawableAnimatable>(); }
std::shared_ptr<CDrawable> CRendererBase::CreateNonAnimatedDrawable() const { return std::make_shared<CDrawableModel>(); }

void CRendererBase::Update()
{
    V_Update();
}

void CRendererBase::SetupCamera(std::shared_ptr<CCamera> camera)
{

}

void CRendererBase::DrawModel(std::shared_ptr<CModelBase> model, const CTransform& transform)
{
    
}

void CRendererBase::V_Update() {}