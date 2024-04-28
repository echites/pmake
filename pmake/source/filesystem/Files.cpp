#include "pmake/filesystem/Files.hpp"

#include <cassert>
#include <fstream>
#include <sstream>
#include <ranges>
#include <functional>
#include <algorithm>
#include <unordered_map>

namespace pmake {

namespace fs = std::filesystem;

namespace detail {

static void replace(fs::directory_entry const& entry, std::pair<std::string, std::string> const& wildcard)
{
    std::stringstream contentStream {};
    std::ifstream inputStream { entry.path() };
    contentStream << inputStream.rdbuf();

    std::ofstream outputStream { entry.path(), std::ios::trunc };

    outputStream << [&] mutable {
        auto content = contentStream.str();
        for (auto position = content.find(wildcard.first); position != std::string::npos; position = content.find(wildcard.first))
        {
            auto const first = std::next(content.begin(), int(position));
            auto const last  = std::next(first, int(wildcard.first.size()));

            content.replace(first, last, wildcard.second);
        }

        return content;
    }();
}

}

void filesystem::rename_all(fs::path const& where, std::unordered_map<std::string, std::string> const& wildcards)
{
    for (auto const& wildcard : wildcards)
    {
        [&] (this auto self, fs::path where) -> void {
            std::ranges::for_each(fs::directory_iterator(where), [&] (fs::directory_entry entry) {
                if (fs::is_directory(entry)) self(entry);
                if (!entry.path().filename().string().contains(wildcard.first)) return;

                auto const fileName = [&] {
                    auto fileName = entry.path().filename().string();

                    for (auto position = fileName.find(wildcard.first); position != std::string::npos; position = fileName.find(wildcard.first))
                    {
                        auto const first = std::next(fileName.begin(), int(position));
                        auto const last  = std::next(first, int(wildcard.first.size()));

                        fileName.replace(first, last, wildcard.second);
                    }

                    return fileName;
                }();

                fs::rename(entry, fs::path(entry.path().parent_path()).append(fileName));
            });
        }(where);
    }
}

void filesystem::replace_all(fs::path const& where,  std::unordered_map<std::string, std::string> const& wildcards)
{
    std::ranges::for_each(
        fs::recursive_directory_iterator(where) | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); }),
        [&] (fs::directory_entry entry) {
            std::ranges::for_each(wildcards, std::bind_front(detail::replace, entry));
        }
    );
}

}

