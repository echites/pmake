#pragma once

#include "pmake/system/Runtime.hpp"

#include <err_or/ErrorOr.hpp>
#include <err_or/types/TraceError.hpp>
#include <cxxopts.hpp>

namespace pmake {

class PMake
{
    auto get_root_dir() const { return runtime::get_program_root_dir().string(); }
    auto get_templates_dir() const { return std::format("{}\\pmake-templates", this->get_root_dir()); }

public:
    struct Project
    {
        std::string name;
        std::pair<std::string, std::string> language;
        std::pair<std::string, std::string> kind;
    };

    PMake(): options_m("pmake")
    {
        options_m.add_options()
            ("h,help"      , "show this menu")
            ("n,name"      , "name of the project"         , cxxopts::value<std::string>())
            ("l,language"  , "language used in the project", cxxopts::value<std::string>()->default_value("c++"))
            ("k,kind"      , "kind of the project"         , cxxopts::value<std::string>()->default_value("executable"))
            ("m,mode"      , "mode of the project"         , cxxopts::value<std::string>()->default_value("console"))
            ("s,standard"  , "standard used in the project", cxxopts::value<std::string>()->default_value("latest"));
    }

    error::ErrorOr<void> run(std::span<char const*> arguments);

private:
    cxxopts::Options options_m;
    cxxopts::ParseResult parsedOptions_m;

    error::ErrorOr<std::string> setup_name();
    error::ErrorOr<std::pair<std::string, std::string>> setup_language();
    error::ErrorOr<std::pair<std::string, std::string>> setup_kind(PMake::Project const& project);
    error::ErrorOr<std::unordered_map<std::string, std::string>> setup_wildcards(PMake::Project const& project);
    error::ErrorOr<void> create_project(PMake::Project const& project);
};

} // pmake
