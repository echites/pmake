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

    pmake::PMake program { TRY(fnReadJson(pmake::get_pmake_info_path())) };
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
