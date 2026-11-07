# SPDX-FileCopyrightText: Copyright 2026 crueter
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.31)

project(hydrocarbon VERSION 0.1.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CMakeDependentOption)
include(CompilerChecks)

# options
option(HYDROCARBON_BUILD_TESTS "Build Hydrocarbon's test application" ${PROJECT_IS_TOP_LEVEL})
option(HYDROCARBON_INSTALL "Create a Hydrocarbon install target" ${PROJECT_IS_TOP_LEVEL})

if (NOT DEFINED CMAKE_CXX_STANDARD OR "${CMAKE_CXX_STANDARD}" LESS "23")
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)

add_library(hydrocarbon)

target_sources(hydrocarbon PUBLIC
    FILE_SET hydrocarbon TYPE CXX_MODULES
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules FILES
        modules/util.ixx
        modules/hydrocarbon.ixx
        modules/fields.ixx
        modules/enums.ixx
        modules/socket.ixx)

set_target_properties(hydrocarbon PROPERTIES EXPORT_NAME "carbon")
target_link_libraries(hydrocarbon PUBLIC OpenSSL::SSL)

if (WIN32)
    target_link_libraries(hydrocarbon PUBLIC ws2_32)
endif()

add_library(hydro::carbon ALIAS hydrocarbon)

# ccache and sccache do not play very nicely with modules
# This will be removed eventually
include(DisableCCache)

if (HYDROCARBON_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS hydrocarbon
        EXPORT hydrocarbon-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILE_SET hydrocarbon DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hydrocarbon)

    install(EXPORT hydrocarbon-targets
        FILE hydrocarbonTargets.cmake
        NAMESPACE hydro::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/hydrocarbon")

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hydrocarbonConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/hydrocarbonConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hydrocarbon)

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/hydrocarbonConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/hydrocarbonConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/hydrocarbonConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hydrocarbon)
endif()

if (HYDROCARBON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
