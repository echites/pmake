#pragma once

#include <libpreprocessor/Preprocessor.hpp>
#include <liberror/ErrorOr.hpp>

namespace pmake {

liberror::ErrorOr<void> process_all(std::filesystem::path path, libpreprocessor::PreprocessorContext const& context);

} // pmake
