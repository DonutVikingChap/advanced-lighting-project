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

	constexpr image_view(const std::byte* pixels, std::size_t width, std::size_t height, std::size_t pixel_size) noexcept
		: m_pixels(pixels)
		, m_width(width)
		, m_height(height)
		, m_pixel_size(pixel_size) {}

	[[nodiscard]] auto data() const noexcept -> const std::byte* {
		return m_pixels;
	}

	[[nodiscard]] auto size() const noexcept -> std::size_t {
		return m_width * m_height * m_pixel_size;
	}

	[[nodiscard]] auto width() const noexcept -> std::size_t {
		return m_width;
	}

	[[nodiscard]] auto height() const noexcept -> std::size_t {
		return m_height;
	}

	[[nodiscard]] auto pixel_size() const noexcept -> std::size_t {
		return m_pixel_size;
	}

private:
	const std::byte* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_pixel_size = 0;
};

class image final {
public:
	explicit image(const char* filename) {
		int width = 0;
		int height = 0;
		int pixel_size = 0;
		m_pixels = reinterpret_cast<std::byte*>(stbi_load(filename, &width, &height, &pixel_size, 0));
		if (!m_pixels) {
			throw image_error{fmt::format("Failed to load image \"{}\"!", filename)};
		}
		m_width = static_cast<std::size_t>(width);
		m_height = static_cast<std::size_t>(height);
		m_pixel_size = static_cast<std::size_t>(pixel_size);
	}

	~image() {
		stbi_image_free(reinterpret_cast<stbi_uc*>(m_pixels));
	}

	image(const image&) = delete;

	image(image&& other) noexcept {
		*this = std::move(other);
	}

	auto operator=(const image&) -> image& = delete;

	auto operator=(image&& other) noexcept -> image& {
		stbi_image_free(reinterpret_cast<stbi_uc*>(m_pixels));
		m_pixels = std::exchange(other.m_pixels, nullptr);
		m_width = std::exchange(other.m_width, 0);
		m_height = std::exchange(other.m_height, 0);
		m_pixel_size = std::exchange(other.m_pixel_size, 0);
		return *this;
	}

	operator image_view() const noexcept {
		return image_view{m_pixels, m_width, m_height, m_pixel_size};
	}

	[[nodiscard]] auto data() noexcept -> std::byte* {
		return m_pixels;
	}

	[[nodiscard]] auto data() const noexcept -> const std::byte* {
		return m_pixels;
	}

	[[nodiscard]] auto size() const noexcept -> std::size_t {
		return m_width * m_height * m_pixel_size;
	}

	[[nodiscard]] auto width() const noexcept -> std::size_t {
		return m_width;
	}

	[[nodiscard]] auto height() const noexcept -> std::size_t {
		return m_height;
	}

	[[nodiscard]] auto pixel_size() const noexcept -> std::size_t {
		return m_pixel_size;
	}

private:
	std::byte* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_pixel_size = 0;
};

inline auto save_png(const image_view& image, const char* filename, int compression_level = 8) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto pixel_size = static_cast<int>(image.pixel_size());
	const auto* const pixels = reinterpret_cast<const stbi_uc*>(image.data());
	stbi_write_png_compression_level = compression_level;
	if (stbi_write_png(filename, width, height, pixel_size, pixels, 0) == 0) {
		throw image_error{fmt::format("Failed to save PNG image \"{}\"!", filename)};
	}
}

inline auto save_bmp(const image_view& image, const char* filename) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto pixel_size = static_cast<int>(image.pixel_size());
	const auto* const pixels = reinterpret_cast<const stbi_uc*>(image.data());
	if (stbi_write_bmp(filename, width, height, pixel_size, pixels) == 0) {
		throw image_error{fmt::format("Failed to save BMP image \"{}\"!", filename)};
	}
}

inline auto save_tga(const image_view& image, const char* filename, bool use_rle_compression = true) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto pixel_size = static_cast<int>(image.pixel_size());
	const auto* const pixels = reinterpret_cast<const stbi_uc*>(image.data());
	stbi_write_tga_with_rle = (use_rle_compression) ? 1 : 0;
	if (stbi_write_tga(filename, width, height, pixel_size, pixels) == 0) {
		throw image_error{fmt::format("Failed to save TGA image \"{}\"!", filename)};
	}
}

inline auto save_jpg(const image_view& image, const char* filename, int quality) -> void {
	const auto width = static_cast<int>(image.width());
	const auto height = static_cast<int>(image.height());
	const auto pixel_size = static_cast<int>(image.pixel_size());
	const auto* const pixels = reinterpret_cast<const stbi_uc*>(image.data());
	if (stbi_write_jpg(filename, width, height, pixel_size, pixels, quality) == 0) {
		throw image_error{fmt::format("Failed to save JPG image \"{}\"!", filename)};
	}
}

#endif
