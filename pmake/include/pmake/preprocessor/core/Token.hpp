#pragma once

#include <string>

namespace pmake::preprocessor {

struct Token
{
    enum class Type
    {
        BEGIN__,
        PERCENT,
        LEFT_SQUARE_BRACKET,
        RIGHT_SQUARE_BRACKET,
        LEFT_ANGLE_BRACKET,
        RIGHT_ANGLE_BRACKET,
        COLON,
        IDENTIFIER,
        LITERAL,
        KEYWORD,
        CONTENT,
        END__
    };

    constexpr char const* type_as_string() const noexcept
    {
        switch (type)
        {
        case Type::PERCENT: return "Token::Type::PERCENT";
        case Type::LEFT_SQUARE_BRACKET: return "Token::Type::LEFT_SQUARE_BRACKET";
        case Type::RIGHT_SQUARE_BRACKET: return "Token::Type::RIGHT_SQUARE_BRACKET";
        case Type::LEFT_ANGLE_BRACKET: return "Token::Type::LEFT_ANGLE_BRACKET";
        case Type::RIGHT_ANGLE_BRACKET: return "Token::Type::RIGHT_ANGLE_BRACKET";
        case Type::COLON: return "Token::Type::COLON";
        case Type::IDENTIFIER: return "Token::Type::IDENTIFIER";
        case Type::LITERAL: return "Token::Type::LITERAL";
        case Type::KEYWORD: return "Token::Type::KEYWORD";
        case Type::CONTENT: return "Token::Type::CONTENT";

        case Type::BEGIN__:
        case Type::END__: {
            break;
        }
        }

        return "INVALID";
    }

    size_t begin;
    size_t end;
    std::string data;
    Type type;
};

constexpr bool is_keyword(Token const& token) { return token.type == Token::Type::KEYWORD; }
constexpr bool is_identifier(Token const& token) { return token.type == Token::Type::IDENTIFIER; }
constexpr bool is_literal(Token const& token) { return token.type == Token::Type::LITERAL; }
constexpr bool is_percent(Token const& token) { return token.type == Token::Type::PERCENT; }
constexpr bool is_colon(Token const& token) { return token.type == Token::Type::COLON; }
constexpr bool is_content(Token const& token) { return token.type == Token::Type::CONTENT; }
constexpr bool is_right_square_bracket(Token const& token) { return token.type == Token::Type::RIGHT_SQUARE_BRACKET; }
constexpr bool is_left_square_bracket(Token const& token) { return token.type == Token::Type::LEFT_SQUARE_BRACKET; }
constexpr bool is_right_angle_bracket(Token const& token) { return token.type == Token::Type::RIGHT_ANGLE_BRACKET; }
constexpr bool is_left_angle_bracket(Token const& token) { return token.type == Token::Type::LEFT_ANGLE_BRACKET; }

} // pmake::preprocessor
