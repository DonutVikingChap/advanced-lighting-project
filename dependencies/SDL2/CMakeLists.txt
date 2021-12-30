message(STATUS "Fetching SDL2...")

if(BUILD_SHARED_LIBS)
	set(SDL_SHARED ON CACHE BOOL "" FORCE)
	set(SDL_STATIC OFF CACHE BOOL "" FORCE)
else()
	set(SDL_STATIC ON CACHE BOOL "" FORCE)
	set(SDL_SHARED OFF CACHE BOOL "" FORCE)
endif()

if(NOT MINGW)
	link_libraries($<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:wayland-client>)
endif()

FetchContent_Declare(SDL2
	GIT_REPOSITORY https://github.com/libsdl-org/SDL
	GIT_TAG 2e9821423a237a1206e3c09020778faacfe430be # 2.0.18
)
FetchContent_MakeAvailable(SDL2)

add_library(dependency_SDL2 INTERFACE)
if(BUILD_SHARED_LIBS)
	target_include_directories(dependency_SDL2 SYSTEM INTERFACE $<TARGET_PROPERTY:SDL2,INTERFACE_INCLUDE_DIRECTORIES>)
	target_link_libraries(dependency_SDL2 INTERFACE SDL2 SDL2main)
else()
	target_include_directories(dependency_SDL2 SYSTEM INTERFACE $<TARGET_PROPERTY:SDL2-static,INTERFACE_INCLUDE_DIRECTORIES>)
	target_link_libraries(dependency_SDL2 INTERFACE SDL2-static SDL2main)
endif()
