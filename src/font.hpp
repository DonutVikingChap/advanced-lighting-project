#ifndef FONT_HPP
#define FONT_HPP

#include "context.hpp"
#include "framebuffer.hpp"
#include "mesh.hpp"
#include "opengl.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "utf8.hpp"

#include <algorithm>     // std::max
#include <cstddef>       // std::byte, std::size_t
#include <fmt/format.h>  // fmt::...
#include <glm/glm.hpp>   // glm::...
#include <memory>        // std::unique_ptr
#include <stdexcept>     // std::runtime_error
#include <string_view>   // std::u8string_view
#include <type_traits>   // std::remove_pointer_t
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move
#include <vector>        // std::vector

// clang-format off
#include <ft2build.h>  // FT_FREETYPE_H
#include FT_FREETYPE_H // FT_...
// clang-format on

struct font_error : std::runtime_error {
	explicit font_error(const auto& message)
		: std::runtime_error(message) {}

	explicit font_error(const auto& message, FT_Error ft_error)
		: std::runtime_error(fmt::format("{}: {}", message, error_string(ft_error))) {}

private:
	[[nodiscard]] static auto error_string(FT_Error ft_error) noexcept -> const char* {
		const auto* const str = FT_Error_String(ft_error);
		return (str) ? str : "Unknown error!";
	}
};

class font_library final {
public:
	[[nodiscard]] auto get() const noexcept -> FT_Library {
		return m_library.get();
	}

private:
	struct library_deleter final {
		auto operator()(FT_Library p) const noexcept -> void {
			FT_Done_FreeType(p);
		}
	};
	using library_ptr = std::unique_ptr<std::remove_pointer_t<FT_Library>, library_deleter>;

	library_ptr m_library{[] {
		auto* ft = FT_Library{};
		if (const auto ft_error = FT_Init_FreeType(&ft); ft_error != FT_Err_Ok) {
			throw font_error{"Failed to initialize FreeType", ft_error};
		}
		return ft;
	}()};
};

struct font_glyph final {
	glm::vec2 texture_offset{};
	glm::vec2 texture_scale{};
	glm::vec2 position{};
	glm::vec2 size{};
	glm::vec2 bearing{};
	float advance = 0.0f;
};

class font final {
public:
	font(FT_Library library, const char* filename, unsigned int size)
		: m_face([&] {
			auto* face = FT_Face{};
			if (const auto ft_error = FT_New_Face(library, filename, 0, &face); ft_error != FT_Err_Ok) {
				throw font_error{fmt::format("Failed to load font \"{}\"", filename), ft_error};
			}
			return face;
		}()) {
		if (const auto ft_error = FT_Select_Charmap(m_face.get(), FT_ENCODING_UNICODE); ft_error != FT_Err_Ok) {
			throw font_error{fmt::format("Failed to load unicode charmap for font \"{}\"", filename), ft_error};
		}
		if (const auto ft_error = FT_Set_Pixel_Sizes(m_face.get(), 0, static_cast<FT_UInt>(size)); ft_error != FT_Err_Ok) {
			throw font_error{fmt::format("Failed to load font \"{}\" at size {}", filename, size), ft_error};
		}
		m_ascii_glyphs.reserve(128);
		for (auto ch = char32_t{0}; ch < char32_t{128}; ++ch) {
			m_ascii_glyphs.push_back(render_glyph(ch));
		}
	}

	[[nodiscard]] auto find_glyph(char32_t ch) const noexcept -> const font_glyph* {
		if (const auto index = static_cast<std::size_t>(ch); index < m_ascii_glyphs.size()) {
			return &m_ascii_glyphs[index];
		}
		if (const auto it = m_other_glyphs.find(ch); it != m_other_glyphs.end()) {
			return &it->second;
		}
		return nullptr;
	}

