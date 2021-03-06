#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "../core/handle.hpp"
#include "../core/opengl.hpp"

#include <array>        // std::array
#include <cstddef>      // std::byte, std::size_t
#include <fmt/format.h> // fmt::format
#include <stdexcept>    // std::invalid_argument
#include <vector>       // std::vector

struct texture_options final {
	float max_anisotropy = 1.0f;
	bool repeat = true;
	bool black_border = false;
	bool use_linear_filtering = true;
	bool use_mip_map = false;
	bool use_compare_mode = false;
};

class texture final {
public:
	[[nodiscard]] static auto is_depth_internal_format(GLint format) noexcept -> bool {
		return format == GL_DEPTH_COMPONENT || format == GL_DEPTH_COMPONENT16 || format == GL_DEPTH_COMPONENT24 || format == GL_DEPTH_COMPONENT32F;
	}

	[[nodiscard]] static auto channel_count(GLenum format) -> std::size_t {
		switch (format) {
			case GL_DEPTH_COMPONENT: [[fallthrough]];
			case GL_RED: return 1;
			case GL_RG: return 2;
			case GL_RGB: return 3;
			case GL_RGBA: return 4;
			default: break;
		}
		throw std::invalid_argument{fmt::format("Invalid texture format \"{}\"!", format)};
	}

	[[nodiscard]] static auto internal_channel_count(GLint internal_format) -> std::size_t {
		switch (internal_format) {
			case GL_R8: [[fallthrough]];
			case GL_R16F: [[fallthrough]];
			case GL_R32F: return 1;
			case GL_RG8: [[fallthrough]];
			case GL_RG16F: [[fallthrough]];
			case GL_RG32F: return 2;
			case GL_RGB8: [[fallthrough]];
			case GL_RGB16F: [[fallthrough]];
			case GL_RGB32F: return 3;
			case GL_RGBA8: [[fallthrough]];
			case GL_RGBA16F: [[fallthrough]];
			case GL_RGBA32F: return 4;
			default: break;
		}
		throw std::invalid_argument{fmt::format("Invalid internal texture format \"{}\"!", internal_format)};
	}

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

	[[nodiscard]] static constexpr auto null() noexcept {
		return texture{};
	}

