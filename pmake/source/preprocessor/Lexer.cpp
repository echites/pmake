#include "preprocessor/Lexer.hpp"

#include <functional>
#include <algorithm>

static bool is_newline(char value) { return value == '\n'; }
static bool is_space(char value) { return value == ' '; }

static void drop_while(pmake::preprocessor::Lexer& lexer, std::function<bool(char)> const& predicate)
{
    while (!lexer.eof() && predicate(lexer.peek()))
    {
        lexer.take();
    }
}

static error::ErrorOr<void> expect_to_peek(pmake::preprocessor::Lexer& lexer, char expected)
{
    auto fnUnscapedChar = [] (char byte) -> std::string {
        switch (byte)
        {
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\r': return "\\r";
        case '\0': return "\\0";
        }
        return std::format("{}", byte);
    };

    if (lexer.peek() != expected)
    {
        return error::make_error("Expected \"{}\", but found \"{}\" instead.", expected, fnUnscapedChar(lexer.peek()));
    }

    return {};
}

static void tokenize_content(pmake::preprocessor::Lexer& lexer, std::vector<pmake::preprocessor::Token>& tokens)
{
    while (!lexer.eof() && lexer.peek() != '%')
    {
        pmake::preprocessor::Token token {};

        do
        {
            token.data += lexer.take();
        } while (!lexer.eof() && lexer.peek() != '%' && !(is_newline(token.data.front()) || is_newline(lexer.peek())));

        if (is_newline(lexer.peek())) { lexer.take(); }
        if (std::ranges::all_of(token.data, is_space)) { return; }

        token.type  = pmake::preprocessor::Token::Type::CONTENT;
        token.begin = lexer.cursor() - token.data.size();
        token.end   = lexer.cursor();
        tokens.push_back(std::move(token));
    }
}

static void tokenize_keyword(pmake::preprocessor::Lexer& lexer, std::vector<pmake::preprocessor::Token>& tokens)
{
    pmake::preprocessor::Token token {};

    while (!lexer.eof() && lexer.peek() != ':' && !(is_space(lexer.peek()) || is_newline(lexer.peek())))
    {
        token.data += lexer.take();
    }

    token.type  = pmake::preprocessor::Token::Type::KEYWORD;
    token.begin = lexer.cursor() - token.data.size();
    token.end   = lexer.cursor();
    tokens.push_back(std::move(token));

    if (is_newline(lexer.peek()) || is_space(lexer.peek()))
    {
        lexer.take();
        drop_while(lexer, is_space);
    }
}

static void tokenize_literal(pmake::preprocessor::Lexer& lexer, std::vector<pmake::preprocessor::Token>& tokens)
{
    pmake::preprocessor::Token token {};

    while (!lexer.eof() && lexer.peek() != '>')
    {
        token.data += lexer.take();
    }

    token.type  = (token.data.contains(':')) ? pmake::preprocessor::Token::Type::IDENTIFIER : pmake::preprocessor::Token::Type::LITERAL;
    token.begin = lexer.cursor() - token.data.size();
    token.end   = lexer.cursor();
    tokens.push_back(std::move(token));
}

