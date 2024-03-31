#include "preprocessor/core/Interpreter.hpp"

#include "preprocessor/core/nodes/INode.hpp"
#include "preprocessor/core/nodes/IStatement.hpp"
#include "preprocessor/core/nodes/types/ConditionalStatementNode.hpp"
#include "preprocessor/core/nodes/types/ContentNode.hpp"
#include "preprocessor/core/nodes/types/ExpressionNode.hpp"
#include "preprocessor/core/nodes/types/LiteralNode.hpp"
#include "preprocessor/core/nodes/types/OperatorNode.hpp"
#include "preprocessor/core/nodes/types/PrintNode.hpp"
#include "preprocessor/core/nodes/types/SelectionMatchStatementNode.hpp"
#include "preprocessor/core/nodes/types/SelectionStatementNode.hpp"
#include "preprocessor/core/nodes/types/UnconditionalStatementNode.hpp"

#include <algorithm>
#include <functional>

static std::string unquoted(std::string const& string)
{
    return string.starts_with("\"") ? string.substr(1, string.size() - 2) : string;
}

static liberror::ErrorOr<size_t> decay_to_integer_literal(std::string_view literal)
{
    if (literal.empty())
    {
        return liberror::make_error("Cannot decay an empty literal.");
    }

    std::stringstream stream {};
    stream << literal;
    size_t value {};
    stream >> value;

    if (stream.fail())
    {
        return liberror::make_error("Couldn't decay literal \"{}\" to a integer.", literal);
    }

    return value;
}

static liberror::ErrorOr<bool> decay_to_boolean_literal(std::string_view literal)
{
    if (literal.empty())
    {
        return liberror::make_error("Cannot decay an empty literal.");
    }

    if (literal == "TRUE") return true;
    if (literal == "FALSE") return false;

    auto const result = decay_to_integer_literal(literal);

    if (result.has_error())
    {
        return liberror::make_error("Couldn't decay literal \"{}\" to a boolean.", literal);
    }

    return result;
}

static liberror::ErrorOr<std::string> interpolate_string(std::string_view unquotedValue, pmake::preprocessor::InterpreterContext const& context)
{
    if (unquotedValue.empty()) return liberror::make_error("Tried to interpolate an empty string.");

    std::string result {};

    for (auto index = 0zu; index < unquotedValue.size(); index += 1)
    {
        if (unquotedValue.at(index) == '<')
        {
            std::function<std::pair<std::string, std::string>()> fnParseIdentifier {};

            fnParseIdentifier = [&] () -> std::pair<std::string, std::string> {
                std::string value {};

                index += 1;
                while (unquotedValue.at(index) != '>')
                {
                    if (unquotedValue.at(index) == '<') value = fnParseIdentifier().first;
                    else
                    {
                        value += unquotedValue.at(index);
                        index += 1;
                    }
                }
                index += 1;

                auto const originalValue = std::format("<{}>", value);

                if (context.localVariables.contains(value)) return { originalValue, context.localVariables.at(value) };
                if (context.environmentVariables.contains(value)) return { originalValue, context.environmentVariables.at(value) };

                return { originalValue, value };
            };

            result += std::format("{}", fnParseIdentifier().second);
            index -= 1;
        }
        else
        {
            result += unquotedValue.at(index);
        }
    }

    return result;
}

