#include "pmake/files/Files.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace pmake {

using namespace liberror;
using namespace nlohmann;

namespace fs = std::filesystem;

namespace detail {

static void replace(fs::directory_entry const& entry, std::pair<std::string, std::string> const& wildcard)
{
    std::stringstream contentStream;
    std::ifstream inputStream(entry.path());
    contentStream << inputStream.rdbuf();

    std::ofstream outputStream(entry.path(), std::ios::trunc);

    outputStream << [&] {
        auto content = contentStream.str();

        for (auto position = content.find(wildcard.first); position != std::string::npos; position = content.find(wildcard.first))
        {
            auto const first = std::next(content.begin(), static_cast<int>(position));
            auto const last  = std::next(first, static_cast<int>(wildcard.first.size()));

            content.replace(first, last, wildcard.second);
        }

        return content;
    }();
}

}

void rename_all(fs::path const& where, std::unordered_map<std::string, std::string> const& wildcards)
{
    [&wildcards] (this auto self, fs::path const& where) -> void {
        auto const iterator = fs::directory_iterator(where);

        std::ranges::for_each(iterator, [&] (fs::directory_entry const& entry) {
            if (fs::is_directory(entry)) self(entry);
            std::ranges::for_each(wildcards, [&] (auto&& wildcard) {
                auto filename = entry.path().filename().string();

                if (!filename.contains(wildcard.first)) return;

                for (auto position = filename.find(wildcard.first)
                        ; position != std::string::npos
                        ; position = filename.find(wildcard.first))
                {
                    auto const first = std::next(filename.begin(), static_cast<int>(position));
                    auto const last  = std::next(first, static_cast<int>(wildcard.first.size()));
                    filename.replace(first, last, wildcard.second);
                }

                fs::rename(entry, fs::path(entry.path().parent_path()).append(filename));
            });
        });
    }(where);
}

void replace_all(fs::path const& where,  std::unordered_map<std::string, std::string> const& wildcards)
{
    auto const iterator = fs::recursive_directory_iterator(where);

    std::ranges::for_each(
        iterator | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); }),
        [&] (fs::directory_entry entry) {
            std::ranges::for_each(wildcards, std::bind_front(detail::replace, entry));
        }
    );
}

ErrorOr<json> read_json(std::filesystem::path const& path)
{
    ErrorOr<json> info = json::parse(std::ifstream(path), nullptr, false);
    return info->is_discarded()
            ? make_error(PREFIX_ERROR": Couldn't open {}.", path.string())
            : info;
}

}

