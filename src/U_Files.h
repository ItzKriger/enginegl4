#pragma once
#include <filesystem>
#include <optional>

namespace FileUtils
{
    std::filesystem::path get_executable_path();
    bool is_within_root(const std::filesystem::path& root, const std::filesystem::path& user_path);
    std::optional<std::filesystem::path> find_first_by_name(const std::filesystem::path& root, const std::string& name, bool case_insensitive = false, bool follow_symlinks = false);
}
