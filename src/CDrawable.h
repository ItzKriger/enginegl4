#pragma once
#include "CScriptObject.h"
#include "CCallbackHandler.h"

class CRendererBase;
class CDrawable : public CScriptObject
{
public:
    bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    void Update();
    virtual void V_Update();

    DEFINE_SOL_USERTYPE();
    CCallbackHandler<void, CDrawable*> OnUpdate;

    virtual void Draw(CRendererBase* renderer) = 0;
    virtual ~CDrawable() = default;
};
