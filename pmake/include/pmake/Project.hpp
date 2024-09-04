#pragma once

#include "program/Context.hpp"

#include <liberror/ErrorOr.hpp>

#include <string>

namespace pmake {

struct Project
{
    std::string name;
    std::string language;
    std::string standard;
    std::string kind;
    std::string mode;
    std::string features;
    std::unordered_map<std::string, std::string> wildcards;
};

liberror::ErrorOr<Project> setup_project(program::Context const& context);
liberror::ErrorOr<void> create_project(Project const& project);

}
