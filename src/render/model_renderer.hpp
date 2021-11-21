#ifndef MODEL_RENDERER_HPP
#define MODEL_RENDERER_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/brdf.hpp"
#include "../resources/cubemap.hpp"
#include "../resources/light.hpp"
#include "../resources/model.hpp"
#include "../resources/shader.hpp"

#include <algorithm>                  // std::ranges::sort
#include <array>                      // std::array
#include <cstddef>                    // std::size_t
#include <glm/gtc/matrix_inverse.hpp> // glm::inverseTranspose
#include <glm/gtc/type_ptr.hpp>       // glm::value_ptr
#include <glm/gtx/norm.hpp>           // glm::distance2
#include <memory>                     // std::shared_ptr
#include <span>                       // std::span
#include <unordered_map>              // std::unordered_map
#include <utility>                    // std::move
#include <vector>                     // std::vector

class model_renderer final {
public:
	static constexpr auto gamma = 2.2f;
	static constexpr auto brdf_lookup_table_resolution = std::size_t{512};
	static constexpr auto directional_light_count = std::size_t{1};
	static constexpr auto point_light_count = std::size_t{2};
	static constexpr auto spot_light_count = std::size_t{2};

	static constexpr auto reserved_texture_units_begin = GLint{0};
	static constexpr auto lightmap_texture_unit = GLint{reserved_texture_units_begin};
	static constexpr auto environment_cubemap_texture_unit = GLint{lightmap_texture_unit + 1};
	static constexpr auto irradiance_cubemap_texture_unit = GLint{environment_cubemap_texture_unit + 1};
	static constexpr auto prefilter_cubemap_texture_unit = GLint{irradiance_cubemap_texture_unit + 1};
	static constexpr auto brdf_lookup_table_texture_unit = GLint{prefilter_cubemap_texture_unit + 1};
	static constexpr auto directional_light_texture_units_begin = GLint{brdf_lookup_table_texture_unit + 1};
	static constexpr auto point_light_texture_units_begin = GLint{directional_light_texture_units_begin + directional_light_count * 2};
	static constexpr auto spot_light_texture_units_begin = GLint{point_light_texture_units_begin + point_light_count};
	static constexpr auto reserved_texture_units_end = GLint{spot_light_texture_units_begin + spot_light_count};

	auto resize(const mat4& projection_matrix) -> void {
		m_model_shader.upload_projection_matrix(projection_matrix);
		m_model_shader_with_alpha_test.upload_projection_matrix(projection_matrix);
		m_model_shader_with_alpha_blending.upload_projection_matrix(projection_matrix);
	}

	auto reload_shaders(const mat4& projection_matrix) -> void {
		m_model_shader = model_shader{false, false};
		m_model_shader.upload_projection_matrix(projection_matrix);
		m_model_shader_with_alpha_test = model_shader{true, false};
		m_model_shader_with_alpha_test.upload_projection_matrix(projection_matrix);
		m_model_shader_with_alpha_blending = model_shader{false, true};
		m_model_shader_with_alpha_blending.upload_projection_matrix(projection_matrix);
	}

	auto draw_lightmap(std::shared_ptr<texture> lightmap) -> void {
		m_lightmap = std::move(lightmap);
	}

