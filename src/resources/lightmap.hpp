#ifndef LIGHTMAP_HPP
#define LIGHTMAP_HPP

#include "../core/opengl.hpp"
#include "texture.hpp"

#include <array>     // std::array
#include <cstddef>   // std::size_t
#include <memory>    // std::shared_ptr, std::make_shared
#include <stdexcept> // std::runtime_error
#include <utility>   // std::move

struct lightmap_error : std::runtime_error {
	explicit lightmap_error(const auto& message)
		: std::runtime_error(message) {}
};

class lightmap_texture final {
public:
	static constexpr auto channel_count = std::size_t{4};
	static constexpr auto padding = std::size_t{4};
	static constexpr auto internal_format = GLint{GL_RGBA16F};
	static constexpr auto format = GLenum{GL_RGBA};
	static constexpr auto type = GLenum{GL_FLOAT};
	static constexpr auto options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.use_linear_filtering = true,
		.use_mip_map = true,
	};

	[[nodiscard]] static auto get_default() -> const std::shared_ptr<lightmap_texture>& {
		static const auto map = [] {
			const auto pixel = std::array<float, 4>{1.0f, 1.0f, 1.0f, 0.0f};
			return std::make_shared<lightmap_texture>(lightmap_texture::create(1, pixel.data()));
		}();
		return map;
	}

	[[nodiscard]] static auto create(std::size_t resolution, const void* pixels) -> lightmap_texture {
		return lightmap_texture{texture::create_2d(internal_format, resolution, resolution, format, type, pixels, options)};
	}

	explicit lightmap_texture(texture texture)
		: m_texture(std::move(texture)) {}

	[[nodiscard]] auto get_texture() const noexcept -> const texture& {
		return m_texture;
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_texture.get();
	}

private:
	texture m_texture;
};

#endif
