cmake_minimum_required(VERSION 3.27)

project(
    MythECS
    VERSION 0.0.1
    DESCRIPTION "A Header-only ECS Framework Based On C++20 And Cmake."
    HOMEPAGE_URL "https://github.com/GameMytho/MythECS"
    LANGUAGES CXX
)

option(ENABLE_TESTS "Enable buiding unit tests" OFF)

add_subdirectory(src)

if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()