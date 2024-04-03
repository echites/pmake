#include "PMake.hpp"

#include "filesystem/Files.hpp"
#include "filesystem/preprocessor/Preprocessor.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <algorithm>
#include <fstream>
#include <filesystem>

namespace pmake {

using namespace liberror;
using namespace libpreprocessor;

void print_project_information(PMake::Project const& project)
{
    std::println("┌– [pmake] –––");
    std::println("| name.......: {}", project.name);
    std::println("| language...: {} ({})", project.language.first, project.language.second);
    std::println("| kind.......: {} ({})", project.kind.first, project.kind.second);
    std::println("| features...: [{}]", project.features);
    std::println("└–––––––––––––");
}

ErrorOr<std::string> PMake::setup_name()
{
    if (!parsedOptions_m.count("name"))
        return make_error(PREFIX_ERROR": You must specify a project name.");
    return parsedOptions_m["name"].as<std::string>();
}

ErrorOr<std::pair<std::string, std::string>> PMake::setup_language()
{
    using namespace nlohmann;

    auto const informationJsonPath = std::format("{}\\pmake-info.json", PMake::get_templates_dir());
    auto const informationJson     = json::parse(std::ifstream { informationJsonPath }, nullptr, true);

    if (informationJson.is_discarded()) return make_error(PREFIX_ERROR": Couldn't open {}.", informationJsonPath);

    auto language = TRY([&] -> ErrorOr<std::string> {
        // cppcheck-suppress shadowVariable
        auto const language = parsedOptions_m["language"].as<std::string>();
        if (!informationJson["languages"].contains(language))
            return make_error(PREFIX_ERROR": Language \"{}\" isn't supported.", language);
        return language;
    }());

    auto standard = TRY([&] -> ErrorOr<std::string> {
        // cppcheck-suppress shadowVariable
        auto const standard  = parsedOptions_m["standard"].as<std::string>();
        auto const standards = informationJson["languages"][language]["standards"];
        if (standard == "latest")
            return informationJson["languages"][language]["standards"].front().get<std::string>();
        if (std::find(standards.begin(), standards.end(), standard) == standards.end())
            return make_error(PREFIX_ERROR": Standard \"{}\" is not available for {}.", standard, language);
        return standard;
    }());

    return {{ language, standard }};
}

ErrorOr<std::pair<std::string, std::string>> PMake::setup_kind(PMake::Project const& project)
{
    using namespace nlohmann;

    auto const informationJsonPath = std::format("{}\\pmake-info.json", PMake::get_templates_dir());
    auto const informationJson     = json::parse(std::ifstream { informationJsonPath }, nullptr, true);

    if (informationJson.is_discarded()) return make_error(PREFIX_ERROR": Couldn't open {}.", informationJsonPath);

    auto const& language = project.language.first;

    auto kind = TRY([&] -> ErrorOr<std::string> {
        // cppcheck-suppress shadowVariable
        auto const kind  = parsedOptions_m["kind"].as<std::string>();
        if (!informationJson["languages"][language]["templates"].contains(kind))
            return make_error(PREFIX_ERROR": Kind \"{}\" is not available for {}.", kind, language);
        return kind;
    }());

    auto mode = TRY([&] -> ErrorOr<std::string> {
        // cppcheck-suppress shadowVariable
        auto const mode  = parsedOptions_m["mode"].as<std::string>();
        if (!informationJson["languages"][language]["templates"][kind]["modes"].contains(mode))
            return make_error(PREFIX_ERROR": Template kind \"{}\" mode \"{}\" is not available for {}.", kind, mode, language);
        return mode;
    }());

    return {{ kind, mode }};
}

std::string PMake::setup_features()
{
    return parsedOptions_m["features"].as<std::vector<std::string>>()
                | std::views::join_with(',')
                | std::ranges::to<std::string>();
}

std::unordered_map<std::string, std::string> PMake::setup_wildcards(PMake::Project const& project)
{
    using namespace nlohmann;

    std::unordered_map<std::string, std::string> wildcards {
        { informationJson_m["wildcards"]["name"], project.name },
        { informationJson_m["wildcards"]["language"], project.language.first },
        { informationJson_m["wildcards"]["standard"], project.language.second },
    };

    return wildcards;
}

void PMake::install_required_features(PMake::Project const& project, std::filesystem::path destination)
{
    namespace fs = std::filesystem;

    if (parsedOptions_m["features"].has_default()) return;

    auto const& language = project.language.first;
    auto const& kind     = project.kind.first;
    auto const& mode     = project.kind.second;

    for (auto const& feature : parsedOptions_m["features"].as<std::vector<std::string>>())
    {
        auto const& features = informationJson_m["languages"][language]["templates"][kind]["modes"][mode]["features"];

        if (std::find(features.begin(), features.end(), feature) == features.end())
        {
            std::println(PREFIX_WARN": Skipping unavailable feature \"{}\".", feature);
            continue;
        }

        fs::copy(std::format("{}\\{}", PMake::get_features_dir(), feature), destination, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    }
}

ErrorOr<void> PMake::create_project(PMake::Project const& project)
{
    namespace fs = std::filesystem;

    if (fs::exists(project.name)) return make_error(PREFIX_ERROR": There already is a directory named {} in the current working directory.", project.name);

    auto const& to       = project.name;
    auto const from      = std::format("{}\\common", PMake::get_templates_dir());
    auto const wildcards = setup_wildcards(project);

    fs::create_directory(to);
    fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    install_required_features(project, to);

    TRY(filesystem::process_all(to, PreprocessorContext {
        .localVariables = {},
        .environmentVariables = {
            { "ENV:LANGUAGE", project.language.first },
            { "ENV:STANDARD", project.language.second },
            { "ENV:KIND", project.kind.first },
            { "ENV:MODE", project.kind.second },
            { "ENV:FEATURES", project.features },
        }
    }));

    filesystem::rename_all(to, wildcards);
    filesystem::replace_all(to, wildcards);

    return {};
}

ErrorOr<void> PMake::run(std::span<char const*> arguments)
{
    using namespace nlohmann;

    parsedOptions_m = options_m.parse(int(arguments.size()), arguments.data());
    if (parsedOptions_m.arguments().empty()) return make_error(options_m.help());
    if (parsedOptions_m.count("help")) return make_error(options_m.help());

    informationJson_m = json::parse(std::ifstream { PMake::get_info_path() }, nullptr, false);
    if (informationJson_m.is_discarded()) return make_error(PREFIX_ERROR": Couldn't open {}.", PMake::get_info_path());

    PMake::Project project {};

    project.name     = TRY(setup_name());
    project.language = TRY(setup_language());
    project.kind     = TRY(setup_kind(project));
    project.features = setup_features();

    TRY(PMake::create_project(project));

    print_project_information(project);

    return {};
}

}
