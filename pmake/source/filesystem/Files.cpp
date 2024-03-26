#include "pmake/filesystem/Files.hpp"

#include <print>
#include <cassert>
#include <fstream>
#include <sstream>
#include <ranges>
#include <functional>
#include <unordered_map>

namespace pmake {

    namespace detail {

    static void replace(std::filesystem::directory_entry entry, std::pair<std::string, std::string> const& wildcard)
    {
        std::stringstream contentStream {};
        std::ifstream inputStream { entry.path() };
        contentStream << inputStream.rdbuf();

        std::ofstream outputStream { entry.path(), std::ios::in | std::ios::out | std::ios::trunc };

        outputStream << [&, content = contentStream.str()] mutable {
            while (auto const position = content.find(wildcard.first))
            {
                if (position == std::string::npos) break;

                auto const first = std::next(content.begin(), int(position));
                auto const last  = std::next(first, int(wildcard.first.size()));

                content.replace(first, last, wildcard.second);
            }

            return content;
        }();
    }

    }

void filesystem::rename_all(std::filesystem::path where, std::unordered_map<std::string, std::string> const& wildcards)
{
    namespace fs = std::filesystem;

    for (auto const& wildcard : wildcards)
    {
        [&] (this auto self, fs::path where) -> void {
            std::ranges::for_each(fs::directory_iterator(where), [&] (fs::directory_entry entry) {
                if (fs::is_directory(entry)) self(entry);
                if (!entry.path().filename().string().contains(wildcard.first)) return;
                fs::rename(entry, fs::path(entry.path().parent_path()).append(wildcard.second));
            });
        }(where);
    }
}

void filesystem::replace_all(std::filesystem::path where,  std::unordered_map<std::string, std::string> const& wildcards)
{
    namespace fs = std::filesystem;

    std::ranges::for_each(
        fs::recursive_directory_iterator(where) | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); }),
        [&] (fs::directory_entry entry) {
            std::ranges::for_each(wildcards, std::bind_front(detail::replace, entry));
        }
    );
}

}

