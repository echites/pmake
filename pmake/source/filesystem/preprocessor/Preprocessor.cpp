#include "filesystem/preprocessor/Preprocessor.hpp"

#include <filesystem>
#include <fstream>
#include <print>

namespace pmake::preprocessor {

liberror::ErrorOr<void> process_all(std::filesystem::path path, libpreprocessor::PreprocessorContext const& context)
{
    namespace fs = std::filesystem;

    for (auto const& entry : fs::recursive_directory_iterator(path))
    {
        if (!entry.is_regular_file()) { continue; }

        auto const result = TRY(libpreprocessor::preprocess(entry.path(), context));

        std::ofstream outputStream { entry.path() };
        outputStream << result;
    }

    return {};
}

}
