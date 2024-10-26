#pragma once

#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>

#include <array>
#include <cassert>
#include <filesystem>

namespace detail {

inline std::filesystem::path const& get_program_root_dir()
{
    std::filesystem::path static programPath {};

    if (!programPath.empty())
    {
        return programPath;
    }

    std::array<char, PATH_MAX> buffer {};
    auto _ = readlink("/proc/self/exe", buffer.data(), buffer.size());

    programPath = buffer.data();
    programPath = programPath.parent_path();

    return programPath;
}

}

