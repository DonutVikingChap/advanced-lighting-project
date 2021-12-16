#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "camera.hpp"
#include "shader.hpp"
#include "texture.hpp"

#include <array>                        // std::array
#include <cstddef>                      // std::size_t
#include <fmt/format.h>                 // fmt::format
#include <glm/gtc/matrix_transform.hpp> // glm::perspective, glm::lookAt
#include <limits>                       // std::numeric_limits
#include <string_view>                  // std::string_view

// clang-format off
static constexpr auto light_depth_conversion_matrix = mat4{
	0.5f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.5f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.5f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f,
};
// clang-format on

struct directional_light_options final {
	vec3 direction{0.0f, -1.0f, 0.0f};
	vec3 color{1.0f, 1.0f, 1.0f};
	float shadow_offset_factor = 1.1f;
	float shadow_offset_units = 128.0f;
	float shadow_light_size = 0.414f;
	std::size_t shadow_resolution = 2048;
	bool is_shadow_mapped = true;
};

struct directional_light final {
	static constexpr auto shadow_map_max_depth = std::numeric_limits<float>::max();
	static constexpr auto shadow_map_internal_format = GLint{GL_DEPTH_COMPONENT};
	static constexpr auto shadow_map_format = GLenum{GL_DEPTH_COMPONENT};
	static constexpr auto shadow_map_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.black_border = true,
		.use_linear_filtering = true,
		.use_mip_map = false,
		.use_compare_mode = true,
	};
	static constexpr auto depth_sampler_options = sampler_options{
		.repeat = false,
		.black_border = true,
		.use_linear_filtering = false,
	};

	[[nodiscard]] static auto default_shadow_map() -> GLuint {
		static const auto shadow_map = [] {
			auto depth = std::array<float, 1 * 1 * camera_cascade_count>{};
			depth.fill(shadow_map_max_depth);
			return texture::create_2d_array(shadow_map_internal_format, 1, 1, camera_cascade_count, shadow_map_format, GL_FLOAT, depth.data(), shadow_map_options);
		}();
		return shadow_map.get();
	}

	[[nodiscard]] static auto depth_sampler() -> GLuint {
		static const auto shared_sampler = sampler::create(depth_sampler_options);
		return shared_sampler.get();
	}

	explicit directional_light(const directional_light_options& options)
		: direction(options.direction)
		, color(options.color)
		, shadow_offset_factor(options.shadow_offset_factor)
		, shadow_offset_units(options.shadow_offset_units)
		, shadow_light_size(options.shadow_light_size) {
		if (options.is_shadow_mapped) {
			shadow_map = texture::create_2d_array_uninitialized(
				shadow_map_internal_format, options.shadow_resolution, options.shadow_resolution, camera_cascade_count, shadow_map_options);
			update_shadow_transform();
		}
	}

	auto update_shadow_transform() -> void {
		shadow_view_matrix = glm::lookAt(vec3{}, direction, vec3{0.0f, 1.0f, 0.0f});
	}

	vec3 direction;
	vec3 color;
	float shadow_offset_factor;
	float shadow_offset_units;
	float shadow_light_size;
	mat4 shadow_view_matrix{};
	std::array<mat4, camera_cascade_count> shadow_matrices{};
	std::array<float, camera_cascade_count> shadow_uv_sizes{};
	std::array<float, camera_cascade_count> shadow_near_planes{};
	texture shadow_map = texture::null();
};

struct point_light_options final {
	vec3 position{0.0f, 0.0f, 0.0f};
	vec3 color{1.0f, 1.0f, 1.0f};
	float constant = 1.0f;
	float linear = 0.045f;
	float quadratic = 0.0075f;
	float shadow_near_z = 0.01f;
	float shadow_far_z = 100.0f;
	float shadow_offset_factor = 1.1f;
	float shadow_offset_units = 128.0f;
	float shadow_filter_radius = 0.04f;
	std::size_t shadow_resolution = 512;
	bool is_shadow_mapped = true;
};

