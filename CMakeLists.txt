cmake_minimum_required(VERSION 3.25)

project(pmake LANGUAGES CXX)

set(EXPORT_DIR "${CMAKE_SOURCE_DIR}/build/cmake")

include(cmake/static_analyzers.cmake)
include(cmake/CPM.cmake)
include(cmake/remote_clone.cmake)

option(ENABLE_CLANGTIDY "" TRUE)
option(ENABLE_CPPCHECK "" TRUE)

set(pmake_CompilerOptions ${pmake_CompilerOptions} -Wno-gnu-statement-expression-from-macro-expansion)
# set(pmake_LinkerOptions ${pmake_LinkerOptions})

CPMAddPackage("gh:jarro2783/cxxopts@3.0.0")
CPMAddPackage("gh:nlohmann/json@3.11.3")
CPMAddPackage("gh:nyyakko/LibError#8a2a5aa")
CPMAddPackage("gh:nyyakko/LibPreprocessor#2fd3bb9")

set(pmake_ExternalLibraries cxxopts nlohmann_json::nlohmann_json LibError::LibError LibPreprocessor::LibPreprocessor)

add_subdirectory(pmake)
