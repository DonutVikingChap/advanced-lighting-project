#ifndef CUBEMAP_HPP
#define CUBEMAP_HPP

#include "../core/opengl.hpp"
#include "../resources/image.hpp"
#include "../resources/texture.hpp"

#include <array>        // std::array
#include <fmt/format.h> // fmt::format
#include <span>         // std::span
#include <stdexcept>    // std::runtime_error
#include <string_view>  // std::string_view

struct cubemap_error : std::runtime_error {
	explicit cubemap_error(const auto& message)
		: std::runtime_error(message) {}
};

class cubemap final {
public:
	[[nodiscard]] static auto load(std::string_view filename_prefix, std::string_view extension, const texture_options& options) -> cubemap {
		return load_images<image>(filename_prefix, extension, options);
	}

	[[nodiscard]] static auto load_hdr(std::string_view filename_prefix, std::string_view extension, const texture_options& options) -> cubemap {
		return load_images<image_hdr>(filename_prefix, extension, options);
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_texture.get();
	}

private:
	template <typename Image>
	[[nodiscard]] static auto load_images(std::string_view filename_prefix, std::string_view extension, const texture_options& options) -> cubemap {
		const auto images = std::array<Image, 6>{
			Image::load(fmt::format("{}px{}", filename_prefix, extension).c_str()),
			Image::load(fmt::format("{}nx{}", filename_prefix, extension).c_str()),
			Image::load(fmt::format("{}py{}", filename_prefix, extension).c_str()),
			Image::load(fmt::format("{}ny{}", filename_prefix, extension).c_str()),
			Image::load(fmt::format("{}pz{}", filename_prefix, extension).c_str()),
			Image::load(fmt::format("{}nz{}", filename_prefix, extension).c_str()),
		};
		const auto width = images[0].width();
		const auto height = images[0].height();
		const auto channel_count = images[0].channel_count();
		for (const auto& image : std::span{images}.subspan(1)) {
			if (image.width() != width || image.height() != height) {
				throw cubemap_error{fmt::format("Cubemap images {}...{} have inconsistent dimensions!", filename_prefix, extension)};
			}
			if (image.channel_count() != channel_count) {
				throw cubemap_error{fmt::format("Cubemap images {}...{} have inconsistent pixel formats!", filename_prefix, extension)};
			}
		}
		return cubemap{width, height, channel_count, images, options};
	}

	cubemap(std::size_t width, std::size_t height, std::size_t channel_count, const std::array<image, 6>& images, const texture_options& options)
		: m_texture(texture::create_cubemap(texture::internal_pixel_format_ldr(channel_count), width, height, texture::pixel_format(channel_count), GL_UNSIGNED_BYTE,
			  images[0].data(), images[1].data(), images[2].data(), images[3].data(), images[4].data(), images[5].data(), options)) {}

	cubemap(std::size_t width, std::size_t height, std::size_t channel_count, const std::array<image_hdr, 6>& images, const texture_options& options)
		: m_texture(texture::create_cubemap(texture::internal_pixel_format_hdr(channel_count), width, height, texture::pixel_format(channel_count), GL_FLOAT, images[0].data(),
			  images[1].data(), images[2].data(), images[3].data(), images[4].data(), images[5].data(), options)) {}

	texture m_texture;
};

#endif
