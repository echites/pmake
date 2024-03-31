function(enable_tests)

    message(STATUS "[${PROJECT_NAME}] project will be building the test suit.")

    if (NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
        message(STATUS "[${PROJECT_NAME}] running on ${CMAKE_HOST_SYSTEM_NAME}, sanitizers for tests are enabled.")
        set(!PROJECT!_TestsLinkerOptions ${!PROJECT!_TestsLinkerOptions} -fsanitize=undefined,leak,address)
    else()
        message(STATUS "[${PROJECT_NAME}] running on ${CMAKE_HOST_SYSTEM_NAME}, sanitizers for tests are disabled.")
    endif()

    set(!PROJECT!_TestsCompilerOptions ${!PROJECT!_TestsCompilerOptions} ${!PROJECT!_CompilerOptions})

    include(GoogleTest)
    find_package(GTest CONFIG REQUIRED)

    enable_testing()
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)

endfunction()
