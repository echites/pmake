#pragma once

#include "../IStatement.hpp"

namespace pmake::preprocessor::core {

struct SelectionMatchStatementNode: IStatementNode
{
    STATEMENT_TYPE(IStatementNode::Type::MATCH_CASE);

    virtual ~SelectionMatchStatementNode() override = default;

    std::unique_ptr<INode> match {};
    std::unique_ptr<INode> branch {};
};

} // pmake::preprocessor::core
