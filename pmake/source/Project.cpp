#include "pmake/Project.hpp"

#include "pmake/program/Context.hpp"
#include "pmake/program/Information.hpp"
#include "pmake/File.hpp"

#include <cxxopts.hpp>
#include <fplus/fplus.hpp>
#include <liberror/ErrorOr.hpp>
#include <libpreprocessor/Preprocessor.hpp>
#include <nlohmann/json.hpp>

#define ERROR(fmt, ...) make_error(PREFIX_ERROR ": " fmt __VA_OPT__(,) __VA_ARGS__)

namespace fs = std::filesystem;

using namespace liberror;
using namespace libpreprocessor;
using namespace std::literals;

ErrorOr<std::string> setup_name(pmake::program::Context const& context)
{
    return context.arguments["name"].as<std::string>();
}

ErrorOr<std::string> setup_language(pmake::program::Context const& context)
{
    ErrorOr<std::string> language = context.arguments["language"].as<std::string>();
    if (!context.configuration["languages"].contains(*language))
        return ERROR("Language \"{}\" isn't supported.", *language);
    return language;
}

ErrorOr<std::string> setup_standard(pmake::program::Context const& context, pmake::Project const& project)
{
    ErrorOr<std::string> standard = context.arguments["standard"].as<std::string>();
    auto const& standards = context.configuration["languages"][project.language]["standards"];
    if (standard == "latest") return standards.front().get<std::string>();
    if (std::find(standards.begin(), standards.end(), *standard) == standards.end())
        return ERROR("Standard \"{}\" is not available for {}.", *standard, project.language);
    return standard;
}

ErrorOr<std::string> setup_kind(pmake::program::Context const& context, pmake::Project const& project)
{
    ErrorOr<std::string> kind = context.arguments["kind"].as<std::string>();
    if (!context.configuration["languages"][project.language]["templates"].contains(*kind))
        return ERROR("Kind \"{}\" is not available for {}.", *kind, project.language);
    return kind;
}

ErrorOr<std::string> setup_mode(pmake::program::Context const& context, pmake::Project const& project)
{
    ErrorOr<std::string> mode = context.arguments["mode"].as<std::string>();
    if (!context.configuration["languages"][project.language]["templates"][project.kind]["modes"].contains(*mode))
        return ERROR("Template kind \"{}\" in mode \"{}\" is not available for {}.", project.kind, *mode, project.language);
    return mode;
}

std::string setup_features(pmake::program::Context const& context)
{
    if (context.arguments["features"].count())
        return fplus::join(","s, context.arguments["features"].as<std::vector<std::string>>());
    else
        return "";
}

std::unordered_map<std::string, std::string> setup_wildcards(pmake::program::Context const& context, pmake::Project const& project)
{
    return {
        { context.configuration["wildcards"]["name"], project.name },
        { context.configuration["wildcards"]["language"], project.language },
        { context.configuration["wildcards"]["standard"], project.standard },
    };
}

ErrorOr<pmake::Project> pmake::setup_project(pmake::program::Context const& context)
{
    ErrorOr<pmake::Project> project {};
    project->name = TRY(setup_name(context));
    project->language = TRY(setup_language(context));
    project->standard = TRY(setup_standard(context, *project));
    project->kind = TRY(setup_kind(context, *project));
    project->mode = TRY(setup_mode(context, *project));
    project->features = setup_features(context);
    project->wildcards = setup_wildcards(context, *project);
    return project;
}

ErrorOr<void> install_project_features(pmake::Project const& project)
{
    for (auto const& feature : fplus::split(',', false, project.features))
    {
        auto const featurePath = fmt::format("{}/{}", pmake::program::get_features_dir(), feature);
        if (fs::is_directory(featurePath))
            TRY(pmake::file::copy(featurePath, project.name));
        else
            fmt::println(PREFIX_WARN": Feature \"{}\" is unavailable.", feature);
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

ErrorOr<void> pmake::create_project(pmake::Project const& project)
{
    if (std::filesystem::exists(project.name))
    {
        return make_error(PREFIX_ERROR": Directory \"{}\" already exists.", project.name);
    }

    TRY(pmake::file::copy(fmt::format("{}/common", pmake::program::get_templates_dir()), project.name));

    if (!project.features.empty())
    {
        TRY(install_project_features(project));
    }

    TRY(preprocess_project_files(project.name, PreprocessorContext {
        .environmentVariables = {
            { "ENV:LANGUAGE", project.language },
            { "ENV:STANDARD", project.standard },
            { "ENV:KIND", project.kind },
            { "ENV:MODE", project.mode },
            { "ENV:FEATURES", project.features },
        }
    }));

    pmake::file::replace_file_name_wildcards(project.name, project.wildcards);
    pmake::file::replace_file_wildcards(project.name, project.wildcards);

    return {};
}
