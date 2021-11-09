message(STATUS "Fetching lightmapper...")

FetchContent_Declare(lightmapper
	GIT_REPOSITORY https://github.com/ands/lightmapper
	GIT_TAG eeaad30be20158d6164d0becceda8d7d742adc1f # 1
)
FetchContent_MakeAvailable(lightmapper)

add_library(lightmapper STATIC "lightmapper.c")
target_include_directories(lightmapper PUBLIC "${lightmapper_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(lightmapper PUBLIC
	dependency_GLEW
	dependency_OpenGL)

add_library(dependency_lightmapper INTERFACE)
target_include_directories(dependency_lightmapper SYSTEM INTERFACE $<TARGET_PROPERTY:lightmapper,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(dependency_lightmapper INTERFACE lightmapper)
