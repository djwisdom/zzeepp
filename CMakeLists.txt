cmake_minimum_required(VERSION 3.14)

message(STATUS "CMakeLists: Zep")

# Global Options
option(BUILD_QT "Make Qt Library" OFF)
option(BUILD_IMGUI "Make Imgui Library" OFF)
option(BUILD_DEMOS "Make the demo app" ON)
option(BUILD_TESTS "Make the tests" ON)
option(ZEP_FEATURE_CPP_FILE_SYSTEM "Default File system enabled" ON)

# Global Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)

set(CMAKE_DEBUG_POSTFIX "-debug")
set(CMAKE_RELWITHDEBINFO_POSTFIX "-reldbg")

# Vcpkg integration for demos (only if building demos and vcpkg exists)
if (BUILD_DEMOS AND WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static-md)
    
    # Only set vcpkg toolchain if the directory exists
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake 
            CACHE STRING "Vcpkg toolchain file" FORCE)
    else()
        message(WARNING "Vcpkg not found, demos may fail to build without dependencies")
    endif()
elseif (BUILD_DEMOS AND NOT WIN32)
    message(WARNING "Vcpkg integration is only configured for Windows demos")
endif()

project(Zep
    LANGUAGES CXX C
    VERSION 0.5.0
    DESCRIPTION "A simple embeddable editor with Vim mode"
    HOMEPAGE_URL "https://github.com/Rezonality/zep"
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Feature definitions
if (ZEP_FEATURE_CPP_FILE_SYSTEM)
    add_definitions(-DZEP_FEATURE_CPP_FILE_SYSTEM)
endif()

# Generate config file
set (ZEP_ROOT ${CMAKE_CURRENT_LIST_DIR})
configure_file(${ZEP_ROOT}/cmake/config_app.h.cmake ${CMAKE_BINARY_DIR}/config_app.h)

include(GNUInstallDirs)

# Set compiler flags
include(cmake/all.cmake)

# File copying utilities
include(cmake/copy_files.cmake)

enable_testing()

# Main library
add_subdirectory(src)

# Tests
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Demos
if (BUILD_DEMOS)
    add_subdirectory(demos)
endif()

# CMake package configuration
include(CMakePackageConfigHelpers)

# Export targets
install(EXPORT zep-targets
    FILE zep-targets.cmake
    NAMESPACE Zep::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zep
)

# Config files
configure_package_config_file(
    ${ZEP_ROOT}/cmake/zep-config.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/zep-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zep
)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/cmake/zep-config-version.cmake
    VERSION 1.0
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES
        ${CMAKE_BINARY_DIR}/cmake/zep-config.cmake
        ${CMAKE_BINARY_DIR}/cmake/zep-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zep
)

# CPack installer
include(${CMAKE_CURRENT_LIST_DIR}/cmake/cpack_installer.cmake)