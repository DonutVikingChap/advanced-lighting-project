message(STATUS "Fetching lightmapper...")

FetchContent_Declare(lightmapper
	GIT_REPOSITORY https://github.com/ands/lightmapper
	GIT_TAG eeaad30be20158d6164d0becceda8d7d742adc1f # 2018-05-14
)
FetchContent_MakeAvailable(lightmapper)

add_library(lightmapper STATIC "lightmapper.cpp")
target_include_directories(lightmapper PUBLIC "${lightmapper_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(lightmapper PRIVATE cxx_std_20)
target_compile_options(lightmapper PRIVATE
	$<$<CXX_COMPILER_ID:GNU>:           $<$<CONFIG:Debug>:-g>   $<$<NOT:$<CONFIG:Debug>>:-O3>>
	$<$<CXX_COMPILER_ID:Clang>:         $<$<CONFIG:Debug>:-g>   $<$<NOT:$<CONFIG:Debug>>:-O3>>
	$<$<CXX_COMPILER_ID:MSVC>:  /utf-8  $<$<CONFIG:Debug>:/Od>  $<$<NOT:$<CONFIG:Debug>>:/Ot>>)
target_link_libraries(lightmapper PUBLIC
	dependency_GLEW
	dependency_OpenGL)

add_library(dependency_lightmapper INTERFACE)
target_include_directories(dependency_lightmapper SYSTEM INTERFACE $<TARGET_PROPERTY:lightmapper,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(dependency_lightmapper INTERFACE lightmapper)
