#ifndef CUBEMAP_GENERATOR_HPP
#define CUBEMAP_GENERATOR_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/cubemap.hpp"
#include "../resources/framebuffer.hpp"
#include "../resources/shader.hpp"
#include "../resources/texture.hpp"

#include <array>                        // std::array
#include <cmath>                        // std::pow
#include <cstddef>                      // std::size_t
#include <glm/gtc/matrix_transform.hpp> // glm::perspective, glm::lookAt
#include <glm/gtc/type_ptr.hpp>         // glm::value_ptr
#include <utility>                      // std::move

class cubemap_generator final {
public:
	static constexpr auto irradiance_sample_delta_angle = 0.025f;
	static constexpr auto prefilter_sample_count = 1024u;

	auto reload_shaders() -> void {
		m_equirectangular_shader = equirectangular_shader{};
		m_irradiance_shader = irradiance_shader{};
		m_prefilter_shader = prefilter_shader{};
	}

	[[nodiscard]] auto generate_cubemap_from_equirectangular_2d(GLint internal_format, const texture& equirectangular_texture, std::size_t resolution) const -> cubemap_texture {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glUseProgram(m_equirectangular_shader.program.get());
		glBindVertexArray(m_cubemap_mesh.get());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, equirectangular_texture.get());
		auto result = texture::create_cubemap_uninitialized(internal_format, resolution, cubemap_texture::options);
		m_equirectangular_shader.generate(result, 0, resolution);
		if constexpr (cubemap_texture::options.use_mip_map) {
			glBindTexture(GL_TEXTURE_CUBE_MAP, result.get());
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
		return cubemap_texture{std::move(result)};
	}

	[[nodiscard]] auto generate_irradiance_map(GLint internal_format, const cubemap_texture& cubemap, std::size_t resolution) const -> cubemap_texture {
		const auto preserver = state_preserver{GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glUseProgram(m_irradiance_shader.program.get());
		glBindVertexArray(m_cubemap_mesh.get());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.get());
		auto result = texture::create_cubemap_uninitialized(
			internal_format, resolution, {.max_anisotropy = 1.0f, .repeat = false, .use_linear_filtering = true, .use_mip_map = false});
		m_irradiance_shader.generate(result, 0, resolution);
		return cubemap_texture{std::move(result)};
	}

