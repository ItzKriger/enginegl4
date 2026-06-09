#include "CCamerasManager.h"

void CCamerasManager::AddCamera(std::shared_ptr<CCamera> camera)
{
    Cameras.push_back(camera);
}

void CCamerasManager::SetActiveCamera(std::shared_ptr<CCamera> camera)
{
    ActiveCamera = camera;
}

void CCamerasManager::Update()
{
    for(auto& camera : Cameras)
    {
        //camera->Update(); TODO
    }
}
