add_subdirectory(files)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(pmake_SourceFiles ${pmake_SourceFiles}
    "${DIR}/Main.cpp"
    "${DIR}/PMake.cpp"

    PARENT_SCOPE
)
