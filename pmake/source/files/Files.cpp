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

static void replace_filename(fs::directory_entry const& entry, Wildcard const& wildcard);
static void replace_content(fs::directory_entry const& entry, Wildcard const& wildcard);

}

ErrorOr<void> preprocess_files(fs::path const& path, PreprocessorContext const& context)
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

void replace_filenames(fs::path const& where, Wildcards const& wildcards)
{
    std::ranges::for_each(fs::directory_iterator(where), [&] (auto&& entry) {
        if (fs::is_directory(entry)) replace_filenames(entry, wildcards);
        std::ranges::for_each(wildcards, std::bind_front(detail::replace_filename, entry));
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
    std::string content { string };

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

static void replace_filename(fs::directory_entry const& entry, Wildcard const& wildcard)
{
    if (!entry.path().filename().string().contains(wildcard.first)) return;
    auto const filename = replace(entry.path().filename().string(), wildcard);
    fs::rename(entry, fs::path(entry.path().parent_path()).append(filename));
}

static void replace_content(fs::directory_entry const& entry, Wildcard const& wildcard)
{
    std::ofstream(entry.path(), std::ios::trunc) << [&] {
        std::stringstream contentStream {};
        std::ifstream inputStream(entry.path());
        contentStream << inputStream.rdbuf();
        return replace(contentStream.str(), wildcard);
    }();
}

}

