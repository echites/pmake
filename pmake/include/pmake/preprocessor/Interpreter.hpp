#pragma once

#include "nodes/INode.hpp"

#include <liberror/ErrorOr.hpp>
#include <liberror/types/TraceError.hpp>

#include <unordered_map>

namespace pmake::preprocessor {

struct InterpreterContext
{
    std::unordered_map<std::string, std::string> localVariables;
    std::unordered_map<std::string, std::string> environmentVariables;
};

liberror::ErrorOr<std::string> traverse(std::unique_ptr<core::INode> const& head, InterpreterContext const& context);

} // pmake::preprocessor
