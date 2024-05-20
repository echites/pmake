#include "pmake/PMake.hpp"

#include <fstream>

using namespace liberror;
using namespace nlohmann;

ErrorOr<void> pmake_main(std::span<char const*> arguments)
{
    auto const fnReadJson = [] (auto&& path) -> ErrorOr<json>
    {
        ErrorOr<json> info = json::parse(std::ifstream(path), nullptr, false);
        if (info->is_discarded())
            return make_error(PREFIX_ERROR": Couldn't open {}.", path);
        return info;
    };

    auto const configPath = fmt::format("{}/pmake-info.json", pmake::templates_dir());
    auto const config     = TRY(fnReadJson(configPath));

    pmake::PMake program(config);
    TRY(program.run(arguments));

    return {};
}

int main(int count, char const** arguments)
{
    auto const result = pmake_main({ arguments, size_t(count) });

    if (!result.has_value())
    {
        fmt::println("{}", result.error().message());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
