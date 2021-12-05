#ifndef CUBEMAP_HPP
#define CUBEMAP_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "image.hpp"
#include "mesh.hpp"
#include "texture.hpp"

#include <array>        // std::array
#include <cstddef>      // std::size_t
#include <fmt/format.h> // fmt::format
#include <memory>       // std::shared_ptr
#include <stdexcept>    // std::runtime_error
#include <string_view>  // std::string_view
#include <tuple>        // std::tuple
#include <utility>      // std::move

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

class cubemap_texture final {
public:
	static constexpr auto equirectangular_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = true,
		.use_linear_filtering = true,
		.use_mip_map = false,
	};

	static constexpr auto options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.use_linear_filtering = true,
		.use_mip_map = true,
	};

	[[nodiscard]] static auto load(std::string_view filename_prefix, std::string_view extension) -> cubemap_texture {
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
		return cubemap_texture{texture::create_cubemap(internal_format,
			resolution,
			format,
			GL_UNSIGNED_BYTE,
			images[0].data(),
			images[1].data(),
			images[2].data(),
			images[3].data(),
			images[4].data(),
			images[5].data(),
			options)};
	}

	[[nodiscard]] static auto load_hdr(std::string_view filename_prefix, std::string_view extension) -> cubemap_texture {
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
		return cubemap_texture{texture::create_cubemap(
			internal_format, resolution, format, GL_FLOAT, images[0].data(), images[1].data(), images[2].data(), images[3].data(), images[4].data(), images[5].data(), options)};
	}

	cubemap_texture(texture texture) noexcept
		: m_texture(std::move(texture)) {}

	[[nodiscard]] auto get_texture() const noexcept -> const texture& {
		return m_texture;
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

	texture m_texture;
};

class environment_cubemap final {
public:
	[[nodiscard]] static auto get_default() -> const std::shared_ptr<environment_cubemap>& {
		static constexpr auto create_default = [] {
			const auto pixel = std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f};
			return cubemap_texture{texture::create_cubemap(
				GL_RGBA16F, 1, GL_RGBA, GL_FLOAT, pixel.data(), pixel.data(), pixel.data(), pixel.data(), pixel.data(), pixel.data(), cubemap_texture::options)};
		};
		static const auto env = std::make_shared<environment_cubemap>(std::make_shared<cubemap_texture>(create_default()), create_default(), create_default());
		return env;
	}

	environment_cubemap(std::shared_ptr<cubemap_texture> environment, cubemap_texture irradiance, cubemap_texture prefilter)
		: m_environment_cubemap(std::move(environment))
		, m_irradiance_cubemap(std::move(irradiance))
		, m_prefilter_cubemap(std::move(prefilter)) {}

	[[nodiscard]] auto original() const noexcept -> const std::shared_ptr<cubemap_texture>& {
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
	std::shared_ptr<cubemap_texture> m_environment_cubemap;
	cubemap_texture m_irradiance_cubemap;
	cubemap_texture m_prefilter_cubemap;
};

#endif
