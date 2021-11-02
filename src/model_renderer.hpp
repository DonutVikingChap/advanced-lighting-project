#ifndef MODEL_RENDERER_HPP
#define MODEL_RENDERER_HPP

#include "glsl.hpp"
#include "light.hpp"
#include "model.hpp"
#include "opengl.hpp"
#include "passkey.hpp"
#include "shader.hpp"

#include <cstddef>                    // std::size_t
#include <glm/glm.hpp>                // glm::perspective
#include <glm/gtc/matrix_inverse.hpp> // glm::inverseTranspose
#include <glm/gtc/type_ptr.hpp>       // glm::value_ptr
#include <memory>                     // std::shared_ptr
#include <span>                       // std::span
#include <unordered_map>              // std::unordered_map
#include <utility>                    // std::move
#include <vector>                     // std::vector

class renderer;

class model_renderer final {
public:
	static constexpr auto directional_light_count = std::size_t{1};
	static constexpr auto point_light_count = std::size_t{2};
	static constexpr auto spot_light_count = std::size_t{1};
	static constexpr auto csm_cascade_count = std::size_t{8};

	auto resize(passkey<renderer>, int width, int height, float vertical_fov, float near_z, float far_z) -> void { // NOLINT(readability-make-member-function-const)
		const auto aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
		const auto projection_matrix = glm::perspective(vertical_fov, aspect_ratio, near_z, far_z);
		glUseProgram(m_model_shader.program.get());
		glUniformMatrix4fv(m_model_shader.projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
	}

#if 0
	auto draw_directional_light(vec3 direction, vec3 ambient, vec3 diffuse, vec3 specular, std::span<const mat4, csm_cascade_count> shadow_matrices,
		std::span<const float, csm_cascade_count> shadow_uv_sizes, std::span<const float, csm_cascade_count> shadow_near_planes, std::shared_ptr<texture> shadow_map,
		std::shared_ptr<texture> depth_map) -> void {
		// TODO
	}

	auto draw_point_light(vec3 position, vec3 ambient, vec3 diffuse, vec3 specular, float constant, float linear, float quadratic, float shadow_near_plane, float shadow_far_plane,
		std::shared_ptr<texture> shadow_map) -> void {
		// TODO
	}

	auto draw_spot_light(vec3 position, vec3 direction, vec3 ambient, vec3 diffuse, vec3 specular, float constant, float linear, float quadratic, float inner_cutoff,
		float outer_cutoff, float shadow_near_plane, float shadow_far_plane, const mat4& shadow_matrix, std::shared_ptr<texture> shadow_map) -> void {
		// TODO
	}
#endif

	auto draw_model(std::shared_ptr<textured_model> model, const mat4& transform) -> void {
		m_model_instances[std::move(model)].emplace_back(transform);
	}

	auto render(passkey<renderer>, const mat4& view_matrix, vec3 view_position) -> void {
		glUseProgram(m_model_shader.program.get());
		glUniformMatrix4fv(m_model_shader.view_matrix.location(), 1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniform3fv(m_model_shader.view_position.location(), 1, glm::value_ptr(view_position));

		for (const auto& [model, instances] : m_model_instances) {
			auto i = 0;
			for (const auto& texture : model->textures()) {
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, texture->get());
				++i;
			}
			for (const auto& mesh : model->meshes()) {
				glBindVertexArray(mesh.get());
				const auto& material = mesh.material();
				glUniform1i(m_model_shader.material_diffuse.location(), static_cast<GLint>(material.diffuse_texture_offset));
				glUniform1i(m_model_shader.material_specular.location(), static_cast<GLint>(material.specular_texture_offset));
				glUniform1i(m_model_shader.material_normal.location(), static_cast<GLint>(material.normal_texture_offset));
				glUniform1f(m_model_shader.material_shininess.location(), material.shininess);
				for (const auto& instance : instances) {
					const auto& model_matrix = instance.transform;
					const auto normal_matrix = glm::inverseTranspose(mat3{model_matrix});
					glUniformMatrix4fv(m_model_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(model_matrix));
					glUniformMatrix3fv(m_model_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(normal_matrix));
					glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.index_count()), GL_UNSIGNED_INT, nullptr);
				}
			}
		}
		m_model_instances.clear();
	}

private:
	struct model_shader final {
		shader_program program{"assets/shaders/phong.vert", "assets/shaders/phong.frag"};
		shader_uniform projection_matrix{program.get(), "projection_matrix"};
		shader_uniform view_matrix{program.get(), "view_matrix"};
		shader_uniform model_matrix{program.get(), "model_matrix"};
		shader_uniform normal_matrix{program.get(), "normal_matrix"};
		shader_uniform view_position{program.get(), "view_position"};
		shader_uniform material_diffuse{program.get(), "material_diffuse"};
		shader_uniform material_specular{program.get(), "material_specular"};
		shader_uniform material_normal{program.get(), "material_normal"};
		shader_uniform material_shininess{program.get(), "material_shininess"};
		shader_array<directional_light_uniform, directional_light_count> directional_lights{program.get(), "directional_lights"};
		shader_array<point_light_uniform, point_light_count> point_lights{program.get(), "point_lights"};
		shader_array<spot_light_uniform, spot_light_count> spot_lights{program.get(), "spot_lights"};
		shader_array<shader_uniform, directional_light_count> directional_shadow_maps{program.get(), "directional_shadow_maps"};
		shader_array<shader_uniform, directional_light_count> directional_depth_maps{program.get(), "directional_depth_maps"};
		shader_array<shader_uniform, directional_light_count * csm_cascade_count> directional_shadow_matrices{program.get(), "directional_shadow_matrices"};
		shader_array<shader_uniform, directional_light_count * csm_cascade_count> directional_shadow_uv_sizes{program.get(), "directional_shadow_uv_sizes"};
		shader_array<shader_uniform, directional_light_count * csm_cascade_count> directional_shadow_near_planes{program.get(), "directional_shadow_near_planes"};
		shader_array<shader_uniform, point_light_count> point_shadow_maps{program.get(), "point_shadow_maps"};
		shader_array<shader_uniform, spot_light_count> spot_shadow_maps{program.get(), "spot_shadow_maps"};
		shader_array<shader_uniform, spot_light_count> spot_shadow_matrices{program.get(), "spot_shadow_matrices"};
		shader_array<shader_uniform, csm_cascade_count> cascade_levels_frustrum_depths{program.get(), "cascade_levels_frustrum_depths"};
	};

	struct model_instance final {
		explicit model_instance(const mat4& transform) noexcept
			: transform(transform) {}

		mat4 transform;
	};

	using model_instance_map = std::unordered_map<std::shared_ptr<textured_model>, std::vector<model_instance>>;

	model_shader m_model_shader{};
	model_instance_map m_model_instances{};
};

#endif
