#include "pmake/PMake.hpp"

int main(int count, char const** arguments)
{
    pmake::PMake program {};
    auto const result = program.run({ arguments, size_t(count) });

    if (!result.has_value())
    {
        fmt::println("{}", result.error().message());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
