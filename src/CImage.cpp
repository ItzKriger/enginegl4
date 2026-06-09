#include "CImage.h"
#include "CEngine.h"

#include "U_Files.h"

#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

CImage::~CImage()
{
    if(m_Data)
    {
        stbi_image_free(m_Data);
        m_Data = nullptr;
    }
}

bool CImage::V_Load(const std::filesystem::path& path)
{
    auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "textures", path.string());
    if(!found_path.has_value()) { return false; }

    int width, height, channels;
    unsigned char* data = stbi_load(found_path.value().string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data)
    {
        return false;
    }

    m_Data = data;
    m_Width = width;
    m_Height = height;
    m_Channels = channels;

    return true;
}

glm::ivec2 CImage::GetSize() const
{
    return { m_Width, m_Height };
}

int CImage::GetChannels() const
{
    return m_Channels;
}

unsigned char* CImage::GetRawData()
{
    return m_Data;
}

void CImage::V_SetupLoadPipeline(CLoadPipeline& pipeline)
{
    pipeline.push_back //load image (async)
    (
        {
            [](std::shared_ptr<CLoadingContext> context) -> bool
            {
                auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "textures", context->LoadPath.string());
                if(!found_path.has_value()) { return false; }

                int width, height, channels;
                unsigned char* data = stbi_load(found_path.value().string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
                if (!data)
                {
                    return false;
                }

                CImage* img = dynamic_cast<CImage*>(context->CurrentResource);

                img->m_Data = data;
                img->m_Width = width;
                img->m_Height = height;
                img->m_Channels = channels;

                //Log::Instance() << "CImage async pipeline has finished work!\n"; LOGLOG
                return true;
            },
            true //async (is_async)
        }
    );
}

LINK_RESOURCE_TO_CLASS(CImage, image);