static liberror::ErrorOr<std::string> evaluate_expression(std::unique_ptr<pmake::preprocessor::INode> const& head, pmake::preprocessor::InterpreterContext const& context)
{
    if (!head)
    {
        return liberror::make_error("Head node was nullptr.");
    }

    if (head->type() != pmake::preprocessor::INode::Type::EXPRESSION)
    {
        return liberror::make_error("Head node is expected to be of type \"INode::Type::EXPRESSION\", instead it was \"{}\".", head->type_as_string());
    }

    auto* headExpression = static_cast<pmake::preprocessor::ExpressionNode*>(head.get());

    switch (headExpression->value->type())
    {
    case pmake::preprocessor::INode::Type::OPERATOR: {
        auto* operatorNode = static_cast<pmake::preprocessor::OperatorNode*>(headExpression->value.get());

        if (!operatorNode->lhs)
        {
            return liberror::make_error("For any operator, it must have atleast one value for it to work on.");
        }

        if (operatorNode->lhs->type() == pmake::preprocessor::INode::Type::EXPRESSION)
        {
            auto lhs          = std::make_unique<pmake::preprocessor::LiteralNode>();
            lhs->value        = TRY(evaluate_expression(operatorNode->lhs, context));
            operatorNode->lhs = std::move(lhs);
        }

        if (operatorNode->rhs && operatorNode->rhs->type() == pmake::preprocessor::INode::Type::EXPRESSION)
        {
            auto rhs          = std::make_unique<pmake::preprocessor::LiteralNode>();
            rhs->value        = TRY(evaluate_expression(operatorNode->rhs, context));
            operatorNode->rhs = std::move(rhs);
        }

        switch (operatorNode->arity)
        {
        case pmake::preprocessor::OperatorNode::Arity::UNARY: {
            auto const& lhs = static_cast<pmake::preprocessor::LiteralNode*>(operatorNode->lhs.get())->value;

            if (operatorNode->name == "NOT")
            {
                return TRY(decay_to_boolean_literal(lhs)) ? "FALSE" : "TRUE";
            }

            break;
        }
        case pmake::preprocessor::OperatorNode::Arity::BINARY: {
            if (!operatorNode->rhs)
            {
                return liberror::make_error("Operator \"{}\" is a binary operator and expects both an left-hand and an right-hand side, but only the former was given.", operatorNode->name);
            }

            auto const lhs = [&] {
                auto value = unquoted(static_cast<pmake::preprocessor::LiteralNode*>(operatorNode->lhs.get())->value);
                if (context.localVariables.contains(value)) return context.localVariables.at(value);
                if (context.environmentVariables.contains(value)) return context.environmentVariables.at(value);
                return value;
            }();

            auto const rhs = [&] {
                auto value = unquoted(static_cast<pmake::preprocessor::LiteralNode*>(operatorNode->rhs.get())->value);
                if (context.localVariables.contains(value)) return context.localVariables.at(value);
                if (context.environmentVariables.contains(value)) return context.environmentVariables.at(value);
                return value;
            }();

            if (operatorNode->name == "CONTAINS") { return std::ranges::contains_subrange(lhs, rhs) ? "TRUE" : "FALSE"; }
            if (operatorNode->name == "EQUALS") { return lhs == rhs ? "TRUE" : "FALSE"; }
            if (operatorNode->name == "AND") { return lhs == "TRUE" && rhs == "TRUE" ? "TRUE" : "FALSE"; }
            if (operatorNode->name == "OR") { return lhs == "TRUE" || rhs == "TRUE" ? "TRUE" : "FALSE"; }

            break;
        }

        case pmake::preprocessor::OperatorNode::Arity::BEGIN__:
        case pmake::preprocessor::OperatorNode::Arity::END__:
        default: {
            return liberror::make_error("Operator \"{}\" had an invalid arity.", operatorNode->name);
        }
        }
        break;
    }
    case pmake::preprocessor::INode::Type::LITERAL: {
        auto const* literalNode  = static_cast<pmake::preprocessor::LiteralNode*>(headExpression->value.get());
        auto const unquotedValue = unquoted(literalNode->value);

        if (context.localVariables.contains(unquotedValue)) return context.localVariables.at(unquotedValue);
        if (context.environmentVariables.contains(unquotedValue)) return context.environmentVariables.at(unquotedValue);

        return interpolate_string(unquotedValue, context);
    }

    case pmake::preprocessor::INode::Type::START__:
    case pmake::preprocessor::INode::Type::EXPRESSION:
    case pmake::preprocessor::INode::Type::STATEMENT:
    case pmake::preprocessor::INode::Type::CONTENT:
    case pmake::preprocessor::INode::Type::CONDITION:
    case pmake::preprocessor::INode::Type::BRANCH:
    case pmake::preprocessor::INode::Type::END__: {
        return liberror::make_error("Unexpected node of type \"{}\" was reached.", headExpression->type_as_string());
    }
    }

    return "FALSE";
}

namespace pmake::preprocessor {

    namespace detail {

    static liberror::ErrorOr<void> traverse(std::unique_ptr<INode> const& head, std::stringstream& stream, InterpreterContext const& context, size_t depth)
    {
        if (!head)
        {
            return liberror::make_error("Head node was nullptr.");
        }

        switch (head->type())
        {
        case INode::Type::STATEMENT: {
            auto* statement = static_cast<IStatementNode*>(head.get());

            switch (statement->statement_type())
            {
            case IStatementNode::Type::CONDITIONAL: {
                auto* node = static_cast<ConditionalStatementNode*>(statement);

                if (TRY(evaluate_expression(node->condition, context)) == "TRUE")
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

                if (node->match->type() != INode::Type::EXPRESSION)
                {
                    return liberror::make_error("%SWITCH statement expects an \"INode::Type::EXPRESSION\" as match, but instead got \"{}\".", node->match->type_as_string());
                }

                auto match = TRY(evaluate_expression(node->match, context));

                for (auto* cases = static_cast<SelectionMatchStatementNode*>(node->branches.first.get());
                        cases != nullptr; cases = static_cast<SelectionMatchStatementNode*>(cases->next.get()))
                {
                    if (unquoted(static_cast<LiteralNode*>(cases->match.get())->value) == match)
                    {
                        TRY(traverse(cases->branch, stream, context, depth + 1));
                        return {};
                    }
                }

                if (node->branches.second)
                {
                    TRY(traverse(static_cast<SelectionMatchStatementNode*>(node->branches.second.get())->branch, stream, context, depth = 1));
                }

                break;
            }

            case IStatementNode::Type::PRINT: {
                auto* node = static_cast<PrintStatementNode*>(statement);

                if (node->content->type() != INode::Type::EXPRESSION)
                {
                    return liberror::make_error("%PRINT statement expects an \"INode::Type::EXPRESSION\" as argument, but instead got \"{}\".", node->content->type_as_string());
                }

                std::println("{}", TRY(evaluate_expression(node->content, context)));

                break;
            }

            case IStatementNode::Type::MATCH_CASE:

            case IStatementNode::Type::START__:
            case IStatementNode::Type::END__:
            default: {
                return liberror::make_error("Unexpected statement \"{}\" reached.", statement->type_as_string());
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
            return liberror::make_error("Unexpected node of type \"{}\" was reached.", head->type_as_string());
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

liberror::ErrorOr<std::string> traverse(std::unique_ptr<INode> const& head, InterpreterContext const& context)
{
    std::stringstream sourceStream {};
    TRY(detail::traverse(head, sourceStream, context, 0));
    return sourceStream.str();
}

}