	[[nodiscard]] static auto create_2d(
		GLint internal_format, std::size_t width, std::size_t height, GLenum format, GLenum type, const void* pixels, const texture_options& options) -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};
		auto result = texture{internal_format, width, height};
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, result.get());
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, pixels);
		set_options(GL_TEXTURE_2D, options);
		return result;
	}

	[[nodiscard]] static auto create_2d_uninitialized(GLint internal_format, std::size_t width, std::size_t height, const texture_options& options) -> texture {
		const auto is_depth = is_depth_internal_format(internal_format);
		const auto format = (is_depth) ? GLenum{GL_DEPTH_COMPONENT} : GLenum{GL_RED};
		const auto type = (is_depth) ? GLenum{GL_FLOAT} : GLenum{GL_UNSIGNED_BYTE};
		return create_2d(internal_format, width, height, format, type, nullptr, options);
	}

	[[nodiscard]] static auto create_2d_array(GLint internal_format, std::size_t width, std::size_t height, std::size_t depth, GLenum format, GLenum type, const void* pixels,
		const texture_options& options) -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY};
		auto result = texture{internal_format, width, height};
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, result.get());
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, format, type, pixels);
		set_options(GL_TEXTURE_2D_ARRAY, options);
		return result;
	}

	[[nodiscard]] static auto create_2d_array_uninitialized(GLint internal_format, std::size_t width, std::size_t height, std::size_t depth, const texture_options& options)
		-> texture {
		const auto is_depth = is_depth_internal_format(internal_format);
		const auto format = (is_depth) ? GLenum{GL_DEPTH_COMPONENT} : GLenum{GL_RED};
		const auto type = (is_depth) ? GLenum{GL_FLOAT} : GLenum{GL_UNSIGNED_BYTE};
		return create_2d_array(internal_format, width, height, depth, format, type, nullptr, options);
	}

	[[nodiscard]] static auto create_cubemap(GLint internal_format, std::size_t resolution, GLenum format, GLenum type, const void* pixels_px, const void* pixels_nx,
		const void* pixels_py, const void* pixels_ny, const void* pixels_pz, const void* pixels_nz, const texture_options& options) -> texture {
		const auto preserver = state_preserver{GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP};
		auto result = texture{internal_format, resolution, resolution};
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, result.get());
		auto target = GLenum{GL_TEXTURE_CUBE_MAP_POSITIVE_X};
		for (const auto* const pixels : {pixels_px, pixels_nx, pixels_py, pixels_ny, pixels_pz, pixels_nz}) {
			glTexImage2D(target, 0, internal_format, static_cast<GLint>(resolution), static_cast<GLint>(resolution), 0, format, type, pixels);
			++target;
		}
		set_options(GL_TEXTURE_CUBE_MAP, options);
		return result;
	}

	[[nodiscard]] static auto create_cubemap_uninitialized(GLint internal_format, std::size_t resolution, const texture_options& options) -> texture {
		const auto is_depth = is_depth_internal_format(internal_format);
		const auto format = (is_depth) ? GLenum{GL_DEPTH_COMPONENT} : GLenum{GL_RED};
		const auto type = (is_depth) ? GLenum{GL_FLOAT} : GLenum{GL_UNSIGNED_BYTE};
		return create_cubemap(internal_format, resolution, format, type, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, options);
	}

	constexpr explicit operator bool() const noexcept {
		return static_cast<bool>(m_texture);
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

	[[nodiscard]] auto read_pixels_2d(GLenum format) const -> std::vector<std::byte> {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, m_texture.get());
		auto result = std::vector<std::byte>(m_width * m_height * channel_count(format));
		glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, result.data());
		return result;
	}

	[[nodiscard]] auto read_pixels_2d_hdr(GLenum format) const -> std::vector<float> {
		const auto preserver = state_preserver{GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D};
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, m_texture.get());
		auto result = std::vector<float>(m_width * m_height * channel_count(format));
		glGetTexImage(GL_TEXTURE_2D, 0, format, GL_FLOAT, result.data());
		return result;
	}

	[[nodiscard]] auto internal_format() const noexcept -> GLint {
		return m_internal_format;
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
	constexpr texture() noexcept = default;

	texture(GLint internal_format, std::size_t width, std::size_t height)
		: m_texture([] {
			auto p = GLuint{};
			glGenTextures(1, &p);
			if (p == 0) {
				throw opengl_error{"Failed to create texture!"};
			}
			return p;
		}())
		, m_internal_format(internal_format)
		, m_width(width)
		, m_height(height) {}

	class state_preserver final {
	public:
		[[nodiscard]] state_preserver(GLenum texture_target, GLenum texture_target_binding) noexcept
			: m_texture_target(texture_target) {
			glGetIntegerv(GL_PACK_ALIGNMENT, &m_pack_alignment);
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &m_unpack_alignment);
			glGetIntegerv(texture_target_binding, &m_texture);
		}

		~state_preserver() {
			glBindTexture(m_texture_target, m_texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, m_unpack_alignment);
			glPixelStorei(GL_PACK_ALIGNMENT, m_pack_alignment);
		}

		state_preserver(const state_preserver&) = delete;
		state_preserver(state_preserver&&) = delete;
		auto operator=(const state_preserver&) -> state_preserver& = delete;
		auto operator=(state_preserver &&) -> state_preserver& = delete;

	private:
		GLenum m_texture_target;
		GLint m_pack_alignment = 0;
		GLint m_unpack_alignment = 0;
		GLint m_texture = 0;
	};

	static auto set_options(GLenum target, const texture_options& options) noexcept -> void {
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY, options.max_anisotropy);
		if (options.repeat) {
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
			if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) {
				glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);
			}
		} else if (options.black_border) {
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) {
				glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
			}
			constexpr auto border_color = std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f};
			glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, border_color.data());
		} else {
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) {
				glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			}
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
		if (options.use_compare_mode) {
			glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		}
	}

	struct texture_deleter final {
		auto operator()(GLuint p) const noexcept -> void {
			glDeleteTextures(1, &p);
		}
	};
	using texture_ptr = unique_handle<texture_deleter>;

	texture_ptr m_texture{};
	GLint m_internal_format = 0;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
};

struct sampler_options final {
	bool repeat = true;
	bool black_border = false;
	bool use_linear_filtering = true;
};

class sampler final {
public:
	[[nodiscard]] static constexpr auto null() noexcept {
		return sampler{};
	}

	[[nodiscard]] static auto create(const sampler_options& options) -> sampler {
		return sampler{options};
	}

	constexpr explicit operator bool() const noexcept {
		return static_cast<bool>(m_sampler);
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_sampler.get();
	}

private:
	constexpr sampler() noexcept = default;

	explicit sampler(const sampler_options& options)
		: m_sampler([] {
			auto p = GLuint{};
			glGenSamplers(1, &p);
			if (p == 0) {
				throw opengl_error{"Failed to create sampler!"};
			}
			return p;
		}()) {
		if (options.repeat) {
			glSamplerParameteri(m_sampler.get(), GL_TEXTURE_WRAP_S, GL_REPEAT);
			glSamplerParameteri(m_sampler.get(), GL_TEXTURE_WRAP_T, GL_REPEAT);
		} else if (options.black_border) {
			glSamplerParameteri(m_sampler.get(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glSamplerParameteri(m_sampler.get(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			constexpr auto border_color = std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f};
			glSamplerParameterfv(m_sampler.get(), GL_TEXTURE_BORDER_COLOR, border_color.data());
		} else {
			glSamplerParameteri(m_sampler.get(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glSamplerParameteri(m_sampler.get(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glSamplerParameteri(m_sampler.get(), GL_TEXTURE_MIN_FILTER, (options.use_linear_filtering) ? GL_LINEAR : GL_NEAREST);
		glSamplerParameteri(m_sampler.get(), GL_TEXTURE_MAG_FILTER, (options.use_linear_filtering) ? GL_LINEAR : GL_NEAREST);
	}

	struct sampler_deleter final {
		auto operator()(GLuint p) const noexcept -> void {
			glDeleteSamplers(1, &p);
		}
	};
	using sampler_ptr = unique_handle<sampler_deleter>;

	sampler_ptr m_sampler{};
};

#endif
