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
%IF [<ENV:FEATURES> CONTAINS <"testable">]:
include(cmake/enable_tests.cmake)
%END
%IF [[<ENV:FEATURES> CONTAINS <"installable">] OR [<ENV:KIND> EQUALS <"library">]]:
include(GNUInstallDirs)
%END

%IF [<ENV:FEATURES> CONTAINS <"testable">]:
if (ENABLE_TESTING)
    enable_tests(${PROJECT_NAME})
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

