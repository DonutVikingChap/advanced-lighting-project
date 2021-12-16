#ifndef SHADER_HPP
#define SHADER_HPP

#include "../core/handle.hpp"
#include "../core/opengl.hpp"
#include "../utilities/preprocessor.hpp"

#include <array>            // std::array
#include <cstddef>          // std::size_t
#include <fmt/format.h>     // fmt::format
#include <fstream>          // std::ifstream
#include <initializer_list> // std::initializer_list
#include <sstream>          // std::ostringstream
#include <stdexcept>        // std:runtime_error
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <unordered_map>    // std::unordered_map
#include <utility>          // std::move, std::index_sequence, std::make_index_sequence
#include <vector>           // std::vector

struct shader_error : std::runtime_error {
	explicit shader_error(const auto& message)
		: std::runtime_error(message) {}
};

struct shader_definition final {
	shader_definition(std::string_view name, const auto& value)
		: string(fmt::format("#define {} {}\n", name, value)) {}

	std::string string;
};

struct shader_definition_list final {
	constexpr shader_definition_list() noexcept = default;

	constexpr shader_definition_list(std::span<const shader_definition> values) noexcept
		: values(values) {}

	constexpr shader_definition_list(std::initializer_list<shader_definition> values) noexcept
		: values(values.begin(), values.size()) {}

	std::span<const shader_definition> values{};
};

class shader final {
public:
	static constexpr auto default_glsl_version = std::string_view{"330 core"};

	explicit shader(GLenum type, const char* filename = nullptr, std::string_view glsl_version = default_glsl_version, shader_definition_list definitions = {}) {
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
		file.close();

		auto processed_strings = std::vector<std::string>{};
		auto environment = preprocessor_environment{};
		auto file_cache = preprocessor::file_content_map{};
		preprocessor::process_file(filename, fmt::format("#version {}\n", glsl_version), processed_strings, environment, file_cache);
		for (const auto& definition : definitions.values) {
			preprocessor::process_file(filename, definition.string, processed_strings, environment, file_cache);
		}
		preprocessor::process_file(filename, source, processed_strings, environment, file_cache);

		auto strings = std::vector<const GLchar*>{};
		auto lengths = std::vector<GLint>{};
		for (const auto& processed_string : processed_strings) {
			strings.push_back(processed_string.data());
			lengths.push_back(static_cast<GLint>(processed_string.size()));
		}

		glShaderSource(m_shader.get(), static_cast<GLsizei>(strings.size()), strings.data(), lengths.data());
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

struct shader_program_options final {
	const char* vertex_shader_filename = nullptr;
	const char* fragment_shader_filename = nullptr;
	const char* geometry_shader_filename = nullptr;
	const char* tesselation_control_shader_filename = nullptr;
	const char* tesselation_evaluation_shader_filename = nullptr;
	std::string_view glsl_version = shader::default_glsl_version;
	shader_definition_list definitions{};
};

class shader_program final {
public:
	explicit shader_program(const shader_program_options& options)
		: m_vertex_shader(GL_VERTEX_SHADER, options.vertex_shader_filename, options.glsl_version, options.definitions)
		, m_fragment_shader(GL_FRAGMENT_SHADER, options.fragment_shader_filename, options.glsl_version, options.definitions)
		, m_geometry_shader(GL_GEOMETRY_SHADER, options.geometry_shader_filename, options.glsl_version, options.definitions)
		, m_tesselation_control_shader(GL_TESS_CONTROL_SHADER, options.tesselation_control_shader_filename, options.glsl_version, options.definitions)
		, m_tesselation_evaluation_shader(GL_TESS_EVALUATION_SHADER, options.tesselation_evaluation_shader_filename, options.glsl_version, options.definitions) {
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

class shader_uniform final {
public:
	shader_uniform(GLuint program, const char* name) noexcept
		: m_location(glGetUniformLocation(program, name)) {}

	[[nodiscard]] auto location() const noexcept -> GLint {
		return m_location;
	}

private:
	GLint m_location;
};

template <typename T, std::size_t N>
class shader_array final {
public:
	shader_array(GLuint program, const char* name) noexcept
		: m_arr([&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
			return std::array<T, N>{(T{program, fmt::format("{}[{}]", name, Indices).c_str()})...};
		}(std::make_index_sequence<N>{})) {}

	[[nodiscard]] auto size() const noexcept -> std::size_t {
		return m_arr.size();
	}

	[[nodiscard]] auto operator[](std::size_t i) -> T& {
		return m_arr[i];
	}

	[[nodiscard]] auto operator[](std::size_t i) const -> const T& {
		return m_arr[i];
	}

	[[nodiscard]] auto begin() const noexcept -> decltype(auto) {
		return m_arr.begin();
	}

	[[nodiscard]] auto end() const noexcept -> decltype(auto) {
		return m_arr.begin();
	}

private:
	std::array<T, N> m_arr;
};

#endif
