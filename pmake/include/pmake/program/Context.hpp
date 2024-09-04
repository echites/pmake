#pragma once

#include <cxxopts.hpp>
#include <nlohmann/json.hpp>

namespace pmake::program {

struct Context
{
    cxxopts::ParseResult const& arguments;
    nlohmann::json const& configuration;
};

}
