#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "font.hpp"
#include "image.hpp"
#include "mesh.hpp"
#include "texture.hpp"

#include <array>         // std::array
#include <cstddef>       // std::size_t
#include <functional>    // std::hash
#include <glm/glm.hpp>   // glm::...
#include <memory>        // std::unique_ptr, std::shared_ptr, std::weak_ptr, std::make_shared
#include <string>        // std::string
#include <unordered_map> // std::unordered_map, std::erase_if
#include <utility>       // std::move

class asset_manager final {
public:
	[[nodiscard]] auto load_quad_mesh() -> std::shared_ptr<textured_2d_mesh> {
		return m_quad_mesh;
	}

	[[nodiscard]] auto load_font(std::string filename, unsigned int size) -> std::shared_ptr<font> {
		const auto it = m_fonts.try_emplace(font_key{std::move(filename), size}).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<font>(m_font_library.get(), it->first.filename.c_str(), it->first.size);
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_image(std::string filename) -> std::shared_ptr<image> {
		const auto it = m_images.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<image>(it->first.c_str());
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_texture(std::string filename) -> std::shared_ptr<texture> {
		const auto it = m_textures.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		const auto img = image{it->first.c_str()};
		auto ptr = std::make_shared<texture>(internal_texture_format(img.pixel_size()), img, default_texture_options);
		it->second = ptr;
		return ptr;
	}

	auto cleanup() -> void {
		std::erase_if(m_fonts, [](const auto& kv) { return kv.second.expired(); });
		std::erase_if(m_images, [](const auto& kv) { return kv.second.expired(); });
		std::erase_if(m_textures, [](const auto& kv) { return kv.second.expired(); });
	}

private:
	template <typename... Ts>
	[[nodiscard]] static auto hash_combine(const Ts&... args) -> std::size_t {
		return (std::hash<Ts>{}(args) ^ ...);
	}

	[[nodiscard]] static auto internal_texture_format(std::size_t pixel_size) noexcept -> GLenum {
		// TODO: SRGB
		switch (pixel_size) {
			case 1: return GL_R8;
			case 2: return GL_RG8;
			case 3: return GL_RGB8;
			case 4: return GL_RGBA8;
			default: break;
		}
		return 0;
	}

	static constexpr auto default_texture_options = texture_options{
		.max_anisotropy = 8.0f,
		.repeat = true,
		.use_linear_filtering = true,
		.use_mip_map = true,
		// TODO: SRGB
	};

	struct font_key final {
		std::string filename{};
		unsigned int size = 0u;

		[[nodiscard]] auto operator==(const font_key&) const -> bool = default;
		struct hash final {
			[[nodiscard]] auto operator()(const font_key& key) const -> std::size_t {
				return hash_combine(key.filename, key.size);
			}
		};
	};

	using font_cache = std::unordered_map<font_key, std::weak_ptr<font>, font_key::hash>;
	using image_cache = std::unordered_map<std::string, std::weak_ptr<image>>;
	using texture_cache = std::unordered_map<std::string, std::weak_ptr<texture>>;

	std::shared_ptr<textured_2d_mesh> m_quad_mesh = std::make_shared<textured_2d_mesh>(GL_STATIC_DRAW,
		std::array<textured_2d_vertex, 6>{
			textured_2d_vertex{glm::vec2{0.0f, 1.0f}, glm::vec2{0.0f, 0.0f}},
			textured_2d_vertex{glm::vec2{0.0f, 0.0f}, glm::vec2{0.0f, 1.0f}},
			textured_2d_vertex{glm::vec2{1.0f, 0.0f}, glm::vec2{1.0f, 1.0f}},
			textured_2d_vertex{glm::vec2{0.0f, 1.0f}, glm::vec2{0.0f, 0.0f}},
			textured_2d_vertex{glm::vec2{1.0f, 0.0f}, glm::vec2{1.0f, 1.0f}},
			textured_2d_vertex{glm::vec2{1.0f, 1.0f}, glm::vec2{1.0f, 0.0f}},
		},
		&textured_2d_vertex::position, &textured_2d_vertex::texture_coordinates);
	font_library m_font_library{};
	font_cache m_fonts{};
	image_cache m_images{};
	texture_cache m_textures{};
};

#endif
