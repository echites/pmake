#include "preprocessor/Interpreter.hpp"

#include "preprocessor/nodes/INode.hpp"
#include "preprocessor/nodes/IStatement.hpp"
#include "preprocessor/nodes/types/ConditionalStatementNode.hpp"
#include "preprocessor/nodes/types/ContentNode.hpp"
#include "preprocessor/nodes/types/ExpressionNode.hpp"
#include "preprocessor/nodes/types/LiteralNode.hpp"
#include "preprocessor/nodes/types/OperatorNode.hpp"
#include "preprocessor/nodes/types/PrintNode.hpp"
#include "preprocessor/nodes/types/SelectionMatchStatementNode.hpp"
#include "preprocessor/nodes/types/SelectionStatementNode.hpp"
#include "preprocessor/nodes/types/UnconditionalStatementNode.hpp"

#include <algorithm>

static std::string unquoted(std::string const& string)
{
    return string.starts_with("\"") ? string.substr(1, string.size() - 2) : string;
}

static error::ErrorOr<size_t> decay_to_integer_literal(std::string_view literal)
{
    if (literal.empty())
    {
        return error::make_error("Cannot decay an empty literal.");
    }

    std::stringstream stream {};
    stream << literal;
    size_t value {};
    stream >> value;

    if (stream.fail())
    {
        return error::make_error("Couldn't decay literal \"{}\" to a integer.", literal);
    }

    return value;
}

static error::ErrorOr<bool> decay_to_boolean_literal(std::string_view literal)
{
    if (literal.empty())
    {
        return error::make_error("Cannot decay an empty literal.");
    }

    if (literal == "TRUE") return true;
    if (literal == "FALSE") return false;

    auto const result = decay_to_integer_literal(literal);

    if (result.has_error())
    {
        return error::make_error("Couldn't decay literal \"{}\" to a boolean.", literal);
    }

    return result;
}

static error::ErrorOr<bool> evaluate_expression(std::unique_ptr<pmake::preprocessor::core::INode> const& head, pmake::preprocessor::InterpreterContext const& context)
{
    if (!head)
    {
        return error::make_error("Head node was nullptr.");
    }

    if (head->type() != pmake::preprocessor::core::INode::Type::EXPRESSION)
    {
        return error::make_error("Head node is expected to be of type \"INode::Type::EXPRESSION\", instead it was \"{}\".", head->type_as_string());
    }

    auto* headExpression = static_cast<pmake::preprocessor::core::ExpressionNode*>(head.get());

    if (headExpression->value->type() != pmake::preprocessor::core::INode::Type::OPERATOR)
    {
        return error::make_error("Head expression was expected to hold an \"INode::Type::OPERATOR\", instead it holds \"{}\".", headExpression->value->type_as_string());
    }

    auto* operatorNode = static_cast<pmake::preprocessor::core::OperatorNode*>(headExpression->value.get());

    if (!operatorNode->lhs)
    {
        return error::make_error("For any operator, it must have atleast one value for it to work on.");
    }

    if (operatorNode->lhs->type() == pmake::preprocessor::core::INode::Type::EXPRESSION)
    {
        auto lhs       = std::make_unique<pmake::preprocessor::core::LiteralNode>();
        lhs->value     = TRY(evaluate_expression(operatorNode->lhs, context)) ? "TRUE" : "FALSE";
        operatorNode->lhs = std::move(lhs);
    }

    if (operatorNode->rhs && operatorNode->rhs->type() == pmake::preprocessor::core::INode::Type::EXPRESSION)
    {
        auto rhs       = std::make_unique<pmake::preprocessor::core::LiteralNode>();
        rhs->value     = TRY(evaluate_expression(operatorNode->rhs, context)) ? "TRUE" : "FALSE";
        operatorNode->rhs = std::move(rhs);
    }

    switch (operatorNode->arity)
    {
    case pmake::preprocessor::core::OperatorNode::Arity::UNARY: {
        auto const& lhs = static_cast<pmake::preprocessor::core::LiteralNode*>(operatorNode->lhs.get())->value;

        if (operatorNode->name == "NOT")
        {
            return !TRY(decay_to_boolean_literal(lhs));
        }

        break;
    }
    case pmake::preprocessor::core::OperatorNode::Arity::BINARY: {
        if (!operatorNode->rhs)
        {
            return error::make_error("Operator \"{}\" is a binary operator and expects both an left-hand and an right-hand side, but only the former was given.", operatorNode->name);
        }

        auto const lhs = [&] {
            auto value = unquoted(static_cast<pmake::preprocessor::core::LiteralNode*>(operatorNode->lhs.get())->value);
            if (context.localVariables.contains(value)) return context.localVariables.at(value);
            if (context.environmentVariables.contains(value)) return context.environmentVariables.at(value);
            return value;
        }();

        auto const rhs = [&] {
            auto value = unquoted(static_cast<pmake::preprocessor::core::LiteralNode*>(operatorNode->rhs.get())->value);
            if (context.localVariables.contains(value)) return context.localVariables.at(value);
            if (context.environmentVariables.contains(value)) return context.environmentVariables.at(value);
            return value;
        }();

        if (operatorNode->name == "CONTAINS") { return std::ranges::contains_subrange(lhs, rhs); }
        if (operatorNode->name == "EQUALS") { return lhs == rhs; }
        if (operatorNode->name == "AND") { return lhs == "TRUE" && rhs == "TRUE"; }
        if (operatorNode->name == "OR") { return lhs == "TRUE" || rhs == "TRUE"; }

        break;
    }

    case pmake::preprocessor::core::OperatorNode::Arity::BEGIN__:
    case pmake::preprocessor::core::OperatorNode::Arity::END__:
    default: {
        return error::make_error("Operator \"{}\" had an invalid arity.", operatorNode->name);
    }
    }

    return false;
}

