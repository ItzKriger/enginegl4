#include "CTestRender.h"
#include "CEngine.h"

#include "CResourcesManager.h"
#include "CWindowManager.h"
#include "CGL430Model.h"
#include "COpenGL430Renderer.h"
#include "CCommandProcessor.h"
#include "U_Scripting.h"
#include "CConVarManager.h"
#include "U_Numbers.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

bool CTestRender::V_Init()
{
    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
    auto& renderer = winman->Renderer;

    Model = resman->GetOrCreate(renderer->GetModelType(), "tankroad_terrain.emdl");

    return true;
}

void CTestRender::Draw()
{
    float WindowAspect = 1.0f;
    COMPONENT_CALL_GET(WindowAspect, CWindowManager, GetWindowAspect());

    glm::mat4 view = glm::lookAt(CameraTransform.GetPosition(), CameraTransform.GetPosition() + CameraTransform.GetForwardVector(), CameraTransform.GetUpVector());
    glm::mat4 proj = glm::perspective(glm::radians(CameraFOV), WindowAspect, 0.01f, 10000.0f);
    glm::mat4 modl = ModelTransform.GetModelMatrix();

    //glm::mat4 modl = glm::mat4(1.0f);
    //glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    //glm::mat4 proj = glm::perspective(glm::radians(60.0f), WindowAspect, 0.1f, 100.0f);

    auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
    auto _renderer = winman->Renderer.get();
    auto renderer = dynamic_cast<COpenGL430Renderer*>(_renderer);

    //renderer->S_Default.Bind();

    renderer->S_Default.setUniform("model", modl);
    renderer->S_Default.setUniform("view", view);
    renderer->S_Default.setUniform("proj", proj);

    auto mdl = std::dynamic_pointer_cast<CModelBase>(Model);
    mdl->V_Draw();
}

bool CTestRender::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    auto fieldsMan = CScriptFieldsManager::ValidateFieldsManager(table, true).first;

    sol::table camera = CameraTransform.GetScriptTable(state);
    sol::table model = ModelTransform.GetScriptTable(state);

    table["camera"] = camera;
    table["model"] = model;

    auto scFov = std::make_unique<CProxyScriptField<float>>(CameraFOV);
    fieldsMan->AddField<std::string>("fov", std::move(scFov));

    sol::table keys = table.create_named("key");

    {
        auto keys_man = CScriptFieldsManager::CreateFieldsManager(keys);

        auto scForward = std::make_unique<CProxyScriptField<bool>>(Forward, true);
        auto scBack = std::make_unique<CProxyScriptField<bool>>(Back, true);
        auto scLeft = std::make_unique<CProxyScriptField<bool>>(Left, true);
        auto scRight = std::make_unique<CProxyScriptField<bool>>(Right, true);
        auto scShift = std::make_unique<CProxyScriptField<bool>>(Shift, true);
        auto scCtrl = std::make_unique<CProxyScriptField<bool>>(Ctrl, true);

        keys_man->AddField("forward", std::move(scForward));
        keys_man->AddField("back", std::move(scBack));
        keys_man->AddField("left", std::move(scLeft));
        keys_man->AddField("right", std::move(scRight));
        keys_man->AddField("shift", std::move(scShift));
        keys_man->AddField("ctrl", std::move(scCtrl));

        keys_man->CreateMetaTable(keys);
    }

    auto SetTransformFuncs = [](sol::table tabl, CTransform& trans) -> void
    {
        sol::state_view st = tabl.lua_state();

        tabl.set_function("setPosition", [&trans](sol::object vec) { trans.SetPosition(ScriptUtils::FromObject<glm::vec3>(vec)); });
        tabl.set_function("setRotation", [&trans](sol::object quat) { trans.SetRotation(ScriptUtils::FromObject<glm::quat>(quat)); });
        tabl.set_function("setScale", [&trans](sol::object scl) { trans.SetScale(ScriptUtils::FromObject<glm::vec3>(scl)); });

        tabl.set_function("getPosition", [&trans, st]() -> sol::object { return ScriptUtils::ToObject(trans.GetPosition(), st); });
        tabl.set_function("getRotation", [&trans, st]() -> sol::object { return ScriptUtils::ToObject(trans.GetRotation(), st); });

        tabl.set_function("getRotationEuler", [&trans, st]() -> sol::object
        {
            glm::quat rot = trans.GetRotation();
            glm::vec3 euler = glm::eulerAngles(rot);

            return ScriptUtils::ToObject(euler, st);
        });

        tabl.set_function("getRotationEulerDegrees", [&trans, st]() -> sol::object
        {
            glm::quat rot = trans.GetRotation();
            glm::vec3 eulerDegrees = glm::eulerAngles(rot);

            eulerDegrees.x = glm::degrees(eulerDegrees.x);
            eulerDegrees.y = glm::degrees(eulerDegrees.y);
            eulerDegrees.z = glm::degrees(eulerDegrees.z);

            return ScriptUtils::ToObject(eulerDegrees, st);
        });

        tabl.set_function("getScale", [&trans, st]() -> sol::object { return ScriptUtils::ToObject(trans.GetScale(), st); });
    };
    return true;
}

