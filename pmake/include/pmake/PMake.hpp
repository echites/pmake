#pragma once

#include "pmake/system/Runtime.hpp"

#include <cxxopts.hpp>
#include <liberror/ErrorOr.hpp>
#include <liberror/types/TraceError.hpp>
#include <nlohmann/json.hpp>

namespace pmake {

class PMake
{
    auto static get_root_dir() { return runtime::get_program_root_dir().string(); }
    auto static get_templates_dir() { return std::format("{}\\pmake-templates", get_root_dir()); }
    auto static get_features_dir() { return std::format("{}\\features", PMake::get_templates_dir()); }

    auto static get_info_path() { return std::format("{}\\pmake-info.json", PMake::get_templates_dir()); }

public:
    struct Project
    {
        std::string name;
        std::pair<std::string, std::string> language;
        std::pair<std::string, std::string> kind;
        std::string features;
    };

    PMake(): options_m { "pmake" }
    {
        options_m.add_options()
            ("h,help"      , "show this menu")
            ("n,name"      , "name of the project"         , cxxopts::value<std::string>())
            ("l,language"  , "language used in the project", cxxopts::value<std::string>()->default_value("c++"))
            ("k,kind"      , "kind of the project"         , cxxopts::value<std::string>()->default_value("executable"))
            ("m,mode"      , "mode of the project"         , cxxopts::value<std::string>()->default_value("console"))
            ("s,standard"  , "standard used in the project", cxxopts::value<std::string>()->default_value("latest"))
            ("features"    , "features to use in the project", cxxopts::value<std::vector<std::string>>()->default_value({}));
    }

    liberror::ErrorOr<void> run(std::span<char const*> arguments);

private:
    cxxopts::Options options_m;
    cxxopts::ParseResult parsedOptions_m;
    nlohmann::json informationJson_m;

    liberror::ErrorOr<std::string> setup_name() const;
    liberror::ErrorOr<std::pair<std::string, std::string>> setup_language();
    liberror::ErrorOr<std::pair<std::string, std::string>> setup_kind(PMake::Project const& project);
    std::string setup_features() const;
    std::unordered_map<std::string, std::string> setup_wildcards(PMake::Project const& project);
    void install_required_features(PMake::Project const& project, std::filesystem::path destination);
    liberror::ErrorOr<void> create_project(PMake::Project const& project);
};

inline liberror::ErrorOr<std::string> PMake::setup_name() const
{
    if (!parsedOptions_m.count("name"))
        return liberror::make_error(PREFIX_ERROR": You must specify a project name.");
    return parsedOptions_m["name"].as<std::string>();
}

inline std::string PMake::setup_features() const
{
    return
        parsedOptions_m["features"].as<std::vector<std::string>>()
                                   | std::views::join_with(',')
                                   | std::ranges::to<std::string>();
}

} // pmake
