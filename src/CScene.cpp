#include "CScene.h"
#include "CRendererBase.h"

void CScene::Update()
{
    SpacesManager.Update();
    CamerasManager.Update();
}

void CScene::Render(CRendererBase* renderer)
{
    renderer->SetupCamera(CamerasManager.ActiveCamera.lock());
    SpacesManager.Render(renderer);
}
