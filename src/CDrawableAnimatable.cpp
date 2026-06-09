#include "CDrawableAnimatable.h"
#include "U_Log.h"
#include "CEngine.h"

#include "glm/glm.hpp"
#include "glm/gtx/compatibility.hpp"

CDrawableAnimatable::CJoint::CJoint(const std::string& name, CDrawableAnimatable::CJoint* parent) : Name(name), Parent(parent) {}

//BONEMATRIX calculating transform pose here
glm::mat4x4 CDrawableAnimatable::CJoint::GetMatrix()
{
    glm::mat4x4 GlobalMatrix = glm::mat4x4();
    auto LocalMatrix = BindPose * Transform.GetModelMatrix();

    if(Parent)
    {
        GlobalMatrix = Parent->GetMatrix() * LocalMatrix;
    }
    else
    {
        GlobalMatrix = LocalMatrix;
    }
    return GlobalMatrix;
}

CDrawableAnimatable::CDrawableAnimatable()
{
    OnSetModel += [this](CDrawableModel* _this, CModelBase* _model)
    {
        auto setup = [this, _model]()
        {
            Joints.reserve(_model->BonesData.size());
            for(auto& bone : _model->BonesData)
            {
                //BONEMATRIX applying bone params here
                Joints.push_back({ bone.Name, nullptr });
                auto& joint = Joints.back();

                joint.BindPose = bone.BindTransform;
                joint.GlobalBind = bone.GlobalBind;

                if(bone.ParentIndex > -1)
                {
                    for(auto& parent_joint : Joints)
                    {
                        if(parent_joint.Name == bone.ParentName)
                        {
                            joint.Parent = &parent_joint;
                        }
                    }
                }

                //joint.CalculateBind();
            }
        };

        if(_model->IsReady())
        {
            setup();
        }
        else
        {
            _model->OnDoneLoading += [this, _model, setup](CResource* __this)
            {
                setup();
            };
        }
    };

    OnUpdate += [this](CDrawable* _this)
    {
        for(auto& anim : Animations)
        {
            if(!anim.Animation || !anim.Animation->IsReady()) { continue; }

            float curPlayback = anim.PlaybackTime;
            anim.PlaybackTime += CEngine::GetInstance()->Time.GetDeltaTime() * anim.PlaybackRate;

            //75 frames
            //at 30 fps
            //2.5
            //75 / 30 = 2.5 - max time

            float maxAnimTime = anim.Animation->Frames.size() / anim.Animation->FPS;
            if(anim.PlaybackTime > maxAnimTime) { anim.PlaybackTime -= maxAnimTime; }

            //time: 2.5
            //FPS: 30.0
            //time * FPS = 2.5 * 30.0 = 75

            //2.51 * 30.0 = 75.3
            //2.52 * 30.0 = 75.6
            //2.53 * 30.0 = 75.9
            //2.533 * 30.0 = 75.99
            //2.54 * 30.0 = 76.2

            if(!anim.OldInterp)
            {
                for(auto& kv : anim.Animation->Splines)
                {
                    CJoint* joint = nullptr;
                    for(auto& j : Joints)
                    {
                        if(j.Name == kv.first)
                        {
                            joint = &j;
                            break;
                        }
                    }

                    if(!joint) { continue; }

                    auto lpos = kv.second.PositionSpline.GetValue(anim.PlaybackTime, anim.InterpType);
                    auto lrot = kv.second.RotationSpline.GetValue(anim.PlaybackTime, anim.InterpType);
                    auto lscl = kv.second.ScaleSpline.GetValue(anim.PlaybackTime, anim.InterpType);

                    CTransformBase::CMeasurePack LerpMeasurePack;

                    LerpMeasurePack.Position = lpos;
                    LerpMeasurePack.Rotation = lrot;
                    LerpMeasurePack.Scale = lscl;

                    joint->Transform.SetPRS(LerpMeasurePack);
                }
                continue;
            }

            size_t FrameIndex = floorf(curPlayback * anim.Animation->FPS);
            size_t NextFrameIndex = FrameIndex + 1;

            if(FrameIndex >= anim.Animation->Frames.size()) { FrameIndex = 0; }

            //75 / 30.0 = 2.5
            //76 / 30.0 = 2.5(33)
            //90 / 30.0 = 3

            float FrameTime = FrameIndex / anim.Animation->FPS;
            float NextFrameTime = NextFrameIndex / anim.Animation->FPS;

            //curtime 2.51
            //2.5 - 75
            //2.533 - 76
            //2.533 - 2.5 = 0.033 (delta)
            //2.51 - 2.5 = 0.01 (delta curtime)

            //curtime 2.5165
            //2.5 - 75
            //2.533 - 76
            //2.533 - 2.5 = 0.33 (delta)
            //2.5165 - 2.5 = 0.0165 (delta curtime)

            float LerpFactor = (curPlayback - FrameTime) / (NextFrameTime - FrameTime);
            LerpFactor = glm::clamp(LerpFactor, 0.0f, 1.0f);

            if(NextFrameIndex >= anim.Animation->Frames.size())
            {
                NextFrameIndex = 0;
            }

            auto& Frame = anim.Animation->Frames[FrameIndex];
            auto& NextFrame = anim.Animation->Frames[NextFrameIndex];

            for(auto& capture : Frame.Bones) //TODO splines
            {
                CAnimation::CBoneCapture* nextCapture = nullptr;
                for(auto& next_capture : NextFrame.Bones)
                {
                    if(capture.Name == next_capture.Name)
                    {
                        nextCapture = &next_capture;
                        break;
                    }
                }

                if(!nextCapture) { continue; }
                if(nextCapture->Matrix == capture.Matrix) { continue; }

                CTransform Transform, NextTransform;

                Transform.SetFromMatrix(capture.Matrix);
                NextTransform.SetFromMatrix(nextCapture->Matrix);

                CTransformBase::CMeasurePack MeasurePack, NextMeasurePack, LerpMeasurePack;

                MeasurePack = Transform.GetPRS();
                NextMeasurePack = NextTransform.GetPRS();

                LerpMeasurePack.Position = glm::lerp(MeasurePack.Position, NextMeasurePack.Position, LerpFactor);
                LerpMeasurePack.Rotation = glm::slerp(MeasurePack.Rotation, NextMeasurePack.Rotation, LerpFactor);
                LerpMeasurePack.Scale = glm::lerp(MeasurePack.Scale, NextMeasurePack.Scale, LerpFactor);

                CJoint* joint = nullptr;
                for(auto& j : Joints)
                {
                    if(j.Name == capture.Name)
                    {
                        joint = &j;
                        break;
                    }
                }

                if(!joint) { continue; }
                joint->Transform.SetPRS(LerpMeasurePack);
            }
        }
    };
}

void CDrawableAnimatable::AddAnimation(std::shared_ptr<CResource> _anim, float rate, float weight, float startTime)
{
    auto __anim = std::dynamic_pointer_cast<CAnimation>(_anim);
    if(!__anim) { return; } //not an animation!

    CAnimationInstance instance;

    instance.Animation = __anim;
    instance.AnimationWeight = weight;
    instance.PlaybackRate = rate;
    instance.PlaybackTime = startTime;

    Animations.push_back(std::move(instance));
}

LINK_SOL_USERTYPE(CDrawableAnimatable);