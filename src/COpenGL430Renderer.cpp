#include "COpenGL430Renderer.h"
#include "CEngine.h"
#include "CLogger.h"
#include "CWindowManager.h"

#include "U_General.h"
#include "U_Log.h"
#include "U_Files.h"

#include "CGL430Drawable.h"

#include "CConVarManager.h"
#include "CWrapable.h"
#include <boost/stacktrace.hpp>

COpenGL430Renderer::COpenGL430Renderer()
{

}

COpenGL430Renderer::~COpenGL430Renderer()
{
    if(Context)
    {
        SDL_GL_DestroyContext(Context);
        Context = nullptr;
    }
}

void GLAPIENTRY debugMessageCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam
)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) { return; }

    auto& log = std::cout;

    log << "OpenGL Debug Message:\n";
    log << "  Source: ";
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             log << "API";                                         break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   log << "Window System";                               break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: log << "Shader Compiler";                             break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     log << "Third Party";                                 break;
        case GL_DEBUG_SOURCE_APPLICATION:     log << "Application";                                 break;
        case GL_DEBUG_SOURCE_OTHER:           log << "Other";                                       break;
        default:                              log << "Unknown (0x" << std::hex << source << ")";    break;
    }
    log << "\n";

    log << "  Type: ";
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               log << "Error";                                     break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: log << "Deprecated Behavior";                       break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  log << "Undefined Behavior";                        break;
        case GL_DEBUG_TYPE_PORTABILITY:         log << "Portability";                               break;
        case GL_DEBUG_TYPE_PERFORMANCE:         log << "Performance";                               break;
        case GL_DEBUG_TYPE_MARKER:              log << "Marker";                                    break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          log << "Push Group";                                break;
        case GL_DEBUG_TYPE_POP_GROUP:           log << "Pop Group";                                 break;
        case GL_DEBUG_TYPE_OTHER:               log << "Other";                                     break;
        default:                                log << "Unknown (0x" << std::hex << type << ")";    break;
    }
    log << "\n";

    log << "  Severity: ";
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:            log << "High";                                      break;
        case GL_DEBUG_SEVERITY_MEDIUM:          log << "Medium";                                    break;
        case GL_DEBUG_SEVERITY_LOW:             log << "Low";                                       break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:    log << "Notification";                              break;
        default:                                log << "Unknown(0x" << std::hex << severity << ")"; break;
    }
    log << "\n";

    log << "  ID: 0x" << std::hex << id << "\n";
    log << "  Message: " << message << "\n\n";

    //std::cout << "stacktrace:\n";
    //std::cout << boost::stacktrace::stacktrace();
    //std::cout << std::endl;
}

void COpenGL430Renderer::BindMaterialsSSBO(GLuint binding)
{
    m_MaterialsSSBO.Bind();
}

void COpenGL430Renderer::Init(SDL_Window* window)
{
    COMPONENT_CALL(CConVarManager, AddConVar("forcereupload", new CWrapable<bool>(false)));

    Log::Instance() << "COpenGL430Renderer::Init\n";
    Window = window;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    Context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(Window, Context);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDL_GL_SetSwapInterval(0); //TODO toggle vsync ability

    glewExperimental = true;
    if (glewInit() != GLEW_OK) { COMPONENT_CALL(CLogger, Err(L"Can't initialize GLEW. Rendering will not be possible.")); }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugMessageCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    GLint flags; 
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        std::cout << "Debug Context is active\n";
    }
    else
    {
        std::cout << "Debug Context is NOT active\n";
    }

    //TODO this entire block code below is temporary
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_TEXTURE_2D);

    glClearColor(0.0f, 0.1f, 0.1f, 0.0f);
    glClearDepth(1.0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    //glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendEquation(GL_FUNC_ADD);

    glm::ivec2 winsize;
    COMPONENT_CALL_GET(winsize, CWindowManager, GetWindowSize());

    glViewport(0, 0, winsize.x, winsize.y);

    auto file_vs = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "shaders", "default.vs");
    auto file_fs = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "shaders", "default.fs");

    Log::Instance() << "Vertex shader is \"" << file_vs.value().string() << "\"\n";
    Log::Instance() << "Fragment shader is \"" << file_fs.value().string() << "\"\n";

    if(file_vs.has_value() && file_fs.has_value()) {  S_Default.LoadFromFile(file_vs.value().string(), file_fs.value().string()); }
    else { Log::ErrInstance() << "Renderer couldn't load default shaders\n"; }

    //S_Default.LoadFromFile("default.vs", "default.fs"); //TODO hardcoded and no filesystem paths
    //S_Default.LoadFromFile("resources\\shaders\\default.vs", "resources\\shaders\\default.fs");

    S_Default.Bind();

    constexpr size_t MaterialsAmount = 2048;
    m_Materials.resize(MaterialsAmount); //TODO hardcoded materials amount

    m_MaterialsSSBO.Create(MaterialsAmount * sizeof(CMaterialGPU), GL_DYNAMIC_DRAW, 3);
    m_MaterialsSSBO.ReflectStructArray(S_Default.GetShaderID(), "MaterialBuffer", "materials");
    m_MaterialsSSBO.Bind();
}

void COpenGL430Renderer::Display()
{
    SDL_GL_SwapWindow(Window);
}

void COpenGL430Renderer::ClearWindow()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //TODO does color buffer need to be cleared?
}

