#include "CDrawableModel.h"
#include "CRendererBase.h"

void CDrawableModel::SetModel(std::shared_ptr<CResource> model)
{
    if(!model) { return; } //nullptr

    auto mdl = std::dynamic_pointer_cast<CModelBase>(model);
    if(!mdl) { return; } //not a model!

    Model = mdl;
    OnSetModel(this, mdl.get());
}

std::shared_ptr<CModelBase> CDrawableModel::GetModel()
{
    return Model;
}

void CDrawableModel::Draw(CRendererBase* renderer)
{
    if(!Model || !Model->IsReady())
    {
        return; //not ready!
    }

    OnPreDraw(this, Model.get());
    renderer->DrawModel(Model, this->Transform);
    OnPostDraw(this, Model.get());
}
