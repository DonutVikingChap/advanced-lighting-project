#ifndef BRDF_GENERATOR_HPP
#define BRDF_GENERATOR_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/brdf.hpp"
#include "../resources/framebuffer.hpp"
#include "../resources/shader.hpp"
#include "../resources/texture.hpp"

#include <array>   // std::array
#include <cstddef> // std::size_t

class brdf_generator final {
public:
	static constexpr auto lookup_table_internal_format = GLint{GL_R16F};
	static constexpr auto lookup_table_resolution = std::size_t{512};
	static constexpr auto lookup_table_sample_count = 1024u;

	static constexpr auto lookup_table_texture_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.use_linear_filtering = true,
		.use_mip_map = false,
	};

	[[nodiscard]] static auto get_lookup_table() -> const texture& {
		static const auto lookup_table = brdf_generator{}.generate_lookup_table();
		return lookup_table;
	}

	auto reload_shaders() -> void {
		m_lookup_table_shader = lookup_table_shader{};
	}

	[[nodiscard]] auto generate_lookup_table() const -> texture {
		const auto preserver = state_preserver{};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glUseProgram(m_lookup_table_shader.program.get());
		glBindVertexArray(m_lookup_table_mesh.get());
		auto result = texture::create_2d_uninitialized(lookup_table_internal_format, lookup_table_resolution, lookup_table_resolution, lookup_table_texture_options);
		glViewport(0, 0, static_cast<GLsizei>(lookup_table_resolution), static_cast<GLsizei>(lookup_table_resolution));
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
