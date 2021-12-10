#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "../core/glsl.hpp"

#include <array>                        // std::array
#include <cstddef>                      // std::size_t
#include <glm/gtc/matrix_transform.hpp> // glm::perspective, glm::lookAt

static constexpr auto camera_cascade_count = std::size_t{4};

struct camera_options final {
	float vertical_fov = radians(90.0f);
	float aspect_ratio = 1.0f;
	float near_z = 0.01f;
	float far_z = 1000.0f;
	std::array<float, camera_cascade_count> cascade_levels{
		0.004f,
		0.013f,
		0.035f,
		0.1f,
	};
};

struct camera final {
	camera(vec3 position, vec3 direction, vec3 up, const camera_options& options)
		: position(position)
		, direction(direction)
		, up(up)
		, vertical_fov(options.vertical_fov)
		, aspect_ratio(options.aspect_ratio)
		, near_z(options.near_z)
		, far_z(options.far_z)
		, cascade_levels(options.cascade_levels) {
		update_projection();
		update_view();
	}

	auto update_projection() -> void {
		projection_matrix = glm::perspective(vertical_fov, aspect_ratio, near_z, far_z);
		update_cascade_frustums();
	}

	auto update_cascade_frustums() -> void {
		const auto inverse_projection_matrix = inverse(projection_matrix);

		const auto frustum_length = far_z - near_z;
		const auto frustum_left = inverse_projection_matrix * vec4{-1.0f, 0.0f, -1.0f, 1.0f};
		const auto frustum_right = inverse_projection_matrix * vec4{1.0f, 0.0f, -1.0f, 1.0f};
		const auto frustum_bottom = inverse_projection_matrix * vec4{0.0f, -1.0f, -1.0f, 1.0f};
		const auto frustum_top = inverse_projection_matrix * vec4{0.0f, 1.0f, -1.0f, 1.0f};

		const auto frustum_left_slope = frustum_left.x / frustum_left.z;
		const auto frustum_right_slope = frustum_right.x / frustum_right.z;
		const auto frustum_bottom_slope = frustum_bottom.y / frustum_bottom.z;
		const auto frustum_top_slope = frustum_top.y / frustum_top.z;

		for (auto cascade_level = std::size_t{0}; cascade_level < camera_cascade_count; ++cascade_level) {
			cascade_frustum_depths[cascade_level] = -(cascade_levels[cascade_level] * frustum_length);

			const auto near_z = 0.0f;
			const auto far_z = cascade_frustum_depths[cascade_level] * 2.0f;

			const auto near_left = frustum_left_slope * near_z;
			const auto near_right = frustum_right_slope * near_z;
			const auto near_bottom = frustum_bottom_slope * near_z;
			const auto near_top = frustum_top_slope * near_z;

			const auto far_left = frustum_left_slope * far_z;
			const auto far_right = frustum_right_slope * far_z;
			const auto far_bottom = frustum_bottom_slope * far_z;
			const auto far_top = frustum_top_slope * far_z;

			cascade_frustum_corners[cascade_level] = {
				vec3{near_right, near_top, near_z},
				vec3{near_left, near_top, near_z},
				vec3{near_left, near_bottom, near_z},
				vec3{near_right, near_bottom, near_z},
				vec3{far_right, far_top, far_z},
				vec3{far_left, far_top, far_z},
				vec3{far_left, far_bottom, far_z},
				vec3{far_right, far_bottom, far_z},
			};
		}
	}

	auto update_view() -> void {
		view_matrix = glm::lookAt(position, position + direction, up);
	}

	vec3 position;
	vec3 direction;
	vec3 up;
	float vertical_fov;
	float aspect_ratio;
	float near_z;
	float far_z;
	std::array<float, camera_cascade_count> cascade_levels;
	std::array<std::array<vec3, 8>, camera_cascade_count> cascade_frustum_corners{};
	std::array<float, camera_cascade_count> cascade_frustum_depths{};
	mat4 projection_matrix{};
	mat4 view_matrix{};
};

#endif
