#pragma once

#include <libpreprocessor/Preprocessor.hpp>
#include <liberror/ErrorOr.hpp>

namespace pmake::filesystem {

liberror::ErrorOr<void> process_all(std::filesystem::path path, libpreprocessor::PreprocessorContext const& context);

} // pmake::preprocessor
