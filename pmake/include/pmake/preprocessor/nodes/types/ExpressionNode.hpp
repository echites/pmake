#pragma once

#include "../INode.hpp"

namespace pmake::preprocessor::core {

struct ExpressionNode : INode
{
    NODE_TYPE(INode::Type::EXPRESSION);
    virtual ~ExpressionNode() override = default;
    std::unique_ptr<INode> value {};
};

} // pmake::preprocessor::core