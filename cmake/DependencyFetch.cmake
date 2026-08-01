include(FetchContent)

#
# Fetches and makes available an external dependency using CMake's FetchContent module.
#
# Parameters:
#   NAME: Dependency name (e.g., googletest)
#   URL: Download URL (zip/tar.gz)
#

function(fetch_dependency NAME URL)
    FetchContent_Declare(
        ${NAME}
        URL ${URL}
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(${NAME})
endfunction()