struct point_light final {
	static constexpr auto shadow_map_max_depth = std::numeric_limits<float>::max();
	static constexpr auto shadow_map_internal_format = GLint{GL_DEPTH_COMPONENT};
	static constexpr auto shadow_map_format = GLenum{GL_DEPTH_COMPONENT};
	static constexpr auto shadow_map_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.black_border = false,
		.use_linear_filtering = true,
		.use_mip_map = false,
		.use_compare_mode = true,
	};

	[[nodiscard]] static auto default_shadow_map() -> GLuint {
		static const auto shadow_map = [] {
			auto depth = std::array<float, 1 * 1>{};
			depth.fill(shadow_map_max_depth);
			return texture::create_cubemap(
				shadow_map_internal_format, 1, shadow_map_format, GL_FLOAT, depth.data(), depth.data(), depth.data(), depth.data(), depth.data(), depth.data(), shadow_map_options);
		}();
		return shadow_map.get();
	}

	explicit point_light(const point_light_options& options)
		: position(options.position)
		, color(options.color)
		, constant(options.constant)
		, linear(options.linear)
		, quadratic(options.quadratic)
		, shadow_near_z(options.shadow_near_z)
		, shadow_far_z(options.shadow_far_z)
		, shadow_offset_factor(options.shadow_offset_factor)
		, shadow_offset_units(options.shadow_offset_units)
		, shadow_filter_radius(options.shadow_filter_radius) {
		if (options.is_shadow_mapped) {
			shadow_map = texture::create_cubemap_uninitialized(shadow_map_internal_format, options.shadow_resolution, shadow_map_options);
			update_shadow_transform();
		}
	}

	auto update_shadow_transform() -> void {
		const auto projection_matrix = glm::perspective(radians(90.0f), 1.0f, shadow_near_z, shadow_far_z);
		shadow_projection_view_matrices = {
			projection_matrix * glm::lookAt(position, position + vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f}),
			projection_matrix * glm::lookAt(position, position + vec3{-1.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f}),
			projection_matrix * glm::lookAt(position, position + vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.0f, 1.0f}),
			projection_matrix * glm::lookAt(position, position + vec3{0.0f, -1.0f, 0.0f}, vec3{0.0f, 0.0f, -1.0f}),
			projection_matrix * glm::lookAt(position, position + vec3{0.0f, 0.0f, 1.0f}, vec3{0.0f, -1.0f, 0.0f}),
			projection_matrix * glm::lookAt(position, position + vec3{0.0f, 0.0f, -1.0f}, vec3{0.0f, -1.0f, 0.0f}),
		};
	}

	vec3 position;
	vec3 color;
	float constant;
	float linear;
	float quadratic;
	float shadow_near_z;
	float shadow_far_z;
	float shadow_offset_factor;
	float shadow_offset_units;
	float shadow_filter_radius;
	std::array<mat4, 6> shadow_projection_view_matrices{};
	texture shadow_map = texture::null();
};

struct spot_light_options final {
	vec3 position{0.0f, 0.0f, 0.0f};
	vec3 direction{0.0f, -1.0f, 0.0f};
	vec3 color{1.0f, 1.0f, 1.0f};
	float constant = 1.0f;
	float linear = 0.045f;
	float quadratic = 0.0075f;
	float inner_cutoff = cos(radians(40.0f));
	float outer_cutoff = cos(radians(50.0f));
	float shadow_near_z = 0.01f;
	float shadow_far_z = 100.0f;
	float shadow_offset_factor = 1.1f;
	float shadow_offset_units = 128.0f;
	float shadow_filter_radius = 2.0f;
	std::size_t shadow_resolution = 512;
	bool is_shadow_mapped = true;
};

struct spot_light final {
	static constexpr auto shadow_map_max_depth = std::numeric_limits<float>::max();
	static constexpr auto shadow_map_internal_format = GLint{GL_DEPTH_COMPONENT};
	static constexpr auto shadow_map_format = GLenum{GL_DEPTH_COMPONENT};
	static constexpr auto shadow_map_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.black_border = true,
		.use_linear_filtering = true,
		.use_mip_map = false,
		.use_compare_mode = true,
	};

	[[nodiscard]] static auto default_shadow_map() -> GLuint {
		static const auto shadow_map = [] {
			auto depth = std::array<float, 1 * 1>{};
			depth.fill(shadow_map_max_depth);
			return texture::create_2d(shadow_map_internal_format, 1, 1, shadow_map_format, GL_FLOAT, depth.data(), shadow_map_options);
		}();
		return shadow_map.get();
	}

	explicit spot_light(const spot_light_options& options)
		: position(options.position)
		, direction(options.direction)
		, color(options.color)
		, constant(options.constant)
		, linear(options.linear)
		, quadratic(options.quadratic)
		, inner_cutoff(options.inner_cutoff)
		, outer_cutoff(options.outer_cutoff)
		, shadow_near_z(options.shadow_near_z)
		, shadow_far_z(options.shadow_far_z)
		, shadow_offset_factor(options.shadow_offset_factor)
		, shadow_offset_units(options.shadow_offset_units)
		, shadow_filter_radius(options.shadow_filter_radius) {
		if (options.is_shadow_mapped) {
			shadow_map = texture::create_2d_uninitialized(shadow_map_internal_format, options.shadow_resolution, options.shadow_resolution, shadow_map_options);
			update_shadow_transform();
		}
	}

	auto update_shadow_transform() -> void {
		const auto projection_matrix = glm::perspective(2.0f * acos(outer_cutoff), 1.0f, shadow_near_z, shadow_far_z);
		const auto view_matrix = glm::lookAt(position, position + direction, vec3{0.0f, 1.0f, 0.0f});
		shadow_projection_view_matrix = projection_matrix * view_matrix;
		shadow_matrix = light_depth_conversion_matrix * shadow_projection_view_matrix;
	}

	vec3 position;
	vec3 direction;
	vec3 color;
	float constant;
	float linear;
	float quadratic;
	float inner_cutoff;
	float outer_cutoff;
	float shadow_near_z;
	float shadow_far_z;
	float shadow_offset_factor;
	float shadow_offset_units;
	float shadow_filter_radius;
	mat4 shadow_projection_view_matrix{};
	mat4 shadow_matrix{};
	texture shadow_map = texture::null();
};

