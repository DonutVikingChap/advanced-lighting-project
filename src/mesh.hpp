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
#include <tuple>       // std::tuple, std::apply
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
struct no_instance final {};

template <typename Vertex, typename Index = no_index, typename Instance = no_instance>
class mesh final {
public:
	static constexpr auto is_indexed = !std::is_same_v<Index, no_index>;
	static constexpr auto is_instanced = !std::is_same_v<Instance, no_instance>;

	template <typename... Ts>
	mesh(GLenum vertices_usage, std::span<const Vertex> vertices, std::tuple<Ts Vertex::*...> vertex_attributes) requires(!is_indexed && !is_instanced) {
		const auto preserver = state_preserver{};
		glBindVertexArray(m_vao.get());
		std::apply([&](auto... attributes) { buffer_vertex_data(vertices_usage, vertices, 0, attributes...); }, vertex_attributes);
	}

	template <typename... Ts>
	mesh(GLenum vertices_usage, GLenum indices_usage, std::span<const Vertex> vertices, std::span<const Index> indices, std::tuple<Ts Vertex::*...> vertex_attributes) requires(
		is_indexed && !is_instanced) {
		const auto preserver = state_preserver{};
		glBindVertexArray(m_vao.get());
		std::apply([&](auto... attributes) { buffer_vertex_data(vertices_usage, vertices, 0, attributes...); }, vertex_attributes);
		buffer_index_data(indices_usage, indices);
	}

	template <typename... Ts, typename... Us>
	mesh(GLenum vertices_usage, GLenum instances_usage, std::span<const Vertex> vertices, std::span<const Instance> instances, std::tuple<Ts Vertex::*...> vertex_attributes,
		std::tuple<Us Instance::*...> instance_attributes) requires(!is_indexed && is_instanced) {
		const auto preserver = state_preserver{};
		glBindVertexArray(m_vao.get());
		std::apply([&](auto... attributes) { buffer_vertex_data(vertices_usage, vertices, 0, attributes...); }, vertex_attributes);
		std::apply([&](auto... attributes) { buffer_instance_data(instances_usage, instances, sizeof...(Ts), attributes...); }, instance_attributes);
	}

	template <typename... Ts, typename... Us>
	mesh(GLenum vertices_usage, GLenum indices_usage, GLenum instances_usage, std::span<const Vertex> vertices, std::span<const Index> indices, std::span<const Instance> instances,
		std::tuple<Ts Vertex::*...> vertex_attributes, std::tuple<Us Instance::*...> instance_attributes) requires(is_indexed&& is_instanced) {
		const auto preserver = state_preserver{};
		glBindVertexArray(m_vao.get());
		std::apply([&](auto... attributes) { buffer_vertex_data(vertices_usage, vertices, 0, attributes...); }, vertex_attributes);
		buffer_index_data(indices_usage, indices);
		std::apply([&](auto... attributes) { buffer_instance_data(instances_usage, instances, sizeof...(Ts), attributes...); }, instance_attributes);
	}

