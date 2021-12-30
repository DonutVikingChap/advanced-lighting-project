message(STATUS "Fetching assimp...")

set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_MINIZIP ON CACHE BOOL "" FORCE)
add_compile_options($<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-O2>)

FetchContent_Declare(assimp
	GIT_REPOSITORY https://github.com/assimp/assimp
	GIT_TAG 80b0b897edfc8d97a3071033b95dcd6b799e403b # 5.1.4
)
FetchContent_MakeAvailable(assimp)

add_library(dependency_assimp INTERFACE)
target_include_directories(dependency_assimp SYSTEM INTERFACE $<TARGET_PROPERTY:assimp,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(dependency_assimp INTERFACE assimp)