namespace pmake::preprocessor {

using namespace pmake::preprocessor::core;

    namespace detail {

    static error::ErrorOr<void> traverse(std::unique_ptr<INode> const& head, std::stringstream& stream, InterpreterContext const& context, size_t depth)
    {
        if (!head)
        {
            return error::make_error("Head node was nullptr.");
        }

        switch (head->type())
        {
        case INode::Type::STATEMENT: {
            auto* statement = static_cast<IStatementNode*>(head.get());

            switch (statement->statement_type())
            {
            case IStatementNode::Type::CONDITIONAL: {
                auto* node = static_cast<ConditionalStatementNode*>(statement);

                if (TRY(evaluate_expression(node->condition, context)))
                {
                    for (auto body = std::ref(node->branch.first); body.get() != nullptr; body = body.get()->next)
                    {
                        TRY(traverse(body.get(), stream, context, depth + 1));
                    }
                }
                else if (node->branch.second)
                {
                    TRY(traverse(node->branch.second, stream, context, depth + 1));
                }

                break;
            }
            case IStatementNode::Type::UNCONDITIONAL: {
                auto* node = static_cast<UnconditionalStatementNode*>(statement);

                for (auto body = std::ref(node->branch); body.get() != nullptr; body = body.get()->next)
                {
                    TRY(traverse(body.get(), stream, context, depth + 1));
                }

                break;
            }
            case IStatementNode::Type::MATCH: {
                auto* node = static_cast<SelectionStatementNode*>(statement);

                if (node->match->type() != INode::Type::LITERAL)
                {
                    return error::make_error("%SWITCH statement expects an \"INode::Type::LITERAL\" as match, but instead got \"{}\".", node->match->type_as_string());
                }

                // FIXME: i should be able to also match normal expressions.
                auto const* matchNode = static_cast<LiteralNode*>(node->match.get());

                for (auto* cases = static_cast<SelectionMatchStatementNode*>(node->branches.first.get());
                        cases != nullptr; cases = static_cast<SelectionMatchStatementNode*>(cases->next.get()))
                {
                    if (static_cast<LiteralNode*>(cases->match.get())->value == matchNode->value)
                    {
                        traverse(cases->branch, stream, context, depth + 1);
                        return {};
                    }
                }

                if (node->branches.second)
                {
                    traverse(static_cast<SelectionMatchStatementNode*>(node->branches.second.get())->branch, stream, context, depth = 1);
                }

                break;
            }

            case IStatementNode::Type::PRINT: {
                auto* node = static_cast<PrintStatementNode*>(statement);

                if (node->content->type() != INode::Type::EXPRESSION)
                {
                    return error::make_error("%PRINT statement expects an \"INode::Type::EXPRESSION\" as argument, but instead got \"{}\".", node->content->type_as_string());
                }

                auto* expressionNode = static_cast<ExpressionNode*>(node->content.get());

                if (expressionNode->value->type() != INode::Type::LITERAL)
                {
                    return error::make_error("%PRINT expects the expression to decay to an \"INode::Type::LITERAL\", but the given expression decays to \"{}\"", expressionNode->value->type_as_string());
                }

                auto const* contentNode = static_cast<LiteralNode*>(expressionNode->value.get());
                std::println("{}", unquoted(contentNode->value));

                break;
            }

            case IStatementNode::Type::MATCH_CASE:

            case IStatementNode::Type::START__:
            case IStatementNode::Type::END__:
            default: {
                return error::make_error("Unexpected statement \"{}\" reached.", statement->type_as_string());
            }
            }

            break;
        }
        case INode::Type::CONTENT: {
            ContentNode* node = static_cast<ContentNode*>(head.get());
            stream << node->content;

            if (!((node->content.front() == node->content.back()) && node->content.front() == '\n'))
            {
                stream << '\n';
            }

            break;
        }

        case INode::Type::EXPRESSION:
        case INode::Type::CONDITION:
        case INode::Type::OPERATOR:
        case INode::Type::LITERAL:
        case INode::Type::BRANCH:

        case INode::Type::START__:
        case INode::Type::END__:
        default: {
            return error::make_error("Unexpected node of type \"{}\" was reached.", head->type_as_string());
        }
        }

        if (depth == 0)
        {
            for (auto currentHead = std::ref(head->next); currentHead.get() != nullptr; currentHead = currentHead.get()->next)
            {
                TRY(traverse(currentHead.get(), stream, context, depth + 1));
            }
        }

        return {};
    }

    }

error::ErrorOr<std::string> traverse(std::unique_ptr<INode> const& head, InterpreterContext const& context)
{
    std::stringstream sourceStream {};
    TRY(detail::traverse(head, sourceStream, context, 0));
    return sourceStream.str();
}

}

