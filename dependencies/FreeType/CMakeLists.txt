message(STATUS "Fetching FreeType...")

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(FreeType
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_TAG 6a2b3e4007e794bfc6c91030d0ed987f925164a8 # 2.10.4
)
FetchContent_MakeAvailable(FreeType)

add_library(dependency_FreeType INTERFACE)
target_include_directories(dependency_FreeType SYSTEM INTERFACE "${freetype_SOURCE_DIR}/include")
target_link_libraries(dependency_FreeType INTERFACE freetype)
