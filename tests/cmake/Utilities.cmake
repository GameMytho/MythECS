#
# Creates a unit test executable and registers it with CTest.
#
# Parameters:
#   INCLUDE_DIRS: Directories to include for the test
#   SRC_FILES: Source files for the test
#   LABEL: Label to categorize the test
#   INTF_LIB_NAMES: Interface library names to link against
#   ENTRY: Entry point for the test (e.g., gtest_main)
#   TARGET_NAME: Optional, custom target name
#

function(add_unit_test INCLUDE_DIRS SRC_FILES LABEL INTF_LIB_NAMES ENTRY)
    set(oneValueArgs TARGET_NAME)
    cmake_parse_arguments(MAT "" "TARGET_NAME" "" "${ARGN}")

    if(MAT_TARGET_NAME)
        set(TARGET ${MAT_TARGET_NAME})
    else()
        set(TARGET "MythECS_${LABEL}Tests")
    endif()

    set(TEST_SOURCES ${SRC_FILES})

    add_executable(${TARGET} ${TEST_SOURCES})

    target_include_directories(${TARGET} PRIVATE ${INCLUDE_DIRS})

    target_link_libraries(${TARGET} PRIVATE ${INTF_LIB_NAMES} ${ENTRY})

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