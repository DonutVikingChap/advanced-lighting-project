message(STATUS "Fetching glm...")

add_compile_definitions(GLM_FORCE_XYZW_ONLY)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glm
	GIT_REPOSITORY https://github.com/g-truc/glm
	GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e # 0.9.9.8
)
FetchContent_MakeAvailable(glm)

add_library(dependency_glm INTERFACE)
target_include_directories(dependency_glm SYSTEM INTERFACE $<TARGET_PROPERTY:glm,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(dependency_glm INTERFACE GLM_FORCE_XYZW_ONLY)
target_link_libraries(dependency_glm INTERFACE glm)
