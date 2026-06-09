#include "CGL430Material.h"
#include "CWindowManager.h"
#include "COpenGL430Renderer.h"
#include "CEngine.h"
#include "CScopeExit.h"
#include "CImage.h"
#include "CResourcesManager.h"

#include "U_String.h"
#include "U_Files.h"

#include <fstream>

namespace //local space (scope)
{
    COpenGL430Renderer* GetRenderer()
    {
        if(CEngine::GetInstance()->GetStopFlag()) { return nullptr; }

        CWindowManager* winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
        if(!winman) { return nullptr; }

        return dynamic_cast<COpenGL430Renderer*>(winman->Renderer.get());
    }
}

CGL430Material::CGL430Material()
{
    auto renderer = GetRenderer();
    if(renderer) { renderer->RegisterMaterial(this); }
}

CGL430Material::~CGL430Material()
{
    auto renderer = GetRenderer();
    if(renderer)
    {
        renderer->UnregisterMaterial(this);

        glMakeTextureHandleNonResidentARB(DiffuseTextureHandle);
        glDeleteTextures(1, &DiffuseTextureNativeHandle);
    }
}

bool CGL430Material::V_Load(const std::filesystem::path& path)
{
    std::shared_ptr<CImage> diffuseTexture;

    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto& time = CEngine::GetInstance()->Time;

    auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "materials", path.string());
    if(!found_path.has_value()) { return false; }
    
    std::ifstream objfile(found_path.value());
    if (!objfile.is_open()) { return false; }

    CScopeExit streamExiter([&objfile]() { objfile.close(); });

    std::string ReadLine;
    while (!objfile.eof())
    {
        std::getline(objfile, ReadLine, '\n');

        std::vector<std::string> splitted;
        StringUtils::split_str(ReadLine, ' ', splitted);

        std::string cmd = splitted[0];
        std::string param = StringUtils::merge_arg(splitted, 1, splitted.size() - 1);

        //Log::Instance() << "Parsed material property \"" << cmd << "\": \"" << param << "\"\n"; LOGLOG

        if(cmd == "texture")
        {
            //Log::Instance() << "Loading texture " << param << Log::Endl; LOGLOG

            auto res = resman->LoadResource("image", param);
            diffuseTexture = std::dynamic_pointer_cast<CImage>(res);

            CTimePoint now = CEngine::GetInstance()->Time.GetCurrent();
			CTimePoint expires = TIME_EXPR(now + std::chrono::seconds(5));

            resman->Cache.Add(res, expires);

            glGenTextures(1, &DiffuseTextureNativeHandle);
            glBindTexture(GL_TEXTURE_2D, DiffuseTextureNativeHandle);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, diffuseTexture->GetSize().x, diffuseTexture->GetSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, diffuseTexture->GetRawData());

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);

            DiffuseTextureHandle = glGetTextureHandleARB(DiffuseTextureNativeHandle);
            glMakeTextureHandleResidentARB(DiffuseTextureHandle);
        }
        else if(cmd == "diffuse")
        {
            DiffuseColor = StringUtils::FromStr<CColor>(param);
        }
        else if(cmd == "shininess")
        {
            Shininess = StringUtils::FromStr<float>(param);
        }
    }
    Log::Instance() << "Material done loading with handles " << DiffuseTextureHandle << " and " << DiffuseTextureNativeHandle << "\n";
    return true;
}

std::shared_ptr<CLoadingContext> CGL430Material::CreatePipelineContext(const std::filesystem::path& path)
{
    return std::make_shared<CGL430MaterialLoadingContext>(this, path);
}

void CGL430Material::V_SetupLoadPipeline(CLoadPipeline& pipeline)
{
    pipeline.push_back //load material data (async)
    (
        {
            [](std::shared_ptr<CLoadingContext> context) -> bool
            {
                auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
                auto& time = CEngine::GetInstance()->Time;

                auto matContext = std::dynamic_pointer_cast<CGL430MaterialLoadingContext>(context);
                auto thisMaterial = dynamic_cast<CGL430Material*>(context->CurrentResource);

                auto& path = context->LoadPath;

                auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "materials", path.string());
                if(!found_path.has_value()) { return false; }
                
                std::ifstream objfile(found_path.value());
                if (!objfile.is_open()) { return false; }

                CScopeExit streamExiter([&objfile]() { objfile.close(); });

                std::string ReadLine;
                while (!objfile.eof())
                {
                    std::getline(objfile, ReadLine, '\n');

                    std::vector<std::string> splitted;
                    StringUtils::split_str(ReadLine, ' ', splitted);

                    std::string cmd = splitted[0];
                    std::string param = StringUtils::merge_arg(splitted, 1, splitted.size() - 1);

                    //Log::Instance() << "Parsed material property \"" << cmd << "\": \"" << param << "\"\n"; //LOGLOG

                    if(cmd == "texture")
                    {
                        matContext->DiffusePath = param;
                        //Log::Instance() << "Loading texture " << param << Log::Endl; //LOGLOG

                        auto img = resman->GetOrCreate("image", param);
                        context->RequiredResources.push_back(img);

                        //img->Wait(); NO WAITING IN ASYNC THREAD
                    }
                    else if(cmd == "diffuse")
                    {
                        thisMaterial->DiffuseColor = StringUtils::FromStr<CColor>(param);
                    }
                    else if(cmd == "shininess")
                    {
                        thisMaterial->Shininess = StringUtils::FromStr<float>(param);
                    }
                }
                return true;
            },
            true //async (is_async)
        }
    );

    pipeline.push_back //upload image to gpu (sync)
    (
        {
            [](std::shared_ptr<CLoadingContext> context) -> bool
            {
                auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
                auto matContext = std::dynamic_pointer_cast<CGL430MaterialLoadingContext>(context);
                auto thisMaterial = dynamic_cast<CGL430Material*>(context->CurrentResource);

                std::shared_ptr<CResource> _res = nullptr;
                for(auto res : context->RequiredResources)
                {
                    //Log::Instance() << "Required res " << res->Name << " with status of " << (int)res->LoadingStatus << Log::Endl;
                    if(res->Name == matContext->DiffusePath.stem().string() && res->LoadingStatus == CResource::CLoadingStatus::Done)
                    {
                        _res = res;
                        break;
                    }
                }

                if(!_res)
                {
                    //Log::ErrInstance() << "NO RES FOR UPLOAD " << matContext->DiffusePath.stem().string() << Log::Endl;
                    return false;
                }
                auto diffuseTexture = std::dynamic_pointer_cast<CImage>(_res);

                //&thisMaterial->DiffuseTextureNativeHandle

                glGenTextures(1, &thisMaterial->DiffuseTextureNativeHandle);
                
                glBindTexture(GL_TEXTURE_2D, thisMaterial->DiffuseTextureNativeHandle);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, diffuseTexture->GetSize().x, diffuseTexture->GetSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, diffuseTexture->GetRawData());

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);

                thisMaterial->DiffuseTextureHandle = glGetTextureHandleARB(thisMaterial->DiffuseTextureNativeHandle);
                glMakeTextureHandleResidentARB(thisMaterial->DiffuseTextureHandle);

                //Log::Instance() << "Uploaded image to gpu\n"; //LOGLOG
                return true;
            },
            false //sync (is_async)
        }
    );
}

LINK_RESOURCE_TO_CLASS(CGL430Material, gl430material)