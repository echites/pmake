#include "files/preprocessor/Preprocessor.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>

namespace pmake {

using namespace liberror;
using namespace libpreprocessor;

namespace fs = std::filesystem;

ErrorOr<void> process_all(fs::path path, PreprocessorContext const& context)
{
    auto iterator =
        fs::recursive_directory_iterator(path)
            | std::views::filter([] (auto&& entry) { return fs::is_regular_file(entry); });

    for (auto const& entry : iterator)
    {
        std::ofstream(entry.path()) << TRY(preprocess(entry.path(), context));
    }

    return {};
}

}
