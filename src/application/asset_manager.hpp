#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "../core/opengl.hpp"
#include "../render/cubemap_generator.hpp"
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

	[[nodiscard]] auto load_cubemap(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<cubemap_texture> {
		const auto it = m_cubemaps.try_emplace(fmt::format("{}%{}", filename_prefix, extension)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap_texture>(cubemap_texture::load(filename_prefix, extension));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_hdr(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<cubemap_texture> {
		const auto it = m_cubemaps_hdr.try_emplace(fmt::format("{}%{}", filename_prefix, extension)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap_texture>(cubemap_texture::load_hdr(filename_prefix, extension));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_equirectangular(const char* filename, std::size_t resolution) -> std::shared_ptr<cubemap_texture> {
		const auto it = m_cubemaps.try_emplace(fmt::format("{}@{}", filename, resolution)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		const auto img = image::load(filename, {.flip_vertically = true});
		const auto internal_format = texture::internal_pixel_format_ldr(img.channel_count());
		const auto format = texture::pixel_format(img.channel_count());
		const auto equirectangular_texture = texture::create_2d(
			internal_format, img.width(), img.height(), format, GL_UNSIGNED_BYTE, img.data(), cubemap_texture::equirectangular_options);
		auto ptr = std::make_shared<cubemap_texture>(m_cubemap_generator.generate_cubemap_from_equirectangular_2d(internal_format, equirectangular_texture, resolution));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_equirectangular_hdr(const char* filename, std::size_t resolution) -> std::shared_ptr<cubemap_texture> {
		const auto it = m_cubemaps_hdr.try_emplace(fmt::format("{}@{}", filename, resolution)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		const auto img = image::load_hdr(filename, {.flip_vertically = true});
		const auto internal_format = texture::internal_pixel_format_hdr(img.channel_count());
		const auto format = texture::pixel_format(img.channel_count());
		const auto equirectangular_texture = texture::create_2d(internal_format, img.width(), img.height(), format, GL_FLOAT, img.data(), cubemap_texture::equirectangular_options);
		auto ptr = std::make_shared<cubemap_texture>(m_cubemap_generator.generate_cubemap_from_equirectangular_2d(internal_format, equirectangular_texture, resolution));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_environment_cubemap(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<environment_cubemap> {
		auto environment = load_cubemap(filename_prefix, extension);
		auto irradiance = m_cubemap_generator.generate_irradiance_map(irradiance_map_internal_format, *environment, irradiance_map_resolution);
		auto prefilter = m_cubemap_generator.generate_prefilter_map(prefilter_map_internal_format, *environment, prefilter_map_resolution, prefilter_map_mip_level_count);
		return std::make_shared<environment_cubemap>(std::move(environment), std::move(irradiance), std::move(prefilter));
	}

	[[nodiscard]] auto load_environment_cubemap_hdr(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<environment_cubemap> {
		auto environment = load_cubemap_hdr(filename_prefix, extension);
		auto irradiance = m_cubemap_generator.generate_irradiance_map(irradiance_map_internal_format, *environment, irradiance_map_resolution);
		auto prefilter = m_cubemap_generator.generate_prefilter_map(prefilter_map_internal_format, *environment, prefilter_map_resolution, prefilter_map_mip_level_count);
		return std::make_shared<environment_cubemap>(std::move(environment), std::move(irradiance), std::move(prefilter));
	}

	[[nodiscard]] auto load_environment_cubemap_equirectangular(const char* filename, std::size_t resolution) -> std::shared_ptr<environment_cubemap> {
		auto environment = load_cubemap_equirectangular(filename, resolution);
		auto irradiance = m_cubemap_generator.generate_irradiance_map(irradiance_map_internal_format, *environment, irradiance_map_resolution);
		auto prefilter = m_cubemap_generator.generate_prefilter_map(prefilter_map_internal_format, *environment, prefilter_map_resolution, prefilter_map_mip_level_count);
		return std::make_shared<environment_cubemap>(std::move(environment), std::move(irradiance), std::move(prefilter));
	}

	[[nodiscard]] auto load_environment_cubemap_equirectangular_hdr(const char* filename, std::size_t resolution) -> std::shared_ptr<environment_cubemap> {
		auto environment = load_cubemap_equirectangular_hdr(filename, resolution);
		auto irradiance = m_cubemap_generator.generate_irradiance_map(irradiance_map_internal_format, *environment, irradiance_map_resolution);
		auto prefilter = m_cubemap_generator.generate_prefilter_map(prefilter_map_internal_format, *environment, prefilter_map_resolution, prefilter_map_mip_level_count);
		return std::make_shared<environment_cubemap>(std::move(environment), std::move(irradiance), std::move(prefilter));
	}

	[[nodiscard]] auto load_model(std::string filename, std::string_view textures_filename_prefix) -> std::shared_ptr<model> {
		const auto it = m_models.try_emplace(std::move(filename)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<model>(model::load(it->first.c_str(), textures_filename_prefix, m_model_texture_cache));
		it->second = ptr;
		return ptr;
	}

	auto clear() noexcept -> void {
		m_models.clear();
		m_cubemaps_hdr.clear();
		m_cubemaps.clear();
		m_model_texture_cache.clear();
		m_images_hdr.clear();
		m_images.clear();
		m_fonts.clear();
	}

	auto cleanup() -> void {
		static constexpr auto has_expired = [](const auto& kv) {
			return kv.second.expired();
		};
		std::erase_if(m_models, has_expired);
		std::erase_if(m_cubemaps_hdr, has_expired);
		std::erase_if(m_cubemaps, has_expired);
		std::erase_if(m_model_texture_cache, has_expired);
		std::erase_if(m_images_hdr, has_expired);
		std::erase_if(m_images, has_expired);
		std::erase_if(m_fonts, has_expired);
	}

	auto reload_shaders() -> void {
		m_cubemap_generator.reload_shaders();
	}

private:
	static constexpr auto irradiance_map_internal_format = GLint{GL_RGB16F};
	static constexpr auto irradiance_map_resolution = std::size_t{32};
	static constexpr auto prefilter_map_internal_format = GLint{GL_RGB16F};
	static constexpr auto prefilter_map_resolution = std::size_t{128};
	static constexpr auto prefilter_map_mip_level_count = std::size_t{5};

	using font_cache = std::unordered_map<std::string, std::weak_ptr<font>>;
	using image_cache = std::unordered_map<std::string, std::weak_ptr<image>>;
	using cubemap_cache = std::unordered_map<std::string, std::weak_ptr<cubemap_texture>>;
	using model_cache = std::unordered_map<std::string, std::weak_ptr<model>>;

	font_library m_font_library{};
	cubemap_generator m_cubemap_generator{};
	font_cache m_fonts{};
	image_cache m_images{};
	image_cache m_images_hdr{};
	model_texture_cache m_model_texture_cache{};
	cubemap_cache m_cubemaps{};
	cubemap_cache m_cubemaps_hdr{};
	model_cache m_models{};
};

#endif