	auto load_glyph(char32_t ch) -> const font_glyph& {
		if (const auto index = static_cast<std::size_t>(ch); index < m_ascii_glyphs.size()) {
			return m_ascii_glyphs[index];
		}
		const auto [it, inserted] = m_other_glyphs.try_emplace(ch);
		if (inserted) {
			it->second = render_glyph(ch);
		}
		return it->second;
	}

	auto load_glyphs(std::u8string_view str) -> void {
		for (const auto ch : utf8_view{str}) {
			load_glyph(ch);
		}
	}

	[[nodiscard]] auto line_space() const noexcept -> float {
		return static_cast<float>(m_face->size->metrics.height) * 0.015625f;
	}

	[[nodiscard]] auto kerning(char32_t left, char32_t right) const noexcept -> float {
		if (left == 0 || right == 0 || !FT_HAS_KERNING(m_face.get())) {
			return 0.0f;
		}
		const auto left_index = FT_Get_Char_Index(m_face.get(), FT_ULong{left});
		const auto right_index = FT_Get_Char_Index(m_face.get(), FT_ULong{right});
		auto kerning = FT_Vector{};
		FT_Get_Kerning(m_face.get(), left_index, right_index, FT_KERNING_DEFAULT, &kerning);
		const auto kerning_x = static_cast<float>(kerning.x);
		return (FT_IS_SCALABLE(m_face.get())) ? kerning_x * 0.015625f : kerning_x;
	}

	[[nodiscard]] auto text_size(glm::vec2 scale, std::u8string_view str) const noexcept -> glm::vec2 {
		auto size = glm::vec2{};
		auto x = 0.0f;
		auto top = true;
		const auto code_points = utf8_view{str};
		for (auto it = code_points.begin(); it != code_points.end();) {
			if (const auto ch = *it++; ch == '\n') {
				x = 0.0f;
				top = false;
				size.y += line_space();
			} else if (const auto* const glyph = find_glyph(ch)) {
				if (top) {
					size.y = std::max(size.y, glyph->size.y);
				}
				x += glyph->advance + kerning(ch, (it == code_points.end()) ? 0 : *it);
				size.x = std::max(size.x, x);
			}
		}
		return size * scale;
	}

	[[nodiscard]] auto atlas() const noexcept -> const texture& {
		return m_atlas;
	}

private:
	class [[nodiscard]] state_preserver final {
	public:
		state_preserver() noexcept {
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_framebuffer_binding);
		}

