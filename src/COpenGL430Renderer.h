#pragma once
#include "CRendererBase.h"
#include "GL/glew.h"
#include "CGL430Material.h"
#include "CGL430SSBO.h"

#include <vector>
#include <mutex>

#include "CDrawable.h"
#include "CShaderGL.h"

class COpenGL430Renderer : public virtual CRendererBase //TODO COpenGLRendererBase
{
public:
    COpenGL430Renderer();
    ~COpenGL430Renderer();

    void Init(SDL_Window* window) override;

    size_t GetGenericFlag() const override;
    void ClearWindow() override;
    void Display() override;

    void V_Update() override;

    std::string GetModelType() const override;
    std::string GetMaterialType() const override;
    std::shared_ptr<CDrawable> CreateAnimatedDrawable() const override;
    //std::shared_ptr<CDrawable> CreateNonAnimatedDrawable() const override;

    DEFINE_RENDERER();
    SDL_GLContext Context = nullptr;

    void RegisterMaterial(CGL430Material* material);
    void UnregisterMaterial(CGL430Material* material);
    void BindMaterialsSSBO(GLuint binding = 3);

    void SetupCamera(std::shared_ptr<CCamera> camera) override;
    void DrawModel(std::shared_ptr<CModelBase> model, const CTransform& transform) override;

    int GetMaterialIndex(CGL430Material* material);
    CShaderGL S_Default; //TODO temporary
private:
    /*struct alignas(16) CMaterialGPU
    {
        alignas(8) glm::uvec2 diffuse;
        alignas(8) glm::uvec2 specular;
        alignas(8) glm::uvec2 reflection_map;

        alignas(4) float shininess;
        alignas(4) int use_specular_map;
        alignas(4) float reflectivity;
        alignas(4) int reflection_type;
    };*/

    #pragma pack(push, 1)
    struct CMaterialGPU
    {
        glm::uvec2 diffuse;        //8
        glm::uvec2 specular;       //8
        glm::uvec2 reflection_map; //8
        float shininess;           //4
        int use_specular_map;      //4
        float reflectivity;        //4
        int reflection_type;       //4
    }; //8 + 8 + 8 + 4 + 4 + 4 + 4 = 40 bytes
    #pragma pack(pop)

    int GetFreeMaterialIndex();
    void ReuploadMaterial(CGL430Material* material);
    CMaterialGPU BuildMaterialForGPU(CGL430Material* material);

    SDL_Window* Window = nullptr; //don't destroy it in renderer, CWindowManager does that
    CGL430SSBO m_MaterialsSSBO;

    std::vector<CGL430Material*> m_Materials;
    std::vector<CGL430Material*> m_PendingMaterials;
    mutable std::mutex m_MaterialMutex;

    std::vector<CGL430Material*> m_materialsToUpload;
};
