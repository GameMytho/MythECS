# import gtest
include(${CMAKE_SOURCE_DIR}/cmake/DependencyFetch.cmake)
include(${CMAKE_SOURCE_DIR}/tests/cmake/Utilities.cmake)

# Prefer a system-installed GoogleTest; fall back to fetching a pinned release.
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    if(MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    fetch_dependency(googletest https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip)
endif()

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/core/concept.cc"
    concept
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_ConceptTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/core/type_info.cc"
    type_info
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_TypeInfoTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/core/compressed_pair.cc"
    compressed_pair
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_CompressedPairTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/core/container/sparse_set.cc"
    sparse_set
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_SparseSetTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/core/container/any_vector.cc"
    any_vector
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_AnyVectorTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/ecs/entity.cc"
    entity
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_EntityTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/storage/entity_set.cc"
    entity_set
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_EntitySetTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/storage/entity_storage.cc"
    entity_storage
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_EntityStorageTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/core/container/dense_set.cc"
    dense_set
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_DenseSetTest
)

add_unit_test(
    "src/unit/src/include"
    "src/unit/src/storage/archtype.cc"
    archtype
    "MythECS;GTest::gtest"
    GTest::gtest_main
    TARGET_NAME MythECS_ArchtypeTest
)