#pragma once
#include "CModelBase.h"
#include "GL/glew.h"
#include "CGL430Material.h"

#include <vector>
#include <future>

#include "glm/glm.hpp"

class CGL430ModelLoadingContext : public CModelLoadingContext
{
public:
    CGL430ModelLoadingContext(CResource* _current_resource = nullptr, const std::filesystem::path& _path = {});
    struct PackedVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
        glm::ivec4 BoneIds = { 0, 0, 0, 0 };
        glm::vec4 BoneWeights = { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    std::vector<PackedVertex> vertexBuffer;
    std::vector<unsigned int> indexBuffer;
    size_t indexCount = 0;
    std::vector<int> materialPerTriangle;
};

class CGL430Model : public CModelBase
{
public:
    CGL430Model();
    ~CGL430Model();

    std::shared_ptr<CLoadingContext> CreatePipelineContext(const std::filesystem::path& path) override;
    void V_Load(const CModelLoadInfo& loadinfo) override;
    void V_Draw() override;
    void Unload();

    GLuint vao = 0, vbo = 0, ebo = 0, materialSSBO = 0, bonesSSBO = 0;
    size_t indexCount = 0;

    DEFINE_RESOURCE();
    std::vector<std::shared_ptr<CGL430Material>> UsedMaterials;
private:
    int LoadMaterial(const std::string& name);
};
