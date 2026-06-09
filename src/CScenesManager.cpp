#include "CScenesManager.h"
#include "CWindowManager.h"
#include "CEngine.h"

bool CScenesManager::V_Init()
{
    auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
    if(!winman) { return true; } //TODO true or false?

    winman->OnDraw += [this]()
    {
        auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();

        if(!ActiveScene.expired() && ActiveScene.lock())
        {
            ActiveScene.lock()->Render(winman->Renderer.get());
        }
    };

    OnDrawCallback = winman->OnDraw.GetLastFunctionHandleShared();
    return true;
}

void CScenesManager::V_Update()
{
    for(auto& scene : Scenes)
    {
        scene->Update();
    }
}

void CScenesManager::V_DeInit()
{
    Scenes.clear();
}

void CScenesManager::AddScene(std::shared_ptr<CScene> scene)
{
    Scenes.push_back(scene);
}

int CScenesManager::GetSceneID(CScene* _scene) const
{
    auto it = std::find_if(Scenes.begin(), Scenes.end(), [_scene](auto& sc) -> bool { return sc.get() == _scene; });
    if(it != Scenes.end())
    {
        return std::distance(Scenes.begin(), it);
    }
    return -1;
}

void CScenesManager::SetActiveScene(std::shared_ptr<CScene> _scene) { ActiveScene = _scene; }
void CScenesManager::RenderActive(CRendererBase* renderer)
{
    if (!ActiveScene.expired() && ActiveScene.lock())
    {
        ActiveScene.lock()->Render(renderer);
    }
}

LINK_SOL_USERTYPE(CScenesManager);
LINK_COMPONENT_TO_CLASS(CScenesManager, scenesmanager)