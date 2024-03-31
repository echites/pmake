#include "preprocessor/Preprocessor.hpp"

#include "preprocessor/core/Lexer.hpp"
#include "preprocessor/core/Parser.hpp"
#include "preprocessor/core/Interpreter.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace pmake::preprocessor {

liberror::ErrorOr<void> process_all(std::filesystem::path path, InterpreterContext const& context)
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
