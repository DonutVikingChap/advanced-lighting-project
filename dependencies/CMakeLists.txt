include(FetchContent)

add_subdirectory(assimp)
add_subdirectory(fmt)
add_subdirectory(FreeType)
add_subdirectory(glm)
add_subdirectory(OpenGL)
add_subdirectory(GLEW) # Depends on OpenGL.
add_subdirectory(lightmapper) # Depends on GLEW and OpenGL.
add_subdirectory(SDL2)
add_subdirectory(stb)
add_subdirectory(imgui) # Depends on SDL2, GLEW and OpenGL.
