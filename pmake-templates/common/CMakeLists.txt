cmake_minimum_required(VERSION 3.25)

%SWITCH [<ENV:LANGUAGE>]:
    %CASE [<"c++">]:
        << project(!PROJECT! LANGUAGES CXX)
    %END
    %CASE [<"c">]:
        << project(!PROJECT! LANGUAGES C)
    %END
%END

set(EXPORT_DIR "${CMAKE_SOURCE_DIR}/build/cmake")

include(cmake/static_analyzers.cmake)
include(cmake/CPM.cmake)
include(cmake/remote_clone.cmake)
%IF [[<ENV:FEATURES> CONTAINS <"testable">] OR [<ENV:KIND> EQUALS <"library">]]:
include(cmake/enable_tests.cmake)
%END
%IF [[<ENV:FEATURES> CONTAINS <"installable">] OR [<ENV:KIND> EQUALS <"library">]]:
include(GNUInstallDirs)
%END

option(ENABLE_CLANGTIDY "" TRUE)
option(ENABLE_CPPCHECK "" TRUE)
%IF [[<ENV:FEATURES> CONTAINS <"testable">] OR [<ENV:KIND> EQUALS <"library">]]:
option(ENABLE_TESTING "" TRUE)

if (ENABLE_TESTING)
    enable_tests()
endif()
%END

# set(!PROJECT!_CompilerOptions ${!PROJECT!_CompilerOptions})
# set(!PROJECT!_LinkerOptions ${!PROJECT!_LinkerOptions})

# CPMAddPackage("gh:repo/package1#REF")
# CPMAddPackage("gh:repo/package2#REF")
# ...
#
# set(!PROJECT!_ExternalLibraries package1 package2 ...)

add_subdirectory(!PROJECT!)

