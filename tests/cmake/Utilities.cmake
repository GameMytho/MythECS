# ============================================================
# Utilities.cmake
#
# Purpose:
#   Collection of custom CMake helper functions for the tests.
#
# Functions List:
#   - add_unit_test(SRC_DIR LABEL [TARGET_NAME <name>])
#       Creates a unit test executable and registers it with CTest.
#       Parameters:
#         SRC_DIR: Source directory containing test files
#         LABEL: Label to categorize the test
#         TARGET_NAME: Optional, custom target name
#       Usage:
#         include(${CMAKE_SOURCE_DIR}/cmake/Utilities.cmake)
#         add_unit_test(./ main TARGET_NAME MythoECS_MainTest)
#
# ============================================================

function(add_unit_test SRC_DIR LABEL INTF_LIB_NAME)
    set(oneValueArgs TARGET_NAME)
    cmake_parse_arguments(MAT "" "TARGET_NAME" "" "${ARGN}")

    if(MAT_TARGET_NAME)
        set(TARGET ${MAT_TARGET_NAME})
    else()
        set(TARGET "MythECS_${LABEL}Tests")
    endif()

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/src/*.cc)

    add_executable(${TARGET} ${TEST_SOURCES})

    target_include_directories(${TARGET} PRIVATE ${SRC_DIR}/include)

    target_link_libraries(${TARGET} PRIVATE ${INTF_LIB_NAME} GTest::gtest GTest::gtest_main)

    # set compile options for the test executable
    target_compile_options(${TARGET} PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:MinSizeRel>:/O1>
      >
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:MinSizeRel>:-Os>
      >
    )

    # set output directory for the test executable
    set_target_properties(${TARGET} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin/$<CONFIG>"
    )

    # add test to CMake
    add_test(NAME ${TARGET} COMMAND $<TARGET_FILE:${TARGET}>)
    set_tests_properties(${TARGET} PROPERTIES LABELS "${LABEL}")
endfunction()