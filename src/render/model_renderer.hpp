#ifndef MODEL_RENDERER_HPP
#define MODEL_RENDERER_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/cubemap.hpp"
#include "../resources/light.hpp"
#include "../resources/model.hpp"
#include "../resources/shader.hpp"
#include "../utilities/passkey.hpp"

#include <array>                      // std::array
#include <cstddef>                    // std::size_t
#include <glm/glm.hpp>                // glm::perspective
#include <glm/gtc/matrix_inverse.hpp> // glm::inverseTranspose
#include <glm/gtc/type_ptr.hpp>       // glm::value_ptr
#include <memory>                     // std::shared_ptr
#include <span>                       // std::span
#include <unordered_map>              // std::unordered_map
#include <utility>                    // std::move
#include <vector>                     // std::vector

class rendering_pipeline;

class model_renderer final {
public:
	static constexpr auto gamma = 2.2f;
	static constexpr auto directional_light_count = std::size_t{1};
	static constexpr auto point_light_count = std::size_t{4};
	static constexpr auto spot_light_count = std::size_t{4};

	static constexpr auto reserved_texture_units_begin = GLint{0};
	static constexpr auto cubemap_texture_unit = GLint{reserved_texture_units_begin};
	static constexpr auto directional_light_texture_units_begin = GLint{cubemap_texture_unit + 1};
	static constexpr auto point_light_texture_units_begin = GLint{directional_light_texture_units_begin + directional_light_count * 2};
	static constexpr auto spot_light_texture_units_begin = GLint{point_light_texture_units_begin + point_light_count};
	static constexpr auto reserved_texture_units_end = GLint{spot_light_texture_units_begin + spot_light_count};

	auto resize(passkey<rendering_pipeline>, int width, int height, float vertical_fov, float near_z, float far_z) -> void {
		m_model_shader.resize(width, height, vertical_fov, near_z, far_z);
	}

	auto draw_cubemap(std::shared_ptr<cubemap> cubemap) -> void {
		m_cubemap = std::move(cubemap);
	}

	auto draw_directional_light(const directional_light& light) -> void {
		m_directional_lights.push_back(light);
	}

	auto draw_point_light(const point_light& light) -> void {
		m_point_lights.push_back(light);
	}

	auto draw_spot_light(const spot_light& light) -> void {
		m_spot_lights.push_back(light);
	}

	auto draw_model(std::shared_ptr<textured_model> model, const mat4& transform) -> void {
		m_model_instances[std::move(model)].emplace_back(transform);
	}

