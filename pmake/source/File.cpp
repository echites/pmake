#include "pmake/File.hpp"

#include <cxxopts.hpp>
#include <fplus/fplus.hpp>
#include <liberror/ErrorOr.hpp>
#include <libpreprocessor/Preprocessor.hpp>
#include <nlohmann/json.hpp>

#define ERROR(fmt, ...) make_error(PREFIX_ERROR ": " fmt __VA_OPT__(,) __VA_ARGS__)

namespace fs = std::filesystem;

using namespace liberror;

ErrorOr<void> pmake::file::copy(fs::path const& source, fs::path const& destination)
{
    try
    {
        fs::copy(source, destination, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    }
    catch (fs::filesystem_error const& error)
    {
        return ERROR("{}", error.what());
    }
    return {};
}

std::string replace(std::string_view string, std::pair<std::string, std::string> const& wildcard)
{
    std::string content(string);
    for (auto position = content.find(wildcard.first); position != std::string::npos; position = content.find(wildcard.first))
    {
        auto const first = std::next(content.begin(), static_cast<int>(position));
        auto const last  = std::next(first, static_cast<int>(wildcard.first.size()));
        content.replace(first, last, wildcard.second);
    }

    return content;
}

void pmake::file::replace_file_name_wildcards(fs::path const& path, std::unordered_map<std::string, std::string> const& wildcards)
{
    auto const fnReplace = [] (fs::path const& entry, std::pair<std::string, std::string> const& wildcard) {
        fs::rename(entry, auto(entry.parent_path()).append(replace(entry.filename().string(), wildcard)));
    };

    auto iterator = fs::directory_iterator(path) | std::views::transform(&fs::directory_entry::path);
    std::ranges::for_each(iterator, [&] (auto&& entry) {
        if (fs::is_directory(entry)) replace_file_name_wildcards(entry, wildcards);
        std::ranges::for_each(
            wildcards | std::views::filter([&] (auto&& wildcard) {
                return entry.filename().string().contains(wildcard.first);
            }),
            std::bind_front(fnReplace, entry)
        );
    });
}

void pmake::file::replace_file_wildcards(fs::path const& path, std::unordered_map<std::string, std::string> const& wildcards)
{
    auto const fnReplace = [] (fs::path const& entry, std::pair<std::string, std::string> const& wildcard) {
        auto const content = [&] {
            std::stringstream contentStream {};
            contentStream << std::ifstream(entry).rdbuf();
            return replace(contentStream.str(), wildcard);
        }();
        std::ofstream outputStream(entry, std::ios::trunc);
        outputStream << content;
    };

    auto iterator = fs::recursive_directory_iterator(path) | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); });
    std::ranges::for_each(iterator, [&] (auto&& entry) {
        std::ranges::for_each(wildcards, std::bind_front(fnReplace, entry));
    });
}
