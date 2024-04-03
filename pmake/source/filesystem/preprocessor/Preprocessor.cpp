#include "filesystem/preprocessor/Preprocessor.hpp"

#include <filesystem>
#include <fstream>
#include <print>

namespace pmake::filesystem {

using namespace liberror;
using namespace libpreprocessor;

namespace fs = std::filesystem;

ErrorOr<void> process_all(fs::path path, PreprocessorContext const& context)
{
    for (auto const& entry : fs::recursive_directory_iterator(path))
    {
        if (!entry.is_regular_file()) { continue; }

        auto const result = TRY(preprocess(entry.path(), context));

        std::ofstream outputStream { entry.path() };
        outputStream << result;
    }

    return {};
}

}
