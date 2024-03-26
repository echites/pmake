cmake_minimum_required(VERSION 3.25)

project(pmake LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(EXPORT_DIR "${CMAKE_SOURCE_DIR}/build/cmake")

include(static_analyzers)
include(enable_vcpkg)
include(remote_clone)

option(ENABLE_CLANGTIDY "" TRUE)
option(ENABLE_CPPCHECK "" TRUE)

enable_vcpkg()

set(pmake_CompilerOptions ${pmake_CompilerOptions} -Wno-gnu-statement-expression-from-macro-expansion)
# set(pmake_LinkerOptions ${pmake_LinkerOptions})

find_package(cxxopts CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(err_or CONFIG REQUIRED)

set(pmake_ExternalLibraries cxxopts::cxxopts nlohmann_json::nlohmann_json err_or::err_or)

add_subdirectory(pmake)
