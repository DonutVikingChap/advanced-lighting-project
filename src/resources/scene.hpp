#ifndef SCENE_HPP
#define SCENE_HPP

#include "../core/glsl.hpp"
#include "cubemap.hpp"
#include "light.hpp"
#include "model.hpp"
#include "texture.hpp"

#include <cstddef> // std::size_t
#include <memory>  // std::shared_ptr
#include <vector>  // std::vector

struct scene_object final {
	std::shared_ptr<model> model_ptr{};
	mat4 transform{1.0f};
	vec2 lightmap_offset{0.0f, 0.0f};
	vec2 lightmap_scale{1.0f, 1.0f};
};

struct scene final {
	std::shared_ptr<environment_cubemap> sky{};
	std::vector<std::shared_ptr<directional_light>> directional_lights{};
	std::vector<std::shared_ptr<point_light>> point_lights{};
	std::vector<std::shared_ptr<spot_light>> spot_lights{};
	std::vector<scene_object> objects{};
	std::shared_ptr<texture> lightmap{};
	vec2 default_lightmap_offset{0.0f, 0.0f};
	vec2 default_lightmap_scale{1.0f, 1.0f};
};

#endif