	auto render(passkey<rendering_pipeline>, const mat4& view_matrix, vec3 view_position) -> void {
		glUseProgram(m_model_shader.program.get());
		glUniformMatrix4fv(m_model_shader.view_matrix.location(), 1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniform3fv(m_model_shader.view_position.location(), 1, glm::value_ptr(view_position));

		// Upload cubemap.
		if (m_cubemap) {
			glActiveTexture(GL_TEXTURE0 + cubemap_texture_unit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap->get());
			glUniform1i(m_model_shader.cubemap_texture.location(), cubemap_texture_unit);
			m_cubemap.reset();
		}

		// Upload directional lights.
		auto light_index = std::size_t{0};
		for (const auto& light : m_directional_lights) {
			if (light_index >= directional_light_count) {
				break;
			}
			auto& light_uniform = m_model_shader.directional_lights[light_index];
			glUniform3fv(light_uniform.direction.location(), 1, glm::value_ptr(light.direction));
			glUniform3fv(light_uniform.ambient.location(), 1, glm::value_ptr(light.ambient));
			glUniform3fv(light_uniform.color.location(), 1, glm::value_ptr(light.color));
			if (light.shadow_map && light.depth_map) {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_TRUE);
				const auto shadow_map_texture_unit = directional_light_texture_units_begin + static_cast<GLint>(light_index) * GLint{2};
				const auto depth_map_texture_unit = shadow_map_texture_unit + GLint{1};
				glActiveTexture(GL_TEXTURE0 + shadow_map_texture_unit);
				glBindTexture(GL_TEXTURE_2D_ARRAY, light.shadow_map->get());
				glUniform1i(m_model_shader.directional_shadow_maps[light_index].location(), shadow_map_texture_unit);
				glActiveTexture(GL_TEXTURE0 + depth_map_texture_unit);
				glBindTexture(GL_TEXTURE_2D, light.depth_map->get());
				glUniform1i(m_model_shader.directional_depth_maps[light_index].location(), depth_map_texture_unit);
				const auto cascade_offset = light_index * directional_light::csm_cascade_count;
				for (auto cascade_level = std::size_t{0}; cascade_level < directional_light::csm_cascade_count; ++cascade_level) {
					glUniformMatrix4fv(
						m_model_shader.directional_shadow_matrices[cascade_offset + cascade_level].location(), 1, GL_FALSE, glm::value_ptr(light.shadow_matrices[cascade_level]));
					glUniform1f(m_model_shader.directional_shadow_uv_sizes[cascade_offset + cascade_level].location(), light.shadow_uv_sizes[cascade_level]);
					glUniform1f(m_model_shader.directional_shadow_near_planes[cascade_offset + cascade_level].location(), light.shadow_near_planes[cascade_level]);
				}
			} else {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_FALSE);
			}
			glUniform1i(light_uniform.is_active.location(), GL_TRUE);
			++light_index;
		}
		m_directional_lights.clear();
		for (; light_index < directional_light_count; ++light_index) {
			auto& light_uniform = m_model_shader.directional_lights[light_index];
			glUniform1i(light_uniform.is_active.location(), GL_FALSE);
		}

