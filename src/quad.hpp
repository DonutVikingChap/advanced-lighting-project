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
	static constexpr auto vertex_count = std::size_t{6};

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<quad_vertex> m_mesh{GL_STATIC_DRAW,
		std::array<quad_vertex, vertex_count>{
			quad_vertex{vec2{0.0f, 1.0f}, vec2{0.0f, 0.0f}},
			quad_vertex{vec2{0.0f, 0.0f}, vec2{0.0f, 1.0f}},
			quad_vertex{vec2{1.0f, 0.0f}, vec2{1.0f, 1.0f}},
			quad_vertex{vec2{0.0f, 1.0f}, vec2{0.0f, 0.0f}},
			quad_vertex{vec2{1.0f, 0.0f}, vec2{1.0f, 1.0f}},
			quad_vertex{vec2{1.0f, 1.0f}, vec2{1.0f, 0.0f}},
		},
		&quad_vertex::position,
		&quad_vertex::texture_coordinates};
};

#endif
