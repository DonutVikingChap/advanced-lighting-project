#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "handle.hpp"
#include "image.hpp"
#include "opengl.hpp"

#include <cstddef>      // std::byte, std::size_t
#include <fmt/format.h> // fmt::format
#include <span>         // std::span
#include <stdexcept>    // std::invalid_argument

struct texture_options final {
	float max_anisotropy = 1.0f;
	bool repeat = true;
	bool use_linear_filtering = true;
	bool use_mip_map = false;
};

class texture final {
public:
	[[nodiscard]] static auto pixel_format(std::size_t pixel_size) -> GLenum {
		switch (pixel_size) {
			case 1: return GL_RED;
			case 2: return GL_RG;
			case 3: return GL_RGB;
			case 4: return GL_RGBA;
			default: break;
		}
		throw std::invalid_argument{fmt::format("Invalid texture pixel size \"{}\"!", pixel_size)};
	}

	texture(GLint internal_format, const std::byte* pixels, std::size_t width, std::size_t height, GLenum format, const texture_options& options = {})
		: m_width(width)
		, m_height(height) {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};

		set_unpack_alignment(format);

		glBindTexture(GL_TEXTURE_2D, m_texture.get());
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, GL_UNSIGNED_BYTE, pixels);

		set_options(GL_TEXTURE_2D, options);
	}

	texture(GLint internal_format, const std::byte* pixels, std::size_t width, std::size_t height, std::size_t depth, GLenum format, const texture_options& options = {})
		: m_width(width)
		, m_height(height) {
		const auto preserver = state_preserver{GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY};

		set_unpack_alignment(format);

		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture.get());
		glTexImage3D(
			GL_TEXTURE_2D_ARRAY, 0, internal_format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, format, GL_UNSIGNED_BYTE, pixels);

		set_options(GL_TEXTURE_2D_ARRAY, options);
	}

	texture(GLint internal_format, const image_view& image, const texture_options& options = {})
		: texture(internal_format, image.data(), image.width(), image.height(), pixel_format(image.pixel_size()), options) {}

	texture(GLint internal_format, std::span<const image_view, 6> cube_map_images, const texture_options& options = {.repeat = false}) {
		const auto preserver = state_preserver{GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP};

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture.get());
		for (auto i = std::size_t{0}; i < cube_map_images.size(); ++i) {
			const auto& image = cube_map_images[i];
			const auto format = pixel_format(image.pixel_size());
			set_unpack_alignment(format);
			glTexImage2D(static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), 0, internal_format, image.width(), image.height(), 0, format, GL_UNSIGNED_BYTE, image.data());
		}

		set_options(GL_TEXTURE_CUBE_MAP, options);
	}

	auto paste(const std::byte* pixels, std::size_t width, std::size_t height, GLenum format, std::size_t x, std::size_t y) -> void {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};

		set_unpack_alignment(format);

		glBindTexture(GL_TEXTURE_2D, m_texture.get());
		glTexSubImage2D(
			GL_TEXTURE_2D, 0, static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLsizei>(width), static_cast<GLsizei>(height), format, GL_UNSIGNED_BYTE, pixels);
	}

	auto paste(const std::byte* pixels, std::size_t width, std::size_t height, std::size_t depth, GLenum format, std::size_t x, std::size_t y, std::size_t z) -> void {
		const auto preserver = state_preserver{GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY};

		set_unpack_alignment(format);

		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture.get());
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
			0,
			static_cast<GLint>(x),
			static_cast<GLint>(y),
			static_cast<GLint>(z),
			static_cast<GLsizei>(width),
			static_cast<GLsizei>(height),
			static_cast<GLsizei>(depth),
			format,
			GL_UNSIGNED_BYTE,
			pixels);
	}

	auto paste(const image_view& image, std::size_t x, std::size_t y) -> void {
		paste(image.data(), image.width(), image.height(), pixel_format(image.pixel_size()), x, y);
	}

	auto paste(const image_view& image, std::size_t x, std::size_t y, std::size_t z) -> void {
		paste(image.data(), image.width(), image.height(), std::size_t{1}, pixel_format(image.pixel_size()), x, y, z);
	}

	[[nodiscard]] auto width() const noexcept -> std::size_t {
		return m_width;
	}

	[[nodiscard]] auto height() const noexcept -> std::size_t {
		return m_height;
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_texture.get();
	}

private:
	class [[nodiscard]] state_preserver final {
	public:
		state_preserver(GLenum texture_target, GLenum texture_target_binding) noexcept
			: m_texture_target(texture_target) {
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &m_unpack_alignment);
			glGetIntegerv(texture_target_binding, &m_texture);
		}

		~state_preserver() {
			glBindTexture(m_texture_target, m_texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, m_unpack_alignment);
		}

		state_preserver(const state_preserver&) = delete;
		state_preserver(state_preserver &&) = delete;
		auto operator=(const state_preserver&)->state_preserver& = delete;
		auto operator=(state_preserver &&)->state_preserver& = delete;

	private:
		GLenum m_texture_target;
		GLint m_unpack_alignment = 0;
		GLint m_texture = 0;
	};

	static auto set_unpack_alignment(GLenum format) noexcept -> void {
		glPixelStorei(GL_UNPACK_ALIGNMENT, (format == GL_RED) ? 1 : 4);
	}

	static auto set_options(GLenum target, const texture_options& options) noexcept -> void {
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY, options.max_anisotropy);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, (options.repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, (options.repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) {
			glTexParameteri(target, GL_TEXTURE_WRAP_R, (options.repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		}
		if (options.use_mip_map) {
			glGenerateMipmap(target);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (options.use_linear_filtering) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, (options.use_linear_filtering) ? GL_LINEAR : GL_NEAREST);
		} else {
			glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (options.use_linear_filtering) ? GL_LINEAR : GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, (options.use_linear_filtering) ? GL_LINEAR : GL_NEAREST);
		}
	}

	struct texture_deleter final {
		auto operator()(GLuint p) const noexcept -> void {
			glDeleteTextures(1, &p);
		}
	};
	using texture_ptr = unique_handle<texture_deleter>;

	texture_ptr m_texture{[] {
		auto tex = GLuint{};
		glGenTextures(1, &tex);
		if (tex == 0) {
			throw opengl_error{"Failed to create texture!"};
		}
		return tex;
	}()};
	std::size_t m_width = 0;
	std::size_t m_height = 0;
};

#endif
