#pragma once
#include "CResource.h"
#include "CSpline.h"

#include <string>
#include <unordered_map>
#include "glm/glm.hpp"

class CAnimation : public CResource
{
public:
    class CBoneCapture
    {
    public:
        std::string Name;
        glm::mat4 Matrix;
    };

    class CFrame
    {
    public:
        std::vector<std::string> Events;
        std::vector<CBoneCapture> Bones;
    };

    struct CLerped
    {
    public:
        CSpline<float, glm::vec3> PositionSpline;
        CSpline<float, glm::quat> RotationSpline;
        CSpline<float, glm::vec3> ScaleSpline;
    };

    double FPS = 30.0;
    std::vector<CFrame> Frames;

    std::unordered_map<std::string, CLerped> Splines;

    void V_SetupLoadPipeline(CLoadPipeline& pipeline) override;
    DEFINE_RESOURCE();
};