	[[nodiscard]] auto generate_prefilter_map(GLint internal_format, const cubemap_texture& cubemap, std::size_t resolution, std::size_t mip_level_count) -> cubemap_texture {
		const auto preserver = state_preserver{GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glUseProgram(m_prefilter_shader.program.get());
		glBindVertexArray(m_cubemap_mesh.get());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.get());
		glUniform1f(m_prefilter_shader.cubemap_resolution.location(), static_cast<float>(cubemap.get_texture().width()));
		auto result = texture::create_cubemap_uninitialized(
			internal_format, resolution, {.max_anisotropy = 1.0f, .repeat = false, .use_linear_filtering = true, .use_mip_map = true});
		for (auto mip = std::size_t{0}; mip < mip_level_count; ++mip) {
			const auto mip_resolution = static_cast<std::size_t>(static_cast<float>(resolution) * std::pow(0.5f, mip));
			const auto roughness = static_cast<float>(mip) / static_cast<float>(mip_level_count - 1);
			glUniform1f(m_prefilter_shader.roughness.location(), roughness);
			m_prefilter_shader.generate(result, mip, mip_resolution);
		}
		return cubemap_texture{std::move(result)};
	}

private:
	class state_preserver final {
	public:
		[[nodiscard]] state_preserver(GLenum texture_target, GLenum texture_target_binding) noexcept
			: m_texture_target(texture_target) {
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_framebuffer_binding);
			glGetIntegerv(GL_VIEWPORT, m_viewport.data());
			glGetIntegerv(GL_CURRENT_PROGRAM, &m_current_program);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertex_array_binding);
			glGetIntegerv(GL_ACTIVE_TEXTURE, &m_active_texture);
			glGetIntegerv(texture_target_binding, &m_texture_binding);
		}

		~state_preserver() {
			glBindTexture(m_texture_target, m_texture_binding);
			glActiveTexture(m_active_texture);
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
		GLenum m_texture_target;
		GLint m_framebuffer_binding = 0;
		std::array<GLint, 4> m_viewport{};
		GLint m_current_program = 0;
		GLint m_vertex_array_binding = 0;
		GLint m_active_texture = 0;
		GLint m_texture_binding = 0;
	};

	struct cubemap_shader {
		cubemap_shader(const char* fragment_shader_filename, const char* texture_uniform_name, shader_definition_list definitions)
			: program({
				  .vertex_shader_filename = "assets/shaders/cubemap.vert",
				  .fragment_shader_filename = fragment_shader_filename,
				  .definitions = definitions,
			  })
			, texture_uniform(program.get(), texture_uniform_name) {
			const auto projection_matrix = glm::perspective(radians(90.0f), 1.0f, 0.1f, 10.0f);
			glUseProgram(program.get());
			glUniformMatrix4fv(this->projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
			glUniform1i(texture_uniform.location(), 0);
		}

		auto generate(texture& result, std::size_t level, std::size_t resolution) const -> void {
			glViewport(0, 0, static_cast<GLsizei>(resolution), static_cast<GLsizei>(resolution));
			auto target = GLenum{GL_TEXTURE_CUBE_MAP_POSITIVE_X};
			const auto view_matrices = std::array<mat3, 6>{
				mat3{glm::lookAt(vec3{}, vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f})},
				mat3{glm::lookAt(vec3{}, vec3{-1.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f})},
				mat3{glm::lookAt(vec3{}, vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.0f, 1.0f})},
				mat3{glm::lookAt(vec3{}, vec3{0.0f, -1.0f, 0.0f}, vec3{0.0f, 0.0f, -1.0f})},
				mat3{glm::lookAt(vec3{}, vec3{0.0f, 0.0f, 1.0f}, vec3{0.0f, -1.0f, 0.0f})},
				mat3{glm::lookAt(vec3{}, vec3{0.0f, 0.0f, -1.0f}, vec3{0.0f, -1.0f, 0.0f})},
			};
			for (const auto& view_matrix : view_matrices) {
				glUniformMatrix3fv(this->view_matrix.location(), 1, GL_FALSE, glm::value_ptr(view_matrix));
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, result.get(), static_cast<GLint>(level));
				opengl_context::check_framebuffer_status();
				glDrawArrays(cubemap_mesh::primitive_type, 0, static_cast<GLsizei>(cubemap_mesh::vertices.size()));
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, 0, static_cast<GLint>(level));
				++target;
			}
		}

		shader_program program;
		shader_uniform projection_matrix{program.get(), "projection_matrix"};
		shader_uniform view_matrix{program.get(), "view_matrix"};
		shader_uniform texture_uniform;
	};

	struct equirectangular_shader final : cubemap_shader {
		equirectangular_shader()
			: cubemap_shader("assets/shaders/equirectangular.frag", "equirectangular_texture", {}) {}
	};

	struct irradiance_shader final : cubemap_shader {
		irradiance_shader()
			: cubemap_shader("assets/shaders/irradiance.frag", "cubemap_texture", {{"SAMPLE_DELTA_ANGLE", irradiance_sample_delta_angle}}) {}
	};

	struct prefilter_shader final : cubemap_shader {
		prefilter_shader()
			: cubemap_shader("assets/shaders/prefilter.frag", "cubemap_texture", {{"SAMPLE_COUNT", prefilter_sample_count}}) {}

		shader_uniform cubemap_resolution{program.get(), "cubemap_resolution"};
		shader_uniform roughness{program.get(), "roughness"};
	};

	cubemap_mesh m_cubemap_mesh{};
	equirectangular_shader m_equirectangular_shader{};
	irradiance_shader m_irradiance_shader{};
	prefilter_shader m_prefilter_shader{};
};

#endif
