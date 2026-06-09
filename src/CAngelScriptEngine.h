#pragma once
#include <angelscript.h>
#include <assert.h>

#include <string>
#include <vector>
#include <filesystem>

class CAngelScriptEngine
{
public:
    CAngelScriptEngine();
    ~CAngelScriptEngine();

    void LoadModule(const std::string& name, const std::vector<std::pair<std::string, std::string>>& scripts);

    void Init();
    void Reset();

    void CallInitFunctions();
    void LoadDirectory(const std::string& moduleName, const std::filesystem::path& dir);

    asIScriptEngine* Engine = nullptr;
    asIScriptModule* Module = nullptr;
    asIScriptContext* Context = nullptr;

    std::vector<std::string> SectionNames;
};
