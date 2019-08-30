cmake_minimum_required(VERSION 3.8)

if (${CMAKE_VERSION} VERSION_LESS 3.13)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else ()
    cmake_policy(VERSION 3.13)
endif ()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INSTALL_GTEST OFF)
set(BUILD_GMOCK OFF)
set(gtest_force_shared_crt ON)

option(ENABLE_TESTS "Generate test target" ON)

project(scope VERSION 1.0.0)

add_library(scope INTERFACE)

target_sources(scope INTERFACE
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/BaseUniqueResource.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/ScopeBox.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/Policy.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/ScopeGuard.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/Traits.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/ResourceBox.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Scope.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/UniqueResource.h)
target_include_directories(scope INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Public>)

add_executable(scope-example main.cpp)
target_link_libraries(scope-example PRIVATE scope)

if (ENABLE_TESTS)
    enable_testing()

    add_subdirectory(googletest EXCLUDE_FROM_ALL)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(PEDANTIC_COMPILE_FLAGS
                -Werror -Wall -Wextra -Wpedantic -Wconversion -Wno-sign-conversion
                -Wold-style-cast -Wcovered-switch-default -Wundefined-func-template
                -Wnarrowing -Wundefined-reinterpret-cast -Wredundant-parens
                -Wmissing-exception-spec -Wundef -Wpointer-arith -Wshadow-all -Wno-shadow-field-in-constructor)
    endif ()

    add_executable(scope-test tests/Scope.cpp)
    target_compile_options(scope-test PRIVATE ${PEDANTIC_COMPILE_FLAGS} #[[-fprofile-instr-generate -fcoverage-mapping]])
    #[[target_link_options(scope-test PRIVATE -fprofile-instr-generate -fcoverage-mapping)]]
    target_link_libraries(scope-test PRIVATE scope gtest_main)
    add_test(NAME scope COMMAND scope-test)
endif ()
