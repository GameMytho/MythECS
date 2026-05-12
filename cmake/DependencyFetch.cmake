# ============================================================
# DependencyFetch.cmake
#
# Purpose:
#   Centrally manage the third-party dependencies for projects(downloaded and enabled via FetchContent).
#
# Functions List:
#   - fetch_dependency(NAME URL)
#       Fetches and makes available an external dependency using CMake's FetchContent module.
#       Parameters:
#         NAME: Dependency name (e.g., googletest)
#         URL: Download URL (zip/tar.gz)
#       Usage:
#         include(${CMAKE_SOURCE_DIR}/cmake/DependencyFetch.cmake)
#         fetch_dependency(googletest https://github.com/google/googletest/archive/refs/heads/main.zip)
#
# ============================================================

include(FetchContent)

function(fetch_dependency NAME URL)
    FetchContent_Declare(
        ${NAME}
        URL ${URL}
    )
    FetchContent_MakeAvailable(${NAME})
endfunction()