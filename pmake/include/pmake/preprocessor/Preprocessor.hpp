#pragma once

#include "Interpreter.hpp"

namespace pmake::preprocessor {

error::ErrorOr<void> process_all(std::filesystem::path path, InterpreterContext const& context);

} // pmake::preprocessor
