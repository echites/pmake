#include "preprocessor/Parser.hpp"

#include "preprocessor/nodes/INode.hpp"
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

namespace pmake::preprocessor {

using namespace core;

static liberror::ErrorOr<void> parse_statement_body(Parser& parser, std::unique_ptr<INode>& root, size_t depth)
{
    auto body = std::ref(root);

    while (!parser.eof())
    {
        auto token = parser.take();

        if (is_percent(token))
        {
            auto const& front = parser.peek();
            parser.tokens().push(std::move(token));
            if (!(is_keyword(front) && (front.data == "END" || front.data == "ELSE" || front.data == "DEFAULT")))
                body.get() = TRY(parser.parse(depth));
            else
                break;
        }
        else
        {
            auto content     = std::make_unique<ContentNode>();
            content->content = token.data;
            body.get()       = std::move(content);
        }

        body = body.get()->next;
    }

    return {};
}

liberror::ErrorOr<std::unique_ptr<INode>> Parser::parse(size_t depth = 0)
{
    std::unique_ptr<INode> root {};

    while (!eof())
    {
        auto const token = take();

        switch (token.type)
        {
        case Token::Type::PERCENT: {
            if (!is_keyword(peek()))
            {
                return liberror::make_error("Expecting a node of type \"Token::Type::KEYWORD\" after \"%\", but instead got \"{}\".", token.type_as_string());
            }

            if (peek().data == "END") return root;

            auto const innerToken = take();

            if (innerToken.data == "IF")
            {
                auto statementNode           = std::make_unique<ConditionalStatementNode>();
                statementNode->condition     = TRY(parse(depth + 1));
                statementNode->branch.first  = TRY(parse(depth + 1));
                statementNode->branch.second = TRY(parse(depth + 1));

                if (eof() || peek().data != "END")
                {
                    return liberror::make_error("An \"%IF\" statement missing its \"%END\" was reached.");
                }

                take();

                if (depth != 0) { return statementNode; }

                root = std::move(statementNode);
                root->next = TRY(parse(depth));
            }
            else if (innerToken.data == "ELSE")
            {
                auto statementNode    = std::make_unique<UnconditionalStatementNode>();
                statementNode->branch = TRY(parse(depth + 1));
                root                  = std::move(statementNode);
            }
            else if (innerToken.data == "SWITCH")
            {
                auto statementNode   = std::make_unique<SelectionStatementNode>();
                // FIXME: i should be able to also match normal expressions. also what the heck?
                statementNode->match = std::unique_ptr<LiteralNode>(
                     static_cast<LiteralNode*>(
                            static_cast<ExpressionNode*>(TRY(parse(depth + 1)).release())->value.release()
                    ));
                statementNode->branches.first  = TRY(parse(depth + 1));
                statementNode->branches.second = TRY(parse(depth + 1));

                if (eof() || peek().data != "END")
                {
                    return liberror::make_error("An \"%SWITCH\" statement missing its \"%END\" was reached.");
                }

                take();

                if (depth != 0) { return statementNode; }

                root = std::move(statementNode);
                root->next = TRY(parse(depth));
            }
            else if (innerToken.data == "CASE")
            {
                auto statementNode = std::make_unique<SelectionMatchStatementNode>();
                // FIXME: i should be able to also match normal expressions. also what the heck?
                statementNode->match = std::unique_ptr<LiteralNode>(
                     static_cast<LiteralNode*>(
                            static_cast<ExpressionNode*>(TRY(parse(depth + 1)).release())->value.release()
                    ));
                statementNode->branch = TRY(parse(depth + 1));

                take();

                if (eof() || peek().data != "END")
                {
                    return liberror::make_error("An \"%CASE\" statement missing its \"%END\" was reached.");
                }

                take();

                return statementNode;
            }
            else if (innerToken.data == "DEFAULT")
            {
                auto statementNode   = std::make_unique<SelectionMatchStatementNode>();
                statementNode->match = [] {
                    auto literalNode   = std::make_unique<LiteralNode>();
                    literalNode->value = "DEFAULT";
                    return literalNode;
                }();
                statementNode->branch = TRY(parse(depth + 1));

                root = std::move(statementNode);

                take();

                if (eof() || peek().data != "END")
                {
                    return liberror::make_error("An \"%DEFAULT\" statement missing its \"%END\" was reached.");
                }

                take();
            }
            else if (innerToken.data == "PRINT")
            {
                auto statementNode = std::make_unique<PrintStatementNode>();
                statementNode->content = TRY(parse(depth + 1));
                if (depth != 0) { return statementNode; }
                root = std::move(statementNode);
                root->next = TRY(parse(depth));
            }
            else
            {
                return liberror::make_error("Unexpected keyword \"{}\" was reached.", innerToken.data);
            }

            break;
        }
        case Token::Type::LEFT_SQUARE_BRACKET: {
            auto expressionNode   = std::make_unique<ExpressionNode>();
            expressionNode->value = TRY(parse(depth + 1));

            if (eof() || !is_right_square_bracket(peek()))
            {
                return liberror::make_error("\"[\" missing its \"]\"");
            }

            root = std::move(expressionNode);

            break;
        }
        case Token::Type::RIGHT_SQUARE_BRACKET: {
            if (!is_identifier(peek())) return root;
            break;
        }
        case Token::Type::LEFT_ANGLE_BRACKET: {
            auto literalNode   = std::make_unique<LiteralNode>();
            literalNode->value = take().data;
            root               = std::move(literalNode);
            break;
        }
        case Token::Type::RIGHT_ANGLE_BRACKET: {
            if (is_right_square_bracket(peek())) return root;
            break;
        }
        case Token::Type::COLON: {
            TRY(parse_statement_body(*this, root, depth));
            return root;
        }
        case Token::Type::IDENTIFIER: {
            // FIXME: for now, all identifiers *must* be an operator. maybe we should be fixin' this in the future?
            auto operatorNode   = std::make_unique<OperatorNode>();
            operatorNode->name  = token.data;
            operatorNode->arity = [&operatorNode] {
                if (operatorNode->name == "NOT")
                    return OperatorNode::Arity::UNARY;
                else if (std::ranges::contains(std::array { "AND", "OR", "EQUALS", "CONTAINS" }, operatorNode->name))
                    return OperatorNode::Arity::BINARY;
                [[unlikely]];
                return OperatorNode::Arity{};
            }();

            operatorNode->lhs = (root == nullptr) ? TRY(parse(depth + 1)) : std::move(root);

            if (operatorNode->arity == OperatorNode::Arity::BINARY)
            {
                operatorNode->rhs = TRY(parse(depth + 1));
            }

            return operatorNode;
        }

        case Token::Type::CONTENT: {
            auto contentNode     = std::make_unique<ContentNode>();
            contentNode->content = token.data;
            root                 = std::move(contentNode);
            root->next           = TRY(parse(depth));
            break;
        }

        case Token::Type::LITERAL:
        case Token::Type::KEYWORD:

        case Token::Type::BEGIN__:
        case Token::Type::END__:
        default: {
            return liberror::make_error("Unexpected token of kind \"{}\" was reached.", token.type_as_string());
        }
        }
    }

    return root;
}

}
