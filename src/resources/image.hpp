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

	constexpr image_view(const std::byte* pixels, std::size_t width, std::size_t height, std::size_t channel_count) noexcept
		: m_pixels(pixels)
		, m_width(width)
		, m_height(height)
		, m_channel_count(channel_count) {}

	[[nodiscard]] auto data() const noexcept -> const std::byte* {
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

	[[nodiscard]] auto pixel_size() const noexcept -> std::size_t {
		return m_channel_count * sizeof(std::byte);
	}

	[[nodiscard]] auto size() const noexcept -> std::size_t {
		return width() * height() * pixel_size();
	}

private:
	const std::byte* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_channel_count = 0;
};

class image_view_hdr final {
public:
	constexpr image_view_hdr() noexcept = default;

	constexpr image_view_hdr(const float* pixels, std::size_t width, std::size_t height, std::size_t channel_count) noexcept
		: m_pixels(pixels)
		, m_width(width)
		, m_height(height)
		, m_channel_count(channel_count) {}

	[[nodiscard]] auto data() const noexcept -> const float* {
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

	[[nodiscard]] auto pixel_size() const noexcept -> std::size_t {
		return m_channel_count * sizeof(float);
	}

	[[nodiscard]] auto size() const noexcept -> std::size_t {
		return width() * height() * pixel_size();
	}

private:
	const float* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_channel_count = 0;
};

class image final {
public:
	[[nodiscard]] static auto load(const char* filename, int desired_channel_count = 0) -> image {
		int width = 0;
		int height = 0;
		int channel_count = 0;
		auto* const pixels = reinterpret_cast<std::byte*>(stbi_load(filename, &width, &height, &channel_count, desired_channel_count));
		if (!pixels) {
			throw image_error{fmt::format("Failed to load image \"{}\"!", filename)};
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

	[[nodiscard]] auto data() noexcept -> std::byte* {
		return m_pixels;
	}

	[[nodiscard]] auto data() const noexcept -> const std::byte* {
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

	[[nodiscard]] auto pixel_size() const noexcept -> std::size_t {
		return m_channel_count * sizeof(std::byte);
	}

	[[nodiscard]] auto size() const noexcept -> std::size_t {
		return width() * height() * pixel_size();
	}

private:
	image(std::byte* pixels, std::size_t width, std::size_t height, std::size_t channel_count) noexcept
		: m_pixels(pixels)
		, m_width(width)
		, m_height(height)
		, m_channel_count(channel_count) {}

	std::byte* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_channel_count = 0;
};

class image_hdr final {
public:
	[[nodiscard]] static auto load(const char* filename, int desired_channel_count = 0) -> image_hdr {
		int width = 0;
		int height = 0;
		int channel_count = 0;
		auto* const pixels = stbi_loadf(filename, &width, &height, &channel_count, desired_channel_count);
		if (!pixels) {
			throw image_error{fmt::format("Failed to load HDR image \"{}\"!", filename)};
		}
		return image_hdr{pixels, static_cast<std::size_t>(width), static_cast<std::size_t>(height), static_cast<std::size_t>(channel_count)};
	}

	~image_hdr() {
		stbi_image_free(m_pixels);
	}

	image_hdr(const image_hdr&) = delete;

	image_hdr(image_hdr&& other) noexcept {
		*this = std::move(other);
	}

	auto operator=(const image_hdr&) -> image_hdr& = delete;

	auto operator=(image_hdr&& other) noexcept -> image_hdr& {
		stbi_image_free(m_pixels);
		m_pixels = std::exchange(other.m_pixels, nullptr);
		m_width = std::exchange(other.m_width, 0);
		m_height = std::exchange(other.m_height, 0);
		m_channel_count = std::exchange(other.m_channel_count, 0);
		return *this;
	}

	operator image_view_hdr() const noexcept {
		return image_view_hdr{m_pixels, m_width, m_height, m_channel_count};
	}

	[[nodiscard]] auto data() noexcept -> float* {
		return m_pixels;
	}

	[[nodiscard]] auto data() const noexcept -> const float* {
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

	[[nodiscard]] auto pixel_size() const noexcept -> std::size_t {
		return m_channel_count * sizeof(float);
	}

	[[nodiscard]] auto size() const noexcept -> std::size_t {
		return width() * height() * pixel_size();
	}

private:
	image_hdr(float* pixels, std::size_t width, std::size_t height, std::size_t channel_count) noexcept
		: m_pixels(pixels)
		, m_width(width)
		, m_height(height)
		, m_channel_count(channel_count) {}

	float* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_channel_count = 0;
};

inline auto save_png(image_view image, const char* filename, int compression_level = 8) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = reinterpret_cast<const stbi_uc*>(image.data());
	stbi_write_png_compression_level = compression_level;
	if (stbi_write_png(filename, width, height, channel_count, pixels, 0) == 0) {
		throw image_error{fmt::format("Failed to save PNG image \"{}\"!", filename)};
	}
}

inline auto save_bmp(image_view image, const char* filename) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = reinterpret_cast<const stbi_uc*>(image.data());
	if (stbi_write_bmp(filename, width, height, channel_count, pixels) == 0) {
		throw image_error{fmt::format("Failed to save BMP image \"{}\"!", filename)};
	}
}

inline auto save_tga(image_view image, const char* filename, bool use_rle_compression = true) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = reinterpret_cast<const stbi_uc*>(image.data());
	stbi_write_tga_with_rle = (use_rle_compression) ? 1 : 0;
	if (stbi_write_tga(filename, width, height, channel_count, pixels) == 0) {
		throw image_error{fmt::format("Failed to save TGA image \"{}\"!", filename)};
	}
}

inline auto save_jpg(image_view image, const char* filename, int quality) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = reinterpret_cast<const stbi_uc*>(image.data());
	if (stbi_write_jpg(filename, width, height, channel_count, pixels, quality) == 0) {
		throw image_error{fmt::format("Failed to save JPG image \"{}\"!", filename)};
	}
}

inline auto save_hdr(image_view_hdr image, const char* filename) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto channel_count = static_cast<int>(image.channel_count());
	const auto* const pixels = image.data();
	if (stbi_write_hdr(filename, width, height, channel_count, pixels) == 0) {
		throw image_error{fmt::format("Failed to save HDR image \"{}\"!", filename)};
	}
}

#endif
