#ifndef CUBE_MAP_HPP
#define CUBE_MAP_HPP

#include "glsl.hpp"
#include "image.hpp"
#include "mesh.hpp"
#include "opengl.hpp"
#include "texture.hpp"

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <memory>  // std::shared_ptr
#include <span>    // std::span
#include <tuple>   // std::tuple

struct cube_map_vertex final {
	vec3 position{};
};

inline constexpr auto cube_map_vertex_attributes = std::tuple{
	&cube_map_vertex::position,
};

class cube_map_mesh final {
public:
	static constexpr auto primitive_type = GLenum{GL_TRIANGLES};
	static constexpr auto vertices = std::array<cube_map_vertex, 36>{
		cube_map_vertex{{-1.0f, 1.0f, -1.0f}},
		cube_map_vertex{{-1.0f, -1.0f, -1.0f}},
		cube_map_vertex{{1.0f, -1.0f, -1.0f}},
		cube_map_vertex{{1.0f, -1.0f, -1.0f}},
		cube_map_vertex{{1.0f, 1.0f, -1.0f}},
		cube_map_vertex{{-1.0f, 1.0f, -1.0f}},

		cube_map_vertex{{-1.0f, -1.0f, 1.0f}},
		cube_map_vertex{{-1.0f, -1.0f, -1.0f}},
		cube_map_vertex{{-1.0f, 1.0f, -1.0f}},
		cube_map_vertex{{-1.0f, 1.0f, -1.0f}},
		cube_map_vertex{{-1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{-1.0f, -1.0f, 1.0f}},

		cube_map_vertex{{1.0f, -1.0f, -1.0f}},
		cube_map_vertex{{1.0f, -1.0f, 1.0f}},
		cube_map_vertex{{1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{1.0f, 1.0f, -1.0f}},
		cube_map_vertex{{1.0f, -1.0f, -1.0f}},

		cube_map_vertex{{-1.0f, -1.0f, 1.0f}},
		cube_map_vertex{{-1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{1.0f, -1.0f, 1.0f}},
		cube_map_vertex{{-1.0f, -1.0f, 1.0f}},

		cube_map_vertex{{-1.0f, 1.0f, -1.0f}},
		cube_map_vertex{{1.0f, 1.0f, -1.0f}},
		cube_map_vertex{{1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{-1.0f, 1.0f, 1.0f}},
		cube_map_vertex{{-1.0f, 1.0f, -1.0f}},

		cube_map_vertex{{-1.0f, -1.0f, -1.0f}},
		cube_map_vertex{{-1.0f, -1.0f, 1.0f}},
		cube_map_vertex{{1.0f, -1.0f, -1.0f}},
		cube_map_vertex{{1.0f, -1.0f, -1.0f}},
		cube_map_vertex{{-1.0f, -1.0f, 1.0f}},
		cube_map_vertex{{1.0f, -1.0f, 1.0f}},
	};

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<cube_map_vertex> m_mesh{GL_STATIC_DRAW, vertices, cube_map_vertex_attributes};
};

class cube_map_texture final {
public:
	cube_map_texture(GLenum internal_texture_format, std::span<const image_view, 6> images, const texture_options& options)
		: m_texture(internal_texture_format, images, options) {}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_texture.get();
	}

private:
	texture m_texture;
};

#endif
