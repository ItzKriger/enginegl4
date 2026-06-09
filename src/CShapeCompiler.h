#pragma once
#include <filesystem>

class CShapeCompiler
{
public:
    static bool CompileShape(const std::filesystem::path& srcpath);
};
