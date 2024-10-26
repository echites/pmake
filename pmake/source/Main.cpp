#include "pmake/Project.hpp"
#include "pmake/program/Information.hpp"

#include <cxxopts.hpp>
#include <fplus/fplus.hpp>
#include <fplus/result.hpp>
#include <liberror/ErrorOr.hpp>
#include <libpreprocessor/Preprocessor.hpp>
#include <nlohmann/json.hpp>

#define ERROR(fmt, ...) make_error(PREFIX_ERROR ": " fmt __VA_OPT__(,) __VA_ARGS__)
#define THROW(fmt, ...) return ERROR(fmt, __VA_ARGS__)

namespace fs = std::filesystem;

using namespace std::literals;
using namespace liberror;
using namespace libpreprocessor;
using namespace nlohmann;
using namespace cxxopts;

struct Context
{
    cxxopts::ParseResult const& arguments;
    nlohmann::json const& configuration;
};

struct Project
{
    std::string name;
    std::string language;
    std::string standard;
    std::string kind;
    std::string mode;
    std::string features;
};

ErrorOr<void> copy(fs::path const& source, fs::path const& destination)
{
    try
    {
        fs::copy(source, destination, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    }
    catch (fs::filesystem_error const& error)
    {
        THROW("{}", error.what());
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

void replace_file_name_wildcards(fs::path const& path, std::unordered_map<std::string, std::string> const& wildcards)
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

void replace_file_wildcards(fs::path const& path, std::unordered_map<std::string, std::string> const& wildcards)
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

std::string setup_name(Context const& context)
{
    return context.arguments["name"].as<std::string>();
}

ErrorOr<std::string> setup_language(Context const& context)
{
    ErrorOr<std::string> language = context.arguments["language"].as<std::string>();
    if (!context.configuration["languages"].contains(*language))
        THROW("Language \"{}\" isn't supported.", *language);
    return language;
}

ErrorOr<std::string> setup_standard(Context const& context, Project const& project)
{
    ErrorOr<std::string> standard = context.arguments["standard"].as<std::string>();
    auto const& standards = context.configuration["languages"][project.language]["standards"];
    if (standard == "latest") return standards.front().get<std::string>();
    if (std::find(standards.begin(), standards.end(), *standard) == standards.end())
        THROW("Standard \"{}\" is not available for {}.", *standard, project.language);
    return standard;
}

ErrorOr<std::string> setup_kind(Context const& context, Project const& project)
{
    ErrorOr<std::string> kind = context.arguments["kind"].as<std::string>();
    if (!context.configuration["languages"][project.language]["templates"].contains(*kind))
        THROW("Kind \"{}\" is not available for {}.", *kind, project.language);
    return kind;
}

ErrorOr<std::string> setup_mode(Context const& context, Project const& project)
{
    ErrorOr<std::string> mode = context.arguments["mode"].as<std::string>();
    if (!context.configuration["languages"][project.language]["templates"][project.kind]["modes"].contains(*mode))
        THROW("Template kind \"{}\" in mode \"{}\" is not available for {}.", project.kind, *mode, project.language);
    return mode;
}

std::string setup_features(Context const& context)
{
    if (context.arguments["features"].count())
        return fplus::join(","s, context.arguments["features"].as<std::vector<std::string>>());
    else
        return "";
}

ErrorOr<Project> setup_project(Context const& context)
{
    ErrorOr<Project> project {};
    project->name = setup_name(context);
    project->language = TRY(setup_language(context));
    project->standard = TRY(setup_standard(context, *project));
    project->kind = TRY(setup_kind(context, *project));
    project->mode = TRY(setup_mode(context, *project));
    project->features = setup_features(context);
    return project;
}

ErrorOr<void> install_project_features(Project const& project)
{
    for (auto const& feature : fplus::split(',', false, project.features))
    {
        auto const featurePath = fmt::format("{}/{}", get_features_dir(), feature);
        if (fs::is_directory(featurePath)) TRY(copy(featurePath, project.name));
        else fmt::println(PREFIX_WARN": Feature \"{}\" is unavailable.", feature);
    }
    return {};
}

ErrorOr<void> preprocess_project_files(std::filesystem::path const& path, libpreprocessor::PreprocessorContext const& context)
{
    auto iterator = fs::recursive_directory_iterator(path) | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); });
    for (auto const& entry : iterator)
    {
        auto const content = TRY(preprocess(entry.path(), context));
        std::ofstream outputStream(entry.path());
        outputStream << content;
    }

    return {};
}

ErrorOr<void> create_project(cxxopts::ParseResult const& arguments, nlohmann::json const& configuration)
{
    auto const project = TRY(setup_project({ arguments, configuration }));

    if (std::filesystem::exists(project.name)) THROW(PREFIX_ERROR": Directory \"{}\" already exists.", project.name);
    TRY(copy(fmt::format("{}/common", get_templates_dir()), project.name));
    if (!project.features.empty()) TRY(install_project_features(project));

    TRY(preprocess_project_files(project.name, PreprocessorContext {
        .environmentVariables = {
            { "ENV:LANGUAGE", project.language },
            { "ENV:STANDARD", project.standard },
            { "ENV:KIND", project.kind },
            { "ENV:MODE", project.mode },
            { "ENV:FEATURES", project.features },
        }
    }));

    std::unordered_map<std::string, std::string> const wildcards {
        { configuration["wildcards"]["name"], project.name },
        { configuration["wildcards"]["language"], project.language },
        { configuration["wildcards"]["standard"], project.standard },
    };

    replace_file_name_wildcards(project.name, wildcards);
    replace_file_wildcards(project.name, wildcards);

    return {};
}

ErrorOr<json> parse_configuration()
{
    auto const configPath = fmt::format("{}/pmake-info.json", get_templates_dir());
    ErrorOr<json> const config = json::parse(std::ifstream(configPath), nullptr, false);
    if (config->is_discarded()) THROW(PREFIX_ERROR": Couldn't open {}.", configPath);
    return config;
}

ErrorOr<void> pmake_main(std::span<char const*> const& arguments)
{
    Options options("pmake");
    options.add_options()("h,help", "");
    options.add_options()("n,name", "", value<std::string>()->default_value("my_project"));
    options.add_options()("l,language", "", value<std::string>()->default_value("c++"));
    options.add_options()("s,standard", "", value<std::string>()->default_value("latest"));
    options.add_options()("k,kind", "", value<std::string>()->default_value("executable"));
    options.add_options()("m,mode", "", value<std::string>()->default_value("console"));
    options.add_options()("features", "", value<std::vector<std::string>>());

    auto const parsedArguments = options.parse(static_cast<int>(arguments.size()), arguments.data());
    auto const parsedConfiguration = TRY(parse_configuration());

    if (parsedArguments.arguments().empty() || parsedArguments.count("help"))
    {
        fmt::print("{}", options.help());
        return {};
    }

    TRY(create_project(parsedArguments, parsedConfiguration));

    return {};
}

int main(int argc, char const** argv)
{
    auto const result = pmake_main({ argv, size_t(argc) });

    if (!result.has_value())
    {
        fmt::println("{}", result.error().message());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
