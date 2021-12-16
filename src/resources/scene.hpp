#ifndef SCENE_HPP
#define SCENE_HPP

#include "../core/glsl.hpp"
#include "cubemap.hpp"
#include "light.hpp"
#include "lightmap.hpp"
#include "model.hpp"

#include <cstddef>                      // std::size_t
#include <glm/gtc/matrix_transform.hpp> // glm::rotate, glm::scale, glm::translate
#include <memory>                       // std::shared_ptr
#include <vector>                       // std::vector

struct scene_object_options final {
	std::shared_ptr<model> model_ptr{};
	vec3 position{};
	vec3 scale{};
	float angle = 0.0f;
	vec3 axis{0.0f, 1.0f, 0.0f};
};

struct scene_object final {
	explicit scene_object(const scene_object_options& options)
		: model_ptr(options.model_ptr)
		, transform(glm::rotate(glm::scale(glm::translate(mat4{1.0f}, options.position), options.scale), options.angle, options.axis)) {}

	std::shared_ptr<model> model_ptr;
	mat4 transform;
	vec2 lightmap_offset{0.0f, 0.0f};
	vec2 lightmap_scale{1.0f, 1.0f};
};

struct scene final {
	std::shared_ptr<environment_cubemap> sky{};
	std::vector<std::shared_ptr<directional_light>> directional_lights{};
	std::vector<std::shared_ptr<point_light>> point_lights{};
	std::vector<std::shared_ptr<spot_light>> spot_lights{};
	std::vector<scene_object> objects{};
	std::shared_ptr<lightmap_texture> lightmap{};
	vec2 default_lightmap_offset{0.0f, 0.0f};
	vec2 default_lightmap_scale{1.0f, 1.0f};
};

#endif
