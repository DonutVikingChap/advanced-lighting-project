#ifndef SHADOW_RENDERER_HPP
#define SHADOW_RENDERER_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/camera.hpp"
#include "../resources/framebuffer.hpp"
#include "../resources/light.hpp"
#include "../resources/model.hpp"
#include "../resources/shader.hpp"

#include <cstddef>                      // std::size_t
#include <glm/gtc/matrix_transform.hpp> // glm::ortho
#include <glm/gtc/type_ptr.hpp>         // glm::value_ptr
#include <limits>                       // std::numeric_limits
#include <memory>                       // std::shared_ptr
#include <span>                         // std::span
#include <unordered_map>                // std::unordered_map
#include <utility>                      // std::move
#include <vector>                       // std::vector

class shadow_renderer final {
public:
	shadow_renderer() {
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo.get());
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	auto draw_directional_light(std::shared_ptr<directional_light> light) -> void {
		if (light->shadow_map) {
			m_directional_lights.push_back(std::move(light));
		}
	}

	auto draw_point_light(std::shared_ptr<point_light> light) -> void {
		if (light->shadow_map) {
			m_point_lights.push_back(std::move(light));
		}
	}

	auto draw_spot_light(std::shared_ptr<spot_light> light) -> void {
		if (light->shadow_map) {
			m_spot_lights.push_back(std::move(light));
		}
	}

	auto draw_model(std::shared_ptr<model> model, const mat4& transform) -> void {
		const auto position = vec3{transform[3]};
		const auto scale = vec3{transform[0][0], transform[1][1], transform[2][2]};
		const auto extents = vec3{model->bounding_sphere_radius()} * scale;
		m_world_aabb_min = min(m_world_aabb_min, position - extents);
		m_world_aabb_max = max(m_world_aabb_max, position + extents);
		m_model_instances[std::move(model)].emplace_back(transform);
	}

	auto render(const camera& camera) -> void {
		glEnable(GL_POLYGON_OFFSET_FILL);

		glUseProgram(m_shadow_shader.program.get());

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo.get());

		const auto inverse_view_matrix = inverse(camera.view_matrix);
		const auto world_aabb_corners = std::array<vec3, 8>{
			vec3{m_world_aabb_min.x, m_world_aabb_min.y, m_world_aabb_min.z},
			vec3{m_world_aabb_min.x, m_world_aabb_min.y, m_world_aabb_max.z},
			vec3{m_world_aabb_min.x, m_world_aabb_max.y, m_world_aabb_min.z},
			vec3{m_world_aabb_min.x, m_world_aabb_max.y, m_world_aabb_max.z},
			vec3{m_world_aabb_max.x, m_world_aabb_min.y, m_world_aabb_min.z},
			vec3{m_world_aabb_max.x, m_world_aabb_min.y, m_world_aabb_max.z},
			vec3{m_world_aabb_max.x, m_world_aabb_max.y, m_world_aabb_min.z},
			vec3{m_world_aabb_max.x, m_world_aabb_max.y, m_world_aabb_max.z},
		};

