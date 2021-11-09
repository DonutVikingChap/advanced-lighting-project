#ifndef QUAD_HPP
#define QUAD_HPP

#include "glsl.hpp"
#include "mesh.hpp"
#include "opengl.hpp"

#include <array>   // std::array
#include <cstddef> // std::size_t

struct quad_vertex final {
	vec2 position{};
	vec2 texture_coordinates{};
};

class quad_mesh final {
public:
	static constexpr auto vertices = std::array<quad_vertex, 6>{
		quad_vertex{{0.0f, 1.0f}, {0.0f, 0.0f}},
		quad_vertex{{0.0f, 0.0f}, {0.0f, 1.0f}},
		quad_vertex{{1.0f, 0.0f}, {1.0f, 1.0f}},
		quad_vertex{{0.0f, 1.0f}, {0.0f, 0.0f}},
		quad_vertex{{1.0f, 0.0f}, {1.0f, 1.0f}},
		quad_vertex{{1.0f, 1.0f}, {1.0f, 0.0f}},
	};

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<quad_vertex> m_mesh{GL_STATIC_DRAW, vertices, &quad_vertex::position, &quad_vertex::texture_coordinates};
};

#endif
