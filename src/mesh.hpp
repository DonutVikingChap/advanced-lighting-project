#ifndef MESH_HPP
#define MESH_HPP

#include "glsl.hpp"
#include "handle.hpp"
#include "opengl.hpp"

#include <cstddef>     // std::byte, std::size_t
#include <cstdint>     // std::uintptr_t
#include <memory>      // std::addressof
#include <span>        // std::span
#include <stdexcept>   // std::invalid_argument
#include <type_traits> // std::is_same_v, std::is_standard_layout_v, std::conditional_t

class vertex_buffer final {
public:
	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_vbo.get();
	}

private:
	struct vertex_buffer_deleter final {
		auto operator()(GLuint p) const noexcept -> void {
			glDeleteBuffers(1, &p);
		}
	};
	using vertex_buffer_ptr = unique_handle<vertex_buffer_deleter>;

	vertex_buffer_ptr m_vbo{[] {
		auto vbo = GLuint{};
		glGenBuffers(1, &vbo);
		if (vbo == 0) {
			throw opengl_error{"Failed to create vertex buffer object!"};
		}
		return vbo;
	}()};
};

class vertex_array final {
public:
	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_vao.get();
	}

private:
	struct vertex_array_deleter final {
		auto operator()(GLuint p) const noexcept -> void {
			glDeleteVertexArrays(1, &p);
		}
	};
	using vertex_array_ptr = unique_handle<vertex_array_deleter>;

	vertex_array_ptr m_vao{[] {
		auto vao = GLuint{};
		glGenVertexArrays(1, &vao);
		if (vao == 0) {
			throw opengl_error{"Failed to create vertex array object!"};
		}
		return vao;
	}()};
};

struct no_index final {};

template <typename Vertex, typename Index = no_index>
class mesh final {
public:
	static constexpr auto is_indexed = !std::is_same_v<Index, no_index>;

	template <typename... Ts>
	mesh(GLenum usage, std::span<const Vertex> vertices, Ts(Vertex::*... vertex_attributes)) requires(!is_indexed) {
		const auto preserver = state_preserver{};
		glBindVertexArray(m_vao.get());
		buffer_vertex_data(usage, vertices, vertex_attributes...);
	}

	template <typename... Ts>
	mesh(GLenum usage, std::span<const Vertex> vertices, std::span<const Index> indices, Ts(Vertex::*... vertex_attributes)) requires(is_indexed) {
		const auto preserver = state_preserver{};
		glBindVertexArray(m_vao.get());
		buffer_vertex_data(usage, vertices, vertex_attributes...);
		buffer_index_data(usage, indices);
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_vao.get();
	}

private:
	class [[nodiscard]] state_preserver final {
	public:
		state_preserver() noexcept {
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertex_array_binding);
		}

		~state_preserver() {
			glBindVertexArray(m_vertex_array_binding);
		}

		state_preserver(const state_preserver&) = delete;
		state_preserver(state_preserver &&) = delete;
		auto operator=(const state_preserver&)->state_preserver& = delete;
		auto operator=(state_preserver &&)->state_preserver& = delete;

	private:
		GLint m_vertex_array_binding = 0;
	};

	struct empty {};

	template <typename... Ts>
	auto buffer_vertex_data(GLenum usage, std::span<const Vertex> vertices, Ts(Vertex::*... vertex_attributes)) -> void {
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo.get());
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), usage);
		auto attribute_index = GLuint{0};
		(setup_vertex_attribute(attribute_index++, vertex_attributes), ...);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	auto buffer_index_data(GLenum usage, std::span<const Index> indices) -> void requires(is_indexed) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo.get());
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(Index)), indices.data(), usage);
	}

	template <typename T>
	static auto setup_vertex_attribute(GLuint index, T(Vertex::*attribute)) -> void {
		static_assert(std::is_standard_layout_v<Vertex>, "Vertex type must be standard layout!");
		constexpr auto stride = static_cast<GLsizei>(sizeof(Vertex));
		const auto dummy_vertex = Vertex{};
		const auto* const attribute_ptr = reinterpret_cast<const std::byte*>(std::addressof(dummy_vertex.*attribute));
		const auto* const base_ptr = reinterpret_cast<const std::byte*>(std::addressof(dummy_vertex));
		const auto* const offset = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(attribute_ptr - base_ptr)); // NOLINT(performance-no-int-to-ptr)
		glEnableVertexAttribArray(index);
		if constexpr (std::is_same_v<T, float>) {
			glVertexAttribPointer(index, 1, GL_FLOAT, GL_FALSE, stride, offset);
		} else if constexpr (std::is_same_v<T, vec2>) {
			glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, stride, offset);
		} else if constexpr (std::is_same_v<T, vec3>) {
			glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, stride, offset);
		} else if constexpr (std::is_same_v<T, vec4>) {
			glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, stride, offset);
		} else {
			throw std::invalid_argument{"Invalid vertex attribute type!"};
		}
	}

	vertex_array m_vao{};
	vertex_buffer m_vbo{};
	[[no_unique_address]] std::conditional_t<is_indexed, vertex_buffer, empty> m_ebo{}; // NOLINT(clang-diagnostic-unknown-attributes)
};

#endif
