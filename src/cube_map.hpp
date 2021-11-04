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

struct cube_map_vertex final {
	vec3 position{};
};

class cube_map_mesh final {
public:
	static constexpr auto vertex_count = std::size_t{36};

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<cube_map_vertex> m_mesh{GL_STATIC_DRAW,
		std::array<cube_map_vertex, vertex_count>{
			cube_map_vertex{vec3{-1.0f, 1.0f, -1.0f}},
			cube_map_vertex{vec3{-1.0f, -1.0f, -1.0f}},
			cube_map_vertex{vec3{1.0f, -1.0f, -1.0f}},
			cube_map_vertex{vec3{1.0f, -1.0f, -1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, -1.0f}},
			cube_map_vertex{vec3{-1.0f, 1.0f, -1.0f}},

			cube_map_vertex{vec3{-1.0f, -1.0f, 1.0f}},
			cube_map_vertex{vec3{-1.0f, -1.0f, -1.0f}},
			cube_map_vertex{vec3{-1.0f, 1.0f, -1.0f}},
			cube_map_vertex{vec3{-1.0f, 1.0f, -1.0f}},
			cube_map_vertex{vec3{-1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{-1.0f, -1.0f, 1.0f}},

			cube_map_vertex{vec3{1.0f, -1.0f, -1.0f}},
			cube_map_vertex{vec3{1.0f, -1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, -1.0f}},
			cube_map_vertex{vec3{1.0f, -1.0f, -1.0f}},

			cube_map_vertex{vec3{-1.0f, -1.0f, 1.0f}},
			cube_map_vertex{vec3{-1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, -1.0f, 1.0f}},
			cube_map_vertex{vec3{-1.0f, -1.0f, 1.0f}},

			cube_map_vertex{vec3{-1.0f, 1.0f, -1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, -1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{-1.0f, 1.0f, 1.0f}},
			cube_map_vertex{vec3{-1.0f, 1.0f, -1.0f}},

			cube_map_vertex{vec3{-1.0f, -1.0f, -1.0f}},
			cube_map_vertex{vec3{-1.0f, -1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, -1.0f, -1.0f}},
			cube_map_vertex{vec3{1.0f, -1.0f, -1.0f}},
			cube_map_vertex{vec3{-1.0f, -1.0f, 1.0f}},
			cube_map_vertex{vec3{1.0f, -1.0f, 1.0f}},
		},
		&cube_map_vertex::position};
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
