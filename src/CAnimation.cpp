#include "CAnimation.h"
#include "CEngine.h"
#include "CBinaryFile.h"
#include "U_Log.h"
#include "U_Files.h"

void CAnimation::V_SetupLoadPipeline(CLoadPipeline& pipeline)
{
    pipeline.push_back
    (
        {
            [](std::shared_ptr<CLoadingContext> context) -> bool
            {
                auto _this = dynamic_cast<CAnimation*>(context->CurrentResource);
                if(!_this) { Log::ErrInstance() << "CAnimation loading got nullptr _this!\n"; return false; }

                auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "anim", context->LoadPath.string());
                if(!found_path.has_value()) { return false; }

                CBinaryFile file(found_path.value());
                if(!file.IsOpen()) { return false; }

                auto FramesCount = file.Read<std::uint32_t>();
                _this->FPS = file.Read<double>();

                _this->Frames.reserve(FramesCount);

                for(std::uint32_t i = 0; i < FramesCount; i++)
                {
                    CFrame Frame;

                    auto EventsCount = file.Read<std::uint8_t>();
                    auto BonesCount = file.Read<std::uint16_t>();

                    if(EventsCount) { Frame.Events.reserve(EventsCount); }
                    Frame.Bones.reserve(BonesCount);

                    for(std::uint8_t j = 0; j < EventsCount; j++)
                    {
                        auto event = file.ReadLenString<std::uint8_t, std::string>();
                        if(!event.empty())
                        {
                            Frame.Events.push_back(event);
                        }
                    }

                    for(std::uint16_t j = 0; j < BonesCount; j++)
                    {
                        CBoneCapture bone;

                        bone.Name = file.ReadLenString<std::uint8_t, std::string>();
                        if(bone.Name.empty()) { Log::ErrInstance() << "Empty bone name!\n"; }

                        bone.Matrix = file.Read<glm::mat4>();
                        Frame.Bones.push_back(std::move(bone));
                    }

                    _this->Frames.push_back(std::move(Frame));
                }

                float frameTime = 1.0 / _this->FPS;
                float currentFrameTime = 0.0f;

                for(auto& frame : _this->Frames)
                {
                    for(auto& bone : frame.Bones)
                    {
                        auto it = _this->Splines.find(bone.Name);
                        if(it == _this->Splines.end())
                        {
                            auto it2 = _this->Splines.emplace(bone.Name, CLerped());
                            it = it2.first;
                        }

                        CTransform transform;
                        transform.SetFromMatrix(bone.Matrix);

                        it->second.PositionSpline.AddNode(currentFrameTime, transform.GetPosition());
                        it->second.RotationSpline.AddNode(currentFrameTime, transform.GetRotation());
                        it->second.ScaleSpline.AddNode(currentFrameTime, transform.GetScale());
                    }
                    currentFrameTime += frameTime;
                }
                return true;
            },
            true //async (is_async)
        }
    );
}

LINK_RESOURCE_TO_CLASS(CAnimation, animation);