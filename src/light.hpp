#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "opengl.hpp"
#include "shader.hpp"

#include <fmt/format.h> // fmt::format
#include <string_view>  // std::string_view

struct directional_light_uniform final {
	directional_light_uniform(GLuint program, std::string_view name)
		: direction(program, fmt::format("{}.direction", name).c_str())
		, ambient(program, fmt::format("{}.ambient", name).c_str())
		, diffuse(program, fmt::format("{}.diffuse", name).c_str())
		, specular(program, fmt::format("{}.specular", name).c_str())
		, is_shadow_mapped(program, fmt::format("{}.is_shadow_mapped", name).c_str())
		, is_active(program, fmt::format("{}.is_active", name).c_str()) {}

	shader_uniform direction;
	shader_uniform ambient;
	shader_uniform diffuse;
	shader_uniform specular;
	shader_uniform is_shadow_mapped;
	shader_uniform is_active;
};

struct point_light_uniform final {
	point_light_uniform(GLuint program, std::string_view name)
		: position(program, fmt::format("{}.position", name).c_str())
		, ambient(program, fmt::format("{}.ambient", name).c_str())
		, diffuse(program, fmt::format("{}.diffuse", name).c_str())
		, specular(program, fmt::format("{}.specular", name).c_str())
		, constant(program, fmt::format("{}.constant", name).c_str())
		, linear(program, fmt::format("{}.linear", name).c_str())
		, quadratic(program, fmt::format("{}.quadratic", name).c_str())
		, shadow_near_plane(program, fmt::format("{}.shadow_near_plane", name).c_str())
		, shadow_far_plane(program, fmt::format("{}.shadow_far_plane", name).c_str())
		, is_shadow_mapped(program, fmt::format("{}.is_shadow_mapped", name).c_str())
		, is_active(program, fmt::format("{}.is_active", name).c_str()) {}

	shader_uniform position;
	shader_uniform ambient;
	shader_uniform diffuse;
	shader_uniform specular;
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
		, ambient(program, fmt::format("{}.ambient", name).c_str())
		, diffuse(program, fmt::format("{}.diffuse", name).c_str())
		, specular(program, fmt::format("{}.specular", name).c_str())
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
	shader_uniform ambient;
	shader_uniform diffuse;
	shader_uniform specular;
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