		for (auto& light_ptr : m_directional_lights) {
			auto& light = *light_ptr;
			glPolygonOffset(light.shadow_offset_factor, light.shadow_offset_units);

			const auto shadow_map_texel_size = 1.0f / vec2{static_cast<float>(light.shadow_map.width()), static_cast<float>(light.shadow_map.height())};

			for (auto cascade_level = std::size_t{0}; cascade_level < camera_cascade_count; ++cascade_level) {
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, light.shadow_map.get(), 0, static_cast<GLint>(cascade_level));

				glViewport(0, 0, static_cast<GLsizei>(light.shadow_map.width()), static_cast<GLsizei>(light.shadow_map.height()));
				glClear(GL_DEPTH_BUFFER_BIT);

				auto z_min = std::numeric_limits<float>::max();
				auto z_max = -std::numeric_limits<float>::max();

				// Fit shadow near/far plane to world AABB.
				for (const auto& world_aabb_corner : world_aabb_corners) {
					const auto z = (light.shadow_view_matrix * vec4{world_aabb_corner, 1.0f}).z;
					z_min = min(z_min, z);
					z_max = max(z_max, z);
				}

				// Get view frustum in world space.
				const auto view_frustum_corners = std::array<vec3, 8>{
					vec3{inverse_view_matrix * vec4{camera.cascade_frustum_corners[cascade_level][0], 1.0f}},
					vec3{inverse_view_matrix * vec4{camera.cascade_frustum_corners[cascade_level][1], 1.0f}},
					vec3{inverse_view_matrix * vec4{camera.cascade_frustum_corners[cascade_level][2], 1.0f}},
					vec3{inverse_view_matrix * vec4{camera.cascade_frustum_corners[cascade_level][3], 1.0f}},
					vec3{inverse_view_matrix * vec4{camera.cascade_frustum_corners[cascade_level][4], 1.0f}},
					vec3{inverse_view_matrix * vec4{camera.cascade_frustum_corners[cascade_level][5], 1.0f}},
					vec3{inverse_view_matrix * vec4{camera.cascade_frustum_corners[cascade_level][6], 1.0f}},
					vec3{inverse_view_matrix * vec4{camera.cascade_frustum_corners[cascade_level][7], 1.0f}},
				};
				const auto view_frustum_diagonal_length = length(view_frustum_corners[4] - view_frustum_corners[2]);

				auto area_min = vec2{std::numeric_limits<float>::max()};
				auto area_max = vec2{-std::numeric_limits<float>::max()};

				// Fit shadow area to view frustum.
				for (const auto& view_frustum_corner : view_frustum_corners) {
					const auto xy = vec2{light.shadow_view_matrix * vec4{view_frustum_corner, 1.0f}};
					area_min = min(area_min, xy);
					area_max = max(area_max, xy);
				}

				// Pad shadow area.
				const auto padding = (vec2{view_frustum_diagonal_length} - (area_max - area_min)) * 0.5f;
				area_min -= padding;
				area_max += padding;

				// Snap the camera to 1 pixel increments so that moving the camera does not cause shadows to jitter.
				const auto world_units_per_texel = shadow_map_texel_size * view_frustum_diagonal_length;
				area_min = floor(area_min / world_units_per_texel) * world_units_per_texel;
				area_max = floor(area_max / world_units_per_texel) * world_units_per_texel;

				// Calculate projection matrix.
				const auto shadow_projection_matrix = glm::ortho(area_min.x, area_max.x, area_min.y, area_max.y, z_min, z_max);

				// Calculate projection-view matrix.
				const auto shadow_projection_view_matrix = shadow_projection_matrix * light.shadow_view_matrix;

				// Set shadow variables for model rendering.
				light.shadow_matrices[cascade_level] = light_depth_conversion_matrix * shadow_projection_view_matrix;
				light.shadow_uv_sizes[cascade_level] = light.shadow_light_size / length(area_max - area_min);
				light.shadow_near_planes[cascade_level] = light.shadow_near_plane;

				glUniformMatrix4fv(m_shadow_shader.projection_view_matrix.location(), 1, GL_FALSE, glm::value_ptr(shadow_projection_view_matrix));
				for (const auto& [model, instances] : m_model_instances) {
					for (const auto& mesh : model->meshes()) {
						const auto& material = mesh.material();
						if (!material.alpha_blending) {
							glBindVertexArray(mesh.get());
							for (const auto& instance : instances) {
								glUniformMatrix4fv(m_shadow_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(instance.transform));
								glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.indices().size()), model_mesh::index_type, nullptr);
							}
						}
					}
				}
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0, static_cast<GLint>(cascade_level));
			}
		}

		for (auto& light_ptr : m_point_lights) {
			auto& light = *light_ptr;
			glPolygonOffset(light.shadow_offset_factor, light.shadow_offset_units);

			for (auto i = std::size_t{0}; i < light.shadow_projection_view_matrices.size(); ++i) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), light.shadow_map.get(), 0);

				glViewport(0, 0, static_cast<GLsizei>(light.shadow_map.width()), static_cast<GLsizei>(light.shadow_map.height()));
				glClear(GL_DEPTH_BUFFER_BIT);

				const auto& shadow_projection_view_matrix = light.shadow_projection_view_matrices[i];
				glUniformMatrix4fv(m_shadow_shader.projection_view_matrix.location(), 1, GL_FALSE, glm::value_ptr(shadow_projection_view_matrix));
				for (const auto& [model, instances] : m_model_instances) {
					for (const auto& mesh : model->meshes()) {
						const auto& material = mesh.material();
						if (!material.alpha_blending) {
							glBindVertexArray(mesh.get());
							for (const auto& instance : instances) {
								glUniformMatrix4fv(m_shadow_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(instance.transform));
								glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.indices().size()), model_mesh::index_type, nullptr);
							}
						}
					}
				}
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), 0, 0);
			}
		}

		for (auto& light_ptr : m_spot_lights) {
			auto& light = *light_ptr;
			glPolygonOffset(light.shadow_offset_factor, light.shadow_offset_units);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, light.shadow_map.get(), 0);

			glViewport(0, 0, static_cast<GLsizei>(light.shadow_map.width()), static_cast<GLsizei>(light.shadow_map.height()));
			glClear(GL_DEPTH_BUFFER_BIT);

			const auto& shadow_projection_view_matrix = light.shadow_projection_view_matrix;
			glUniformMatrix4fv(m_shadow_shader.projection_view_matrix.location(), 1, GL_FALSE, glm::value_ptr(shadow_projection_view_matrix));
			for (const auto& [model, instances] : m_model_instances) {
				for (const auto& mesh : model->meshes()) {
					const auto& material = mesh.material();
					if (!material.alpha_blending) {
						glBindVertexArray(mesh.get());
						for (const auto& instance : instances) {
							glUniformMatrix4fv(m_shadow_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(instance.transform));
							glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.indices().size()), model_mesh::index_type, nullptr);
						}
					}
				}
			}
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
		}

		glPolygonOffset(0.0f, 0.0f);
		glDisable(GL_POLYGON_OFFSET_FILL);

		m_model_instances.clear();
		m_directional_lights.clear();
		m_point_lights.clear();
		m_spot_lights.clear();
		m_world_aabb_min = vec3{std::numeric_limits<float>::max()};
		m_world_aabb_max = vec3{-std::numeric_limits<float>::max()};
	}

	auto reload_shaders() -> void {
		m_shadow_shader = shadow_shader{};
	}

private:
	struct shadow_shader final {
		shader_program program{{
			.vertex_shader_filename = "assets/shaders/shadow.vert",
			.fragment_shader_filename = "assets/shaders/shadow.frag",
		}};
		shader_uniform projection_view_matrix{program.get(), "projection_view_matrix"};
		shader_uniform model_matrix{program.get(), "model_matrix"};
	};

	struct model_instance final {
		explicit model_instance(const mat4& transform) noexcept
			: transform(transform) {}

		mat4 transform;
	};

	using model_instance_map = std::unordered_map<std::shared_ptr<model>, std::vector<model_instance>>;

	shadow_shader m_shadow_shader{};
	framebuffer m_fbo{};
	model_instance_map m_model_instances{};
	std::vector<std::shared_ptr<directional_light>> m_directional_lights{};
	std::vector<std::shared_ptr<point_light>> m_point_lights{};
	std::vector<std::shared_ptr<spot_light>> m_spot_lights{};
	vec3 m_world_aabb_min{std::numeric_limits<float>::max()};
	vec3 m_world_aabb_max{-std::numeric_limits<float>::max()};
};

#endif
