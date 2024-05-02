#include "pmake/PMake.hpp"
#include "pmake/files/Files.hpp"

using namespace liberror;
using namespace nlohmann;

ErrorOr<void> pmake_main(std::span<char const*> arguments)
{
    pmake::PMake program { TRY(pmake::read_json(pmake::get_pmake_info_path())) };
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