	auto draw_environment(std::shared_ptr<environment_cubemap> environment) -> void {
		m_environment = std::move(environment);
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

	auto draw_model(std::shared_ptr<model> model, const mat4& transform, vec2 lightmap_offset, vec2 lightmap_scale) -> void {
		m_model_instances[std::move(model)].emplace_back(transform, lightmap_offset, lightmap_scale);
	}

	auto render(const mat4& view_matrix, vec3 view_position) -> void {
		// Render meshes without alpha.
		glUseProgram(m_model_shader.program.get());
		upload_uniform_frame_data(m_model_shader, view_matrix, view_position);
		for (const auto& [model, instances] : m_model_instances) {
			const auto model_texture_units_begin = reserved_texture_units_end;
			auto texture_index = 0;
			for (const auto& texture : model->textures()) {
				glActiveTexture(GL_TEXTURE0 + model_texture_units_begin + texture_index);
				glBindTexture(GL_TEXTURE_2D, texture->get());
				++texture_index;
			}
			for (const auto& mesh : model->meshes()) {
				const auto& material = mesh.material();
				if (!material.alpha_test && !material.alpha_blending) {
					glBindVertexArray(mesh.get());
					glUniform1i(m_model_shader.material_albedo.location(), static_cast<GLint>(model_texture_units_begin + material.albedo_texture_offset));
					glUniform1i(m_model_shader.material_normal.location(), static_cast<GLint>(model_texture_units_begin + material.normal_texture_offset));
					glUniform1i(m_model_shader.material_roughness.location(), static_cast<GLint>(model_texture_units_begin + material.roughness_texture_offset));
					glUniform1i(m_model_shader.material_metallic.location(), static_cast<GLint>(model_texture_units_begin + material.metallic_texture_offset));
					for (const auto& instance : instances) {
						const auto& model_matrix = instance.transform;
						const auto normal_matrix = glm::inverseTranspose(mat3{model_matrix});
						glUniformMatrix4fv(m_model_shader.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(model_matrix));
						glUniformMatrix3fv(m_model_shader.normal_matrix.location(), 1, GL_FALSE, glm::value_ptr(normal_matrix));
						glUniform2fv(m_model_shader.lightmap_offset.location(), 1, glm::value_ptr(instance.lightmap_offset));
						glUniform2fv(m_model_shader.lightmap_scale.location(), 1, glm::value_ptr(instance.lightmap_scale));
						glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.indices().size()), model_mesh::index_type, nullptr);
					}
				}
			}
		}

		// Render meshes with alpha.
		glUseProgram(m_model_shader_with_alpha_test.program.get());
		upload_uniform_frame_data(m_model_shader_with_alpha_test, view_matrix, view_position);
		glDisable(GL_CULL_FACE);
		for (const auto& [model, instances] : m_model_instances) {
			const auto model_texture_units_begin = reserved_texture_units_end;
			auto texture_index = 0;
			for (const auto& texture : model->textures()) {
				glActiveTexture(GL_TEXTURE0 + model_texture_units_begin + texture_index);
				glBindTexture(GL_TEXTURE_2D, texture->get());
				++texture_index;
			}
			glActiveTexture(GL_TEXTURE0 + lightmap_texture_unit);
			for (const auto& mesh : model->meshes()) {
				const auto& material = mesh.material();
				if (material.alpha_blending) {
					for (const auto& instance : instances) {
						const auto depth = glm::distance2(view_position, vec3{instance.transform[3]});
						m_alpha_blended_mesh_instances.emplace_back(model, mesh, instance.transform, instance.lightmap_offset, instance.lightmap_scale, depth);
					}
				} else if (material.alpha_test) {
					glBindVertexArray(mesh.get());
					glUniform1i(m_model_shader_with_alpha_test.material_albedo.location(), static_cast<GLint>(model_texture_units_begin + material.albedo_texture_offset));
					glUniform1i(m_model_shader_with_alpha_test.material_normal.location(), static_cast<GLint>(model_texture_units_begin + material.normal_texture_offset));
					glUniform1i(m_model_shader_with_alpha_test.material_roughness.location(), static_cast<GLint>(model_texture_units_begin + material.roughness_texture_offset));
					glUniform1i(m_model_shader_with_alpha_test.material_metallic.location(), static_cast<GLint>(model_texture_units_begin + material.metallic_texture_offset));
					for (const auto& instance : instances) {
						const auto& model_matrix = instance.transform;
						const auto normal_matrix = glm::inverseTranspose(mat3{model_matrix});
						glUniformMatrix4fv(m_model_shader_with_alpha_test.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(model_matrix));
						glUniformMatrix3fv(m_model_shader_with_alpha_test.normal_matrix.location(), 1, GL_FALSE, glm::value_ptr(normal_matrix));
						glUniform2fv(m_model_shader_with_alpha_test.lightmap_offset.location(), 1, glm::value_ptr(instance.lightmap_offset));
						glUniform2fv(m_model_shader_with_alpha_test.lightmap_scale.location(), 1, glm::value_ptr(instance.lightmap_scale));
						glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh.indices().size()), model_mesh::index_type, nullptr);
					}
				}
			}
		}
		glEnable(GL_CULL_FACE);

		// Render alpha blended mesh instances back-to-front.
		std::ranges::sort(m_alpha_blended_mesh_instances, [](const auto& lhs, const auto& rhs) { return lhs.depth > rhs.depth; });
		glUseProgram(m_model_shader_with_alpha_blending.program.get());
		upload_uniform_frame_data(m_model_shader_with_alpha_blending, view_matrix, view_position);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for (const auto& [model, mesh, transform, lightmap_offset, lightmap_scale, depth] : m_alpha_blended_mesh_instances) {
			const auto model_texture_units_begin = reserved_texture_units_end;
			auto texture_index = 0;
			for (const auto& texture : model->textures()) {
				glActiveTexture(GL_TEXTURE0 + model_texture_units_begin + texture_index);
				glBindTexture(GL_TEXTURE_2D, texture->get());
				++texture_index;
			}
			glBindVertexArray(mesh->get());
			const auto& material = mesh->material();
			glUniform1i(m_model_shader_with_alpha_blending.material_albedo.location(), static_cast<GLint>(model_texture_units_begin + material.albedo_texture_offset));
			glUniform1i(m_model_shader_with_alpha_blending.material_normal.location(), static_cast<GLint>(model_texture_units_begin + material.normal_texture_offset));
			glUniform1i(m_model_shader_with_alpha_blending.material_roughness.location(), static_cast<GLint>(model_texture_units_begin + material.roughness_texture_offset));
			glUniform1i(m_model_shader_with_alpha_blending.material_metallic.location(), static_cast<GLint>(model_texture_units_begin + material.metallic_texture_offset));

			const auto& model_matrix = transform;
			const auto normal_matrix = glm::inverseTranspose(mat3{model_matrix});
			glUniformMatrix4fv(m_model_shader_with_alpha_blending.model_matrix.location(), 1, GL_FALSE, glm::value_ptr(model_matrix));
			glUniformMatrix3fv(m_model_shader_with_alpha_blending.normal_matrix.location(), 1, GL_FALSE, glm::value_ptr(normal_matrix));
			glUniform2fv(m_model_shader_with_alpha_blending.lightmap_offset.location(), 1, glm::value_ptr(lightmap_offset));
			glUniform2fv(m_model_shader_with_alpha_blending.lightmap_scale.location(), 1, glm::value_ptr(lightmap_scale));
			glDrawElements(model_mesh::primitive_type, static_cast<GLsizei>(mesh->indices().size()), model_mesh::index_type, nullptr);
		}
		glBlendFunc(GL_ONE, GL_ZERO);
		glDisable(GL_BLEND);

		m_lightmap.reset();
		m_environment.reset();
		m_directional_lights.clear();
		m_point_lights.clear();
		m_spot_lights.clear();
		m_model_instances.clear();
		m_alpha_blended_mesh_instances.clear();
	}

