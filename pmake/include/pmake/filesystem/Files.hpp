#pragma once

#include <filesystem>

namespace pmake::filesystem {

void rename_all(std::filesystem::path where, std::pair<std::string, std::string> const& wildcard);
void replace_all(std::filesystem::path where, std::pair<std::string, std::string> const& wildcard);

} // pmake::filesystem
