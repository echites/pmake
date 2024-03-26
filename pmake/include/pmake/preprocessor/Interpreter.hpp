#pragma once

#include "nodes/INode.hpp"

#include <err_or/ErrorOr.hpp>
#include <err_or/types/TraceError.hpp>

#include <unordered_map>

namespace pmake::preprocessor {

struct InterpreterContext
{
    std::unordered_map<std::string, std::string> localVariables;
    std::unordered_map<std::string, std::string> environmentVariables;
};

error::ErrorOr<std::string> traverse(std::unique_ptr<core::INode> const& head, InterpreterContext const& context);

} // pmake::preprocessor
