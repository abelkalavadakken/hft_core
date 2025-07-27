cmake_minimum_required(VERSION 3.16)
project(hft_core VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add library
add_library(hft_core STATIC
    src/Config.cpp
    src/EventBus.cpp
    src/Logger.cpp
    src/MemoryPool.cpp
    src/ThreadPool.cpp
    src/Timer.cpp
)

# Include directories
target_include_directories(hft_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compiler flags for release builds
target_compile_options(hft_core PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native>
    $<$<CONFIG:Debug>:-g -O0>
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(hft_core PRIVATE pthread)
elseif(APPLE)
    target_link_libraries(hft_core PRIVATE pthread)
endif()

# Option to build shared library
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    add_library(hft_core_shared SHARED
        src/Config.cpp
        src/EventBus.cpp
        src/Logger.cpp
        src/MemoryPool.cpp
        src/ThreadPool.cpp
        src/Timer.cpp
    )
    
    target_include_directories(hft_core_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    
    target_compile_options(hft_core_shared PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native -mtune=native>
        $<$<CONFIG:Debug>:-g -O0>
    )
    
    if(UNIX AND NOT APPLE)
        target_link_libraries(hft_core_shared PRIVATE pthread)
    elseif(APPLE)
        target_link_libraries(hft_core_shared PRIVATE pthread)
    endif()
    
    set_target_properties(hft_core_shared PROPERTIES OUTPUT_NAME hft_core)
endif()

# Enable testing
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install targets
include(GNUInstallDirs)

install(TARGETS hft_core
    EXPORT hft_core-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_SHARED_LIBS)
    install(TARGETS hft_core_shared
        EXPORT hft_core-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT hft_core-targets
    FILE hft_core-targets.cmake
    NAMESPACE hft::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hft_core
)

# Create config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hft_core-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hft_core-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hft_core
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hft_core-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hft_core-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hft_core-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hft_core
)