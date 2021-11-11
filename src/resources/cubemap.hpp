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
#include <fmt/format.h>         // fmt::format
#include <glm/glm.hpp>          // glm::perspective, glm::radians, glm::lookAt
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <span>                 // std::span
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

class cubemap_converter final {
public:
	auto convert_equirectangular(GLint internal_format, std::size_t width, std::size_t height, GLenum format, GLenum type, const void* pixels, std::size_t cubemap_resolution,
		const texture_options& options) const -> texture {
		const auto preserver = state_preserver{};
		auto result = texture::create_cubemap(internal_format, cubemap_resolution, cubemap_resolution, format, type, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, options);

		glViewport(0, 0, static_cast<GLsizei>(cubemap_resolution), static_cast<GLsizei>(cubemap_resolution));
		glUseProgram(m_equirectangular_shader.program.get());
		glBindVertexArray(m_cubemap_mesh.get());

		const auto equirectangular_texture = texture::create_2d(internal_format, width, height, format, type, pixels, options);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, equirectangular_texture.get());

		const auto view_matrices = std::array<mat3, 6>{
			mat3{glm::lookAt(vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f})},
			mat3{glm::lookAt(vec3{0.0f, 0.0f, 0.0f}, vec3{-1.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f})},
			mat3{glm::lookAt(vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.0f, 1.0f})},
			mat3{glm::lookAt(vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f}, vec3{0.0f, 0.0f, -1.0f})},
			mat3{glm::lookAt(vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 0.0f, 1.0f}, vec3{0.0f, -1.0f, 0.0f})},
			mat3{glm::lookAt(vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 0.0f, -1.0f}, vec3{0.0f, -1.0f, 0.0f})},
		};

		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		auto target = GLenum{GL_TEXTURE_CUBE_MAP_POSITIVE_X};
		for (const auto& view_matrix : view_matrices) {
			glUniformMatrix3fv(m_equirectangular_shader.view_matrix.location(), 1, GL_FALSE, glm::value_ptr(view_matrix));
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, result.get(), 0);
			glDrawArrays(cubemap_mesh::primitive_type, 0, static_cast<GLsizei>(cubemap_mesh::vertices.size()));
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, 0, 0);
			++target;
		}
		return result;
	}

	auto reload_shaders() -> void {
		m_equirectangular_shader = equirectangular_shader{};
	}

private:
	class state_preserver final {
	public:
		[[nodiscard]] state_preserver() noexcept {
			glGetIntegerv(GL_VIEWPORT, m_viewport.data());
			glGetIntegerv(GL_ACTIVE_PROGRAM, &m_active_program);
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_framebuffer_binding);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertex_array_binding);
			glGetIntegerv(GL_ACTIVE_TEXTURE, &m_active_texture);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_texture_binding);
		}

		~state_preserver() {
			glBindTexture(GL_TEXTURE_2D, m_texture_binding);
			glActiveTexture(m_active_texture);
			glBindVertexArray(m_vertex_array_binding);
			glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_binding);
			glUseProgram(m_active_program);
			glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
		}

		state_preserver(const state_preserver&) = delete;
		state_preserver(state_preserver&&) = delete;
		auto operator=(const state_preserver&) -> state_preserver& = delete;
		auto operator=(state_preserver &&) -> state_preserver& = delete;

	private:
		std::array<GLint, 4> m_viewport{};
		GLint m_depth_func = 0;
		GLint m_active_program = 0;
		GLint m_framebuffer_binding = 0;
		GLint m_vertex_array_binding = 0;
		GLint m_active_texture = 0;
		GLint m_texture_binding = 0;
	};

	struct equirectangular_shader final {
		equirectangular_shader() {
			glUseProgram(program.get());
			const auto projection_matrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
			glUniformMatrix4fv(this->projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
			glUniform1i(equirectangular_texture.location(), 0);
		}

		shader_program program{{
			.vertex_shader_filename = "assets/shaders/equirectangular.vert",
			.fragment_shader_filename = "assets/shaders/equirectangular.frag",
		}};
		shader_uniform projection_matrix{program.get(), "projection_matrix"};
		shader_uniform view_matrix{program.get(), "view_matrix"};
		shader_uniform equirectangular_texture{program.get(), "equirectangular_texture"};
	};

	cubemap_mesh m_cubemap_mesh{};
	equirectangular_shader m_equirectangular_shader{};
};

