#pragma once
#include <vector>
#include "CDrawable.h"

#include "CCallbackHandler.h"
#include "CScriptObject.h"

class CBaseSpace : public CScriptObject
{
public:
    std::vector<std::shared_ptr<CDrawable>> Drawables;

    bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    void Update();
    virtual void V_Update();

    CCallbackHandler<void, CBaseSpace*> OnUpdate;

    void AddDrawable(std::shared_ptr<CDrawable> dr);
    virtual void Render(CRendererBase* renderer);
};
