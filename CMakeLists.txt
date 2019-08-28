cmake_minimum_required(VERSION 3.14)

project(Scope)

set(CMAKE_CXX_STANDARD 17)

add_executable(Scope)

target_sources(Scope PRIVATE
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/BaseUniqueResource.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/ScopeBox.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/Policy.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/ScopeGuard.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/Traits.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Details/ResourceBox.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/Scope.h
        ${PROJECT_SOURCE_DIR}/Public/Scope/UniqueResource.h
        ${PROJECT_SOURCE_DIR}/main.cpp)

target_include_directories(Scope PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Public>)