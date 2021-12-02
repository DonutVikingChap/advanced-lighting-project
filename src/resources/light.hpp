#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "shader.hpp"
#include "texture.hpp"

#include <array>        // std::array
#include <cstddef>      // std::size_t
#include <fmt/format.h> // fmt::format
#include <glm/glm.hpp>  // glm::perspective, glm::lookAt
#include <string_view>  // std::string_view

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
	float shadow_offset_factor = 2.0f;
	float shadow_offset_units = 128.0f;
	std::size_t shadow_resolution = 2048;
	bool is_shadow_mapped = true;
};

struct directional_light final {
	static constexpr auto csm_cascade_count = std::size_t{4};

	explicit directional_light(const directional_light_options& options)
		: direction(options.direction)
		, color(options.color)
		, shadow_offset_factor(options.shadow_offset_factor)
		, shadow_offset_units(options.shadow_offset_units) {
		if (options.is_shadow_mapped) {
			// TODO: Create shadow map texture/depth map texture/depth sampler.
		}
	}

	vec3 direction;
	vec3 color;
	float shadow_offset_factor;
	float shadow_offset_units;
	std::array<mat4, csm_cascade_count> shadow_matrices{};
	std::array<float, csm_cascade_count> shadow_uv_sizes{};
	std::array<float, csm_cascade_count> shadow_near_planes{};
	texture shadow_map = texture::null();
	texture depth_map = texture::null();
	sampler depth_sampler = sampler::null();
};

struct point_light_options final {
	vec3 position{0.0f, 0.0f, 0.0f};
	vec3 color{1.0f, 1.0f, 1.0f};
	float constant = 1.0f;
	float linear = 0.045f;
	float quadratic = 0.0075f;
	float shadow_near_z = 0.01f;
	float shadow_far_z = 100.0f;
	float shadow_offset_factor = 2.0f;
	float shadow_offset_units = 128.0f;
	std::size_t shadow_resolution = 1024;
	bool is_shadow_mapped = true;
};

struct point_light final {
	explicit point_light(const point_light_options& options)
		: position(options.position)
		, color(options.color)
		, constant(options.constant)
		, linear(options.linear)
		, quadratic(options.quadratic)
		, shadow_near_z(options.shadow_near_z)
		, shadow_far_z(options.shadow_far_z)
		, shadow_offset_factor(options.shadow_offset_factor)
		, shadow_offset_units(options.shadow_offset_units) {
		if (options.is_shadow_mapped) {
			shadow_map = texture::create_cubemap_uninitialized(GL_DEPTH_COMPONENT,
				options.shadow_resolution,
				texture_options{
					.max_anisotropy = 1.0f,
					.repeat = false,
					.use_linear_filtering = true,
					.use_mip_map = false,
					.use_compare_mode = true,
				});
			update_shadow_transform();
		}
	}

	auto update_shadow_transform() -> void {
		const auto projection_matrix = glm::perspective(radians(90.0f), 1.0f, shadow_near_z, shadow_far_z);
		shadow_projection_view_matrices = {
			projection_matrix * glm::lookAt(position, position + vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f}),
			projection_matrix * glm::lookAt(position, position + vec3{-1.0f, 0.0f, 0.0f}, vec3{0.0f, -1.0f, 0.0f}),
			projection_matrix * glm::lookAt(position, position + vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.0f, 1.0f}),
			projection_matrix * glm::lookAt(position, position + vec3{0.0f, -1.0f, 0.0f}, vec3{0.0f, 0.0f, 1.0f}),
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
	float shadow_offset_factor = 2.0f;
	float shadow_offset_units = 128.0f;
	std::size_t shadow_resolution = 1024;
	bool is_shadow_mapped = true;
};

struct spot_light final {
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
		, shadow_offset_units(options.shadow_offset_units) {
		if (options.is_shadow_mapped) {
			shadow_map = texture::create_2d_uninitialized(GL_DEPTH_COMPONENT,
				options.shadow_resolution,
				options.shadow_resolution,
				texture_options{
					.max_anisotropy = 1.0f,
					.repeat = false,
					.use_linear_filtering = true,
					.use_mip_map = false,
					.use_compare_mode = true,
				});
			update_shadow_transform();
		}
	}

	auto update_shadow_transform() -> void {
		const auto projection_matrix = glm::perspective(acos(outer_cutoff), 1.0f, shadow_near_z, shadow_far_z);
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
		, is_shadow_mapped(program, fmt::format("{}.is_shadow_mapped", name).c_str())
		, is_active(program, fmt::format("{}.is_active", name).c_str()) {}

	shader_uniform position;
	shader_uniform color;
	shader_uniform constant;
	shader_uniform linear;
	shader_uniform quadratic;
	shader_uniform shadow_near_z;
	shader_uniform shadow_far_z;
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
	shader_uniform is_shadow_mapped;
	shader_uniform is_active;
};

#endif
