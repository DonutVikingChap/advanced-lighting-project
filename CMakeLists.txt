cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project("TSBK03 Advanced Lighting Project" LANGUAGES C CXX)

option(BUILD_SHARED_LIBS "Build dependencies as shared libraries" OFF)
option(USE_CLANG_TIDY "Use clang-tidy for static analysis warnings" OFF)

add_subdirectory(dependencies)

add_executable(tsbk03 "src/main.cpp")
target_include_directories(tsbk03 PRIVATE "src")
target_compile_features(tsbk03 PRIVATE cxx_std_20)
target_compile_options(tsbk03 PRIVATE
	$<$<CXX_COMPILER_ID:GNU>:   -Wall -Wextra   -Wpedantic      -Werror                 $<$<CONFIG:Debug>:-g>   $<$<NOT:$<CONFIG:Debug>>:-O3>>
	$<$<CXX_COMPILER_ID:Clang>: -Wall -Wextra   -Wpedantic      -Werror                 $<$<CONFIG:Debug>:-g>   $<$<NOT:$<CONFIG:Debug>>:-O3>>
	$<$<CXX_COMPILER_ID:MSVC>:  /W3             /permissive-    /WX     /wd4996 /utf-8  $<$<CONFIG:Debug>:/Od>  $<$<NOT:$<CONFIG:Debug>>:/Ot>>)
target_link_libraries(tsbk03 PRIVATE
	dependency_assimp
	dependency_fmt
	dependency_FreeType
	dependency_GLEW
	dependency_glm
	dependency_imgui
	dependency_lightmapper
	dependency_OpenGL
	dependency_SDL2
	dependency_stb
	dependency_xatlas)

if(USE_CLANG_TIDY)
	find_program(CLANG_TIDY NAMES clang-tidy REQUIRED)
	set_property(TARGET tsbk03 PROPERTY CXX_CLANG_TIDY ${CLANG_TIDY})
	if(MSVC)
		set_target_properties(tsbk03 PROPERTIES
			VS_GLOBAL_RunCodeAnalysis true
			VS_GLOBAL_EnableMicrosoftCodeAnalysis false
			VS_GLOBAL_EnableClangTidyCodeAnalysis true
			VS_GLOBAL_ClangTidyToolExe "${CLANG_TIDY}")
	endif()
endif()

if(BUILD_SHARED_LIBS)
	target_link_libraries(tsbk03 PRIVATE ${CMAKE_DL_LIBS})
	add_custom_command(TARGET tsbk03
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:assimp>
			$<TARGET_FILE:fmt>
			$<TARGET_FILE:GLEW>
			$<TARGET_FILE:SDL2>
			$<TARGET_FILE_DIR:tsbk03>)
endif()

include(GNUInstallDirs)

set_target_properties(tsbk03 PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_INSTALL_BINDIR}")

install(TARGETS tsbk03
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/assets/" DESTINATION "${CMAKE_INSTALL_BINDIR}/assets/")
