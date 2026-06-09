#include "CModelBase.h"
#include "CEngine.h"
#include "CResourcesManager.h"
#include "CWindowManager.h"
#include "U_Log.h"

CModelLoadingContext::CModelLoadingContext(CResource* _current_resource, const std::filesystem::path& _path) : CLoadingContext(_current_resource, _path) { }

CModelBase::~CModelBase() {}
void CModelBase::V_Load(const CModelLoadInfo& loadinfo) {}
void CModelBase::V_Draw() {}

bool CModelBase::V_Load(const std::filesystem::path& path)
{
    CModelLoadInfo mdl;
    mdl.Load(path);

    V_Load(mdl);
    return true; //TODO temporary
}

std::shared_ptr<CLoadingContext> CModelBase::CreatePipelineContext(const std::filesystem::path& path)
{
    return std::make_shared<CModelLoadingContext>(this, path);
}

void CModelBase::V_SetupLoadPipeline(CLoadPipeline& pipeline)
{
    pipeline.push_back //load model info (async)
    (
        {
            [](std::shared_ptr<CLoadingContext> context) -> bool
            {
                auto mdl_context = std::dynamic_pointer_cast<CModelLoadingContext>(context);
                mdl_context->LoadInfo.Load(mdl_context->LoadPath);

                auto thisResource = dynamic_cast<CModelBase*>(context->CurrentResource);

                auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
                auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();

                if(winman)
                {
                    auto& _renderer = winman->Renderer;

                    if(_renderer)
                    {
                        for(auto& matName : mdl_context->LoadInfo.MaterialsList)
                        {
                            auto _material = resman->GetOrCreate(_renderer->GetMaterialType(), matName + ".emtl");
                            context->RequiredResources.push_back(_material);
                        }
                    }
                }
                
                thisResource->BonesData.swap(mdl_context->LoadInfo.Bones);
                //mdl_context->LoadInfo.Bones.swap(thisResource->BonesData);
                //thisResource->BonesData = std::move(mdl_context->LoadInfo.Bones);

                return true;
            },
            true //async (is_async)
        }
    );
}