		~state_preserver() {
			glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_binding);
		}

		state_preserver(const state_preserver&) = delete;
		state_preserver(state_preserver&&) = delete;
		auto operator=(const state_preserver&) -> state_preserver& = delete;
		auto operator=(state_preserver&&) -> state_preserver& = delete;

	private:
		GLint m_framebuffer_binding = 0;
	};

	struct glyph_row final {
		constexpr glyph_row(std::size_t top, std::size_t width, std::size_t height) noexcept
			: top(top)
			, width(width)
			, height(height) {}

		std::size_t top;
		std::size_t width;
		std::size_t height;
	};

	static constexpr auto glyph_format = GLenum{GL_RED};
	static constexpr auto atlas_initial_size = std::size_t{128};
	static constexpr auto atlas_growth_factor = std::size_t{2};
	static constexpr auto atlas_internal_format = GLint{GL_R8};
	static constexpr auto atlas_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.use_linear_filtering = false,
		.use_mip_map = false,
	};

	auto find_suitable_glyph_row(std::size_t padded_width, std::size_t padded_height) -> glyph_row* {
		for (auto& row : m_atlas_rows) {
			if (const auto height_ratio = static_cast<float>(padded_height) / static_cast<float>(row.height);
				height_ratio >= 0.7f && height_ratio <= 1.0f && padded_width <= m_atlas.width() - row.width) {
				return &row;
			}
		}
		return nullptr;
	}

	auto grow_atlas() -> void {
		const auto preserver = state_preserver{};
		auto new_atlas = texture{atlas_internal_format, nullptr, m_atlas.width() * atlas_growth_factor, m_atlas.height() * atlas_growth_factor, glyph_format, atlas_options};
		auto fbo = framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_atlas.get(), 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, new_atlas.get(), 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		context::check_framebuffer_status();
		const auto width = static_cast<GLint>(m_atlas.width());
		const auto height = static_cast<GLint>(m_atlas.height());
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
		m_atlas = std::move(new_atlas);
	}

	auto update_texture_coordinates() -> void {
		const auto texture_size = glm::vec2{static_cast<float>(m_atlas.width()), static_cast<float>(m_atlas.height())};
		for (auto& glyph : m_ascii_glyphs) {
			glyph.texture_offset = glyph.position / texture_size;
			glyph.texture_scale = glyph.size / texture_size;
		}
		for (auto& [ch, glyph] : m_other_glyphs) {
			glyph.texture_offset = glyph.position / texture_size;
			glyph.texture_scale = glyph.size / texture_size;
		}
	}

	[[nodiscard]] auto render_glyph(char32_t ch) -> font_glyph {
		if (const auto ft_error = FT_Load_Char(m_face.get(), FT_ULong{ch}, FT_LOAD_RENDER); ft_error != FT_Err_Ok) {
			throw font_error{fmt::format("Failed to render font glyph for char {}", ch), ft_error};
		}
		auto x = std::size_t{0};
		auto y = std::size_t{0};
		const auto width = static_cast<std::size_t>(m_face->glyph->bitmap.width);
		const auto height = static_cast<std::size_t>(m_face->glyph->bitmap.rows);
		if (const auto* const pixels = m_face->glyph->bitmap.buffer) {
			if (m_face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
				throw font_error{fmt::format("Invalid font glyph pixel mode for char {}!", ch)};
			}
			constexpr auto padding = std::size_t{2};
			const auto padded_width = width + padding * std::size_t{2};
			const auto padded_height = height + padding * std::size_t{2};
			auto* row = find_suitable_glyph_row(padded_width, padded_height);
			if (!row) {
				const auto new_row_top = (m_atlas_rows.empty()) ? std::size_t{0} : m_atlas_rows.back().top + m_atlas_rows.back().height;
				const auto new_row_height = padded_height + padded_height / std::size_t{10};
				while (m_atlas.height() < new_row_top + new_row_height || m_atlas.width() < padded_width) {
					grow_atlas();
					update_texture_coordinates();
				}
				row = &m_atlas_rows.emplace_back(new_row_top, std::size_t{0}, padded_height);
			}
			x = row->width + padding;
			y = row->top + padding;
			m_atlas.paste(reinterpret_cast<const std::byte*>(pixels), width, height, glyph_format, x, y);
			row->width += padded_width;
		}
		const auto texture_size = glm::vec2{static_cast<float>(m_atlas.width()), static_cast<float>(m_atlas.height())};
		const auto position = glm::vec2{static_cast<float>(x), static_cast<float>(y)};
		const auto size = glm::vec2{static_cast<float>(width), static_cast<float>(height)};
		return font_glyph{
			.texture_offset = position / texture_size,
			.texture_scale = size / texture_size,
			.position = position,
			.size = size,
			.bearing = glm::vec2{static_cast<float>(m_face->glyph->bitmap_left), static_cast<float>(m_face->glyph->bitmap_top)},
			.advance = static_cast<float>(m_face->glyph->advance.x) * 0.015625f,
		};
	}

	struct face_deleter final {
		auto operator()(FT_Face p) const noexcept -> void {
			FT_Done_Face(p);
		}
	};
	using face_ptr = std::unique_ptr<std::remove_pointer_t<FT_Face>, face_deleter>;

	face_ptr m_face;
	texture m_atlas{atlas_internal_format, nullptr, atlas_initial_size, atlas_initial_size, glyph_format, atlas_options};
	std::vector<glyph_row> m_atlas_rows{};
	std::vector<font_glyph> m_ascii_glyphs{};
	std::unordered_map<char32_t, font_glyph> m_other_glyphs{};
};

#endif
