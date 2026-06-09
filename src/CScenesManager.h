#pragma once
#include <vector>

#include "CComponent.h"
#include "CScene.h"

class CScenesManager : public CComponent
{
public:
    std::vector<std::shared_ptr<CScene>> Scenes;
    std::weak_ptr<CScene> ActiveScene;

    bool V_Init() override;
    void V_DeInit() override;
    void V_Update() override;

    void AddScene(std::shared_ptr<CScene> scene);

    void SetActiveScene(std::shared_ptr<CScene> _scene);
    void RenderActive(CRendererBase* renderer);

    DEFINE_SOL_USERTYPE();
    int GetSceneID(CScene* _scene) const;

    CSmartCallbackPtr OnDrawCallback;

    DEFINE_COMPONENT();
};
