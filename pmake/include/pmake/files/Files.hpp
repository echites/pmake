#pragma once

#include <liberror/ErrorOr.hpp>
#include <libpreprocessor/Preprocessor.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <unordered_map>

namespace pmake {

using Wildcards = std::unordered_map<std::string, std::string>;

liberror::ErrorOr<void> preprocess_files(std::filesystem::path const& where, libpreprocessor::PreprocessorContext const& context);
void replace_filenames(std::filesystem::path const& where, Wildcards const& wildcards);
void replace_contents(std::filesystem::path const& where, Wildcards const& wildcards);

} // pmake
