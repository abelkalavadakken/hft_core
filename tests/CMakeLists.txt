cmake_minimum_required(VERSION 3.16)

# Find Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # Download and build Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Test executables
add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE hft_core gtest_main)

add_executable(test_eventbus test_eventbus.cpp)
target_link_libraries(test_eventbus PRIVATE hft_core gtest_main)

add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE hft_core gtest_main)

add_executable(test_memorypool test_memorypool.cpp)
target_link_libraries(test_memorypool PRIVATE hft_core gtest_main)

add_executable(test_threadpool test_threadpool.cpp)
target_link_libraries(test_threadpool PRIVATE hft_core gtest_main)

add_executable(test_timer test_timer.cpp)
target_link_libraries(test_timer PRIVATE hft_core gtest_main)

# Register tests
include(GoogleTest)
gtest_discover_tests(test_config)
gtest_discover_tests(test_eventbus)
gtest_discover_tests(test_logger)
gtest_discover_tests(test_memorypool)
gtest_discover_tests(test_threadpool)
gtest_discover_tests(test_timer)