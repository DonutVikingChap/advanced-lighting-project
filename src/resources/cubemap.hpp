#ifndef CUBEMAP_HPP
#define CUBEMAP_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "framebuffer.hpp"
#include "image.hpp"
#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"

#include <array>                // std::array
#include <cmath>                // std::pow
#include <fmt/format.h>         // fmt::format
#include <glm/glm.hpp>          // glm::perspective, glm::radians, glm::lookAt
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <memory>               // std::shared_ptr
#include <stdexcept>            // std::runtime_error
#include <string_view>          // std::string_view
#include <tuple>                // std::tuple
#include <utility>              // std::move

struct cubemap_error : std::runtime_error {
	explicit cubemap_error(const auto& message)
		: std::runtime_error(message) {}
};

struct cubemap_vertex final {
	vec3 position{};
};

class cubemap_mesh final {
public:
	static constexpr auto primitive_type = GLenum{GL_TRIANGLES};
	static constexpr auto vertices = std::array<cubemap_vertex, 36>{
		cubemap_vertex{{-1.0f, 1.0f, -1.0f}},
		cubemap_vertex{{-1.0f, -1.0f, -1.0f}},
		cubemap_vertex{{1.0f, -1.0f, -1.0f}},
		cubemap_vertex{{1.0f, -1.0f, -1.0f}},
		cubemap_vertex{{1.0f, 1.0f, -1.0f}},
		cubemap_vertex{{-1.0f, 1.0f, -1.0f}},

		cubemap_vertex{{-1.0f, -1.0f, 1.0f}},
		cubemap_vertex{{-1.0f, -1.0f, -1.0f}},
		cubemap_vertex{{-1.0f, 1.0f, -1.0f}},
		cubemap_vertex{{-1.0f, 1.0f, -1.0f}},
		cubemap_vertex{{-1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{-1.0f, -1.0f, 1.0f}},

		cubemap_vertex{{1.0f, -1.0f, -1.0f}},
		cubemap_vertex{{1.0f, -1.0f, 1.0f}},
		cubemap_vertex{{1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{1.0f, 1.0f, -1.0f}},
		cubemap_vertex{{1.0f, -1.0f, -1.0f}},

		cubemap_vertex{{-1.0f, -1.0f, 1.0f}},
		cubemap_vertex{{-1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{1.0f, -1.0f, 1.0f}},
		cubemap_vertex{{-1.0f, -1.0f, 1.0f}},

		cubemap_vertex{{-1.0f, 1.0f, -1.0f}},
		cubemap_vertex{{1.0f, 1.0f, -1.0f}},
		cubemap_vertex{{1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{-1.0f, 1.0f, 1.0f}},
		cubemap_vertex{{-1.0f, 1.0f, -1.0f}},

		cubemap_vertex{{-1.0f, -1.0f, -1.0f}},
		cubemap_vertex{{-1.0f, -1.0f, 1.0f}},
		cubemap_vertex{{1.0f, -1.0f, -1.0f}},
		cubemap_vertex{{1.0f, -1.0f, -1.0f}},
		cubemap_vertex{{-1.0f, -1.0f, 1.0f}},
		cubemap_vertex{{1.0f, -1.0f, 1.0f}},
	};

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<cubemap_vertex> m_mesh{GL_STATIC_DRAW, vertices, std::tuple{&cubemap_vertex::position}};
};

class cubemap_generator final {
public:
	static constexpr auto irradiance_sample_delta_angle = 0.025f;
	static constexpr auto prefilter_sample_count = 1024u;

	static constexpr auto equirectangular_texture_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = true,
		.use_linear_filtering = true,
		.use_mip_map = false,
	};

	static constexpr auto cubemap_texture_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.use_linear_filtering = true,
		.use_mip_map = true,
	};

	auto generate_cubemap_from_equirectangular_2d(GLint internal_format, const texture& equirectangular_texture, std::size_t resolution) const -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glUseProgram(m_equirectangular_shader.program.get());
		glBindVertexArray(m_cubemap_mesh.get());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, equirectangular_texture.get());
		auto result = texture::create_cubemap_uninitialized(internal_format, resolution, cubemap_texture_options);
		m_equirectangular_shader.generate(result, 0, resolution);
		if constexpr (cubemap_texture_options.use_mip_map) {
			glBindTexture(GL_TEXTURE_CUBE_MAP, result.get());
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
		return result;
	}

	auto generate_irradiance_map(GLint internal_format, const texture& cubemap_texture, std::size_t resolution) const -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glUseProgram(m_irradiance_shader.program.get());
		glBindVertexArray(m_cubemap_mesh.get());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture.get());
		auto result = texture::create_cubemap_uninitialized(
			internal_format, resolution, {.max_anisotropy = 1.0f, .repeat = false, .use_linear_filtering = true, .use_mip_map = false});
		m_irradiance_shader.generate(result, 0, resolution);
		return result;
	}

	auto generate_prefilter_map(GLint internal_format, const texture& cubemap_texture, std::size_t resolution, std::size_t mip_level_count) -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glUseProgram(m_prefilter_shader.program.get());
		glBindVertexArray(m_cubemap_mesh.get());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture.get());
		glUniform1f(m_prefilter_shader.cubemap_resolution.location(), static_cast<float>(cubemap_texture.width()));
		auto result = texture::create_cubemap_uninitialized(
			internal_format, resolution, {.max_anisotropy = 1.0f, .repeat = false, .use_linear_filtering = true, .use_mip_map = true});
		for (auto mip = std::size_t{0}; mip < mip_level_count; ++mip) {
			const auto mip_resolution = static_cast<std::size_t>(static_cast<float>(resolution) * std::pow(0.5f, mip));
			const auto roughness = static_cast<float>(mip) / static_cast<float>(mip_level_count - 1);
			glUniform1f(m_prefilter_shader.roughness.location(), roughness);
			m_prefilter_shader.generate(result, mip, mip_resolution);
		}
		return result;
	}

	auto reload_shaders() -> void {
		m_equirectangular_shader = equirectangular_shader{};
		m_irradiance_shader = irradiance_shader{};
		m_prefilter_shader = prefilter_shader{};
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
		auto operator=(state_preserver&&) -> state_preserver& = delete;

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
			const auto projection_matrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
			glUseProgram(program.get());
			glUniformMatrix4fv(this->projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
			glUniform1i(texture_uniform.location(), 0);
		}

		auto generate(texture& result, std::size_t level, std::size_t resolution) const -> void {
			glViewport(0, 0, static_cast<GLsizei>(resolution), static_cast<GLsizei>(resolution));
			auto target = GLenum{GL_TEXTURE_CUBE_MAP_POSITIVE_X};
			const auto view_matrices = std::array<mat3, 6>{
				look_at({1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
				look_at({-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
				look_at({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
				look_at({0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}),
				look_at({0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}),
				look_at({0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}),
			};
			for (const auto& view_matrix : view_matrices) {
				glUniformMatrix3fv(this->view_matrix.location(), 1, GL_FALSE, glm::value_ptr(view_matrix));
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, result.get(), static_cast<GLint>(level));
				opengl_context::check_status();
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

	[[nodiscard]] static auto look_at(vec3 direction, vec3 up) -> mat3 {
		return mat3{glm::lookAt(vec3{}, direction, up)};
	}

	cubemap_mesh m_cubemap_mesh{};
	equirectangular_shader m_equirectangular_shader{};
	irradiance_shader m_irradiance_shader{};
	prefilter_shader m_prefilter_shader{};
};

class cubemap final {
public:
	[[nodiscard]] static auto load(std::string_view filename_prefix, std::string_view extension) -> cubemap {
		const auto images = std::array<image, 6>{
			image::load(fmt::format("{}px{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}nx{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}py{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}ny{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}pz{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}nz{}", filename_prefix, extension).c_str()),
		};
		const auto resolution = images[0].width();
		const auto channel_count = images[0].channel_count();
		check_consistency(filename_prefix, extension, images, resolution, channel_count);
		const auto internal_format = texture::internal_pixel_format_ldr(channel_count);
		const auto format = texture::pixel_format(channel_count);
		return cubemap{texture::create_cubemap(internal_format,
			resolution,
			format,
			GL_UNSIGNED_BYTE,
			images[0].data(),
			images[1].data(),
			images[2].data(),
			images[3].data(),
			images[4].data(),
			images[5].data(),
			cubemap_generator::cubemap_texture_options)};
	}

	[[nodiscard]] static auto load_hdr(std::string_view filename_prefix, std::string_view extension) -> cubemap {
		const auto images = std::array<image, 6>{
			image::load_hdr(fmt::format("{}px{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}nx{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}py{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}ny{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}pz{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}nz{}", filename_prefix, extension).c_str()),
		};
		const auto resolution = images[0].width();
		const auto channel_count = images[0].channel_count();
		check_consistency(filename_prefix, extension, images, resolution, channel_count);
		const auto internal_format = texture::internal_pixel_format_hdr(channel_count);
		const auto format = texture::pixel_format(channel_count);
		return cubemap{texture::create_cubemap(internal_format,
			resolution,
			format,
			GL_FLOAT,
			images[0].data(),
			images[1].data(),
			images[2].data(),
			images[3].data(),
			images[4].data(),
			images[5].data(),
			cubemap_generator::cubemap_texture_options)};
	}

	[[nodiscard]] static auto load_equirectangular(cubemap_generator& generator, const char* filename, std::size_t resolution) -> cubemap {
		const auto img = image::load(filename, {.flip_vertically = true});
		const auto internal_format = texture::internal_pixel_format_ldr(img.channel_count());
		const auto format = texture::pixel_format(img.channel_count());
		const auto equirectangular_texture = texture::create_2d(
			internal_format, img.width(), img.height(), format, GL_UNSIGNED_BYTE, img.data(), cubemap_generator::equirectangular_texture_options);
		return cubemap{generator.generate_cubemap_from_equirectangular_2d(internal_format, equirectangular_texture, resolution)};
	}

	[[nodiscard]] static auto load_equirectangular_hdr(cubemap_generator& generator, const char* filename, std::size_t resolution) -> cubemap {
		const auto img = image::load_hdr(filename, {.flip_vertically = true});
		const auto internal_format = texture::internal_pixel_format_hdr(img.channel_count());
		const auto format = texture::pixel_format(img.channel_count());
		const auto equirectangular_texture = texture::create_2d(
			internal_format, img.width(), img.height(), format, GL_FLOAT, img.data(), cubemap_generator::equirectangular_texture_options);
		return cubemap{generator.generate_cubemap_from_equirectangular_2d(internal_format, equirectangular_texture, resolution)};
	}

	[[nodiscard]] static auto generate_irradiance_map(cubemap_generator& generator, GLint internal_format, const cubemap& original_cubemap, std::size_t resolution) -> cubemap {
		return cubemap{generator.generate_irradiance_map(internal_format, original_cubemap.m_texture, resolution)};
	}

	[[nodiscard]] static auto generate_prefilter_map(
		cubemap_generator& generator, GLint internal_format, const cubemap& original_cubemap, std::size_t resolution, std::size_t mip_level_count) -> cubemap {
		return cubemap{generator.generate_prefilter_map(internal_format, original_cubemap.m_texture, resolution, mip_level_count)};
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_texture.get();
	}

private:
	static auto check_consistency(
		std::string_view filename_prefix, std::string_view extension, const std::array<image, 6>& images, std::size_t resolution, std::size_t channel_count) -> void {
		for (const auto& image : images) {
			if (image.width() != resolution || image.height() != resolution) {
				throw cubemap_error{fmt::format("Cubemap images {}...{} have inconsistent dimensions!", filename_prefix, extension)};
			}
			if (image.channel_count() != channel_count) {
				throw cubemap_error{fmt::format("Cubemap images {}...{} have inconsistent pixel formats!", filename_prefix, extension)};
			}
		}
	}

	cubemap(texture&& texture) noexcept
		: m_texture(std::move(texture)) {}

	texture m_texture;
};

struct environment_cubemap_options final {
	std::size_t irradiance_map_resolution = 32;
	std::size_t prefilter_map_resolution = 128;
	std::size_t prefilter_map_mip_level_count = 5;
};

class environment_cubemap final {
public:
	environment_cubemap(cubemap_generator& generator, std::shared_ptr<cubemap> environment, const environment_cubemap_options& options)
		: m_environment_cubemap(std::move(environment))
		, m_irradiance_cubemap(cubemap::generate_irradiance_map(generator, GL_RGB16F, *m_environment_cubemap, options.irradiance_map_resolution))
		, m_prefilter_cubemap(
			  cubemap::generate_prefilter_map(generator, GL_RGB16F, *m_environment_cubemap, options.prefilter_map_resolution, options.prefilter_map_mip_level_count)) {}

	[[nodiscard]] auto original() const noexcept -> const std::shared_ptr<cubemap>& {
		return m_environment_cubemap;
	}

	[[nodiscard]] auto environment_map() const noexcept -> GLuint {
		return m_environment_cubemap->get();
	}

	[[nodiscard]] auto irradiance_map() const noexcept -> GLuint {
		return m_irradiance_cubemap.get();
	}

	[[nodiscard]] auto prefilter_map() const noexcept -> GLuint {
		return m_prefilter_cubemap.get();
	}

private:
	std::shared_ptr<cubemap> m_environment_cubemap;
	cubemap m_irradiance_cubemap;
	cubemap m_prefilter_cubemap;
};

#endif
