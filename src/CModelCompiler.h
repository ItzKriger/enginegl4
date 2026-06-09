#pragma once
#include <filesystem>

class CModelCompiler
{
public:
    static bool CompileModel(const std::filesystem::path& srcpath);
};
