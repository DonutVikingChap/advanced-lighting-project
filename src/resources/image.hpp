#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <cstddef>           // std::byte, std::size_t
#include <fmt/format.h>      // fmt::format
#include <stb_image.h>       // stbi_...
#include <stb_image_write.h> // stbi_write_...
#include <stdexcept>         // std::runtime_error
#include <utility>           // std::move, std::exchange

struct image_error : std::runtime_error {
	explicit image_error(const auto& message)
		: std::runtime_error(message) {}
};

class image_view final {
public:
	constexpr image_view() noexcept = default;

	constexpr image_view(const void* pixels, std::size_t width, std::size_t height, std::size_t channel_count) noexcept
		: m_pixels(pixels)
		, m_width(width)
		, m_height(height)
		, m_channel_count(channel_count) {}

	[[nodiscard]] auto data() const noexcept -> const void* {
		return m_pixels;
	}

	[[nodiscard]] auto width() const noexcept -> std::size_t {
		return m_width;
	}

	[[nodiscard]] auto height() const noexcept -> std::size_t {
		return m_height;
	}

	[[nodiscard]] auto channel_count() const noexcept -> std::size_t {
		return m_channel_count;
	}

private:
	const void* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_channel_count = 0;
};

struct image_options final {
	int desired_channel_count = 0;
	bool flip_vertically = false;
};

class image final {
public:
	[[nodiscard]] static auto load(const char* filename, const image_options& options = {}) -> image {
		stbi_set_flip_vertically_on_load_thread(options.flip_vertically ? 1 : 0);
		int width = 0;
		int height = 0;
		int channel_count = 0;
		auto* const pixels = stbi_load(filename, &width, &height, &channel_count, options.desired_channel_count);
		if (!pixels) {
			throw image_error{fmt::format("Failed to load image \"{}\"!", filename)};
		}
		return image{pixels, static_cast<std::size_t>(width), static_cast<std::size_t>(height), static_cast<std::size_t>(channel_count)};
	}

	[[nodiscard]] static auto load_hdr(const char* filename, const image_options& options = {}) -> image {
		stbi_set_flip_vertically_on_load_thread(options.flip_vertically ? 1 : 0);
		int width = 0;
		int height = 0;
		int channel_count = 0;
		auto* const pixels = stbi_loadf(filename, &width, &height, &channel_count, options.desired_channel_count);
		if (!pixels) {
			throw image_error{fmt::format("Failed to load HDR image \"{}\"!", filename)};
		}
		return image{pixels, static_cast<std::size_t>(width), static_cast<std::size_t>(height), static_cast<std::size_t>(channel_count)};
	}

	~image() {
		stbi_image_free(m_pixels);
	}

	image(const image&) = delete;

	image(image&& other) noexcept {
		*this = std::move(other);
	}

	auto operator=(const image&) -> image& = delete;

	auto operator=(image&& other) noexcept -> image& {
		stbi_image_free(m_pixels);
		m_pixels = std::exchange(other.m_pixels, nullptr);
		m_width = std::exchange(other.m_width, 0);
		m_height = std::exchange(other.m_height, 0);
		m_channel_count = std::exchange(other.m_channel_count, 0);
		return *this;
	}

	operator image_view() const noexcept {
		return image_view{m_pixels, m_width, m_height, m_channel_count};
	}

	[[nodiscard]] auto data() noexcept -> void* {
		return m_pixels;
	}

	[[nodiscard]] auto data() const noexcept -> const void* {
		return m_pixels;
	}

	[[nodiscard]] auto width() const noexcept -> std::size_t {
		return m_width;
	}

	[[nodiscard]] auto height() const noexcept -> std::size_t {
		return m_height;
	}

	[[nodiscard]] auto channel_count() const noexcept -> std::size_t {
		return m_channel_count;
	}

private:
	image(void* pixels, std::size_t width, std::size_t height, std::size_t channel_count) noexcept
		: m_pixels(pixels)
		, m_width(width)
		, m_height(height)
		, m_channel_count(channel_count) {}

	void* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_channel_count = 0;
};

struct image_png_options final {
	int compression_level = 8;
	bool flip_vertically = false;
};

inline auto save_png(image_view image, const char* filename, const image_png_options& options = {}) -> void {
	stbi_flip_vertically_on_write(options.flip_vertically ? 1 : 0);
	stbi_write_png_compression_level = options.compression_level;
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = static_cast<const stbi_uc*>(image.data());
	if (stbi_write_png(filename, width, height, channel_count, pixels, 0) == 0) {
		throw image_error{fmt::format("Failed to save PNG image \"{}\"!", filename)};
	}
}

struct image_bmp_options final {
	bool flip_vertically = false;
};

inline auto save_bmp(image_view image, const char* filename, const image_bmp_options& options = {}) -> void {
	stbi_flip_vertically_on_write(options.flip_vertically ? 1 : 0);
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = static_cast<const stbi_uc*>(image.data());
	if (stbi_write_bmp(filename, width, height, channel_count, pixels) == 0) {
		throw image_error{fmt::format("Failed to save BMP image \"{}\"!", filename)};
	}
}

struct image_tga_options final {
	bool use_rle_compression = true;
	bool flip_vertically = false;
};

inline auto save_tga(image_view image, const char* filename, const image_tga_options& options = {}) -> void {
	stbi_flip_vertically_on_write(options.flip_vertically ? 1 : 0);
	stbi_write_tga_with_rle = (options.use_rle_compression) ? 1 : 0;
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = static_cast<const stbi_uc*>(image.data());
	if (stbi_write_tga(filename, width, height, channel_count, pixels) == 0) {
		throw image_error{fmt::format("Failed to save TGA image \"{}\"!", filename)};
	}
}

struct image_jpg_options final {
	int quality = 90;
	bool flip_vertically = false;
};

inline auto save_jpg(image_view image, const char* filename, const image_jpg_options& options = {}) -> void {
	stbi_flip_vertically_on_write(options.flip_vertically ? 1 : 0);
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = static_cast<const stbi_uc*>(image.data());
	if (stbi_write_jpg(filename, width, height, channel_count, pixels, options.quality) == 0) {
		throw image_error{fmt::format("Failed to save JPG image \"{}\"!", filename)};
	}
}

struct image_hdr_options final {
	bool flip_vertically = false;
};

inline auto save_hdr(image_view image, const char* filename, const image_hdr_options& options = {}) -> void {
	stbi_flip_vertically_on_write(options.flip_vertically ? 1 : 0);
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = static_cast<const float*>(image.data());
	if (stbi_write_hdr(filename, width, height, channel_count, pixels) == 0) {
		throw image_error{fmt::format("Failed to save HDR image \"{}\"!", filename)};
	}
}

#endif