private:
	struct model_shader final {
		model_shader(bool use_alpha_test, bool use_alpha_blending)
			: program({
				  .vertex_shader_filename = "assets/shaders/model.vert",
				  .fragment_shader_filename = "assets/shaders/model.frag",
				  .definitions =
					  {
						  {"GAMMA", gamma},
						  {"DIRECTIONAL_LIGHT_COUNT", directional_light_count},
						  {"POINT_LIGHT_COUNT", point_light_count},
						  {"SPOT_LIGHT_COUNT", spot_light_count},
						  {"CSM_CASCADE_COUNT", directional_light::csm_cascade_count},
						  {"USE_ALPHA_TEST", (use_alpha_test) ? 1 : 0},
						  {"USE_ALPHA_BLENDING", (use_alpha_blending) ? 1 : 0},
					  },
			  }) {
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

		auto upload_projection_matrix(const mat4& projection_matrix) const -> void {
			glUseProgram(program.get());
			glUniformMatrix4fv(this->projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
		}

		shader_program program;
		shader_uniform projection_matrix{program.get(), "projection_matrix"};
		shader_uniform view_matrix{program.get(), "view_matrix"};
		shader_uniform model_matrix{program.get(), "model_matrix"};
		shader_uniform normal_matrix{program.get(), "normal_matrix"};
		shader_uniform view_position{program.get(), "view_position"};
		shader_uniform material_albedo{program.get(), "material_albedo"};
		shader_uniform material_normal{program.get(), "material_normal"};
		shader_uniform material_roughness{program.get(), "material_roughness"};
		shader_uniform material_metallic{program.get(), "material_metallic"};
		shader_uniform lightmap_texture{program.get(), "lightmap_texture"};
		shader_uniform lightmap_offset{program.get(), "lightmap_offset"};
		shader_uniform lightmap_scale{program.get(), "lightmap_scale"};
		shader_uniform environment_cubemap_texture{program.get(), "environment_cubemap_texture"};
		shader_uniform irradiance_cubemap_texture{program.get(), "irradiance_cubemap_texture"};
		shader_uniform prefilter_cubemap_texture{program.get(), "prefilter_cubemap_texture"};
		shader_uniform brdf_lookup_table_texture{program.get(), "brdf_lookup_table_texture"};
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
		model_instance(const mat4& transform, vec2 lightmap_offset, vec2 lightmap_scale) noexcept
			: transform(transform)
			, lightmap_offset(lightmap_offset)
			, lightmap_scale(lightmap_scale) {}

		mat4 transform;
		vec2 lightmap_offset;
		vec2 lightmap_scale;
	};

	using model_instance_map = std::unordered_map<std::shared_ptr<model>, std::vector<model_instance>>;

	struct alpha_blended_mesh_instance final {
		alpha_blended_mesh_instance(std::shared_ptr<model> model, const model_mesh& mesh, const mat4& transform, vec2 lightmap_offset, vec2 lightmap_scale, float depth) noexcept
			: model_ptr(std::move(model))
			, mesh(&mesh)
			, transform(transform)
			, lightmap_offset(lightmap_offset)
			, lightmap_scale(lightmap_scale)
			, depth(depth) {}

		std::shared_ptr<model> model_ptr;
		const model_mesh* mesh;
		mat4 transform;
		vec2 lightmap_offset;
		vec2 lightmap_scale;
		float depth;
	};

	using alpha_blended_mesh_instance_list = std::vector<alpha_blended_mesh_instance>;

	auto upload_uniform_frame_data(model_shader& shader, const mat4& view_matrix, vec3 view_position) const -> void {
		// Upload view matrix and position.
		glUniformMatrix4fv(m_model_shader.view_matrix.location(), 1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniform3fv(m_model_shader.view_position.location(), 1, glm::value_ptr(view_position));

		// Upload lightmap.
		if (m_lightmap) {
			glActiveTexture(GL_TEXTURE0 + lightmap_texture_unit);
			glBindTexture(GL_TEXTURE_2D, m_lightmap->get());
			glUniform1i(shader.lightmap_texture.location(), lightmap_texture_unit);
		}

		// Upload environment maps.
		if (m_environment) {
			glActiveTexture(GL_TEXTURE0 + environment_cubemap_texture_unit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_environment->environment_map());
			glUniform1i(shader.environment_cubemap_texture.location(), environment_cubemap_texture_unit);

			glActiveTexture(GL_TEXTURE0 + irradiance_cubemap_texture_unit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_environment->irradiance_map());
			glUniform1i(shader.irradiance_cubemap_texture.location(), irradiance_cubemap_texture_unit);

			glActiveTexture(GL_TEXTURE0 + prefilter_cubemap_texture_unit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_environment->prefilter_map());
			glUniform1i(shader.prefilter_cubemap_texture.location(), prefilter_cubemap_texture_unit);
		}

		// Upload BRDF LUT texture.
		glActiveTexture(GL_TEXTURE0 + brdf_lookup_table_texture_unit);
		glBindTexture(GL_TEXTURE_2D, m_brdf_lookup_table.get());
		glUniform1i(shader.brdf_lookup_table_texture.location(), brdf_lookup_table_texture_unit);

		// Upload directional lights.
		auto light_index = std::size_t{0};
		for (const auto& light : m_directional_lights) {
			if (light_index >= directional_light_count) {
				break;
			}
			auto& light_uniform = shader.directional_lights[light_index];
			glUniform3fv(light_uniform.direction.location(), 1, glm::value_ptr(light.direction));
			glUniform3fv(light_uniform.color.location(), 1, glm::value_ptr(light.color));
			if (light.shadow_map && light.depth_map) {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_TRUE);
				const auto shadow_map_texture_unit = directional_light_texture_units_begin + static_cast<GLint>(light_index) * GLint{2};
				const auto depth_map_texture_unit = shadow_map_texture_unit + GLint{1};
				glActiveTexture(GL_TEXTURE0 + shadow_map_texture_unit);
				glBindTexture(GL_TEXTURE_2D_ARRAY, light.shadow_map->get());
				glUniform1i(shader.directional_shadow_maps[light_index].location(), shadow_map_texture_unit);
				glActiveTexture(GL_TEXTURE0 + depth_map_texture_unit);
				glBindTexture(GL_TEXTURE_2D, light.depth_map->get());
				glUniform1i(shader.directional_depth_maps[light_index].location(), depth_map_texture_unit);
				const auto cascade_offset = light_index * directional_light::csm_cascade_count;
				for (auto cascade_level = std::size_t{0}; cascade_level < directional_light::csm_cascade_count; ++cascade_level) {
					glUniformMatrix4fv(
						shader.directional_shadow_matrices[cascade_offset + cascade_level].location(), 1, GL_FALSE, glm::value_ptr(light.shadow_matrices[cascade_level]));
					glUniform1f(shader.directional_shadow_uv_sizes[cascade_offset + cascade_level].location(), light.shadow_uv_sizes[cascade_level]);
					glUniform1f(shader.directional_shadow_near_planes[cascade_offset + cascade_level].location(), light.shadow_near_planes[cascade_level]);
				}
			} else {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_FALSE);
			}
			glUniform1i(light_uniform.is_active.location(), GL_TRUE);
			++light_index;
		}
		for (; light_index < directional_light_count; ++light_index) {
			auto& light_uniform = shader.directional_lights[light_index];
			glUniform1i(light_uniform.is_active.location(), GL_FALSE);
		}

		// Upload point lights.
		light_index = 0;
		for (const auto& light : m_point_lights) {
			if (light_index >= point_light_count) {
				break;
			}
			auto& light_uniform = shader.point_lights[light_index];
			glUniform3fv(light_uniform.position.location(), 1, glm::value_ptr(light.position));
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
				glUniform1i(shader.point_shadow_maps[light_index].location(), shadow_map_texture_unit);
			} else {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_FALSE);
			}
			glUniform1i(light_uniform.is_active.location(), GL_TRUE);
			++light_index;
		}
		for (; light_index < point_light_count; ++light_index) {
			auto& light_uniform = shader.point_lights[light_index];
			glUniform1i(light_uniform.is_active.location(), GL_FALSE);
		}

		// Upload spot lights.
		light_index = 0;
		for (const auto& light : m_spot_lights) {
			if (light_index >= spot_light_count) {
				break;
			}
			auto& light_uniform = shader.spot_lights[light_index];
			glUniform3fv(light_uniform.position.location(), 1, glm::value_ptr(light.position));
			glUniform3fv(light_uniform.direction.location(), 1, glm::value_ptr(light.direction));
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
				glUniform1i(shader.spot_shadow_maps[light_index].location(), shadow_map_texture_unit);
				glUniformMatrix4fv(shader.spot_shadow_matrices[light_index].location(), 1, GL_FALSE, glm::value_ptr(light.shadow_matrix));
			} else {
				glUniform1i(light_uniform.is_shadow_mapped.location(), GL_FALSE);
			}
			glUniform1i(light_uniform.is_active.location(), GL_TRUE);
			++light_index;
		}
		for (; light_index < spot_light_count; ++light_index) {
			auto& light_uniform = shader.spot_lights[light_index];
			glUniform1i(light_uniform.is_active.location(), GL_FALSE);
		}
	}

	model_shader m_model_shader{false, false};
	model_shader m_model_shader_with_alpha_test{true, false};
	model_shader m_model_shader_with_alpha_blending{false, true};
	texture m_brdf_lookup_table = brdf_generator{}.generate_lookup_table(GL_RG16F, brdf_lookup_table_resolution);
	std::shared_ptr<texture> m_lightmap{};
	std::shared_ptr<environment_cubemap> m_environment{};
	std::vector<directional_light> m_directional_lights{};
	std::vector<point_light> m_point_lights{};
	std::vector<spot_light> m_spot_lights{};
	model_instance_map m_model_instances{};
	alpha_blended_mesh_instance_list m_alpha_blended_mesh_instances{};
};

#endif
