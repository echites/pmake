#include "filesystem/preprocessor/Preprocessor.hpp"

#include <filesystem>
#include <fstream>
#include <print>

namespace pmake::preprocessor {

liberror::ErrorOr<void> process_all(std::filesystem::path path, libpreprocessor::PreprocessorContext const& context)
{
    namespace fs = std::filesystem;

    auto static constexpr fileSizeThreshold = 8000;

    for (auto const& entry : fs::recursive_directory_iterator(path))
    {
        if (!entry.is_regular_file() || entry.file_size() >= fileSizeThreshold) { continue; }

        auto const result = TRY(libpreprocessor::preprocess(entry.path(), context));

        std::ofstream outputStream { entry.path() };
        outputStream << result;
    }

    return {};
}

}
