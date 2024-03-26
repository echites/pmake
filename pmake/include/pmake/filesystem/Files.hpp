#pragma once

#include <filesystem>
#include <unordered_map>

namespace pmake::filesystem {

void rename_all(std::filesystem::path where, std::unordered_map<std::string, std::string> const& wildcards);
void replace_all(std::filesystem::path where, std::unordered_map<std::string, std::string> const& wildcards);

} // pmake::filesystem
