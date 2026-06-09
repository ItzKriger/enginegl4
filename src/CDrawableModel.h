#pragma once
#include "CDrawable3D.h"
#include "CModelBase.h"
#include "CCallbackHandler.h"

class CDrawableModel : public CDrawable3D
{
public:
    void SetModel(std::shared_ptr<CResource> model);
    std::shared_ptr<CModelBase> GetModel();

    void Draw(CRendererBase* renderer) override;

    CCallbackHandler<void, CDrawableModel*, CModelBase*> OnSetModel;

    CCallbackHandler<void, CDrawableModel*, CModelBase*> OnPreDraw;
    CCallbackHandler<void, CDrawableModel*, CModelBase*> OnPostDraw;
private:
    std::shared_ptr<CModelBase> Model;
};
