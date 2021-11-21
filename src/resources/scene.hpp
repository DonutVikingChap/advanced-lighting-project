#ifndef SCENE_HPP
#define SCENE_HPP

#include "../core/glsl.hpp"
#include "cubemap.hpp"
#include "light.hpp"
#include "model.hpp"

#include <cstddef>     // std::size_t
#include <glm/glm.hpp> // glm::identity
#include <memory>      // std::shared_ptr
#include <vector>      // std::vector

struct scene_object final {
	std::shared_ptr<model> model_ptr{};
	mat4 transform = glm::identity<mat4>();
	vec2 lightmap_offset{};
	vec2 lightmap_scale{};
};

struct scene final {
	std::shared_ptr<environment_cubemap> sky{};
	std::vector<directional_light> directional_lights{};
	std::vector<point_light> point_lights{};
	std::vector<spot_light> spot_lights{};
	std::vector<scene_object> objects{};
	vec2 default_lightmap_offset{};
	vec2 default_lightmap_scale{};
};

#endif
