#include "pmake/files/Files.hpp"

#include <iostream>

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

static void replace_filename(fs::path const& entry, Wildcard const& wildcard);
static void replace_content(fs::path const& entry, Wildcard const& wildcard);

}

ErrorOr<void> preprocess_files(fs::path const& where, PreprocessorContext const& context)
{
    auto iterator =
        fs::recursive_directory_iterator(where)
            | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); });

    for (auto const& entry : iterator)
    {
        auto const content = TRY(preprocess(entry.path(), context));
        std::ofstream outputStream(entry.path());
        outputStream << content;
    }

    return {};
}

void replace_filenames(fs::path const& where, Wildcards const& wildcards)
{
    auto const iterator =
        fs::directory_iterator(where)
            | std::views::transform(&fs::directory_entry::path);

    std::ranges::for_each(iterator, [&] (auto&& entry) {
        auto const filename = entry.filename().string();
        if (fs::is_directory(entry)) replace_filenames(entry, wildcards);
        std::ranges::for_each(
            wildcards | std::views::filter([&] (auto&& wildcard) {
                return filename.contains(wildcard.first);
            }),
            std::bind_front(detail::replace_filename, entry)
        );
    });
}

void replace_contents(fs::path const& where, Wildcards const& wildcards)
{
    auto iterator =
        fs::recursive_directory_iterator(where)
            | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); });

    std::ranges::for_each(iterator, [&] (auto&& entry) {
        std::ranges::for_each(wildcards, std::bind_front(detail::replace_content, entry));
    });
}

}

namespace pmake::detail {

static std::string replace(std::string_view string, Wildcard const& wildcard)
{
    std::string content(string);

    for (auto position = content.find(wildcard.first)
            ; position != std::string::npos
            ; position = content.find(wildcard.first))
    {
        auto const first = std::next(content.begin(), static_cast<int>(position));
        auto const last  = std::next(first, static_cast<int>(wildcard.first.size()));
        content.replace(first, last, wildcard.second);
    }

    return content;
}

static void replace_filename(fs::path const& entry, Wildcard const& wildcard)
{
    auto parentPath = entry.parent_path();
    fs::rename(entry,
        parentPath.append(
            replace(entry.filename().string(), wildcard)
        ));
}

static void replace_content(fs::path const& entry, Wildcard const& wildcard)
{
    auto const content = [&] {
        std::stringstream contentStream {};
        contentStream << std::ifstream(entry).rdbuf();
        return replace(contentStream.str(), wildcard);
    }();

    std::ofstream outputStream(entry, std::ios::trunc);
    outputStream << content;
}

}