class cubemap final {
public:
	[[nodiscard]] static auto load(std::string_view filename_prefix, std::string_view extension, const texture_options& options) -> cubemap {
		const auto images = std::array<image, 6>{
			image::load(fmt::format("{}px{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}nx{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}py{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}ny{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}pz{}", filename_prefix, extension).c_str()),
			image::load(fmt::format("{}nz{}", filename_prefix, extension).c_str()),
		};
		const auto width = images[0].width();
		const auto height = images[0].height();
		const auto channel_count = images[0].channel_count();
		check_consistency(filename_prefix, extension, std::span{images}.subspan<1>(), width, height, channel_count);
		return cubemap{texture{texture::create_cubemap(texture::internal_pixel_format_ldr(channel_count),
			width,
			height,
			texture::pixel_format(channel_count),
			GL_UNSIGNED_BYTE,
			images[0].data(),
			images[1].data(),
			images[2].data(),
			images[3].data(),
			images[4].data(),
			images[5].data(),
			options)}};
	}

	[[nodiscard]] static auto load_hdr(std::string_view filename_prefix, std::string_view extension, const texture_options& options) -> cubemap {
		const auto images = std::array<image, 6>{
			image::load_hdr(fmt::format("{}px{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}nx{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}py{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}ny{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}pz{}", filename_prefix, extension).c_str()),
			image::load_hdr(fmt::format("{}nz{}", filename_prefix, extension).c_str()),
		};
		const auto width = images[0].width();
		const auto height = images[0].height();
		const auto channel_count = images[0].channel_count();
		check_consistency(filename_prefix, extension, std::span{images}.subspan<1>(), width, height, channel_count);
		return cubemap{texture::create_cubemap(texture::internal_pixel_format_hdr(channel_count),
			width,
			height,
			texture::pixel_format(channel_count),
			GL_FLOAT,
			images[0].data(),
			images[1].data(),
			images[2].data(),
			images[3].data(),
			images[4].data(),
			images[5].data(),
			options)};
	}

	[[nodiscard]] static auto load_equirectangular(cubemap_converter& converter, const char* filename, std::size_t resolution, const texture_options& options) -> cubemap {
		const auto equirectangular_image = image::load(filename, {.flip_vertically = true});
		const auto channel_count = equirectangular_image.channel_count();
		return cubemap{converter.convert_equirectangular(texture::internal_pixel_format_ldr(channel_count),
			equirectangular_image.width(),
			equirectangular_image.height(),
			texture::pixel_format(channel_count),
			GL_UNSIGNED_BYTE,
			equirectangular_image.data(),
			resolution,
			options)};
	}

	[[nodiscard]] static auto load_equirectangular_hdr(cubemap_converter& converter, const char* filename, std::size_t resolution, const texture_options& options) -> cubemap {
		const auto equirectangular_image = image::load_hdr(filename, {.flip_vertically = true});
		const auto channel_count = equirectangular_image.channel_count();
		return cubemap{converter.convert_equirectangular(texture::internal_pixel_format_hdr(channel_count),
			equirectangular_image.width(),
			equirectangular_image.height(),
			texture::pixel_format(channel_count),
			GL_FLOAT,
			equirectangular_image.data(),
			resolution,
			options)};
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_texture.get();
	}

private:
	static auto check_consistency(
		std::string_view filename_prefix, std::string_view extension, std::span<const image, 5> images, std::size_t width, std::size_t height, std::size_t channel_count) -> void {
		for (const auto& image : images) {
			if (image.width() != width || image.height() != height) {
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

#endif
