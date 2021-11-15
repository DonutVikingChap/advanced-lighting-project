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

	[[nodiscard]] auto load_cubemap(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<cubemap> {
		const auto it = m_cubemaps.try_emplace(fmt::format("{}%{}", filename_prefix, extension)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap>(cubemap::load(filename_prefix, extension));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_hdr(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<cubemap> {
		const auto it = m_cubemaps_hdr.try_emplace(fmt::format("{}%{}", filename_prefix, extension)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap>(cubemap::load_hdr(filename_prefix, extension));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_equirectangular(const char* filename, std::size_t resolution) -> std::shared_ptr<cubemap> {
		const auto it = m_cubemaps.try_emplace(fmt::format("{}@{}", filename, resolution)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap>(cubemap::load_equirectangular(m_cubemap_generator, filename, resolution));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_cubemap_equirectangular_hdr(const char* filename, std::size_t resolution) -> std::shared_ptr<cubemap> {
		const auto it = m_cubemaps_hdr.try_emplace(fmt::format("{}@{}", filename, resolution)).first;
		if (auto ptr = it->second.lock()) {
			return ptr;
		}
		auto ptr = std::make_shared<cubemap>(cubemap::load_equirectangular_hdr(m_cubemap_generator, filename, resolution));
		it->second = ptr;
		return ptr;
	}

	[[nodiscard]] auto load_environment_cubemap(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<environment_cubemap> {
		return std::make_shared<environment_cubemap>(m_cubemap_generator, load_cubemap(filename_prefix, extension), default_environment_cubemap_options);
	}

	[[nodiscard]] auto load_environment_cubemap_hdr(std::string_view filename_prefix, std::string_view extension) -> std::shared_ptr<environment_cubemap> {
		return std::make_shared<environment_cubemap>(m_cubemap_generator, load_cubemap_hdr(filename_prefix, extension), default_environment_cubemap_options);
	}

	[[nodiscard]] auto load_environment_cubemap_equirectangular(const char* filename, std::size_t resolution) -> std::shared_ptr<environment_cubemap> {
		return std::make_shared<environment_cubemap>(m_cubemap_generator, load_cubemap_equirectangular(filename, resolution), default_environment_cubemap_options);
	}

	[[nodiscard]] auto load_environment_cubemap_equirectangular_hdr(const char* filename, std::size_t resolution) -> std::shared_ptr<environment_cubemap> {
		return std::make_shared<environment_cubemap>(m_cubemap_generator, load_cubemap_equirectangular_hdr(filename, resolution), default_environment_cubemap_options);
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
	static constexpr auto default_environment_cubemap_options = environment_cubemap_options{
		.irradiance_map_resolution = 32,
		.prefilter_map_resolution = 128,
		.prefilter_map_mip_level_count = 5,
	};

	using font_cache = std::unordered_map<std::string, std::weak_ptr<font>>;
	using image_cache = std::unordered_map<std::string, std::weak_ptr<image>>;
	using cubemap_cache = std::unordered_map<std::string, std::weak_ptr<cubemap>>;
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
