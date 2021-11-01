#ifndef OPENGL_HPP
#define OPENGL_HPP

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#ifdef _WIN32
#include <Windows.h>
#undef near
#undef far
#undef min
#undef max
#endif
#endif
#include <GL/glew.h>

#include <stdexcept> // std::runtime_error

struct opengl_error : std::runtime_error {
	explicit opengl_error(const auto& message)
		: std::runtime_error(message) {}
};

#endif
