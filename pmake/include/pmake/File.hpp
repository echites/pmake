#pragma once

#include <liberror/ErrorOr.hpp>

#include <filesystem>
#include <unordered_map>

namespace pmake::file {

liberror::ErrorOr<void> copy(std::filesystem::path const& source, std::filesystem::path const& destination);
void replace_file_name_wildcards(std::filesystem::path const& path, std::unordered_map<std::string, std::string> const& wildcards);
void replace_file_wildcards(std::filesystem::path const& path, std::unordered_map<std::string, std::string> const& wildcards);

}
