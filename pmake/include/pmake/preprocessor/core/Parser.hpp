#pragma once

#include "Token.hpp"
#include "nodes/INode.hpp"

#include <liberror/ErrorOr.hpp>
#include <liberror/types/TraceError.hpp>

#include <vector>
#include <stack>

namespace pmake::preprocessor {

class Parser
{
public:
    explicit Parser(std::vector<Token> const& tokens)
        : tokens_m { tokens | std::views::reverse | std::ranges::to<std::stack>() }
    {}

    liberror::ErrorOr<std::unique_ptr<INode>> parse() { return this->parse(0); }
    liberror::ErrorOr<std::unique_ptr<INode>> parse(size_t depth);

    bool eof() const { return tokens_m.empty(); }
    Token const& peek() { return tokens_m.top(); }
    Token take() { auto value = tokens_m.top(); tokens_m.pop(); return value; }
    std::stack<Token>& tokens() { return tokens_m; }

private:
    std::stack<Token> tokens_m {};
};

} // pmake::preprocessor
