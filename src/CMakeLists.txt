add_library(MythECS INTERFACE)

target_include_directories(MythECS INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/mythecs/
    ${CMAKE_CURRENT_SOURCE_DIR}/mythecs/include
)

target_compile_features(MythECS INTERFACE cxx_std_20)