void CTestRender::V_Update()
{
    auto global_start_point = CEngine::GetInstance()->Time.GetCurrent();

    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();

    float sensitivity = 0.005f;
    float speed = 1.0f;

    auto start_point = std::chrono::high_resolution_clock::now();

    COMPONENT_CALL_GET(sensitivity, CConVarManager, GetConVarValue<float>("cam.sensitivity"));
    auto end_point = std::chrono::high_resolution_clock::now();

    if((end_point - start_point) >= std::chrono::duration<float>(0.1f))
    {
        Log::ErrInstance() << "sensitivity cvar took " << std::chrono::duration_cast<std::chrono::duration<float>>(end_point - start_point) << Log::Endl;
    }

    start_point = std::chrono::high_resolution_clock::now();
    COMPONENT_CALL_GET(speed, CConVarManager, GetConVarValue<float>("cam.speed"));
    end_point = std::chrono::high_resolution_clock::now();

    if((end_point - start_point) >= std::chrono::duration<float>(0.1f))
    {
        Log::ErrInstance() << "speed cvar took " << std::chrono::duration_cast<std::chrono::duration<float>>(end_point - start_point) << Log::Endl;
    }

    float dx, dy;
    start_point = std::chrono::high_resolution_clock::now();
    SDL_GetRelativeMouseState(&dx, &dy);
    end_point = std::chrono::high_resolution_clock::now();

    if((end_point - start_point) >= std::chrono::duration<float>(0.1f))
    {
        Log::ErrInstance() << "SDL_GetRelativeMouseState took " << std::chrono::duration_cast<std::chrono::duration<float>>(end_point - start_point) << Log::Endl;
    }

    float deltaYaw = dx * sensitivity;
    float deltaPitch = dy * sensitivity;

    CameraAngles.y -= CAngle::radians(deltaYaw);
    CameraAngles.x -= CAngle::radians(deltaPitch);

    CameraAngles.x.clamp180deg(-89.0f, 89.0f);

    glm::quat qPitch = glm::angleAxis(CameraAngles.x.asRadians(), glm::vec3(1, 0, 0));
    glm::quat qYaw   = glm::angleAxis(CameraAngles.y.asRadians(),   glm::vec3(0, 1, 0));
    glm::quat orientation = glm::normalize(qYaw * qPitch);

    CameraTransform.SetRotation(glm::normalize(orientation));

    start_point = std::chrono::high_resolution_clock::now();
    SDL_PumpEvents();
    end_point = std::chrono::high_resolution_clock::now();

    if((end_point - start_point) >= std::chrono::duration<float>(0.1f))
    {
        Log::ErrInstance() << "SDL_PumpEvents took " << std::chrono::duration_cast<std::chrono::duration<float>>(end_point - start_point) << Log::Endl;
    }

    start_point = std::chrono::high_resolution_clock::now();
    const bool* keyboard = SDL_GetKeyboardState(NULL);
    end_point = std::chrono::high_resolution_clock::now();

    if((end_point - start_point) >= std::chrono::duration<float>(0.1f))
    {
        Log::ErrInstance() << "SDL_GetKeyboardState took " << std::chrono::duration_cast<std::chrono::duration<float>>(end_point - start_point) << Log::Endl;
    }

    Forward = keyboard[SDL_SCANCODE_W     ];
    Back    = keyboard[SDL_SCANCODE_S     ];
    Left    = keyboard[SDL_SCANCODE_A     ];
    Right   = keyboard[SDL_SCANCODE_D     ];
    Shift   = keyboard[SDL_SCANCODE_LSHIFT];
    Ctrl    = keyboard[SDL_SCANCODE_LCTRL ];

    auto euler_repr = CameraTransform.GetEulerRotation().GetRotation();

    if(dx != 0.0f || dy != 0.0f)
    {
        //Log::Instance() << "Delta mouse: " << dx << ", " << dy << "; Angles: " << euler_repr.x.asDegrees() << ", " << euler_repr.y.asDegrees() << ", " << euler_repr.z.asDegrees() << Log::Endl;
    }

    auto global_end_point = CEngine::GetInstance()->Time.GetCurrent();
	auto update_diff = std::chrono::duration_cast<std::chrono::duration<float>>(global_end_point - global_start_point);

    if(update_diff.count() >= 0.1f)
    {
        Log::ErrInstance() << "global testrender update took " << update_diff.count() << Log::Endl;
    }
}

LINK_COMPONENT_TO_CLASS(CTestRender, testrender)