size_t COpenGL430Renderer::GetGenericFlag() const
{
    return SDL_WINDOW_OPENGL;
}

int COpenGL430Renderer::GetFreeMaterialIndex()
{
    for(int i = 0; i < m_Materials.size(); i++)
    {
        if(!m_Materials.at(i)) { return i; }
    }
    return -1;
}

void COpenGL430Renderer::RegisterMaterial(CGL430Material* material)
{
    std::lock_guard<std::mutex> lock(m_MaterialMutex);
    m_PendingMaterials.push_back(material); //TODO disabled async

    int id = GetFreeMaterialIndex();
    m_Materials[id] = material;

    //Log::Instance() << "Registered material " << reinterpret_cast<uintptr_t>(material) << "\n"; LOGLOG
}

void COpenGL430Renderer::UnregisterMaterial(CGL430Material* material)
{
    std::lock_guard<std::mutex> lock(m_MaterialMutex);
    auto it = std::find(m_Materials.begin(), m_Materials.end(), material);
    if (it != m_Materials.end())
    {
        Log::ErrInstance() << "Unregistered material " << reinterpret_cast<uintptr_t>(material) << "\n";
        *it = nullptr;
    }
    else
    {
        Log::ErrInstance() << "Invalid unregister operation on " << reinterpret_cast<uintptr_t>(material) << "\n";
    }
}

int COpenGL430Renderer::GetMaterialIndex(CGL430Material* material)
{
    std::lock_guard<std::mutex> lock(m_MaterialMutex);
    auto it = std::find(m_Materials.begin(), m_Materials.end(), material);

    if (it != m_Materials.end())
    {
        return std::distance(m_Materials.begin(), it);
    }
    return -1;
}

COpenGL430Renderer::CMaterialGPU COpenGL430Renderer::BuildMaterialForGPU(CGL430Material* material)
{
    CMaterialGPU ret;

    ret.diffuse = REINTERPRET_AS(material->DiffuseTextureHandle, glm::uvec2);
    ret.shininess = material->Shininess;

    return ret;
}

void COpenGL430Renderer::ReuploadMaterial(CGL430Material* material)
{
    //Log::Instance() << "Reuploading " << reinterpret_cast<size_t>(material) << Log::Endl; LOGLOG

    int index = GetMaterialIndex(material);
    if(index < 0) { return; }

    CMaterialGPU gpu_mat = BuildMaterialForGPU(material);

    GLint offset = index * sizeof(CMaterialGPU);
    m_MaterialsSSBO.UploadData(offset, sizeof(CMaterialGPU), &gpu_mat);

    //Log::Instance() << "Reuploaded " << reinterpret_cast<size_t>(material) << " !\n"; LOGLOG
}

void COpenGL430Renderer::V_Update()
{
    //S_Default.Bind();

    bool forceReupload = false;
    COMPONENT_CALL_GET(forceReupload, CConVarManager, GetConVarValue<bool>("forcereupload"));

    if(forceReupload)
    {
        for(auto mat : m_Materials)
        {
            if(!mat) { continue; }
            ReuploadMaterial(mat);
        }
        COMPONENT_CALL(CConVarManager, SetConVarValue<bool>("forcereupload", false));
    }

    std::vector<CGL430Material*> m_PendingCopy;

    {
        std::lock_guard<std::mutex> lock(m_MaterialMutex);
        m_PendingCopy.swap(m_PendingMaterials);
    }

    for(auto mat : m_PendingCopy)
    {
        m_materialsToUpload.push_back(mat);
    }

    std::vector<CGL430Material*> uploaded;
    for(auto mat : m_materialsToUpload)
    {
        if(mat->LoadingStatus != CResource::CLoadingStatus::Done) { continue; }
        //int id = GetFreeMaterialIndex();
        
        //m_Materials[id] = mat;
        ReuploadMaterial(mat);
        uploaded.push_back(mat);
    }

    for(auto mat : uploaded)
    {
        std::erase(m_materialsToUpload, mat);
    }
}

void COpenGL430Renderer::SetupCamera(std::shared_ptr<CCamera> camera)
{
    //S_Default.Bind(); //TODO unnecessary binding

    bool view = camera->AcknowledgeViewChange();
    bool proj = camera->AcknowledgeProjChange();

    if(view) { S_Default.setUniform("view", camera->GetViewMatrix()); }
    if(proj) { S_Default.setUniform("proj", camera->GetProjectionMatrix()); }
}

void COpenGL430Renderer::DrawModel(std::shared_ptr<CModelBase> model, const CTransform& transform)
{
    //S_Default.Bind(); //TODO unnecessary binding

    bool mdl = transform.AcknowledgeChange();

    //if(mdl) { /*Log::Instance() << "Uploaded model uniform\n";*/ S_Default.setUniform("model", transform.GetModelMatrix()); }

    //if transform of other model changes, but this model remains unchanged
    //then model matrix is never being reuploaded, so transform
    //simply being duplicated from prev drawcall

    S_Default.setUniform("model", transform.GetModelMatrix());
    model->V_Draw();
}

std::shared_ptr<CDrawable> COpenGL430Renderer::CreateAnimatedDrawable() const
{
    return std::make_shared<CGL430Drawable>();
}

std::string COpenGL430Renderer::GetModelType() const { return "gl430model"; }
std::string COpenGL430Renderer::GetMaterialType() const { return "gl430material"; }

LINK_RENDERER_TO_CLASS(COpenGL430Renderer, gl430);