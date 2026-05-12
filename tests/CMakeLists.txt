# import gtest
include(${CMAKE_SOURCE_DIR}/cmake/DependencyFetch.cmake)
include(${CMAKE_SOURCE_DIR}/tests/cmake/Utilities.cmake)

fetch_dependency(googletest https://github.com/google/googletest/archive/refs/heads/main.zip)

add_unit_test(./src/unit main MythECS TARGET_NAME MythECS_MainTest)