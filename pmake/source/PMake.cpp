#include "pmake/PMake.hpp"

#include "pmake/filesystem/Files.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <fstream>
#include <filesystem>

using namespace error;

namespace pmake {

void print_project_information(PMake::Project const& project)
{
    std::println("┌– [pmake] –––");
    std::println("| name.......: {}", project.name);
    std::println("| language...: {} ({})", project.language.first, project.language.second);
    std::println("| kind.......: {} ({})", project.kind.first, project.kind.second);
    std::println("└–––––––––––––");
}

ErrorOr<std::string> PMake::setup_name()
{
    if (!parsedOptions_m.count("name")) return make_error("You must specify a project name.");
    return parsedOptions_m["name"].as<std::string>();
}

ErrorOr<std::pair<std::string, std::string>> PMake::setup_language()
{
    using namespace nlohmann;

    auto const language = parsedOptions_m["language"].as<std::string>();
    auto const standard = TRY([&] -> ErrorOr<std::string> {
        auto const path = std::format("{}\\{}\\pmake-info.json", PMake::get_templates_dir(), language);
        auto const info = json::parse(std::ifstream { path }, nullptr, false);

        if (info.is_discarded()) return make_error("Couldn't open {}.", path);

        auto const standard = parsedOptions_m["standard"].as<std::string>();

        if (standard == "latest") return info["standards"].front().get<std::string>();
        if (!info["standards"].contains(standard)) return make_error("Standard {} not available for {}.", standard, language);

        return standard;
    }());

    return {{ language, standard }};
}

ErrorOr<std::pair<std::string, std::string>> PMake::setup_kind(PMake::Project const& project)
{
    namespace fs = std::filesystem;

    auto const& language = project.language.first;
    auto const kind      = parsedOptions_m["kind"].as<std::string>();
    auto const mode      = parsedOptions_m["mode"].as<std::string>();

    auto const path = std::format("{}\\{}\\{}\\{}", PMake::get_templates_dir(), language, kind, mode);

    if (!fs::exists(path)) return make_error("The template {} couldn't be found.", path);

    return {{ kind, mode }};
}

ErrorOr<std::unordered_map<std::string, std::string>> PMake::setup_wildcards(PMake::Project const& project)
{
    using namespace nlohmann;

    auto const path = std::format("{}\\pmake-info.json", PMake::get_templates_dir());
    auto const info = json::parse(std::ifstream { path }, nullptr, false);

    if (info.is_discarded()) return make_error("Couldn't open {}.", path);

    std::unordered_map<std::string, std::string> wildcards {
        { info["wildcards"]["name"], project.name },
        { info["wildcards"]["language"], project.language.first },
        { info["wildcards"]["standard"], project.language.second },
    };

    return wildcards;
}

ErrorOr<void> PMake::create_project(PMake::Project const& project)
{
    namespace fs = std::filesystem;

    auto const& to       = project.name;
    auto const from      = std::format("{}\\{}\\{}\\{}", PMake::get_templates_dir(), project.language.first, project.kind.first, project.kind.second);
    auto const wildcards = TRY(PMake::setup_wildcards(project)) | std::views::transform([] (auto&& wildcard) {
        return std::pair { wildcard.first, wildcard.second };
    });

    if (!fs::exists(to)) fs::create_directory(to);

    fs::copy(from, to, fs::copy_options::recursive);

    std::ranges::for_each(wildcards, std::bind_front(pmake::filesystem::rename_all, to));
    std::ranges::for_each(wildcards, std::bind_front(pmake::filesystem::replace_all, to));

    return {};
}

ErrorOr<void> PMake::run(std::span<char const*> arguments)
{
    parsedOptions_m = options_m.parse(int(arguments.size()), arguments.data());

    if (parsedOptions_m.arguments().empty()) return make_error(options_m.help());
    if (parsedOptions_m.count("help")) return make_error(options_m.help());

    PMake::Project project {};

    project.name     = TRY(this->setup_name());
    project.language = TRY(this->setup_language());
    project.kind     = TRY(this->setup_kind(project));

    MUST(PMake::create_project(project));

    print_project_information(project);

    return {};
}

} // pmake