static error::ErrorOr<void> tokenize_expression(pmake::preprocessor::Lexer& lexer, std::vector<pmake::preprocessor::Token>& tokens, size_t depth = 0)
{
    while (!lexer.eof())
    {
        drop_while(lexer, is_space);

        switch (lexer.peek())
        {
        case '[': {
            pmake::preprocessor::Token token {};
            token.type  = pmake::preprocessor::Token::Type::LEFT_SQUARE_BRACKET;
            token.data += lexer.take();
            token.begin = lexer.cursor() - token.data.size();
            token.end   = lexer.cursor();
            tokens.push_back(std::move(token));
            TRY(tokenize_expression(lexer, tokens, depth + 1));
            break;
        }
        case ']': {
            if (depth == 0) return {};
            pmake::preprocessor::Token token {};
            token.type  = pmake::preprocessor::Token::Type::RIGHT_SQUARE_BRACKET;
            token.data += lexer.take();
            token.begin = lexer.cursor() - token.data.size();
            token.end   = lexer.cursor();
            tokens.push_back(std::move(token));
            return {};
        }
        case '<': {
            pmake::preprocessor::Token token {};
            token.type  = pmake::preprocessor::Token::Type::LEFT_ANGLE_BRACKET;
            token.data += lexer.take();
            token.begin = lexer.cursor() - token.data.size();
            token.end   = lexer.cursor();
            tokens.push_back(std::move(token));
            tokenize_literal(lexer, tokens);
            MUST(expect_to_peek(lexer, '>'));
            break;
        }
        case '>': {
            pmake::preprocessor::Token token {};
            token.type  = pmake::preprocessor::Token::Type::RIGHT_ANGLE_BRACKET;
            token.data += lexer.take();
            token.begin = lexer.cursor() - token.data.size();
            token.end   = lexer.cursor();
            tokens.push_back(std::move(token));
            break;
        }
        default: {
            pmake::preprocessor::Token token {};
            token.type = pmake::preprocessor::Token::Type::IDENTIFIER;

            while (!lexer.eof() && !is_space(lexer.peek()))
            {
                token.data += lexer.take();
            }

            token.begin = lexer.cursor() - token.data.size();
            token.end   = lexer.cursor();
            tokens.push_back(std::move(token));
            break;
        }
        }
    }

    return {};
}

namespace pmake::preprocessor {

error::ErrorOr<std::vector<Token>> Lexer::tokenize()
{
    std::vector<Token> tokens {};

    while (!eof())
    {
        switch (peek())
        {
        case '%': {
            drop_while(*this, is_space);
            Token token {};
            token.type  = Token::Type::PERCENT;
            token.data += take();
            token.begin = cursor_m - token.data.size();
            token.end   = cursor_m;
            tokens.push_back(std::move(token));
            tokenize_keyword(*this, tokens);
            break;
        }
        case '[': {
            drop_while(*this, is_space);
            Token token {};
            token.type  = Token::Type::LEFT_SQUARE_BRACKET;
            token.data += take();
            token.begin = cursor_m - token.data.size();
            token.end   = cursor_m;
            tokens.push_back(std::move(token));
            TRY(tokenize_expression(*this, tokens));
            MUST(expect_to_peek(*this, ']'));
            break;
        }
        case ']': {
            drop_while(*this, is_space);
            Token token {};
            token.type  = Token::Type::RIGHT_SQUARE_BRACKET;
            token.data += take();
            token.begin = cursor_m - token.data.size();
            token.end   = cursor_m;
            tokens.push_back(std::move(token));
            drop_while(*this, is_newline);
            break;
        }
        case ':': {
            drop_while(*this, is_space);
            Token token {};
            token.type  = Token::Type::COLON;
            token.data += take();
            token.begin = cursor_m - token.data.size();
            token.end   = cursor_m;
            tokens.push_back(std::move(token));

            take();

            auto const spaces = [&] {
                auto count = 0zu;
                while (std::isspace(peek())) { count += 1; take(); }
                return count;
            }();

            if (peek() != '%')
            {
                std::ranges::for_each(std::views::iota(0zu, spaces), [&] (auto) { untake(); });
            }

            break;
        }
        case ' ': {
            auto const spaces = [&] {
                auto count = 0zu;
                while (peek() != '<' && !std::isalpha(peek())) { count += 1; take(); }
                return count;
            }();

            if (peek() == '<' && TRY(peek_next()) == '<')
            {
                take();
                take();
                MUST(expect_to_peek(*this, ' '));
                take();
            }
            else
            {
                std::ranges::for_each(std::views::iota(0zu, spaces), [&] (auto) { untake(); });
                tokenize_content(*this, tokens);
            }

            break;
        }
        default: {
            tokenize_content(*this, tokens);
            break;
        }
        }
    }

    return tokens;
}

}
