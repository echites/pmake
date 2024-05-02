#pragma once

#include "pmake/system/Runtime.hpp"

#include <cxxopts.hpp>
#include <fmt/format.h>
#include <liberror/ErrorOr.hpp>
#include <nlohmann/json.hpp>

namespace pmake {

auto inline get_root_dir() { return get_program_root_dir().string(); }
auto inline get_templates_dir() { return fmt::format("{}/pmake-templates", get_root_dir()); }
auto inline get_features_dir() { return fmt::format("{}/features", get_templates_dir()); }
auto inline get_pmake_info_path() { return fmt::format("{}/pmake-info.json", get_templates_dir()); }

class PMake
{
public:
    struct Project
    {
        std::string name;
        std::pair<std::string, std::string> language;
        std::pair<std::string, std::string> kind;
        std::string features;
    };

    explicit PMake(nlohmann::json const& pmakeInfo)
        : options_m("pmake")
        , info_m(pmakeInfo)
    {
        options_m.add_options()("h,help"    , "show this menu");
        options_m.add_options()("n,name"    , "name of the project"         , cxxopts::value<std::string>());
        options_m.add_options()("l,language", "language used in the project", cxxopts::value<std::string>()->default_value("c++"));
        options_m.add_options()("k,kind"    , "kind of the project"         , cxxopts::value<std::string>()->default_value("executable"));
        options_m.add_options()("m,mode"    , "mode of the project"         , cxxopts::value<std::string>()->default_value("console"));
        options_m.add_options()("s,standard", "standard used in the project", cxxopts::value<std::string>()->default_value("latest"));
        options_m.add_options()("upgrade"   , "upgrade the project in the current working directory");
        options_m.add_options()("features"  , "features to use in the project", cxxopts::value<std::vector<std::string>>()->default_value({}));
    }

    liberror::ErrorOr<void> run(std::span<char const*> arguments);

private:
    cxxopts::Options options_m;
    cxxopts::ParseResult parsed_m;
    nlohmann::json info_m;

    liberror::ErrorOr<std::string> setup_name() const;
    liberror::ErrorOr<std::pair<std::string, std::string>> setup_language() const;
    liberror::ErrorOr<std::pair<std::string, std::string>> setup_kind(Project const& project) const;
    std::string setup_features() const;
    std::unordered_map<std::string, std::string> setup_wildcards(Project const& project) const;
    void install_features(Project const& project, std::filesystem::path destination) const;
    liberror::ErrorOr<void> create_project(Project const& project) const;
    void save_project_info_as_json(Project const& project) const;
};

inline liberror::ErrorOr<std::string> PMake::setup_name() const
{
    return parsed_m["name"].as<std::string>();
}

} // pmake
