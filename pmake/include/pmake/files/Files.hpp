#pragma once

#include <nlohmann/json.hpp>
#include <liberror/ErrorOr.hpp>

#include <filesystem>
#include <unordered_map>

namespace pmake {

void rename_all(std::filesystem::path const& where, std::unordered_map<std::string, std::string> const& wildcards);
void replace_all(std::filesystem::path const& where, std::unordered_map<std::string, std::string> const& wildcards);
liberror::ErrorOr<nlohmann::json> read_json(std::filesystem::path const& path);

} // pmake
