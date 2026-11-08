# SPDX-FileCopyrightText: Copyright 2026 crueter
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.31)

project(hydrocarbon_test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

if (PROJECT_IS_TOP_LEVEL)
    find_package(hydrocarbon REQUIRED)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(CPMUtil)

# tests use jacinth for json parsing
AddJsonPackage(jacinth)

# test lib
add_library(tests STATIC)

target_sources(tests PUBLIC
    FILE_SET tests TYPE CXX_MODULES FILES tests.ixx)

target_link_libraries(tests PUBLIC hydro::carbon)

if (HYDROCARBON_STRICT_ERRORS)
    if (MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(tests PUBLIC
            /W4 /WX /we4834 /wd4702)
    else()
        target_compile_options(tests PUBLIC
            -Werror=unused-result -Wextra -Werror -Wall
            -Wno-unused-command-line-argument -Wno-c++17-compat
            -Werror=missing-declarations
            -Wno-c++98-compat -Wno-c++98-compat-pedantic)
    endif()
endif()

# need to link to stdc++exp (on glibc++ only) for std::print on mingw
if (MINGW)
    include(CheckCXXSymbolExists)

    check_cxx_symbol_exists(__GLIBCXX__ "version" HAS_LIBSTDCXX)

    if(HAS_LIBSTDCXX)
        message(STATUS "MinGW libstdc++ needs stdc++exp")
        target_link_libraries(tests PUBLIC stdc++exp)
    endif()
endif()

# test cases
set(TESTS get head post redirect put options bundled timeout)

foreach(test ${TESTS})
    add_executable(${test} cases/${test}.cpp)
    target_link_libraries(${test} PRIVATE tests jacinth::jacinth)
    target_include_directories(${test} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/data)

    add_test(NAME ${test} COMMAND ${test})
endforeach()
