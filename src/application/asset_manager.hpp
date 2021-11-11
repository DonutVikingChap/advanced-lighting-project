#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "../resources/cubemap.hpp"
#include "../resources/font.hpp"
#include "../resources/image.hpp"
#include "../resources/model.hpp"
#include "../resources/texture.hpp"

#include <fmt/format.h>  // fmt::format
#include <memory>        // std::unique_ptr, std::shared_ptr, std::weak_ptr, std::make_shared
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map, std::erase_if
#include <utility>       // std::move

class asset_manager final {
public:
	auto reload_shaders() -> void {
		m_cubemap_converter.reload_shaders();
	}

	[[nodiscard]] auto load_font(const char* filename, unsigned int size) -> std::shared_ptr<font> {
		const auto it = m_fonts.try_emplace(fmt::format("{}@{}", filename, size)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<font>(m_font_library.get(), filename, size);
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_image(std::string filename) -> std::shared_ptr<image> {
		const auto it = m_images.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<image>(image::load(it->first.c_str()));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_image_hdr(std::string filename) -> std::shared_ptr<image> {
		const auto it = m_images_hdr.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<image>(image::load_hdr(it->first.c_str()));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_texture(std::string filename) -> std::shared_ptr<texture> {
		const auto it = m_textures.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		const auto img = load_image(it->first);
		auto ptr = std::make_shared<texture>(texture::create_2d(texture::internal_pixel_format_ldr(img->channel_count()),
			img->width(),
			img->height(),
			texture::pixel_format(img->channel_count()),
			GL_UNSIGNED_BYTE,
			img->data(),
			default_texture_options));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_texture_hdr(std::string filename) -> std::shared_ptr<texture> {
		const auto it = m_textures_hdr.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		const auto img = load_image_hdr(it->first);
		auto ptr = std::make_shared<texture>(texture::create_2d(texture::internal_pixel_format_hdr(img->channel_count()),
			img->width(),
			img->height(),
			texture::pixel_format(img->channel_count()),
			GL_FLOAT,
			img->data(),
			default_texture_options));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<cubemap> {
		const auto it = m_cubemaps.try_emplace(fmt::format("{}%{}", filename_prefix, extension)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap>(cubemap::load(filename_prefix, extension, cubemap_texture_options));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_hdr(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<cubemap> {
		const auto it = m_cubemaps_hdr.try_emplace(fmt::format("{}%{}", filename_prefix, extension)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap>(cubemap::load_hdr(filename_prefix, extension, cubemap_texture_options));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_equirectangular(const char* filename, std::size_t resolution) -> std::shared_ptr<cubemap> {
		const auto it = m_cubemaps.try_emplace(fmt::format("{}@{}", filename, resolution)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap>(cubemap::load_equirectangular(m_cubemap_converter, filename, resolution, cubemap_texture_options));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_equirectangular_hdr(const char* filename, std::size_t resolution) -> std::shared_ptr<cubemap> {
		const auto it = m_cubemaps_hdr.try_emplace(fmt::format("{}@{}", filename, resolution)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap>(cubemap::load_equirectangular_hdr(m_cubemap_converter, filename, resolution, cubemap_texture_options));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_model(std::string filename) -> std::shared_ptr<model> {
		const auto it = m_models.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<model>(model::load(it->first.c_str()));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_model_textures(std::string_view filename_prefix, const model& model) -> std::vector<std::shared_ptr<texture>> {
		auto result = std::vector<std::shared_ptr<texture>>{};
		result.reserve(model.texture_info().size());
		for (const auto& texture_info : model.texture_info()) {
			auto filename = fmt::format("{}{}", filename_prefix, texture_info.filename);
			auto texture = (texture_info.hdr) ? load_texture_hdr(std::move(filename)) : load_texture(std::move(filename));
			result.push_back(std::move(texture));
		}
		return result;
	}

	[[nodiscard]] auto load_textured_model(std::string filename, std::string_view textures_filename_prefix) -> std::shared_ptr<textured_model> {
		const auto it = m_textured_models.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto model = load_model(it->first);
		auto textures = load_model_textures(textures_filename_prefix, *model);
		auto ptr = std::make_shared<textured_model>(std::move(model), std::move(textures));
		it->second = ptr;
		return ptr;
	}

	auto cleanup() -> void {
		static constexpr auto has_expired = [](const auto& kv) {
			return kv.second.expired();
		};
		std::erase_if(m_textured_models, has_expired);
		std::erase_if(m_models, has_expired);
		std::erase_if(m_cubemaps_hdr, has_expired);
		std::erase_if(m_cubemaps, has_expired);
		std::erase_if(m_textures_hdr, has_expired);
		std::erase_if(m_textures, has_expired);
		std::erase_if(m_images_hdr, has_expired);
		std::erase_if(m_images, has_expired);
		std::erase_if(m_fonts, has_expired);
	}

private:
	static constexpr auto default_texture_options = texture_options{
		.max_anisotropy = 8.0f,
		.repeat = true,
		.use_linear_filtering = true,
		.use_mip_map = true,
	};

	static constexpr auto cubemap_texture_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.use_linear_filtering = true,
		.use_mip_map = false,
	};

	using font_cache = std::unordered_map<std::string, std::weak_ptr<font>>;
	using image_cache = std::unordered_map<std::string, std::weak_ptr<image>>;
	using texture_cache = std::unordered_map<std::string, std::weak_ptr<texture>>;
	using cubemap_cache = std::unordered_map<std::string, std::weak_ptr<cubemap>>;
	using model_cache = std::unordered_map<std::string, std::weak_ptr<model>>;
	using textured_model_cache = std::unordered_map<std::string, std::weak_ptr<textured_model>>;

	font_library m_font_library{};
	cubemap_converter m_cubemap_converter{};
	font_cache m_fonts{};
	image_cache m_images{};
	image_cache m_images_hdr{};
	texture_cache m_textures{};
	texture_cache m_textures_hdr{};
	cubemap_cache m_cubemaps{};
	cubemap_cache m_cubemaps_hdr{};
	model_cache m_models{};
	textured_model_cache m_textured_models{};
};

#endif
