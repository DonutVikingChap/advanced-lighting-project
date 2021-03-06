#ifndef GLYPH_HPP
#define GLYPH_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "mesh.hpp"

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <tuple>   // std::tuple

struct glyph_vertex final {
	vec2 position{};
	vec2 texture_coordinates{};
};

struct glyph_instance final {
	vec2 offset{};
	vec2 scale{};
	vec2 texture_offset{};
	vec2 texture_scale{};
	vec4 color{};
};

class glyph_mesh final {
public:
	static constexpr auto primitive_type = GLenum{GL_TRIANGLES};
	static constexpr auto vertices = std::array<glyph_vertex, 6>{
		glyph_vertex{{0.0f, 1.0f}, {0.0f, 0.0f}},
		glyph_vertex{{0.0f, 0.0f}, {0.0f, 1.0f}},
		glyph_vertex{{1.0f, 0.0f}, {1.0f, 1.0f}},
		glyph_vertex{{0.0f, 1.0f}, {0.0f, 0.0f}},
		glyph_vertex{{1.0f, 0.0f}, {1.0f, 1.0f}},
		glyph_vertex{{1.0f, 1.0f}, {1.0f, 0.0f}},
	};

	[[nodiscard]] auto get_instance_buffer() const noexcept -> GLuint {
		return m_mesh.get_instance_buffer();
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<glyph_vertex, no_index, glyph_instance> m_mesh{
		GL_STATIC_DRAW,
		GL_DYNAMIC_DRAW,
		vertices,
		{},
		std::tuple{
			&glyph_vertex::position,
			&glyph_vertex::texture_coordinates,
		},
		std::tuple{
			&glyph_instance::offset,
			&glyph_instance::scale,
			&glyph_instance::texture_offset,
			&glyph_instance::texture_scale,
			&glyph_instance::color,
		},
	};
};

#endif
