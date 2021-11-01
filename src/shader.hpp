#ifndef SHADER_HPP
#define SHADER_HPP

#include "handle.hpp"
#include "opengl.hpp"

#include <fmt/format.h> // fmt::...
#include <fstream>      // std::ifstream
#include <sstream>      // std::ostringstream
#include <stdexcept>    // std:runtime_error
#include <string>       // std::string
#include <utility>      // std::move

struct shader_error : std::runtime_error {
	explicit shader_error(const auto& message)
		: std::runtime_error(message) {}
};

class shader final {
public:
	explicit shader(GLenum type, const char* filename = nullptr) {
		if (!filename) {
			return;
		}

		m_shader.reset(glCreateShader(type));
		if (!m_shader) {
			throw opengl_error{"Failed to create shader!"};
		}

		auto file = std::ifstream{filename};
		if (!file) {
			throw shader_error{fmt::format("Failed to read shader code file \"{}\"!\n", filename)};
		}
		auto stream = std::ostringstream{};
		stream << file.rdbuf();
		const auto source = std::move(stream).str();
		const auto* const source_ptr = static_cast<const GLchar*>(source.c_str());
		file.close();
		glShaderSource(m_shader.get(), 1, &source_ptr, nullptr);

		glCompileShader(m_shader.get());

		auto success = GLint{GL_FALSE};
		glGetShaderiv(m_shader.get(), GL_COMPILE_STATUS, &success);
		if (success != GL_TRUE) {
			auto info_log_length = GLint{0};
			glGetShaderiv(m_shader.get(), GL_INFO_LOG_LENGTH, &info_log_length);
			if (info_log_length > 0) {
				auto info_log = std::string(static_cast<std::size_t>(info_log_length), '\0');
				glGetShaderInfoLog(m_shader.get(), info_log_length, nullptr, info_log.data());
				throw shader_error{fmt::format("Failed to compile shader \"{}\":\n{}", filename, info_log.c_str())};
			}
			throw shader_error{fmt::format("Failed to compile shader \"{}\"!", filename)};
		}
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_shader.get();
	}

private:
	struct shader_deleter final {
		auto operator()(GLuint p) const noexcept -> void {
			glDeleteShader(p);
		}
	};
	using shader_ptr = unique_handle<shader_deleter>;

	shader_ptr m_shader{};
};

class shader_program final {
public:
	explicit shader_program(const char* vert_filename = nullptr, const char* frag_filename = nullptr, const char* geom_filename = nullptr, const char* tc_filename = nullptr,
		const char* te_filename = nullptr)
		: m_vertex_shader(GL_VERTEX_SHADER, vert_filename)
		, m_fragment_shader(GL_FRAGMENT_SHADER, frag_filename)
		, m_geometry_shader(GL_GEOMETRY_SHADER, geom_filename)
		, m_tesselation_control_shader(GL_TESS_CONTROL_SHADER, tc_filename)
		, m_tesselation_evaluation_shader(GL_TESS_EVALUATION_SHADER, te_filename) {
		if (m_vertex_shader.get() != 0) {
			glAttachShader(m_program.get(), m_vertex_shader.get());
		}
		if (m_fragment_shader.get() != 0) {
			glAttachShader(m_program.get(), m_fragment_shader.get());
		}
		if (m_geometry_shader.get() != 0) {
			glAttachShader(m_program.get(), m_geometry_shader.get());
		}
		if (m_tesselation_control_shader.get() != 0) {
			glAttachShader(m_program.get(), m_tesselation_control_shader.get());
		}
		if (m_tesselation_evaluation_shader.get() != 0) {
			glAttachShader(m_program.get(), m_tesselation_evaluation_shader.get());
		}

		glLinkProgram(m_program.get());

		auto success = GLint{GL_FALSE};
		glGetProgramiv(m_program.get(), GL_LINK_STATUS, &success);
		if (success != GL_TRUE) {
			auto info_log_length = GLint{0};
			glGetProgramiv(m_program.get(), GL_INFO_LOG_LENGTH, &info_log_length);
			if (info_log_length > 0) {
				auto info_log = std::string(static_cast<std::size_t>(info_log_length), '\0');
				glGetProgramInfoLog(m_program.get(), info_log_length, nullptr, info_log.data());
				throw shader_error{fmt::format("Failed to link shader program:\n{}", info_log.c_str())};
			}
			throw shader_error{"Failed to link shader program!"};
		}
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_program.get();
	}

private:
	struct program_deleter final {
		auto operator()(GLuint p) const noexcept -> void {
			glDeleteProgram(p);
		}
	};
	using program_ptr = unique_handle<program_deleter>;

	program_ptr m_program{[] {
		auto program = glCreateProgram();
		if (program == 0) {
			throw opengl_error{"Failed to create shader program!"};
		}
		return program;
	}()};
	shader m_vertex_shader;
	shader m_fragment_shader;
	shader m_geometry_shader;
	shader m_tesselation_control_shader;
	shader m_tesselation_evaluation_shader;
};

#endif
