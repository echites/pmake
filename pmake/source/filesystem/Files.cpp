#include "pmake/filesystem/Files.hpp"

#include <print>
#include <cassert>
#include <fstream>
#include <sstream>

namespace pmake {

void filesystem::rename_all(std::filesystem::path where, std::pair<std::string, std::string> const& wildcard)
{
    namespace fs = std::filesystem;

    for (auto const& entry : fs::directory_iterator(where))
    {
        if (fs::is_directory(entry)) rename_all(entry, wildcard);
        if (!entry.path().filename().string().contains(wildcard.first)) continue;
        fs::rename(entry, fs::path(entry.path().parent_path()).append(wildcard.second));
    }
}

void filesystem::replace_all(std::filesystem::path where, std::pair<std::string, std::string> const& wildcard)
{
    namespace fs = std::filesystem;

    for (auto const& entry : fs::directory_iterator(where))
    {
        if (fs::is_directory(entry))
        {
            replace_all(entry, wildcard);
            continue;
        }

        std::stringstream stream {};
        stream << std::ifstream(entry.path()).rdbuf();
        auto content = stream.str();

        while (auto const wildcardPosition = content.find(wildcard.first))
        {
            if (wildcardPosition == std::string::npos) break;

            auto const first = std::next(content.begin(), static_cast<int>(wildcardPosition));
            auto const last  = std::next(first, static_cast<int>(wildcard.first.size()));

            content.replace(first, last, wildcard.second);
        }

        std::ofstream { entry.path(), std::ios::in | std::ios::out | std::ios::trunc } << content;
    }
}

}

