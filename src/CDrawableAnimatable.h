#pragma once
#include "CDrawableModel.h"
#include "CAnimation.h"

class CDrawableAnimatable : public CDrawableModel
{
public:
    class CJoint
    {
    public:
        CJoint(const std::string& name = "", CJoint* parent = nullptr);

        std::string Name;
        CJoint* Parent = nullptr;

        glm::mat4x4 BindPose;

        CTransform Transform;
        glm::mat4x4 GetMatrix();

        glm::mat4x4 GlobalBind;
    };

    //TODO priority (+CPrioritiesManager class)
    //TODO splines (like animgraph?)
    struct CAnimationInstance 
    {
        float AnimationWeight = 1.0f;
        std::shared_ptr<CAnimation> Animation;

        float PlaybackTime = 0.0f;
        float PlaybackRate = 1.0f;

        bool OldInterp = false;
        InterpolationType InterpType = InterpolationType::Linear;
    };

    void AddAnimation(std::shared_ptr<CResource> _anim, float rate = 1.0f, float weight = 1.0f, float startTime = 0.0f);
    std::vector<CAnimationInstance> Animations;

    std::vector<CJoint> Joints;
    CDrawableAnimatable();

    DEFINE_SOL_USERTYPE();
};
