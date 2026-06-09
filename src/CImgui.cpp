#include "CImgui.h"
#include "CEngine.h"

#include "CWindowManager.h"
#include "COpenGL430Renderer.h"
#include "U_Log.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

bool CImgui::V_ScriptInit(std::shared_ptr<sol::state> _state, sol::table _table)
{
    _table.raw_set("ondraw", OnDraw.GetScriptTable(_state));

    _table.raw_set("Begin", [](const std::string& text) { ImGui::Begin(text.c_str()); });
    _table.raw_set("Text", [](const std::string& text) { ImGui::Text(text.c_str()); });
    _table.raw_set("End", []() { ImGui::End(); });

    _table.raw_set("Separator", []() { ImGui::Separator(); });
    _table.raw_set("SameLine", []() { ImGui::SameLine(); });

    _table.raw_set("Button", [](const std::string& label) { return ImGui::Button(label.c_str()); });

    _table.raw_set("Checkbox", [](const std::string& label, bool value)
    {
        bool v = value;
        ImGui::Checkbox(label.c_str(), &v);
        return v;
    });

    _table.raw_set("SliderFloat", [](const std::string& label, float value, float min, float max)
    {
        float v = value;
        ImGui::SliderFloat(label.c_str(), &v, min, max);
        return v;
    });

    _table.raw_set("InputText", [](const std::string& label, std::string text)
    {
        char buffer[256];
        strncpy(buffer, text.c_str(), sizeof(buffer));
        if (ImGui::InputText(label.c_str(), buffer, sizeof(buffer)))
        {
            return std::string(buffer);
        }
        return text;
    });

    _table.raw_set("BeginMainMenuBar", []() { return ImGui::BeginMainMenuBar(); });
    _table.raw_set("EndMainMenuBar", []() { ImGui::EndMainMenuBar(); });
    _table.raw_set("BeginMenu", [](const std::string& label) { return ImGui::BeginMenu(label.c_str()); });
    _table.raw_set("EndMenu", []() { ImGui::EndMenu(); });
    _table.raw_set("MenuItem", [](const std::string& label) { return ImGui::MenuItem(label.c_str()); });

    _table.raw_set("SetNextWindowPos", [](float x, float y) { ImGui::SetNextWindowPos(ImVec2(x, y)); });
    _table.raw_set("SetNextWindowSize", [](float w, float h) { ImGui::SetNextWindowSize(ImVec2(w, h)); });
    _table.raw_set("Columns", [](int count) { ImGui::Columns(count); });
    _table.raw_set("NextColumn", []() { ImGui::NextColumn(); });
    return true;
}

bool CImgui::V_Init()
{
    auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
    if(!winman) { return false; }

    auto& _renderer = winman->Renderer;

    auto renderer = dynamic_cast<COpenGL430Renderer*>(_renderer.get());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; 
    ImGui::StyleColorsDark();

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplSDL3_InitForOpenGL(winman->Window, renderer->Context);
    ImGui_ImplOpenGL3_Init("#version 430");

    winman->OnDraw += [this]()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        OnDraw();

        //ImGui::Begin("Debug Window");
        //ImGui::Text("Hello, Debug Info!");
        //ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    };

    DrawDeleter = winman->OnDraw.GetLastFunctionHandleUnique();

    winman->OnEventHandle += [winman](const SDL_Event& ev)
    {
        if(!SDL_GetWindowRelativeMouseMode(winman->Window))
        {
            ImGui_ImplSDL3_ProcessEvent(&ev);
        }
    };

    EventDeleter = winman->OnEventHandle.GetLastFunctionHandleUnique();
    return true;
}

void CImgui::V_DeInit()
{
    auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
    if(!winman) { return; }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    DrawDeleter.reset();
    EventDeleter.reset();
}

LINK_COMPONENT_TO_CLASS(CImgui, imgui);