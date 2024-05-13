#pragma once

#include <liberror/ErrorOr.hpp>
#include <libpreprocessor/Preprocessor.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <unordered_map>

namespace pmake {

using Wildcards = std::unordered_map<std::string, std::string>;

liberror::ErrorOr<void> process_all(std::filesystem::path const& where, libpreprocessor::PreprocessorContext const& context);
void rename_all(std::filesystem::path const& where, Wildcards const& wildcards);
void replace_all(std::filesystem::path const& where, Wildcards const& wildcards);

} // pmake
