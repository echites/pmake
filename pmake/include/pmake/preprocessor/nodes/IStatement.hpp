#pragma once

#include "INode.hpp"

#define STATEMENT_TYPE(TYPE)                                                        \
constexpr virtual Type statement_type() override { return TYPE; }                   \
constexpr virtual char const* statement_type_as_string() override { return #TYPE; } \

namespace pmake::preprocessor::core {

struct IStatementNode : INode
{
    NODE_TYPE(INode::Type::STATEMENT);

    enum class Type
    {
        START__,
        CONDITIONAL,
        UNCONDITIONAL,
        MATCH,
        MATCH_CASE,
        PRINT,
        END__
    };

    virtual ~IStatementNode() = default;
    virtual Type statement_type() = 0;
    virtual char const* statement_type_as_string() = 0;
};

}
