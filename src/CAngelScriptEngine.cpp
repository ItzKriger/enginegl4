#include "CAngelScriptEngine.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "CEngine.h"
#include "CLogger.h"
#include "U_Log.h"

#include "scriptbuilder/scriptbuilder.h"
#include "scriptstdstring/scriptstdstring.h"
#include "scriptarray/scriptarray.h"

void MessageCallback(const asSMessageInfo* msg, void*)
{
    std::string type = "ERR";
    if (msg->type == asMSGTYPE_WARNING) { type = "WARN"; }
    else if (msg->type == asMSGTYPE_INFORMATION) { type = "INFO"; }

    std::stringstream msg_err;
    msg_err << type << " (" << msg->row << ", " << msg->col << "): " << msg->message;

    Log::Err(msg_err.str());
}

CAngelScriptEngine::CAngelScriptEngine()
{

}

CAngelScriptEngine::~CAngelScriptEngine()
{
    Reset();
}

void CAngelScriptEngine::LoadDirectory(const std::string& moduleName, const std::filesystem::path& dir)
{
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) { return; }
    std::vector<std::filesystem::path> as_files;
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".as")
        {
            as_files.push_back(entry.path());
        }
    }

    std::vector<std::pair<std::string, std::string>> scripts;
    for(auto& path : as_files)
    {
        std::ifstream file(path);
        if(!file.is_open())
        {
            continue;
        }

        std::stringstream stream;

        stream << std::noskipws << file.rdbuf();
        std::string code = stream.str();

        file.close();
        scripts.push_back({ path.stem().string(), std::move(code) });
    }

    LoadModule(moduleName, scripts);
}

void CAngelScriptEngine::CallInitFunctions()
{
    for(const auto& sn : SectionNames)
    {
        std::string funcName = sn + "_init";
        asIScriptFunction* func = Module->GetFunctionByName(funcName.c_str());

        Context->Prepare(func);
        Context->Execute();
        Context->Unprepare();
    }
}

void CAngelScriptEngine::LoadModule(const std::string& name, const std::vector<std::pair<std::string, std::string>>& scripts)
{
    CScriptBuilder builder;
    builder.StartNewModule(Engine, name.c_str());
    
    for (const auto& [nm, sc] : scripts)
    {
        std::cout << "Section " << nm << std::endl;
        auto err = builder.AddSectionFromMemory(nm.c_str(), sc.c_str(), sc.size());

        if(err < 0)
        {
            std::cout << "Err " << err << '\n';
        }
        else
        {
            SectionNames.push_back(nm);
            //TODO error handling
            std::cout << "Added\n";
        }
        
    }
    if (builder.BuildModule() < 0) {  } //TODO error handling

    Module = Engine->GetModule(name.c_str());
    std::cout << "Module " << name << std::endl;
}

void __log(const std::string& msg)
{
    Log::Out(msg);
    std::cout << msg;
}

void __logf(float msg)
{
    Log::Instance() << msg << Log::Endl;
}

CComponent* CM_GetComponent(CComponentsManager* mgr, const std::string& name)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    return mgr->GetComponentRaw(lower);
}

CComponent* CM_Create(CComponentsManager* mgr, const std::string& type, bool instantInit)
{
    std::string lower = type;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if(!mgr->IsComponentPresent(lower))
    {
        mgr->CreateComponent(lower, instantInit);
    }

    return mgr->GetComponentRaw(lower);
}

bool CM_IsComponentPresent(CComponentsManager* _mgr, const std::string& _type)
{
    return _mgr->IsComponentPresent(_type);
}

void RegisterComponentsManager(asIScriptEngine* engine)
{
    engine->RegisterObjectType("CComponentsManager", 0, asOBJ_REF | asOBJ_NOCOUNT);

    engine->RegisterObjectMethod
    (
        "CComponentsManager",
        "bool IsComponentPresent(const string &in name)",
        asFUNCTION(CM_IsComponentPresent),
        asCALL_CDECL_OBJFIRST
    );

    engine->RegisterObjectMethod
    (
        "CComponentsManager",
        "Component@ GetComponent(const string &in name)",
        asFUNCTION(CM_GetComponent),
        asCALL_CDECL_OBJFIRST
    );

    engine->RegisterObjectMethod
    (
        "CComponentsManager",
        "Component@ Create(const string &in type, bool instantInit = true)",
        asFUNCTION(CM_Create),
        asCALL_CDECL_OBJFIRST
    );
}

void RegisterTime(asIScriptEngine* engine)
{
    engine->RegisterObjectType("CTime", 0, asOBJ_REF | asOBJ_NOCOUNT);

    engine->RegisterObjectMethod("CTime", "void Init()", asMETHOD(CTime, Init), asCALL_THISCALL);
    engine->RegisterObjectMethod("CTime", "void NewTick()", asMETHOD(CTime, NewTick), asCALL_THISCALL);
    engine->RegisterObjectMethod("CTime", "void EndTick()", asMETHOD(CTime, EndTick), asCALL_THISCALL);

    engine->RegisterObjectMethod("CTime", "float GetDeltaTime()", asMETHOD(CTime, GetDeltaTime), asCALL_THISCALL);
    engine->RegisterObjectMethod("CTime", "float GetFPS()", asMETHOD(CTime, GetFPS), asCALL_THISCALL);

    engine->RegisterObjectMethod
    (
        "CTime",
        "float get_DeltaTime() property",
        asMETHOD(CTime, GetDeltaTime),
        asCALL_THISCALL
    );
}

void RegisterEngine(asIScriptEngine* engine)
{
    engine->RegisterObjectType("CEngine", 0, asOBJ_REF | asOBJ_NOCOUNT);

    engine->RegisterObjectMethod("CEngine", "void Init()", asMETHOD(CEngine, Init), asCALL_THISCALL);
    engine->RegisterObjectMethod("CEngine", "void Update()", asMETHOD(CEngine, Update), asCALL_THISCALL);
    engine->RegisterObjectMethod("CEngine", "void Quit()", asMETHOD(CEngine, Quit), asCALL_THISCALL);

    engine->RegisterObjectProperty
    (
        "CEngine",
        "CTime Time",
        asOFFSET(CEngine, Time)
    );
}

void CAngelScriptEngine::Init()
{
    Engine = asCreateScriptEngine();
    if(!Engine) { return; }

    RegisterStdString(Engine);
    RegisterScriptArray(Engine, true);

    RegisterTime(Engine);
    RegisterEngine(Engine);

    Engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
    Engine->RegisterGlobalFunction("void log(const string &in)", asFUNCTION(__log), asCALL_CDECL);
    Engine->RegisterGlobalFunction("void log(float)", asFUNCTION(__logf), asCALL_CDECL);

    Engine->RegisterGlobalProperty("CEngine Engine", CEngine::GetInstance());

    Context = Engine->CreateContext();
}

void CAngelScriptEngine::Reset()
{
    if(Context)
    {
        Context->Release();
        Context = nullptr;
    }
    if(Engine)
    {
        Engine->ShutDownAndRelease();
        Engine = nullptr;
    }
}
