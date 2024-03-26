#pragma once

#include "../IStatement.hpp"

namespace pmake::preprocessor::core {

struct PrintStatementNode : IStatementNode
{
    STATEMENT_TYPE(IStatementNode::Type::PRINT);
    virtual ~PrintStatementNode() override = default;
    std::unique_ptr<INode> content {};
};

} // pmake::preprocessor::core