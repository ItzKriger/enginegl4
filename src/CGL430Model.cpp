#include "CGL430Model.h"
#include "CEngine.h"
#include "CWindowManager.h"
#include "CResourcesManager.h"
#include "COpenGL430Renderer.h"
#include "CConVarManager.h"

CGL430ModelLoadingContext::CGL430ModelLoadingContext(CResource* _current_resource, const std::filesystem::path& _path) : CModelLoadingContext(_current_resource, _path) { }

CGL430Model::CGL430Model()
{
    auto& callback = GetLoadPipelineSetupCallback();
    if(callback.IsEmpty())
    {
        callback += [](CLoadPipeline& pipeline)
        {
            pipeline.push_back //build model data for gpu (async)
            (
                {
                    [](std::shared_ptr<CLoadingContext> context) -> bool
                    {
                        auto mdlContext = std::dynamic_pointer_cast<CGL430ModelLoadingContext>(context);
                        auto thisResource = dynamic_cast<CGL430Model*>(context->CurrentResource);

                        std::map<std::tuple<uint64_t, uint64_t, uint64_t, int>, unsigned int> vertexMap;
                        unsigned int currentIndex = 0;

                        for (const auto& tri : mdlContext->LoadInfo.Triangles)
                        {
                            for (int i = 0; i < 3; i++)
                            {
                                uint64_t vIdx = tri.VertexIndex[i];
                                uint64_t nIdx = tri.NormalIndex[i];
                                uint64_t tIdx = tri.UvIndex[i];

                                auto key = std::make_tuple(vIdx, nIdx, tIdx, tri.MaterialIndex);
                                auto it = vertexMap.find(key);

                                if (it == vertexMap.end())
                                {
                                    const auto& v = mdlContext->LoadInfo.Vertices[vIdx];
                                    CGL430ModelLoadingContext::PackedVertex pv;
                                    pv.Position = v.Position;
                                    pv.Normal = (nIdx < mdlContext->LoadInfo.Normals.size()) ? mdlContext->LoadInfo.Normals[nIdx] : glm::vec3(0);
                                    pv.TexCoord = (tIdx < mdlContext->LoadInfo.UvCoords.size()) ? mdlContext->LoadInfo.UvCoords[tIdx] : glm::vec2(0);

                                    int count = std::min<int>(v.Bones.size(), 4);
                                    for (int j = 0; j < count; j++)
                                    {
                                        pv.BoneIds[j] = v.Bones[j].BoneIndex;
                                        pv.BoneWeights[j] = v.Bones[j].Weight;
                                    }

                                    mdlContext->vertexBuffer.push_back(pv);
                                    mdlContext->indexBuffer.push_back(currentIndex);
                                    vertexMap[key] = currentIndex++;
                                }
                                else
                                {
                                    mdlContext->indexBuffer.push_back(it->second);
                                }
                            }
                        }

                        mdlContext->indexCount = mdlContext->indexBuffer.size();
                        thisResource->indexCount = mdlContext->indexCount;

                        auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
                        auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
                        auto& _renderer = winman->Renderer;
                        auto renderer = dynamic_cast<COpenGL430Renderer*>(_renderer.get());

                        for(auto& res : context->RequiredResources)
                        {
                            if(res && res->GetType() == renderer->GetMaterialType())
                            {
                                //res->Wait(); NO WAITING IN ASYNC THREAD
                                auto material = std::dynamic_pointer_cast<CGL430Material>(res);
                                thisResource->UsedMaterials.push_back(material);
                            }
                        }

                        mdlContext->materialPerTriangle.reserve(mdlContext->LoadInfo.Triangles.size());
                        for (const auto& tri : mdlContext->LoadInfo.Triangles)
                        {
                            std::string matName = mdlContext->LoadInfo.MaterialsList.at(tri.MaterialIndex);
                            auto it = std::find_if(thisResource->UsedMaterials.begin(), thisResource->UsedMaterials.end(), [&matName](auto mat) -> bool
                            {
                                return mat->Name == matName;
                            });

                            int id = renderer->GetMaterialIndex(it->get());
                            mdlContext->materialPerTriangle.push_back(id);
                        }
                        return true;
                    },
                    true //async (is_async)
                }
            );

            pipeline.push_back //upload model data to gpu (sync)
            (
                {
                    [](std::shared_ptr<CLoadingContext> context) -> bool
                    {
                        auto mdlContext = std::dynamic_pointer_cast<CGL430ModelLoadingContext>(context);
                        auto thisResource = dynamic_cast<CGL430Model*>(context->CurrentResource);

                        using PackedVertex = CGL430ModelLoadingContext::PackedVertex;

                        glGenVertexArrays(1, &thisResource->vao);
                        glBindVertexArray(thisResource->vao);

                        glGenBuffers(1, &thisResource->vbo);
                        glBindBuffer(GL_ARRAY_BUFFER, thisResource->vbo);
                        glBufferData(GL_ARRAY_BUFFER, mdlContext->vertexBuffer.size() * sizeof(PackedVertex), mdlContext->vertexBuffer.data(), GL_STATIC_DRAW);

                        glGenBuffers(1, &thisResource->ebo);
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, thisResource->ebo);
                        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mdlContext->indexBuffer.size() * sizeof(unsigned int), mdlContext->indexBuffer.data(), GL_STATIC_DRAW);

                        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), (void*)offsetof(PackedVertex, Position));
                        glEnableVertexAttribArray(0);
                        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), (void*)offsetof(PackedVertex, Normal));
                        glEnableVertexAttribArray(1);
                        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), (void*)offsetof(PackedVertex, TexCoord));
                        glEnableVertexAttribArray(2);
                        glVertexAttribIPointer(3, 4, GL_INT, sizeof(PackedVertex), (void*)offsetof(PackedVertex, BoneIds));
                        glEnableVertexAttribArray(3);
                        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), (void*)offsetof(PackedVertex, BoneWeights));
                        glEnableVertexAttribArray(4);
                        
                        glBindVertexArray(0);

                        glGenBuffers(1, &thisResource->materialSSBO);
                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, thisResource->materialSSBO);
                        glBufferData(GL_SHADER_STORAGE_BUFFER, mdlContext->materialPerTriangle.size() * sizeof(int), mdlContext->materialPerTriangle.data(), GL_STATIC_DRAW);
                        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, thisResource->materialSSBO); //TODO change binding
                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
                        return true;
                    },
                    false //sync (is_async)
                }
            );
        };
    }
}