		// Upload point lights.
		light_index = 0;
		for (const auto& light : m_point_lights) {
			if (light_index >= point_light_count) {
				break;
			}
			auto& light_uniform = m_model_shader.point_lights[light_index];
			glUniform3fv(light_uniform.position.location(), 1, glm::value_ptr(light.position));
			glUniform3fv(light_uniform.ambient.location(), 1, glm::value_ptr(light.ambient));
			glUniform3fv(light_uniform.color.location(), 1, glm::value_ptr(light.color));
			glUniform1f(light_uniform.constant.location(), light.constant);
			glUniform1f(light_uniform.linear.location(), light.linear);
			glUniform1f(light_uniform.quadratic.location(), light.quadratic);
			glUniform1f(light_uniform.shadow_near_plane.location(), light.shadow_near_plane);
			glUniform1f(light_uniform.shadow_far_plane.location(), light.shadow_far_plane);
			if (light.shadow_map) {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_TRUE);
				const auto shadow_map_texture_unit = point_light_texture_units_begin + static_cast<GLint>(light_index);
				glActiveTexture(GL_TEXTURE0 + shadow_map_texture_unit);
				glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, light.shadow_map->get());
				glUniform1i(m_model_shader.point_shadow_maps[light_index].location(), shadow_map_texture_unit);
			} else {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_FALSE);
			}
			glUniform1i(light_uniform.is_active.location(), GL_TRUE);
			++light_index;
		}
		m_point_lights.clear();
		for (; light_index < point_light_count; ++light_index) {
			auto& light_uniform = m_model_shader.point_lights[light_index];
			glUniform1i(light_uniform.is_active.location(), GL_FALSE);
		}

		// Upload spot lights.
		light_index = 0;
		for (const auto& light : m_spot_lights) {
			if (light_index >= spot_light_count) {
				break;
			}
			auto& light_uniform = m_model_shader.spot_lights[light_index];
			glUniform3fv(light_uniform.position.location(), 1, glm::value_ptr(light.position));
			glUniform3fv(light_uniform.direction.location(), 1, glm::value_ptr(light.direction));
			glUniform3fv(light_uniform.ambient.location(), 1, glm::value_ptr(light.ambient));
			glUniform3fv(light_uniform.color.location(), 1, glm::value_ptr(light.color));
			glUniform1f(light_uniform.constant.location(), light.constant);
			glUniform1f(light_uniform.linear.location(), light.linear);
			glUniform1f(light_uniform.quadratic.location(), light.quadratic);
			glUniform1f(light_uniform.inner_cutoff.location(), light.inner_cutoff);
			glUniform1f(light_uniform.outer_cutoff.location(), light.outer_cutoff);
			glUniform1f(light_uniform.shadow_near_plane.location(), light.shadow_near_plane);
			glUniform1f(light_uniform.shadow_far_plane.location(), light.shadow_far_plane);
			if (light.shadow_map) {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_TRUE);
				const auto shadow_map_texture_unit = spot_light_texture_units_begin + static_cast<GLint>(light_index);
				glActiveTexture(GL_TEXTURE0 + shadow_map_texture_unit);
				glBindTexture(GL_TEXTURE_2D, light.shadow_map->get());
				glUniform1i(m_model_shader.spot_shadow_maps[light_index].location(), shadow_map_texture_unit);
				glUniformMatrix4fv(m_model_shader.spot_shadow_matrices[light_index].location(), 1, GL_FALSE, glm::value_ptr(light.shadow_matrix));
			} else {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_FALSE);
			}
			glUniform1i(light_uniform.is_active.location(), GL_TRUE);
			++light_index;
		}
		m_spot_lights.clear();
		for (; light_index < spot_light_count; ++light_index) {
			auto& light_uniform = m_model_shader.spot_lights[light_index];
			glUniform1i(light_uniform.is_active.location(), GL_FALSE);
		}

		// Render models.
		for (const auto& [model, instances] : m_model_instances) {
			const auto model_texture_units_begin = reserved_texture_units_end;
			auto i = 0;
			for (const auto& texture : model->textures()) {
				glActiveTexture(GL_TEXTURE0 + model_texture_units_begin + i);
				glBindTexture(GL_TEXTURE_2D, texture->get());
				++i;
			}
			for (const auto& mesh : model->meshes()) {
				glBindVertexArray(mesh.get());
				const auto& material = mesh.material();
				glUniform1i(m_model_shader.material_albedo.location(), static_cast<GLint>(model_texture_units_begin + material.albedo_texture_offset));
				glUniform1i(m_model_shader.material_normal.location(), static_cast<GLint>(model_texture_units_begin + material.normal_texture_offset));
				glUniform1i(m_model_shader.material_roughness.location(), static_cast<GLint>(model_texture_units_begin + material.roughness_texture_offset));
				glUniform1i(m_model_shader.material_metallic.location(), static_cast<GLint>(model_texture_units_begin + material.metallic_texture_offset));
				for (const auto& instance : instances) {
					const auto& model_matrix = instance.transform;
					const auto normal_matrix = glm::inverseTranspose(mat3{model_matrix});
					glUniformMatrix4fv(m_model_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(model_matrix));
					glUniformMatrix3fv(m_model_shader.normal_matrix.location(), 1, GL_FALSE, glm::value_ptr(normal_matrix));
					glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.index_count()), model_mesh::index_type, nullptr);
				}
			}
		}
		m_model_instances.clear();
	}

	auto reload_shaders(int width, int height, float vertical_fov, float near_z, float far_z) -> void {
		m_model_shader = model_shader{};
		m_model_shader.resize(width, height, vertical_fov, near_z, far_z);
	}

