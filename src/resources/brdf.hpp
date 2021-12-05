#ifndef BRDF_HPP
#define BRDF_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "mesh.hpp"

#include <array> // std::array
#include <tuple> // std::tuple

struct brdf_lookup_table_vertex final {
	vec2 position{};
	vec2 texture_coordinates{};
};

class brdf_lookup_table_mesh final {
public:
	static constexpr auto primitive_type = GLenum{GL_TRIANGLE_STRIP};
	static constexpr auto vertices = std::array<brdf_lookup_table_vertex, 4>{
		brdf_lookup_table_vertex{{-1.0f, 1.0f}, {0.0f, 1.0f}},
		brdf_lookup_table_vertex{{-1.0f, -1.0f}, {0.0f, 0.0f}},
		brdf_lookup_table_vertex{{1.0f, 1.0f}, {1.0f, 1.0f}},
		brdf_lookup_table_vertex{{1.0f, -1.0f}, {1.0f, 0.0f}},
	};

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<brdf_lookup_table_vertex> m_mesh{GL_STATIC_DRAW, vertices, std::tuple{&brdf_lookup_table_vertex::position, &brdf_lookup_table_vertex::texture_coordinates}};
};

#endif
