#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "../core/opengl.hpp"
#include "shader.hpp"
#include "texture.hpp"

#include <array>        // std::array
#include <cstddef>      // std::size_t
#include <fmt/format.h> // fmt::format
#include <memory>       // std::shared_ptr
#include <string_view>  // std::string_view

struct directional_light final {
	static constexpr auto csm_cascade_count = std::size_t{4};

	vec3 direction{};
	vec3 color{};
	std::array<mat4, csm_cascade_count> shadow_matrices{};
	std::array<float, csm_cascade_count> shadow_uv_sizes{};
	std::array<float, csm_cascade_count> shadow_near_planes{};
	std::shared_ptr<texture> shadow_map{};
	std::shared_ptr<texture> depth_map{};
};

struct point_light final {
	vec3 position{};
	vec3 color{};
	float constant = 0.0f;
	float linear = 0.0f;
	float quadratic = 0.0f;
	float shadow_near_plane = 0.0f;
	float shadow_far_plane = 0.0f;
	std::shared_ptr<texture> shadow_map{};
};

struct spot_light final {
	vec3 position{};
	vec3 direction{};
	vec3 color{};
	float constant = 0.0f;
	float linear = 0.0f;
	float quadratic = 0.0f;
	float inner_cutoff = 0.0f;
	float outer_cutoff = 0.0f;
	float shadow_near_plane = 0.0f;
	float shadow_far_plane = 0.0f;
	mat4 shadow_matrix{};
	std::shared_ptr<texture> shadow_map{};
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
		, shadow_near_plane(program, fmt::format("{}.shadow_near_plane", name).c_str())
		, shadow_far_plane(program, fmt::format("{}.shadow_far_plane", name).c_str())
		, is_shadow_mapped(program, fmt::format("{}.is_shadow_mapped", name).c_str())
		, is_active(program, fmt::format("{}.is_active", name).c_str()) {}

	shader_uniform position;
	shader_uniform color;
	shader_uniform constant;
	shader_uniform linear;
	shader_uniform quadratic;
	shader_uniform shadow_near_plane;
	shader_uniform shadow_far_plane;
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
		, shadow_near_plane(program, fmt::format("{}.shadow_near_plane", name).c_str())
		, shadow_far_plane(program, fmt::format("{}.shadow_far_plane", name).c_str())
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
	shader_uniform shadow_near_plane;
	shader_uniform shadow_far_plane;
	shader_uniform is_shadow_mapped;
	shader_uniform is_active;
};

#endif
