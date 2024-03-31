set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(pmake_SourceFiles ${pmake_SourceFiles}
    "${DIR}/Parser.cpp"
    "${DIR}/Lexer.cpp"
    "${DIR}/Interpreter.cpp"

    PARENT_SCOPE
)
