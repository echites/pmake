add_subdirectory(filesystem)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(pmake_SourceFiles ${pmake_SourceFiles}
    "${DIR}/main.cpp"
    "${DIR}/PMake.cpp"

    PARENT_SCOPE
)
