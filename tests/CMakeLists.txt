# import gtest
include(${CMAKE_SOURCE_DIR}/cmake/DependencyFetch.cmake)
include(${CMAKE_SOURCE_DIR}/tests/cmake/Utilities.cmake)

fetch_dependency(googletest https://github.com/google/googletest/archive/refs/heads/main.zip)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/core/concept.cc"
    concept
    "MythECS;gtest"
    gtest_main
    TARGET_NAME MythECS_ConceptTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/core/container/sparse_set.cc"
    sparse_set
    "MythECS;gtest"
    gtest_main
    TARGET_NAME MythECS_SparseSetTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/ecs/entity.cc"
    entity
    "MythECS;gtest"
    gtest_main
    TARGET_NAME MythECS_EntityTest
)