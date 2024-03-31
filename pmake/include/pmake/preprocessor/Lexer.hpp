#pragma once

#include "Token.hpp"

#include <liberror/ErrorOr.hpp>
#include <liberror/types/TraceError.hpp>

namespace pmake::preprocessor {

class Lexer
{
    #define LEXER_EOF() if (eof()) return liberror::make_error("End of file reached")
    #define LEXER_SOF() if (sof()) return liberror::make_error("Start of file reached")

public:
    explicit Lexer(std::string_view source): source_m{source} {}

    liberror::ErrorOr<std::vector<Token>> tokenize();

    char take() { return source_m.at(cursor_m++); }
    liberror::ErrorOr<char> untake() { LEXER_SOF(); return source_m.at(cursor_m--); }
    liberror::ErrorOr<char> take_next() { LEXER_EOF(); return source_m.at(cursor_m += 2); }
    char peek() { return source_m.at(cursor_m); }
    liberror::ErrorOr<char> peek_next() { LEXER_EOF(); return source_m.at(cursor_m + 1); }
    bool eof() const { return !((cursor_m + 1) < source_m.size()); }
    bool sof() const { return cursor_m == 0; }
    size_t cursor() const { return cursor_m; }

private:
    std::string source_m {};
    size_t cursor_m {};
};

} // pmake::preprocessor
