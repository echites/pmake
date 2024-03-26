#include "preprocessor/Preprocessor.hpp"

#include "preprocessor/Lexer.hpp"
#include "preprocessor/Parser.hpp"
#include "preprocessor/Interpreter.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace pmake::preprocessor {

error::ErrorOr<void> process_all(std::filesystem::path path, InterpreterContext const& context)
{
    namespace fs = std::filesystem;

    for (auto const& entry : fs::recursive_directory_iterator(path))
    {
        if (!entry.is_regular_file()) { continue; }

        std::stringstream contentStream {};
        std::ifstream inputStream { entry.path() };
        contentStream << inputStream.rdbuf();

        Lexer lexer { contentStream.str() };
        Parser parser { TRY(lexer.tokenize()) };
        auto const ast = TRY(parser.parse());

        std::ofstream outputStream { entry.path() };
        outputStream << TRY(traverse(ast, context));
    }

    return {};
}

}
