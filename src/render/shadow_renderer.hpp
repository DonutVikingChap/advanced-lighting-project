#ifndef SHADOW_RENDERER_HPP
#define SHADOW_RENDERER_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/framebuffer.hpp"
#include "../resources/light.hpp"
#include "../resources/model.hpp"
#include "../resources/shader.hpp"

#include <cstddef>              // std::size_t
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <memory>               // std::shared_ptr
#include <span>                 // std::span
#include <unordered_map>        // std::unordered_map
#include <utility>              // std::move
#include <vector>               // std::vector

class shadow_renderer final {
public:
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
		m_model_instances[std::move(model)].emplace_back(transform);
	}

	auto render(const mat4& view_matrix) -> void {
		glEnable(GL_POLYGON_OFFSET_FILL);

		glUseProgram(m_shadow_shader.program.get());

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo.get());

		for (auto& light_ptr : m_directional_lights) {
			auto& light = *light_ptr;
			glPolygonOffset(light.shadow_offset_factor, light.shadow_offset_units);

			auto projection_view_matrices = std::array<mat4, directional_light::csm_cascade_count>{};
			// TODO: Calculate projection view matrices.

			// TODO: CSM
			(void)view_matrix;

			for (auto cascade_level = std::size_t{0}; cascade_level < directional_light::csm_cascade_count; ++cascade_level) {
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, light.shadow_map.get(), 0, static_cast<GLint>(cascade_level));
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);

				glViewport(0, 0, static_cast<GLsizei>(light.shadow_map.width()), static_cast<GLsizei>(light.shadow_map.height()));
				glClear(GL_DEPTH_BUFFER_BIT);

				const auto& projection_view_matrix = projection_view_matrices[cascade_level];
				glUniformMatrix4fv(m_shadow_shader.projection_view_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_view_matrix));
				for (const auto& [model, instances] : m_model_instances) {
					for (const auto& mesh : model->meshes()) {
						glBindVertexArray(mesh.get());
						for (const auto& instance : instances) {
							const auto& model_matrix = instance.transform;
							glUniformMatrix4fv(m_shadow_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(model_matrix));
							glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.indices().size()), model_mesh::index_type, nullptr);
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
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);

				glViewport(0, 0, static_cast<GLsizei>(light.shadow_map.width()), static_cast<GLsizei>(light.shadow_map.height()));
				glClear(GL_DEPTH_BUFFER_BIT);

				const auto& projection_view_matrix = light.shadow_projection_view_matrices[i];
				glUniformMatrix4fv(m_shadow_shader.projection_view_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_view_matrix));
				for (const auto& [model, instances] : m_model_instances) {
					for (const auto& mesh : model->meshes()) {
						glBindVertexArray(mesh.get());
						for (const auto& instance : instances) {
							const auto& model_matrix = instance.transform;
							glUniformMatrix4fv(m_shadow_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(model_matrix));
							glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.indices().size()), model_mesh::index_type, nullptr);
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
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);

			glViewport(0, 0, static_cast<GLsizei>(light.shadow_map.width()), static_cast<GLsizei>(light.shadow_map.height()));
			glClear(GL_DEPTH_BUFFER_BIT);

			const auto& projection_view_matrix = light.shadow_projection_view_matrix;
			glUniformMatrix4fv(m_shadow_shader.projection_view_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_view_matrix));
			for (const auto& [model, instances] : m_model_instances) {
				for (const auto& mesh : model->meshes()) {
					glBindVertexArray(mesh.get());
					for (const auto& instance : instances) {
						const auto& model_matrix = instance.transform;
						glUniformMatrix4fv(m_shadow_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(model_matrix));
						glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.indices().size()), model_mesh::index_type, nullptr);
					}
				}
			}
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glPolygonOffset(0.0f, 0.0f);
		glDisable(GL_POLYGON_OFFSET_FILL);

		m_model_instances.clear();
		m_directional_lights.clear();
		m_point_lights.clear();
		m_spot_lights.clear();
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
};

#endif
