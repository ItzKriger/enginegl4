#include "U_Files.h"
#include <string>

#include "U_Log.h"

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#else
    #include <unistd.h>
    #include <limits.h>
    #include <errno.h>
#endif

std::filesystem::path FileUtils::get_executable_path()
{
#if defined(_WIN32)

    std::wstring buffer(MAX_PATH, L'\0');
    DWORD len = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

    if (len == 0)
    {
        return {};
    }

    buffer.resize(len);
    return std::filesystem::path(buffer).parent_path();

#elif defined(__APPLE__)


    uint32_t bufsize = 0;
    _NSGetExecutablePath(nullptr, &bufsize);
    std::vector<char> buffer(bufsize);

    if (_NSGetExecutablePath(buffer.data(), &bufsize) != 0)
    {
        return {};
    }

    char resolved[PATH_MAX];
    if (realpath(buffer.data(), resolved))
    {
        return std::filesystem::path(resolved).parent_path();
    }

    return std::filesystem::path(buffer.data()).parent_path();

#else

    std::vector<char> buffer(PATH_MAX);
    ssize_t len = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);

    if (len == -1)
    {
        return {};
    }

    buffer[len] = '\0';
    return std::filesystem::path(buffer.data()).parent_path();

#endif
}

bool FileUtils::is_within_root(const std::filesystem::path& root, const std::filesystem::path& user_path)
{
    try
    {
        auto abs_root = std::filesystem::weakly_canonical(root);
        auto abs_target = std::filesystem::weakly_canonical(abs_root / user_path);

        return std::mismatch(abs_root.begin(), abs_root.end(), abs_target.begin()).first == abs_root.end();
    }
    catch(...)
    {
        return false;
    }
    return false;
}

std::optional<std::filesystem::path> FileUtils::find_first_by_name(const std::filesystem::path& root, const std::string& name, bool case_insensitive, bool follow_symlinks)
{
    try
    {
        auto abs_root = std::filesystem::weakly_canonical(root);

        std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;
        if (follow_symlinks) { options |= std::filesystem::directory_options::follow_directory_symlink; }

        for (std::filesystem::recursive_directory_iterator it(abs_root, options), end; it != end; ++it)
        {
            const std::filesystem::directory_entry& de = *it;
            std::string fname = de.path().filename().string();

            auto eq = [&](const std::string& a, const std::string& b)
            {
                if (!case_insensitive) { return a == b; }
                auto tolow = [](char c){ return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };

                if (a.size() != b.size()) { return false; }
                for (size_t i = 0; i < a.size(); ++i)
                {
                    if (tolow(a[i]) != tolow(b[i])) { return false; }
                }
                return true;
            };

            if (eq(fname, name))
            {
                std::filesystem::path candidate = de.path();
                if (is_within_root(abs_root, candidate))
                {
                    return std::filesystem::weakly_canonical(candidate);
                }
            }

            if (de.is_symlink() && de.is_directory() && !follow_symlinks)
            {
                it.disable_recursion_pending();
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        return std::nullopt;
    }
    return std::nullopt;
}
