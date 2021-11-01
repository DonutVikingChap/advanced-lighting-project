message(STATUS "Fetching assimp...")

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_MINIZIP ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp
    GIT_TAG 8f0c6b04b2257a520aaab38421b2e090204b69df # 5.0.1
)

FetchContent_MakeAvailable(assimp)

add_library(dependency_assimp INTERFACE)
target_include_directories(dependency_assimp SYSTEM INTERFACE $<TARGET_PROPERTY:assimp,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(dependency_assimp INTERFACE assimp)