private:
	struct model_shader final {
		model_shader() {
			glUseProgram(program.get());
			for (auto i = std::size_t{0}; i < directional_light_count; ++i) {
				glUniform1i(directional_lights[i].is_active.location(), GL_FALSE);
			}
			for (auto i = std::size_t{0}; i < point_light_count; ++i) {
				glUniform1i(point_lights[i].is_active.location(), GL_FALSE);
			}
			for (auto i = std::size_t{0}; i < spot_light_count; ++i) {
				glUniform1i(spot_lights[i].is_active.location(), GL_FALSE);
			}
		}

		auto resize(int width, int height, float vertical_fov, float near_z, float far_z) -> void { // NOLINT(readability-make-member-function-const)
			const auto aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
			const auto projection_matrix = glm::perspective(vertical_fov, aspect_ratio, near_z, far_z);
			glUseProgram(program.get());
			glUniformMatrix4fv(this->projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
		}

		shader_program program{{
			.vertex_shader_filename = "assets/shaders/pbr.vert",
			.fragment_shader_filename = "assets/shaders/pbr.frag",
			.definitions =
				{
					{"GAMMA", gamma},
					{"DIRECTIONAL_LIGHT_COUNT", directional_light_count},
					{"POINT_LIGHT_COUNT", point_light_count},
					{"SPOT_LIGHT_COUNT", spot_light_count},
					{"CSM_CASCADE_COUNT", directional_light::csm_cascade_count},
				},
		}};
		shader_uniform projection_matrix{program.get(), "projection_matrix"};
		shader_uniform view_matrix{program.get(), "view_matrix"};
		shader_uniform model_matrix{program.get(), "model_matrix"};
		shader_uniform normal_matrix{program.get(), "normal_matrix"};
		shader_uniform view_position{program.get(), "view_position"};
		shader_uniform material_albedo{program.get(), "material_albedo"};
		shader_uniform material_normal{program.get(), "material_normal"};
		shader_uniform material_roughness{program.get(), "material_roughness"};
		shader_uniform material_metallic{program.get(), "material_metallic"};
		shader_uniform cubemap_texture{program.get(), "cubemap_texture"};
		shader_array<directional_light_uniform, directional_light_count> directional_lights{program.get(), "directional_lights"};
		shader_array<point_light_uniform, point_light_count> point_lights{program.get(), "point_lights"};
		shader_array<spot_light_uniform, spot_light_count> spot_lights{program.get(), "spot_lights"};
		shader_array<shader_uniform, directional_light_count> directional_shadow_maps{program.get(), "directional_shadow_maps"};
		shader_array<shader_uniform, directional_light_count> directional_depth_maps{program.get(), "directional_depth_maps"};
		shader_array<shader_uniform, directional_light_count * directional_light::csm_cascade_count> directional_shadow_matrices{program.get(), "directional_shadow_matrices"};
		shader_array<shader_uniform, directional_light_count * directional_light::csm_cascade_count> directional_shadow_uv_sizes{program.get(), "directional_shadow_uv_sizes"};
		shader_array<shader_uniform, directional_light_count * directional_light::csm_cascade_count> directional_shadow_near_planes{program.get(),
			"directional_shadow_near_planes"};
		shader_array<shader_uniform, point_light_count> point_shadow_maps{program.get(), "point_shadow_maps"};
		shader_array<shader_uniform, spot_light_count> spot_shadow_maps{program.get(), "spot_shadow_maps"};
		shader_array<shader_uniform, spot_light_count> spot_shadow_matrices{program.get(), "spot_shadow_matrices"};
		shader_array<shader_uniform, directional_light::csm_cascade_count> cascade_levels_frustrum_depths{program.get(), "cascade_levels_frustrum_depths"};
	};

	struct model_instance final {
		explicit model_instance(const mat4& transform) noexcept
			: transform(transform) {}

		mat4 transform;
	};

	using model_instance_map = std::unordered_map<std::shared_ptr<textured_model>, std::vector<model_instance>>;

	model_shader m_model_shader{};
	std::shared_ptr<cubemap> m_cubemap{};
	std::vector<directional_light> m_directional_lights{};
	std::vector<point_light> m_point_lights{};
	std::vector<spot_light> m_spot_lights{};
	model_instance_map m_model_instances{};
};

#endif
