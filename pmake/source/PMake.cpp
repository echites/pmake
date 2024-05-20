#include "PMake.hpp"

#include "files/Files.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>

namespace pmake {

using namespace liberror;
using namespace libpreprocessor;
using namespace nlohmann;

namespace fs = std::filesystem;

#define ERROR(fmt, ...) make_error(PREFIX_ERROR ": " fmt __VA_OPT__(,) __VA_ARGS__)

namespace internal {

static void copy(fs::path source, fs::path destination);
static void print_project_information(Project const& project);

}

ErrorOr<Language> PMake::setup_language() const
{
    ErrorOr<Language> result {};

    auto const& languages = info_m["languages"];
    result->first = TRY([&] -> ErrorOr<std::string> {
        ErrorOr<std::string> language = parsed_m["language"].as<std::string>();
        if (!languages.contains(*language))
            return ERROR("Language \"{}\" isn't supported.", *language);
        return language;
    }());

    auto const& standards = languages[result->first]["standards"];
    result->second = TRY([&] -> ErrorOr<std::string> {
        ErrorOr<std::string> standard = parsed_m["standard"].as<std::string>();
        if (standard == "latest") return standards.front().get<std::string>();
        if (std::find(standards.begin(), standards.end(), *standard) == standards.end())
            return ERROR("Standard \"{}\" is not available for {}.", *standard, result->first);
        return standard;
    }());

    return result;
}

ErrorOr<Kind> PMake::setup_kind(Project const& project) const
{
    auto const& language = project.language.first;

    ErrorOr<Kind> result {};

    auto const& templates = info_m["languages"][language]["templates"];
    result->first = TRY([&] -> ErrorOr<std::string> {
        ErrorOr<std::string> kind = parsed_m["kind"].as<std::string>();
        if (!templates.contains(*kind))
            return ERROR("Kind \"{}\" is not available for {}.", *kind, language);
        return kind;
    }());

    auto const& modes = templates[result->first]["modes"];
    result->second = TRY([&] -> ErrorOr<std::string> {
        ErrorOr<std::string> mode = parsed_m["mode"].as<std::string>();
        if (!modes.contains(*mode))
            return ERROR("Template kind \"{}\" in mode \"{}\" is not available for {}.", result->first, *mode, language);
        return mode;
    }());

    return result;
}

Features PMake::setup_features() const
{
    Features result {};

    if (!parsed_m["features"].count()) return result;

    for (std::string_view separator = ""; auto const& feature : parsed_m["features"].as<std::vector<std::string>>())
    {
        result.append(fmt::format("{}{}", separator, feature));
        separator = ",";
    }

    return result;
}

Wildcards PMake::setup_wildcards(Project const& project) const
{
    return {
        { info_m["wildcards"]["name"]    , project.name },
        { info_m["wildcards"]["language"], project.language.first },
        { info_m["wildcards"]["standard"], project.language.second },
    };
}

void PMake::install_features(Project const& project, fs::path destination) const
{
    auto const& language     = project.language.first;
    auto const& [kind, mode] = project.kind;

    auto const& features = info_m["languages"][language]["templates"][kind]["modes"][mode]["features"];

    auto const fnIsAvailable = [&] (auto&& feature) {
        auto const result = std::find(features.begin(), features.end(), feature) != features.end();
        if (!result)
            fmt::println(PREFIX_WARN": Feature \"{}\" is unavailable.", feature);
        return result;
    };

    for (auto const& feature :
            parsed_m["features"].as<std::vector<std::string>>()
                | std::views::filter(fnIsAvailable))
    {
        internal::copy(fmt::format("{}/{}", features_dir(), feature), destination);
    }
}

ErrorOr<void> PMake::create_project(Project const& project, Wildcards const& wildcards) const
{
    auto const& destination = project.name;
    if (fs::exists(destination))
        return ERROR("Directory \"{}\" already exists.", destination);

    internal::copy(fmt::format("{}/common", templates_dir()), destination);

    if (parsed_m["features"].count()) install_features(project, destination);

    TRY(preprocess_files(destination, PreprocessorContext {
        .environmentVariables = {
            { "ENV:LANGUAGE", project.language.first },
            { "ENV:STANDARD", project.language.second },
            { "ENV:KIND"    , project.kind.first },
            { "ENV:MODE"    , project.kind.second },
            { "ENV:FEATURES", project.features },
        }
    }));

    replace_filenames(destination, wildcards);
    replace_contents(destination, wildcards);

    return {};
}

void PMake::save_project_info_as_json(Project const& project) const
{
    std::ofstream(fmt::format("{}/.pmake-project", project.name)) << json {
        { "project" , project.name },
        { "language", { project.language.first, project.language.second } },
        { "kind"    , { project.kind.first, project.kind.second } },
        { "features", parsed_m["features"].count() ? parsed_m["features"].as<std::vector<std::string>>() : std::vector<std::string>{} }
    };

    internal::print_project_information(project);
}

ErrorOr<void> PMake::upgrade_project() const
{
    assert(false && "NOT IMPLEMENTED");
    return {};
}

ErrorOr<void> PMake::run(std::span<char const*> arguments)
{
    parsed_m = options_m.parse(static_cast<int>(arguments.size()), arguments.data());

    if (parsed_m.arguments().empty()) return make_error(options_m.help());
    if (parsed_m.count("help"))       return make_error(options_m.help());
    if (parsed_m.count("upgrade"))    return upgrade_project();

    Project project {};
    project.name         = TRY(setup_name());
    project.language     = TRY(setup_language());
    project.kind         = TRY(setup_kind(project));
    project.features     = setup_features();
    auto const wildcards = setup_wildcards(project);

    TRY(create_project(project, wildcards));

    save_project_info_as_json(project);

    return {};
}

}

namespace pmake::internal {

void copy(fs::path source, fs::path destination)
{
    fs::copy(source, destination, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
}

void print_project_information(Project const& project)
{
    fmt::println("┌– [pmake] –––");
    fmt::println("| name.......: {}"     , project.name);
    fmt::println("| language...: {} ({})", project.language.first, project.language.second);
    fmt::println("| kind.......: {} ({})", project.kind.first, project.kind.second);
    fmt::println("| features...: [{}]"   , project.features);
    fmt::println("└–––––––––––––");
}

}

