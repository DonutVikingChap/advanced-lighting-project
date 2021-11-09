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
#include <string>    // std::string

struct opengl_error : std::runtime_error {
	explicit opengl_error(const auto& message)
		: std::runtime_error(message) {}
};

struct opengl_context final {
	static auto reset_status() -> void {
		while (glGetError() != GL_NO_ERROR) {
		}
	}

	static auto check_status() -> void {
		if (auto error = glGetError(); error != GL_NO_ERROR) {
			auto error_string = std::string{"OpenGL error:"};
			do {
				switch (error) {
					case GL_INVALID_ENUM: error_string.append(" GL_INVALID_ENUM"); break;
					case GL_INVALID_VALUE: error_string.append(" GL_INVALID_VALUE"); break;
					case GL_INVALID_OPERATION: error_string.append(" GL_INVALID_OPERATION"); break;
					case GL_STACK_OVERFLOW: error_string.append(" GL_STACK_OVERFLOW"); break;
					case GL_STACK_UNDERFLOW: error_string.append(" GL_STACK_UNDERFLOW"); break;
					case GL_OUT_OF_MEMORY: error_string.append(" GL_OUT_OF_MEMORY"); break;
					case GL_INVALID_FRAMEBUFFER_OPERATION: error_string.append(" GL_INVALID_FRAMEBUFFER_OPERATION"); break;
					default: error_string.append(" Unknown"); break;
				}
				error = glGetError();
			} while (error != GL_NO_ERROR);
			throw opengl_error{error_string};
		}
	}

	static auto check_framebuffer_status() -> void {
		if (const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER); status != GL_FRAMEBUFFER_COMPLETE) {
			auto error_string = std::string{"OpenGL framebuffer error:"};
			switch (status) {
				case GL_FRAMEBUFFER_COMPLETE: error_string.append(" GL_FRAMEBUFFER_COMPLETE"); break;
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: error_string.append(" GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: error_string.append(" GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
				case GL_FRAMEBUFFER_UNSUPPORTED: error_string.append(" GL_FRAMEBUFFER_UNSUPPORTED"); break;
				default: error_string.append(" Unknown"); break;
			}
			throw opengl_error{error_string};
		}
	}
};

#endif
