#include "pmake/files/Files.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <ranges>
#include <sstream>
#include <utility>

namespace fs = std::filesystem;

using namespace liberror;
using namespace libpreprocessor;
using namespace nlohmann;

using Wildcard = std::pair<std::string, std::string>;

namespace pmake {

namespace detail {

static void replace(fs::directory_entry const& entry, Wildcard const& wildcard);
static void rename(fs::directory_entry const& entry, Wildcard const& wildcard);

}

ErrorOr<void> process_all(fs::path const& path, PreprocessorContext const& context)
{
    auto iterator =
        fs::recursive_directory_iterator(path)
            | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); });

    for (auto const& entry : iterator)
    {
        std::ofstream(entry.path()) << TRY(preprocess(entry.path(), context));
    }

    return {};
}

void rename_all(fs::path const& where, Wildcards const& wildcards)
{
    std::ranges::for_each(fs::directory_iterator(where), [&] (auto&& entry) {
        if (fs::is_directory(entry)) rename_all(entry, wildcards);
        std::ranges::for_each(wildcards, std::bind_front(detail::rename, entry));
    });
}

void replace_all(fs::path const& where, Wildcards const& wildcards)
{
    auto iterator =
        fs::recursive_directory_iterator(where)
            | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); });

    std::ranges::for_each(iterator, [&] (auto&& entry) {
        std::ranges::for_each(wildcards, std::bind_front(detail::replace, entry));
    });
}

}

namespace pmake::detail {

static void replace(fs::directory_entry const& entry, Wildcard const& wildcard)
{
    std::ofstream(entry.path(), std::ios::trunc) << [&] {
        std::stringstream contentStream {};
        std::ifstream inputStream(entry.path());
        contentStream << inputStream.rdbuf();

        auto content = contentStream.str();
        for (auto position = content.find(wildcard.first)
                ; position != std::string::npos
                ; position = content.find(wildcard.first))
        {
            auto const first = std::next(content.begin(), static_cast<int>(position));
            auto const last  = std::next(first, static_cast<int>(wildcard.first.size()));
            content.replace(first, last, wildcard.second);
        }

        return content;
    }();
}

static void rename(fs::directory_entry const& entry, Wildcard const& wildcard)
{
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
}

}

