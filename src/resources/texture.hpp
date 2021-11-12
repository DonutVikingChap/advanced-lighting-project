#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "../core/handle.hpp"
#include "../core/opengl.hpp"

#include <cstddef>      // std::byte, std::size_t
#include <fmt/format.h> // fmt::format
#include <stdexcept>    // std::invalid_argument

struct texture_options final {
	float max_anisotropy = 1.0f;
	bool repeat = true;
	bool use_linear_filtering = true;
	bool use_mip_map = false;
};

class texture final {
public:
	[[nodiscard]] static auto pixel_format(std::size_t channel_count) -> GLenum {
		switch (channel_count) {
			case 1: return GL_RED;
			case 2: return GL_RG;
			case 3: return GL_RGB;
			case 4: return GL_RGBA;
			default: break;
		}
		throw std::invalid_argument{fmt::format("Invalid texture channel count \"{}\"!", channel_count)};
	}

	[[nodiscard]] static auto internal_pixel_format_ldr(std::size_t channel_count) -> GLint {
		switch (channel_count) {
			case 1: return GL_R8;
			case 2: return GL_RG8;
			case 3: return GL_RGB8;
			case 4: return GL_RGBA8;
			default: break;
		}
		throw std::invalid_argument{fmt::format("Invalid texture channel count \"{}\"!", channel_count)};
	}

	[[nodiscard]] static auto internal_pixel_format_hdr(std::size_t channel_count) -> GLint {
		switch (channel_count) {
			case 1: return GL_R16F;
			case 2: return GL_RG16F;
			case 3: return GL_RGB16F;
			case 4: return GL_RGBA16F;
			default: break;
		}
		throw std::invalid_argument{fmt::format("Invalid texture channel count \"{}\"!", channel_count)};
	}

	[[nodiscard]] static auto create_2d(
		GLint internal_format, std::size_t width, std::size_t height, GLenum format, GLenum type, const void* pixels, const texture_options& options) -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};
		auto result = texture{width, height};
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, result.get());
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, pixels);
		set_options(GL_TEXTURE_2D, options);
		if (options.use_mip_map && pixels) {
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		return result;
	}

	[[nodiscard]] static auto create_2d_uninitialized(GLint internal_format, std::size_t width, std::size_t height, const texture_options& options) -> texture {
		return create_2d(internal_format, width, height, GL_RED, GL_UNSIGNED_BYTE, nullptr, options);
	}

	[[nodiscard]] static auto create_2d_array(GLint internal_format, std::size_t width, std::size_t height, std::size_t depth, GLenum format, GLenum type, const void* pixels,
		const texture_options& options) -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY};
		auto result = texture{width, height};
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, result.get());
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, format, type, pixels);
		set_options(GL_TEXTURE_2D_ARRAY, options);
		if (options.use_mip_map && pixels) {
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		}
		return result;
	}

	[[nodiscard]] static auto create_2d_array_uninitialized(GLint internal_format, std::size_t width, std::size_t height, std::size_t depth, const texture_options& options)
		-> texture {
		return create_2d_array(internal_format, width, height, depth, GL_RED, GL_UNSIGNED_BYTE, nullptr, options);
	}

	[[nodiscard]] static auto create_cubemap(GLint internal_format, std::size_t resolution, GLenum format, GLenum type, const void* pixels_px, const void* pixels_nx,
		const void* pixels_py, const void* pixels_ny, const void* pixels_pz, const void* pixels_nz, const texture_options& options) -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP};
		auto result = texture{resolution, resolution};
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, result.get());
		auto target = GLenum{GL_TEXTURE_CUBE_MAP_POSITIVE_X};
		for (const auto* const pixels : {pixels_px, pixels_nx, pixels_py, pixels_ny, pixels_pz, pixels_nz}) {
			glTexImage2D(target, 0, internal_format, resolution, resolution, 0, format, type, pixels);
			++target;
		}
		set_options(GL_TEXTURE_CUBE_MAP, options);
		if (options.use_mip_map && pixels_px && pixels_nx && pixels_py && pixels_ny && pixels_pz && pixels_nz) {
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
		return result;
	}

	[[nodiscard]] static auto create_cubemap_uninitialized(GLint internal_format, std::size_t resolution, const texture_options& options) -> texture {
		return create_cubemap(internal_format, resolution, GL_RED, GL_UNSIGNED_BYTE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, options);
	}

	auto paste_2d(std::size_t width, std::size_t height, GLenum format, GLenum type, const void* pixels, std::size_t x, std::size_t y) -> void {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, m_texture.get());
		glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLsizei>(width), static_cast<GLsizei>(height), format, type, pixels);
	}

	auto paste_3d(std::size_t width, std::size_t height, std::size_t depth, GLenum format, GLenum type, const void* pixels, std::size_t x, std::size_t y, std::size_t z) -> void {
		const auto preserver = state_preserver{GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY};
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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
			type,
			pixels);
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
	texture(std::size_t width, std::size_t height)
		: m_width(width)
		, m_height(height) {}

	class state_preserver final {
	public:
		[[nodiscard]] state_preserver(GLenum texture_target, GLenum texture_target_binding) noexcept
			: m_texture_target(texture_target) {
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &m_unpack_alignment);
			glGetIntegerv(texture_target_binding, &m_texture);
		}

		~state_preserver() {
			glBindTexture(m_texture_target, m_texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, m_unpack_alignment);
		}

		state_preserver(const state_preserver&) = delete;
		state_preserver(state_preserver&&) = delete;
		auto operator=(const state_preserver&) -> state_preserver& = delete;
		auto operator=(state_preserver&&) -> state_preserver& = delete;

	private:
		GLenum m_texture_target;
		GLint m_unpack_alignment = 0;
		GLint m_texture = 0;
	};

	static auto set_options(GLenum target, const texture_options& options) noexcept -> void {
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY, options.max_anisotropy);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, (options.repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, (options.repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) {
			glTexParameteri(target, GL_TEXTURE_WRAP_R, (options.repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		}
		if (options.use_mip_map) {
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
	std::size_t m_width;
	std::size_t m_height;
};

#endif
