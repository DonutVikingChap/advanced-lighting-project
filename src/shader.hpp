#ifndef SHADER_HPP
#define SHADER_HPP

#include "handle.hpp"
#include "opengl.hpp"

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

	shader(GLenum type, const char* filename = nullptr, std::string_view glsl_version = default_glsl_version, shader_definition_list definitions = {}) {
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
		auto file_cache = std::unordered_map<std::string, std::string>{};
		const auto& source = file_cache.emplace(std::string{filename}, std::move(stream).str()).first->second;
		file.close();

		auto processed = processed_sources{};
		auto header = fmt::format("#version {}\n", glsl_version);
		for (const auto& definition : definitions.values) {
			header.append(definition.string);
		}
		processed.add(header);
		preprocess(processed, file_cache, filename, source);

		glShaderSource(m_shader.get(), static_cast<GLsizei>(processed.strings.size()), processed.strings.data(), processed.lengths.data());
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
	static constexpr auto include_directive = std::string_view{"#include"};

	struct processed_sources final {
		std::vector<const GLchar*> strings;
		std::vector<GLint> lengths;

		auto add(std::string_view str) -> void {
			strings.push_back(str.data());
			lengths.push_back(static_cast<GLint>(str.size()));
		}
	};

	auto preprocess(processed_sources& processed, std::unordered_map<std::string, std::string>& file_cache, std::string_view filename, std::string_view source) -> void {
		auto i = std::size_t{0};
		do {
			const auto begin = i;
			i = source.find(include_directive, i);
			const auto end = i;
			if (end != std::string_view::npos) {
				i += include_directive.size();
				if (i = source.find_first_not_of(" \t", i); i != std::string_view::npos) {
					const auto quote_char = source[i++];
					if (quote_char != '\"' && quote_char != '<') {
						throw shader_error{fmt::format("Invalid filename quote in include directive in shader \"{}\"!", filename)};
					}
					const auto quote_begin = i;
					i = source.find(quote_char, i);
					const auto quote_end = i;
					if (quote_end != std::string_view::npos) {
						++i;
						const auto quote = source.substr(quote_begin, quote_end - quote_begin);
						const auto [it, inserted] = file_cache.try_emplace(std::string{quote});
						const auto filename_prefix = filename.substr(0, filename.rfind('/') + 1);
						auto included_filename = std::string{filename_prefix} + it->first;
						if (inserted) {
							auto included_file = std::ifstream{};
							if (quote_char == '\"') {
								included_file.open(included_filename);
							}
							if (!included_file) {
								included_filename.erase(0, filename_prefix.size());
								included_file.open(included_filename);
								if (!included_file) {
									throw shader_error{fmt::format("Failed to open included file \"{}\" in shader \"{}\"!", included_filename, filename)};
								}
							}
							auto stream = std::ostringstream{};
							stream << included_file.rdbuf();
							it->second = std::move(stream).str();
						}
						preprocess(processed, file_cache, included_filename, it->second);
					} else {
						throw shader_error{fmt::format("Missing end quote for include directive filename in shader \"{}\"!", filename)};
					}
				} else {
					throw shader_error{fmt::format("Missing filename for include directive in shader \"{}\"!", filename)};
				}
			}
			processed.add(source.substr(begin, end - begin));
		} while (i < source.size());
	}

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
