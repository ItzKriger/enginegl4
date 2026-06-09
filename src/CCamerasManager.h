#pragma once
#include <memory>
#include <vector>

#include "CCamera.h"

class CCamerasManager
{
public:
    std::vector<std::shared_ptr<CCamera>> Cameras;
    std::weak_ptr<CCamera> ActiveCamera;

    void AddCamera(std::shared_ptr<CCamera> camera);
    void SetActiveCamera(std::shared_ptr<CCamera> camera);
    void Update();
};