int CGL430Model::LoadMaterial(const std::string& name)
{
    std::string materialName = std::filesystem::path(name).stem().string();

    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
    auto& _renderer = winman->Renderer;
    auto renderer = dynamic_cast<COpenGL430Renderer*>(_renderer.get());

    auto it = std::find_if(UsedMaterials.begin(), UsedMaterials.end(), [&materialName](auto mat) -> bool
    {
        return mat->Name == materialName;
    });

    if(it != UsedMaterials.end()) { return renderer->GetMaterialIndex(it->get()); }

    auto _material = resman->GetOrCreate(renderer->GetMaterialType(), name);

    if(!_material)
    {
        Log::ErrInstance() << "Can't load material " << name << "\n";
    }

    auto material = std::dynamic_pointer_cast<CGL430Material>(_material);
    UsedMaterials.push_back(material);

    int ret = renderer->GetMaterialIndex(material.get());
    return ret;
}

void CGL430Model::V_Load(const CModelLoadInfo& loadinfo)
{
    if(!CEngine::GetInstance()->Components.IsComponentPresent<CWindowManager>()) { return; }

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

    std::map<std::tuple<uint64_t, uint64_t, uint64_t, int>, unsigned int> vertexMap;
    unsigned int currentIndex = 0;

    for (const auto& tri : loadinfo.Triangles)
    {
        for (int i = 0; i < 3; i++)
        {
            uint64_t vIdx = tri.VertexIndex[i];
            uint64_t nIdx = tri.NormalIndex[i];
            uint64_t tIdx = tri.UvIndex[i];

            auto key = std::make_tuple(vIdx, nIdx, tIdx, tri.MaterialIndex);
            auto it = vertexMap.find(key);

            if (it == vertexMap.end())
            {
                const auto& v = loadinfo.Vertices[vIdx];
                PackedVertex pv;
                pv.Position = v.Position;
                pv.Normal = (nIdx < loadinfo.Normals.size()) ? loadinfo.Normals[nIdx] : glm::vec3(0);
                pv.TexCoord = (tIdx < loadinfo.UvCoords.size()) ? loadinfo.UvCoords[tIdx] : glm::vec2(0);

                int count = std::min<int>(v.Bones.size(), 4);
                for (int j = 0; j < count; j++)
                {
                    pv.BoneIds[j] = v.Bones[j].BoneIndex;
                    pv.BoneWeights[j] = v.Bones[j].Weight;
                }

                vertexBuffer.push_back(pv);
                indexBuffer.push_back(currentIndex);
                vertexMap[key] = currentIndex++;
            }
            else
            {
                indexBuffer.push_back(it->second);
            }
        }
    }

    indexCount = indexBuffer.size();

    Log::Instance() << "Model indexCount is " << indexCount << "\n";

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(PackedVertex), vertexBuffer.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.size() * sizeof(unsigned int), indexBuffer.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), (void*)offsetof(PackedVertex, Position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), (void*)offsetof(PackedVertex, Normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), (void*)offsetof(PackedVertex, TexCoord));
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(PackedVertex), (void*)offsetof(PackedVertex, BoneIds));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), (void*)offsetof(PackedVertex, BoneWeights));
    glEnableVertexAttribArray(4);
    
    glBindVertexArray(0);

    std::vector<int> materialPerTriangle;
    materialPerTriangle.reserve(loadinfo.Triangles.size());

    for (const auto& tri : loadinfo.Triangles)
    {
        int id = LoadMaterial(loadinfo.MaterialsList.at(tri.MaterialIndex) + ".emtl");
        materialPerTriangle.push_back(id);
    }

    glGenBuffers(1, &materialSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, materialPerTriangle.size() * sizeof(int), materialPerTriangle.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, materialSSBO); //TODO change binding
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

std::shared_ptr<CLoadingContext> CGL430Model::CreatePipelineContext(const std::filesystem::path& path)
{
    return std::make_shared<CGL430ModelLoadingContext>(this, path);
}

void CGL430Model::V_Draw()
{
    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
    auto& _renderer = winman->Renderer;
    auto renderer = dynamic_cast<COpenGL430Renderer*>(_renderer.get());

    //renderer->S_Default.Bind(); //TODO unnecessary binding

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, materialSSBO);
    renderer->BindMaterialsSSBO();

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    //Log::Instance() << "drawn\n";
}

void CGL430Model::Unload() //TODO RAII
{
    if (ebo)
    {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }

    if (vbo)
    {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }

    if (vao)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }

    if (materialSSBO)
    {
        glDeleteBuffers(1, &materialSSBO);
        materialSSBO = 0;
    }

    if (bonesSSBO)
    {
        glDeleteBuffers(1, &bonesSSBO);
        bonesSSBO = 0;
    }

    indexCount = 0;
}

CGL430Model::~CGL430Model()
{
    Unload();
}

LINK_RESOURCE_TO_CLASS(CGL430Model, gl430model);