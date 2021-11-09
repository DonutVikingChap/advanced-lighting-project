#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "mesh.hpp"

#include <array> // std::array
#include <tuple> // std::tuple

struct skybox_vertex final {
	vec3 position{};
};

inline constexpr auto skybox_vertex_attributes = std::tuple{
	&skybox_vertex::position,
};

class skybox_mesh final {
public:
	static constexpr auto primitive_type = GLenum{GL_TRIANGLES};
	static constexpr auto vertices = std::array<skybox_vertex, 36>{
		skybox_vertex{{-1.0f, 1.0f, -1.0f}},
		skybox_vertex{{-1.0f, -1.0f, -1.0f}},
		skybox_vertex{{1.0f, -1.0f, -1.0f}},
		skybox_vertex{{1.0f, -1.0f, -1.0f}},
		skybox_vertex{{1.0f, 1.0f, -1.0f}},
		skybox_vertex{{-1.0f, 1.0f, -1.0f}},

		skybox_vertex{{-1.0f, -1.0f, 1.0f}},
		skybox_vertex{{-1.0f, -1.0f, -1.0f}},
		skybox_vertex{{-1.0f, 1.0f, -1.0f}},
		skybox_vertex{{-1.0f, 1.0f, -1.0f}},
		skybox_vertex{{-1.0f, 1.0f, 1.0f}},
		skybox_vertex{{-1.0f, -1.0f, 1.0f}},

		skybox_vertex{{1.0f, -1.0f, -1.0f}},
		skybox_vertex{{1.0f, -1.0f, 1.0f}},
		skybox_vertex{{1.0f, 1.0f, 1.0f}},
		skybox_vertex{{1.0f, 1.0f, 1.0f}},
		skybox_vertex{{1.0f, 1.0f, -1.0f}},
		skybox_vertex{{1.0f, -1.0f, -1.0f}},

		skybox_vertex{{-1.0f, -1.0f, 1.0f}},
		skybox_vertex{{-1.0f, 1.0f, 1.0f}},
		skybox_vertex{{1.0f, 1.0f, 1.0f}},
		skybox_vertex{{1.0f, 1.0f, 1.0f}},
		skybox_vertex{{1.0f, -1.0f, 1.0f}},
		skybox_vertex{{-1.0f, -1.0f, 1.0f}},

		skybox_vertex{{-1.0f, 1.0f, -1.0f}},
		skybox_vertex{{1.0f, 1.0f, -1.0f}},
		skybox_vertex{{1.0f, 1.0f, 1.0f}},
		skybox_vertex{{1.0f, 1.0f, 1.0f}},
		skybox_vertex{{-1.0f, 1.0f, 1.0f}},
		skybox_vertex{{-1.0f, 1.0f, -1.0f}},

		skybox_vertex{{-1.0f, -1.0f, -1.0f}},
		skybox_vertex{{-1.0f, -1.0f, 1.0f}},
		skybox_vertex{{1.0f, -1.0f, -1.0f}},
		skybox_vertex{{1.0f, -1.0f, -1.0f}},
		skybox_vertex{{-1.0f, -1.0f, 1.0f}},
		skybox_vertex{{1.0f, -1.0f, 1.0f}},
	};

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<skybox_vertex> m_mesh{GL_STATIC_DRAW, vertices, skybox_vertex_attributes};
};

#endif
