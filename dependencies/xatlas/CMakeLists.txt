message(STATUS "Fetching xatlas...")

FetchContent_Declare(xatlas
	GIT_REPOSITORY https://github.com/jpcy/xatlas
	GIT_TAG ec707faeac3b95e6b416076a9509718cce105b6a # 2021-05-12
)
FetchContent_MakeAvailable(xatlas)

add_library(xatlas STATIC "${xatlas_SOURCE_DIR}/source/xatlas/xatlas.cpp")
target_include_directories(xatlas PUBLIC "${xatlas_SOURCE_DIR}/source/xatlas")
target_compile_features(xatlas PRIVATE cxx_std_11)
target_compile_options(xatlas PRIVATE
	$<$<CXX_COMPILER_ID:GNU>:           $<$<CONFIG:Debug>:-g>   $<$<NOT:$<CONFIG:Debug>>:-O3>>
	$<$<CXX_COMPILER_ID:Clang>:         $<$<CONFIG:Debug>:-g>   $<$<NOT:$<CONFIG:Debug>>:-O3>>
	$<$<CXX_COMPILER_ID:MSVC>:  /utf-8  $<$<CONFIG:Debug>:/Od>  $<$<NOT:$<CONFIG:Debug>>:/Ot>>)
target_link_libraries(xatlas PRIVATE
	dependency_Threads)

add_library(dependency_xatlas INTERFACE)
target_include_directories(dependency_xatlas SYSTEM INTERFACE $<TARGET_PROPERTY:xatlas,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(dependency_xatlas INTERFACE xatlas)
