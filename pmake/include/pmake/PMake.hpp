#pragma once

#include "pmake/system/Runtime.hpp"

#include <cxxopts.hpp>
#include <fmt/format.h>
#include <liberror/ErrorOr.hpp>
#include <nlohmann/json.hpp>

namespace pmake {

using Name      = std::string;
using Language  = std::pair<std::string, std::string>;
using Kind      = std::pair<std::string, std::string>;
using Features  = std::string;
using Wildcards = std::unordered_map<std::string, std::string>;

struct Project
{
    Name name;
    Language language;
    Kind kind;
    Features features;
};

auto inline get_root_dir() { return get_program_root_dir().string(); }
auto inline get_templates_dir() { return fmt::format("{}/pmake-templates", get_root_dir()); }
auto inline get_features_dir() { return fmt::format("{}/features", get_templates_dir()); }
auto inline get_pmake_info_path() { return fmt::format("{}/pmake-info.json", get_templates_dir()); }

class PMake
{
public:
    explicit PMake(nlohmann::json const& pmakeInfo)
        : options_m("pmake")
        , info_m(pmakeInfo)
    {
        options_m.add_options()("h,help"    , "");
        options_m.add_options()("n,name"    , "name of the project"         , cxxopts::value<std::string>());
        options_m.add_options()("l,language", "language used in the project", cxxopts::value<std::string>()->default_value("c++"));
        options_m.add_options()("k,kind"    , "kind of the project"         , cxxopts::value<std::string>()->default_value("executable"));
        options_m.add_options()("m,mode"    , "mode of the project"         , cxxopts::value<std::string>()->default_value("console"));
        options_m.add_options()("s,standard", "standard used in the project", cxxopts::value<std::string>()->default_value("latest"));
        options_m.add_options()("upgrade"   , "upgrade current project");
        options_m.add_options()("features"  , "features to use in the project", cxxopts::value<std::vector<std::string>>());
    }

    liberror::ErrorOr<void> run(std::span<char const*> arguments);

private:
    cxxopts::Options options_m;
    cxxopts::ParseResult parsed_m;
    nlohmann::json info_m;

    liberror::ErrorOr<Name> setup_name() const { return parsed_m["name"].as<std::string>(); }
    liberror::ErrorOr<Language> setup_language() const;
    liberror::ErrorOr<Kind> setup_kind(Project const& project) const;
    Features setup_features() const;
    Wildcards setup_wildcards(Project const& project) const;
    void install_features(Project const& project, std::filesystem::path destination) const;
    liberror::ErrorOr<void> create_project(Project const& project) const;
    void save_project_info_as_json(Project const& project) const;
    // cppcheck-suppress functionStatic
    liberror::ErrorOr<void> upgrade_project() const;
};

} // pmake
