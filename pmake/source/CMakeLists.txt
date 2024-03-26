add_subdirectory(filesystem)
add_subdirectory(preprocessor)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(pmake_SourceFiles ${pmake_SourceFiles}
    "${DIR}/Main.cpp"
    "${DIR}/PMake.cpp"
    "${DIR}/preprocessor/Parser.cpp"
    "${DIR}/preprocessor/Lexer.cpp"

    PARENT_SCOPE
)
