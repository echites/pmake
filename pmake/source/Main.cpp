#include "pmake/PMake.hpp"

#include <print>

int main(int count, char const** arguments)
{
    pmake::PMake program {};
    auto const result = program.run({ arguments, size_t(count) });

    if (result.has_error())
    {
        std::println("[Runtime/Error]: {}", result.error().message());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
