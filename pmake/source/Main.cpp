#include "pmake/program/Information.hpp"
#include "pmake/Project.hpp"

#include <cxxopts.hpp>
#include <fplus/fplus.hpp>
#include <fplus/result.hpp>
#include <liberror/ErrorOr.hpp>
#include <libpreprocessor/Preprocessor.hpp>
#include <nlohmann/json.hpp>

#define ERROR(fmt, ...) make_error(PREFIX_ERROR ": " fmt __VA_OPT__(,) __VA_ARGS__)

using namespace liberror;
using namespace libpreprocessor;
using namespace nlohmann;
using namespace cxxopts;

ErrorOr<json> pmake_parse_configuration()
{
    auto const configPath = fmt::format("{}/pmake-info.json", pmake::program::get_templates_dir());
    ErrorOr<json> const config = json::parse(std::ifstream(configPath), nullptr, false);
    if (config->is_discarded())
        return make_error(PREFIX_ERROR": Couldn't open {}.", configPath);
    return config;
}

ErrorOr<void> pmake_main(std::span<char const*> const& arguments)
{
    Options options("pmake");
    options.add_options()("h,help", "");
    options.add_options()("n,name", "", value<std::string>());
    options.add_options()("l,language", "", value<std::string>()->default_value("c++"));
    options.add_options()("s,standard", "", value<std::string>()->default_value("latest"));
    options.add_options()("k,kind", "", value<std::string>()->default_value("executable"));
    options.add_options()("m,mode", "", value<std::string>()->default_value("console"));
    options.add_options()("features", "", value<std::vector<std::string>>());

    auto const parsedArguments = options.parse(static_cast<int>(arguments.size()), arguments.data());
    auto const parsedConfiguration = TRY(pmake_parse_configuration());

    if (parsedArguments.count("help"))
    {
        fmt::print("{}", options.help());
        return {};
    }

    auto const project = TRY(pmake::setup_project({ parsedArguments, parsedConfiguration }));
    TRY(pmake::create_project(project));

    return {};
}

int main(int argc, char const** argv)
{
    auto const result = pmake_main({ argv, size_t(argc) });

    if (!result.has_value())
    {
        fmt::println("{}", result.error().message());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