struct directional_light_uniform final {
	directional_light_uniform(GLuint program, std::string_view name)
		: direction(program, fmt::format("{}.direction", name).c_str())
		, color(program, fmt::format("{}.color", name).c_str())
		, is_shadow_mapped(program, fmt::format("{}.is_shadow_mapped", name).c_str())
		, is_active(program, fmt::format("{}.is_active", name).c_str()) {}

	shader_uniform direction;
	shader_uniform color;
	shader_uniform is_shadow_mapped;
	shader_uniform is_active;
};

struct point_light_uniform final {
	point_light_uniform(GLuint program, std::string_view name)
		: position(program, fmt::format("{}.position", name).c_str())
		, color(program, fmt::format("{}.color", name).c_str())
		, constant(program, fmt::format("{}.constant", name).c_str())
		, linear(program, fmt::format("{}.linear", name).c_str())
		, quadratic(program, fmt::format("{}.quadratic", name).c_str())
		, shadow_near_z(program, fmt::format("{}.shadow_near_z", name).c_str())
		, shadow_far_z(program, fmt::format("{}.shadow_far_z", name).c_str())
		, shadow_filter_radius(program, fmt::format("{}.shadow_filter_radius", name).c_str())
		, is_shadow_mapped(program, fmt::format("{}.is_shadow_mapped", name).c_str())
		, is_active(program, fmt::format("{}.is_active", name).c_str()) {}

	shader_uniform position;
	shader_uniform color;
	shader_uniform constant;
	shader_uniform linear;
	shader_uniform quadratic;
	shader_uniform shadow_near_z;
	shader_uniform shadow_far_z;
	shader_uniform shadow_filter_radius;
	shader_uniform is_shadow_mapped;
	shader_uniform is_active;
};

struct spot_light_uniform final {
	spot_light_uniform(GLuint program, std::string_view name)
		: position(program, fmt::format("{}.position", name).c_str())
		, direction(program, fmt::format("{}.direction", name).c_str())
		, color(program, fmt::format("{}.color", name).c_str())
		, constant(program, fmt::format("{}.constant", name).c_str())
		, linear(program, fmt::format("{}.linear", name).c_str())
		, quadratic(program, fmt::format("{}.quadratic", name).c_str())
		, inner_cutoff(program, fmt::format("{}.inner_cutoff", name).c_str())
		, outer_cutoff(program, fmt::format("{}.outer_cutoff", name).c_str())
		, shadow_near_z(program, fmt::format("{}.shadow_near_z", name).c_str())
		, shadow_far_z(program, fmt::format("{}.shadow_far_z", name).c_str())
		, shadow_filter_radius(program, fmt::format("{}.shadow_filter_radius", name).c_str())
		, is_shadow_mapped(program, fmt::format("{}.is_shadow_mapped", name).c_str())
		, is_active(program, fmt::format("{}.is_active", name).c_str()) {}

	shader_uniform position;
	shader_uniform direction;
	shader_uniform color;
	shader_uniform constant;
	shader_uniform linear;
	shader_uniform quadratic;
	shader_uniform inner_cutoff;
	shader_uniform outer_cutoff;
	shader_uniform shadow_near_z;
	shader_uniform shadow_far_z;
	shader_uniform shadow_filter_radius;
	shader_uniform is_shadow_mapped;
	shader_uniform is_active;
};

#endif
