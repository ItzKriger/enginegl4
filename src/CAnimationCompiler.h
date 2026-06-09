#pragma once
#include <filesystem>

class CAnimationCompiler
{
public:
    static bool CompileAnimation(const std::filesystem::path& srcpath);
};
