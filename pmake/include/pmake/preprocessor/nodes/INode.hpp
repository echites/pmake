#pragma once

#include <memory>

#define NODE_TYPE(TYPE)                                                   \
constexpr virtual Type type() override { return TYPE; }                   \
constexpr virtual char const* type_as_string() override { return #TYPE; } \

namespace pmake::preprocessor::core {

struct INode
{
    enum class Type
    {
        START__,
        STATEMENT,
        EXPRESSION,
        CONTENT,
        CONDITION,
        OPERATOR,
        LITERAL,
        BRANCH,
        END__
    };

    virtual ~INode() = default;

    constexpr virtual Type type() = 0;
    constexpr virtual char const* type_as_string() = 0;

    std::unique_ptr<INode> next;
};

} // pmake::preprocessor::core
