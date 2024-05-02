#include "PMake.hpp"

#include "files/Files.hpp"
#include "files/preprocessor/Preprocessor.hpp"

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

ErrorOr<std::pair<std::string, std::string>> PMake::setup_language() const
{
    ErrorOr<std::pair<std::string, std::string>> result;

    auto const& languages = info_m["languages"];
    result->first = TRY([&] -> ErrorOr<std::string> {
        ErrorOr<std::string> language { parsed_m["language"].as<std::string>() };
        if (!languages.contains(*language))
            return make_error(PREFIX_ERROR": Language \"{}\" isn't supported.", *language);
        return language;
    }());

    auto const& standards = languages[result->first]["standards"];
    result->second = TRY([&] -> ErrorOr<std::string> {
        ErrorOr<std::string> standard { parsed_m["standard"].as<std::string>() };
        if (standard == "latest") return standards.front().get<std::string>();
        if (std::find(standards.begin(), standards.end(), *standard) == standards.end())
            return make_error(PREFIX_ERROR": Standard \"{}\" is not available for {}.", *standard, result->first);
        return standard;
    }());

    return result;
}

ErrorOr<std::pair<std::string, std::string>> PMake::setup_kind(Project const& project) const
{
    ErrorOr<std::pair<std::string, std::string>> result;

    auto const& language = project.language.first;

    auto const& templates = info_m["languages"][language]["templates"];
    result->first = TRY([&] -> ErrorOr<std::string> {
        ErrorOr<std::string> kind { parsed_m["kind"].as<std::string>() };
        if (!templates.contains(*kind))
            return make_error(PREFIX_ERROR": Kind \"{}\" is not available for {}.", *kind, language);
        return kind;
    }());

    auto const& modes = templates[result->first]["modes"];
    result->second = TRY([&] -> ErrorOr<std::string> {
        ErrorOr<std::string> mode { parsed_m["mode"].as<std::string>() };
        if (!modes.contains(*mode))
            return make_error(PREFIX_ERROR": Template kind \"{}\" in mode \"{}\" is not available for {}.", result->first, *mode, language);
        return mode;
    }());

    return result;
}

std::string PMake::setup_features() const
{
    std::string result;

    for (std::string_view separator = ""; auto const& feature : parsed_m["features"].as<std::vector<std::string>>())
    {
        std::ranges::copy(fmt::format("{}{}", separator, feature), std::back_inserter(result));
        separator = ",";
    }

    return result;
}

std::unordered_map<std::string, std::string> PMake::setup_wildcards(Project const& project) const
{
    return {
        { info_m["wildcards"]["name"], project.name },
        { info_m["wildcards"]["language"], project.language.first },
        { info_m["wildcards"]["standard"], project.language.second },
    };
}

void PMake::install_features(Project const& project, fs::path destination) const
{
    if (parsed_m["features"].has_default()) return;

    auto const& language = project.language.first;
    auto const& kind     = project.kind.first;
    auto const& mode     = project.kind.second;

    for (auto const& feature : parsed_m["features"].as<std::vector<std::string>>())
    {
        auto const& features = info_m["languages"][language]["templates"][kind]["modes"][mode]["features"];

        if (std::find(features.begin(), features.end(), feature) == features.end())
        {
            fmt::println(PREFIX_WARN": Skipping unavailable feature \"{}\".", feature);
            continue;
        }

        fs::copy(fmt::format("{}/{}", get_features_dir(), feature), destination, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    }
}

ErrorOr<void> PMake::create_project(Project const& project) const
{
    if (fs::exists(project.name)) return make_error(PREFIX_ERROR": Directory \"{}\" already exists.", project.name);

    auto const& to       = project.name;
    auto const from      = fmt::format("{}/common", get_templates_dir());
    auto const wildcards = setup_wildcards(project);

    fs::create_directory(to);
    fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    install_features(project, to);

    TRY(process_all(to, PreprocessorContext {
        .localVariables = {},
        .environmentVariables = {
            { "ENV:LANGUAGE", project.language.first },
            { "ENV:STANDARD", project.language.second },
            { "ENV:KIND", project.kind.first },
            { "ENV:MODE", project.kind.second },
            { "ENV:FEATURES", project.features },
        }
    }));

    rename_all(to, wildcards);
    replace_all(to, wildcards);

    return {};
}

void PMake::save_project_info_as_json(Project const& project) const
{
    json json {
        { "project", project.name },
        { "language", { project.language.first, project.language.second } },
        { "kind", { project.kind.first, project.kind.second } },
        { "features", parsed_m["features"].as<std::vector<std::string>>() }
    };

    std::ofstream stream { fmt::format("{}/.pmake-project", project.name) };
    stream << json;
}

void print_project_information(PMake::Project const& project)
{
    fmt::println("┌– [pmake] –––");
    fmt::println("| name.......: {}", project.name);
    fmt::println("| language...: {} ({})", project.language.first, project.language.second);
    fmt::println("| kind.......: {} ({})", project.kind.first, project.kind.second);
    fmt::println("| features...: [{}]", project.features);
    fmt::println("└–––––––––––––");
}

ErrorOr<void> PMake::run(std::span<char const*> arguments)
{
    parsed_m = options_m.parse(static_cast<int>(arguments.size()), arguments.data());

    if (parsed_m.arguments().empty()) return make_error(options_m.help());
    if (parsed_m.count("help")) return make_error(options_m.help());

    Project project {};

    project.name     = TRY(setup_name());
    project.language = TRY(setup_language());
    project.kind     = TRY(setup_kind(project));
    project.features = setup_features();

    TRY(create_project(project));

    save_project_info_as_json(project);
    print_project_information(project);

    return {};
}

}
