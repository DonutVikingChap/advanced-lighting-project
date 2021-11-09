#ifndef CUBEMAP_HPP
#define CUBEMAP_HPP

#include "../core/opengl.hpp"
#include "../resources/image.hpp"
#include "../resources/texture.hpp"

#include <algorithm>    // std::min
#include <array>        // std::array
#include <fmt/format.h> // fmt::format
#include <string_view>  // std::string_view

class cubemap final {
public:
	cubemap(std::string_view filename_prefix, std::string_view extension, const texture_options& options)
		: m_texture([&] {
			const auto images = std::array<image, 6>{
				image{fmt::format("{}px{}", filename_prefix, extension).c_str()},
				image{fmt::format("{}nx{}", filename_prefix, extension).c_str()},
				image{fmt::format("{}py{}", filename_prefix, extension).c_str()},
				image{fmt::format("{}ny{}", filename_prefix, extension).c_str()},
				image{fmt::format("{}pz{}", filename_prefix, extension).c_str()},
				image{fmt::format("{}nz{}", filename_prefix, extension).c_str()},
			};
			const auto image_views = std::array<image_view, 6>{
				image_view{images[0]},
				image_view{images[1]},
				image_view{images[2]},
				image_view{images[3]},
				image_view{images[4]},
				image_view{images[5]},
			};
			const auto internal_format = texture::internal_pixel_format(std::min({
				images[0].pixel_size(),
				images[1].pixel_size(),
				images[2].pixel_size(),
				images[3].pixel_size(),
				images[4].pixel_size(),
				images[5].pixel_size(),
			}));
			return texture{internal_format, image_views, options};
		}()) {}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_texture.get();
	}

private:
	texture m_texture;
};

#endif
