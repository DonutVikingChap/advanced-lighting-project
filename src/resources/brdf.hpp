#ifndef BRDF_HPP
#define BRDF_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "framebuffer.hpp"
#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"

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

class brdf_generator final {
public:
	static constexpr auto lookup_table_sample_count = 1024u;

	static constexpr auto lookup_table_texture_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.use_linear_filtering = true,
		.use_mip_map = false,
	};

	auto reload_shaders() -> void {
		m_lookup_table_shader = lookup_table_shader{};
	}

	[[nodiscard]] auto generate_lookup_table(GLint internal_format, std::size_t resolution) const -> texture {
		const auto preserver = state_preserver{};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glUseProgram(m_lookup_table_shader.program.get());
		glBindVertexArray(m_lookup_table_mesh.get());
		auto result = texture::create_2d_uninitialized(internal_format, resolution, resolution, lookup_table_texture_options);
		glViewport(0, 0, static_cast<GLsizei>(resolution), static_cast<GLsizei>(resolution));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.get(), 0);
		glDrawArrays(brdf_lookup_table_mesh::primitive_type, 0, static_cast<GLsizei>(brdf_lookup_table_mesh::vertices.size()));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		return result;
	}

private:
	class state_preserver final {
	public:
		[[nodiscard]] state_preserver() noexcept {
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_framebuffer_binding);
			glGetIntegerv(GL_VIEWPORT, m_viewport.data());
			glGetIntegerv(GL_CURRENT_PROGRAM, &m_current_program);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertex_array_binding);
		}

		~state_preserver() {
			glBindVertexArray(m_vertex_array_binding);
			glUseProgram(m_current_program);
			glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
			glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_binding);
		}

		state_preserver(const state_preserver&) = delete;
		state_preserver(state_preserver&&) = delete;
		auto operator=(const state_preserver&) -> state_preserver& = delete;
		auto operator=(state_preserver &&) -> state_preserver& = delete;

	private:
		GLint m_framebuffer_binding = 0;
		std::array<GLint, 4> m_viewport{};
		GLint m_current_program = 0;
		GLint m_vertex_array_binding = 0;
	};

	struct lookup_table_shader final {
		shader_program program{{
			.vertex_shader_filename = "assets/shaders/plain.vert",
			.fragment_shader_filename = "assets/shaders/brdf.frag",
			.definitions =
				{
					{"SAMPLE_COUNT", lookup_table_sample_count},
				},
		}};
	};

	brdf_lookup_table_mesh m_lookup_table_mesh{};
	lookup_table_shader m_lookup_table_shader{};
};

#endif