	[[nodiscard]] auto instance_buffer() const noexcept -> GLuint requires(is_instanced) {
		return m_ibo.get();
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_vao.get();
	}

private:
	class state_preserver final {
	public:
		[[nodiscard]] state_preserver() noexcept {
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertex_array_binding);
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_array_buffer_binding);
		}

		~state_preserver() {
			glBindBuffer(GL_ARRAY_BUFFER, m_array_buffer_binding);
			glBindVertexArray(m_vertex_array_binding);
		}

		state_preserver(const state_preserver&) = delete;
		state_preserver(state_preserver&&) = delete;
		auto operator=(const state_preserver&) -> state_preserver& = delete;
		auto operator=(state_preserver &&) -> state_preserver& = delete;

	private:
		GLint m_vertex_array_binding = 0;
		GLint m_array_buffer_binding = 0;
	};

	struct empty {};

	template <typename... Ts>
	auto buffer_vertex_data(GLenum usage, std::span<const Vertex> vertices, GLuint attribute_offset, Ts(Vertex::*... vertex_attributes)) -> void {
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo.get());
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), usage);
		(setup_attribute<Vertex>(attribute_offset, vertex_attributes), ...);
	}

	auto buffer_index_data(GLenum usage, std::span<const Index> indices) -> void requires(is_indexed) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo.get());
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(Index)), indices.data(), usage);
	}

	template <typename... Ts>
	auto buffer_instance_data(GLenum usage, std::span<const Instance> instances, GLuint attribute_offset, Ts(Instance::*... instance_attributes)) -> void requires(is_instanced) {
		glBindBuffer(GL_ARRAY_BUFFER, m_ibo.get());
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances.size() * sizeof(Instance)), instances.data(), usage);
		(setup_attribute<Instance>(attribute_offset, instance_attributes), ...);
	}

	template <typename Structure>
	static auto enable_attribute(GLuint index) -> void {
		glEnableVertexAttribArray(index);
		if constexpr (std::is_same_v<Structure, Instance>) {
			glVertexAttribDivisor(index, 1);
		}
	}

	template <typename Structure, typename T>
	static auto setup_attribute(GLuint& index, T(Structure::*attribute)) -> void {
		static_assert(std::is_standard_layout_v<Structure>, "Structure type must have standard layout!");
		constexpr auto stride = static_cast<GLsizei>(sizeof(Structure));
		const auto dummy_struct = Structure{};
		const auto* const attribute_ptr = reinterpret_cast<const std::byte*>(std::addressof(dummy_struct.*attribute));
		const auto* const base_ptr = reinterpret_cast<const std::byte*>(std::addressof(dummy_struct));
		const auto offset = static_cast<std::uintptr_t>(attribute_ptr - base_ptr);
		if constexpr (std::is_same_v<T, float>) {
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset)); // NOLINT(performance-no-int-to-ptr)
		} else if constexpr (std::is_same_v<T, vec2>) {
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset)); // NOLINT(performance-no-int-to-ptr)
		} else if constexpr (std::is_same_v<T, vec3>) {
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset)); // NOLINT(performance-no-int-to-ptr)
		} else if constexpr (std::is_same_v<T, vec4>) {
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset)); // NOLINT(performance-no-int-to-ptr)
		} else if constexpr (std::is_same_v<T, mat2>) {
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset)); // NOLINT(performance-no-int-to-ptr)
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset + sizeof(float) * 2)); // NOLINT(performance-no-int-to-ptr)
		} else if constexpr (std::is_same_v<T, mat3>) {
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset)); // NOLINT(performance-no-int-to-ptr)
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset + sizeof(float) * 3)); // NOLINT(performance-no-int-to-ptr)
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset + sizeof(float) * 6)); // NOLINT(performance-no-int-to-ptr)
		} else if constexpr (std::is_same_v<T, mat4>) {
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset)); // NOLINT(performance-no-int-to-ptr)
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset + sizeof(float) * 4)); // NOLINT(performance-no-int-to-ptr)
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset + sizeof(float) * 8)); // NOLINT(performance-no-int-to-ptr)
			enable_attribute<Structure>(index);
			glVertexAttribPointer(index++, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset + sizeof(float) * 12)); // NOLINT(performance-no-int-to-ptr)
		} else {
			throw std::invalid_argument{"Invalid attribute type!"};
		}
	}

	vertex_array m_vao{};
	vertex_buffer m_vbo{};
	[[no_unique_address]] std::conditional_t<is_indexed, vertex_buffer, empty> m_ebo{};   // NOLINT(clang-diagnostic-unknown-attributes)
	[[no_unique_address]] std::conditional_t<is_instanced, vertex_buffer, empty> m_ibo{}; // NOLINT(clang-diagnostic-unknown-attributes)
};

#endif
