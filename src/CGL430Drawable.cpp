#include "CGL430Drawable.h"

namespace
{
    //BONEMATRIX calculating final matrix here
    glm::mat4 GetJointMatrix(CDrawableAnimatable::CJoint* joint)
    {
        return joint->GetMatrix() * glm::inverse(joint->GlobalBind);
    }
}

CGL430Drawable::CGL430Drawable()
{
    OnSetModel += [this](CDrawableModel* _this, CModelBase* _model)
    {
        auto setup = [this, _model]()
        {
            if (bonesSSBO) //TODO RAII
            {
                glDeleteBuffers(1, &bonesSSBO);
                bonesSSBO = 0;
            }

            constexpr size_t bonesCount_size = sizeof(std::int32_t);
            constexpr size_t std430padding = 16;
            constexpr size_t paddingSize = std430padding - bonesCount_size;
            constexpr size_t headerSize = bonesCount_size + paddingSize;

            constexpr size_t singleMatrixSize = sizeof(glm::mat4x4);

            bones_data.resize(headerSize + (singleMatrixSize * Joints.size()));
            std::int32_t* bonesCount = reinterpret_cast<std::int32_t*>(&bones_data[0]);

            (*bonesCount) = Joints.size();

            for(size_t i = 0; i < Joints.size(); i++)
            {
                size_t offset = headerSize + (singleMatrixSize * i);
                auto& joint = Joints.at(i);

                bones_offsets.emplace(&joint.Transform, offset);
                trans_joint.emplace(&joint.Transform, &joint);

                auto matrix = GetJointMatrix(&joint);
                glm::mat4x4* matrix_ptr = reinterpret_cast<glm::mat4x4*>(&bones_data[offset]);

                (*matrix_ptr) = matrix;
                Joints.at(i).Transform.OnTransformChanged += [this](CTransformBase::CMeasurePack old_mp, CTransformBase* _this)
                {
                    size_t offset = bones_offsets[_this];
                    CJoint* joint = trans_joint[_this];

                    glm::mat4x4* matrix_ptr = reinterpret_cast<glm::mat4x4*>(&bones_data[offset]);

                    auto matrix = GetJointMatrix(joint);

                    (*matrix_ptr) = matrix;
                    m_needToUpdateBones = true;
                };
            }

            glGenBuffers(1, &bonesSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesSSBO);
            glBufferData(GL_SHADER_STORAGE_BUFFER, bones_data.size(), bones_data.data(), GL_STREAM_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bonesSSBO); //TODO change binding
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            m_readyToDraw = true;
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

    OnPreDraw += [this](CDrawableModel* _this, CModelBase* _model)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bonesSSBO);
    };

    OnUpdate += [this](CDrawable* _this)
    {
        if(m_needToUpdateBones)
        {
            constexpr size_t bonesCount_size = sizeof(std::int32_t);
            constexpr size_t std430padding = 16;
            constexpr size_t paddingSize = std430padding - bonesCount_size;
            constexpr size_t headerSize = bonesCount_size + paddingSize;

            constexpr size_t singleMatrixSize = sizeof(glm::mat4x4);

            for(size_t i = 0; i < Joints.size(); i++)
            {
                size_t offset = headerSize + (singleMatrixSize * i);
                auto& joint = Joints.at(i);

                auto matrix = GetJointMatrix(&joint);
                glm::mat4x4* matrix_ptr = reinterpret_cast<glm::mat4x4*>(&bones_data[offset]);

                (*matrix_ptr) = matrix;
            }

            ReuploadBonesData();
            m_needToUpdateBones = false;
        }
    };
}

void CGL430Drawable::ReuploadBonesData()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bones_data.size(), bones_data.data(), GL_STREAM_DRAW); //TODO subdata?
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

CGL430Drawable::~CGL430Drawable()
{
    if (bonesSSBO) //TODO RAII
    {
        glDeleteBuffers(1, &bonesSSBO);
        bonesSSBO = 0;
    }
}
