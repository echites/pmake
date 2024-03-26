set(TEST_NAME example)

project(${TEST_NAME} LANGUAGES CXX)

add_executable(${TEST_NAME} "${TEST_NAME}.cpp")

enable_clang_tidy()
enable_cppcheck()

target_compile_features(${TEST_NAME} PRIVATE cxx_std_23)

target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main !PROJECT!)
target_compile_options(${TEST_NAME} PRIVATE ${!PROJECT!_TestsCompilerOptions})
target_link_options(${TEST_NAME} PRIVATE ${!PROJECT!_TestsLinkerOptions})

gtest_discover_tests(${TEST_NAME})
