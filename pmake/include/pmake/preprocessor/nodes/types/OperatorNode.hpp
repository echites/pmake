#pragma once

#include "../INode.hpp"

#include <string>

namespace pmake::preprocessor::core {

struct OperatorNode : INode
{
    NODE_TYPE(INode::Type::OPERATOR);

    enum class Arity
    {
        BEGIN__,
        UNARY,
        BINARY,
        END__
    };

    virtual ~OperatorNode() override = default;

    std::string name {};
    Arity arity {};
    std::unique_ptr<INode> lhs {};
    std::unique_ptr<INode> rhs {};
};

} // pmake::preprocessor::core
