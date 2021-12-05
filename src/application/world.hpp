#ifndef WORLD_HPP
#define WORLD_HPP

#include "../core/glsl.hpp"
#include "../render/lightmap_generator.hpp"
#include "../render/rendering_pipeline.hpp"
#include "../resources/image.hpp"
#include "../resources/lightmap.hpp"
#include "../resources/scene.hpp"
#include "../resources/texture.hpp"
#include "asset_manager.hpp"
#include "flight_controller.hpp"

#include <SDL.h>                        // SDL_...
#include <chrono>                       // std::chrono
#include <cstddef>                      // std::size_t, std::ptrdiff_t
#include <cstdio>                       // stderr
#include <fmt/format.h>                 // fmt::format, fmt::print
#include <glm/gtc/matrix_transform.hpp> // glm::lookAt, glm::translate, glm::scale
#include <glm/gtc/type_ptr.hpp>         // glm::value_ptr
#include <imgui.h>                      // ImGui
#include <memory>                       // std::shared_ptr, std::make_shared
#include <stdexcept>                    // std::exception
#include <string>                       // std::string
#include <string_view>                  // std::string_view

class world final {
public:
	world(std::string filename, asset_manager& asset_manager)
		: m_filename(std::move(filename))
		, m_point_light_model(asset_manager.load_model("assets/models/point_light.obj", "assets/textures/"))
		, m_spot_light_model(asset_manager.load_model("assets/models/spot_light.obj", "assets/textures/"))
		, m_scene{
			  .sky = asset_manager.load_environment_cubemap_equirectangular_hdr("assets/textures/studio_country_hall_1k_dark.hdr", 512),
			  .directional_lights = {},
			  .point_lights =
				  {
					  std::make_shared<point_light>(point_light_options{
						  .position = {-1.8f, 1.8f, 1.75f},
						  .color = {0.8f, 0.8f, 0.8f},
						  .constant = 1.0f,
						  .linear = 0.045f,
						  .quadratic = 0.0075f,
						  .is_shadow_mapped = true,
					  }),
				  },
			  .spot_lights =
				  {
					  std::make_shared<spot_light>(spot_light_options{
						  .position = {-28.0f, 4.3f, -1.0f},
						  .direction = vec3{-0.85f, -0.48f, 0.0f},
						  .color = {1.0f, 1.0f, 1.0f},
						  .constant = 1.0f,
						  .linear = 0.045f,
						  .quadratic = 0.0075f,
						  .inner_cutoff = cos(radians(20.0f)),
						  .outer_cutoff = cos(radians(45.0f)),
						  .is_shadow_mapped = true,
					  }),
				  },
			  .objects =
				  {
					  {
						  .model_ptr = asset_manager.load_model("assets/models/sponza/sponza.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(mat4{1.0f}, vec3{0.0f, -3.0f, 0.0f}), vec3{0.0254f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/alarm_clock_01_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(mat4{1.0f}, vec3{2.0f, 0.0f, -3.0f}), vec3{15.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/suzanne.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(mat4{1.0f}, vec3{0.0f, 0.0f, 0.0f}), vec3{1.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/tea_set_01_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(mat4{1.0f}, vec3{4.0f, -1.0f, 0.0f}), vec3{10.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/brass_vase_01_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(mat4{1.0f}, vec3{-3.0f, -1.0f, -2.0f}), vec3{6.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/Chandelier_03_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(mat4{1.0f}, vec3{5.0f, 20.0f, -1.0f}), vec3{6.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/Chandelier_03_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(mat4{1.0f}, vec3{-5.0f, 20.0f, -1.0f}), vec3{6.0f}),
					  },
				  },
		  } {
		lightmap_generator::reset_lightmap(m_scene);
	}

	auto handle_event(const SDL_Event& e) -> void {
		m_controller.handle_event(e, mouse_sensitivity);
	}

	auto tick(unsigned int tick_count, float delta_time) -> void { // NOLINT(readability-convert-member-functions-to-static) // TODO
		(void)tick_count;
		(void)delta_time;
	}

	auto update(float elapsed_time, float delta_time) -> void {
		(void)elapsed_time;
		m_controller.update(delta_time, move_acceleration, move_drag, yaw_speed, pitch_speed);
	}

	auto draw(rendering_pipeline& renderer) -> void {
		if (renderer.gui().enabled()) {
			ImGui::Begin("Lightmap");
			if (ImGui::Button("Save lightmap")) {
				save_lightmap();
			}
			if (ImGui::Button("Bake lightmap")) {
				bake_lightmap();
			}
			ImGui::End();

			ImGui::Begin("Objects");
			for (auto i = std::size_t{0}; i < m_scene.objects.size(); ++i) {
				if (ImGui::TreeNodeEx(fmt::format("Object {}", i).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					if (ImGui::Button("Remove")) {
						m_scene.objects.erase(m_scene.objects.begin() + static_cast<std::ptrdiff_t>(i));
						--i;
					}
					ImGui::TreePop();
				}
				ImGui::Separator();
			}
			ImGui::End();

			ImGui::Begin("Lights");
			ImGui::Checkbox("Show Lights", &m_show_lights);
			ImGui::Separator();
			if (ImGui::TreeNode("Directional Lights")) {
				if (ImGui::Button("Add New Directional Light")) {
					m_scene.directional_lights.push_back(std::make_shared<directional_light>(directional_light_options{}));
				}
				ImGui::Separator();
				for (auto i = std::size_t{0}; i < m_scene.directional_lights.size(); ++i) {
					if (ImGui::TreeNodeEx(fmt::format("Directional Light {}", i).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::SliderFloat3("Direction", glm::value_ptr(m_scene.directional_lights[i]->direction), -1.0f, 1.0f)) {
							m_scene.directional_lights[i]->direction = normalize(m_scene.directional_lights[i]->direction);
							if (any(isnan(m_scene.directional_lights[i]->direction))) {
								m_scene.directional_lights[i]->direction = vec3{0.0f, -1.0f, 0.0f};
							}
						}
						ImGui::SliderFloat3("Color", glm::value_ptr(m_scene.directional_lights[i]->color), 0.0f, 5.0f);
						if (ImGui::Button("Remove")) {
							m_scene.directional_lights.erase(m_scene.directional_lights.begin() + static_cast<std::ptrdiff_t>(i));
							--i;
						}
						ImGui::TreePop();
					}
					ImGui::Separator();
				}
				ImGui::TreePop();
			}
			ImGui::Separator();
			if (ImGui::TreeNode("Point Lights")) {
				if (ImGui::Button("Add New Point Light")) {
					m_scene.point_lights.push_back(std::make_shared<point_light>(point_light_options{}));
				}
				ImGui::Separator();
				for (auto i = std::size_t{0}; i < m_scene.point_lights.size(); ++i) {
					if (ImGui::TreeNodeEx(fmt::format("Point Light {}", i).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::SliderFloat3("Position", glm::value_ptr(m_scene.point_lights[i]->position), -50.0f, 50.0f);
						ImGui::SliderFloat3("Color", glm::value_ptr(m_scene.point_lights[i]->color), 0.0f, 5.0f);
						ImGui::SliderFloat("Constant", &m_scene.point_lights[i]->constant, 0.0f, 1.0f);
						ImGui::SliderFloat("Linear", &m_scene.point_lights[i]->linear, 0.0f, 1.0f);
						ImGui::SliderFloat("Quadratic", &m_scene.point_lights[i]->quadratic, 0.0f, 1.0f);
						if (ImGui::Button("Remove")) {
							m_scene.point_lights.erase(m_scene.point_lights.begin() + static_cast<std::ptrdiff_t>(i));
							--i;
						}
						ImGui::TreePop();
					}
					ImGui::Separator();
				}
				ImGui::TreePop();
			}
			ImGui::Separator();
			if (ImGui::TreeNode("Spot Lights")) {
				if (ImGui::Button("Add New Spot Light")) {
					m_scene.spot_lights.push_back(std::make_shared<spot_light>(spot_light_options{}));
				}
				ImGui::Separator();
				for (auto i = std::size_t{0}; i < m_scene.spot_lights.size(); ++i) {
					if (ImGui::TreeNodeEx(fmt::format("Spot Light {}", i).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::SliderFloat3("Position", glm::value_ptr(m_scene.spot_lights[i]->position), -50.0f, 50.0f);
						if (ImGui::SliderFloat3("Direction", glm::value_ptr(m_scene.spot_lights[i]->direction), -1.0f, 1.0f)) {
							m_scene.spot_lights[i]->direction = normalize(m_scene.spot_lights[i]->direction);
							if (any(isnan(m_scene.spot_lights[i]->direction))) {
								m_scene.spot_lights[i]->direction = vec3{0.0f, -1.0f, 0.0f};
							}
						}
						ImGui::SliderFloat3("Color", glm::value_ptr(m_scene.spot_lights[i]->color), 0.0f, 5.0f);
						ImGui::SliderFloat("Constant", &m_scene.spot_lights[i]->constant, 0.0f, 1.0f);
						ImGui::SliderFloat("Linear", &m_scene.spot_lights[i]->linear, 0.0f, 1.0f);
						ImGui::SliderFloat("Quadratic", &m_scene.spot_lights[i]->quadratic, 0.0f, 1.0f);
						ImGui::SliderFloat("Inner cutoff", &m_scene.spot_lights[i]->inner_cutoff, 0.0f, 1.0f);
						ImGui::SliderFloat("Outer cutoff", &m_scene.spot_lights[i]->outer_cutoff, 0.0f, 1.0f);
						if (ImGui::Button("Save shadow map")) {
							const auto filename = fmt::format("spot_light_{}_shadow_map.hdr", i);
							const auto& shadow_map = m_scene.spot_lights[i]->shadow_map;
							const auto pixels = shadow_map.read_pixels_2d_hdr(GL_DEPTH_COMPONENT);
							save_hdr(image_view{pixels.data(), shadow_map.width(), shadow_map.height(), 1}, filename.c_str(), {.flip_vertically = true});
							fmt::print("Shadow map saved as \"{}\".\n", filename);
						}
						if (ImGui::Button("Remove")) {
							m_scene.spot_lights.erase(m_scene.spot_lights.begin() + static_cast<std::ptrdiff_t>(i));
							--i;
						}
						ImGui::TreePop();
					}
					ImGui::Separator();
				}
				ImGui::TreePop();
			}
			ImGui::End();
		}
		renderer.skybox().draw_skybox(m_scene.sky->original());
		renderer.model().draw_lightmap(m_scene.lightmap);
		renderer.model().draw_environment(m_scene.sky);
		for (const auto& light : m_scene.directional_lights) {
			renderer.shadow().draw_directional_light(light);
			renderer.model().draw_directional_light(light);
		}
		for (const auto& light : m_scene.point_lights) {
			renderer.shadow().draw_point_light(light);
			renderer.model().draw_point_light(light);
			if (m_show_lights) {
				renderer.model().draw_model(
					m_point_light_model, glm::scale(glm::translate(mat4{1.0f}, light->position), vec3{0.5f}), m_scene.default_lightmap_offset, m_scene.default_lightmap_scale);
			}
		}
		for (const auto& light : m_scene.spot_lights) {
			renderer.shadow().draw_spot_light(light);
			renderer.model().draw_spot_light(light);
			if (m_show_lights) {
				const auto world_up = vec3{0.0f, 1.0f, 0.0f};
				const auto forward = light->direction;
				const auto right_cross = cross(forward, world_up);
				const auto right = (right_cross == vec3{}) ? vec3{1.0f, 0.0f, 0.0f} : normalize(right_cross);
				const auto up = cross(forward, right);
				const auto transform = glm::scale(glm::translate(mat4{1.0f}, light->position) *
						mat4{
							vec4{right, 0.0f},
							vec4{up, 0.0f},
							vec4{forward, 0.0f},
							vec4{vec3{}, 1.0f},
						},
					vec3{0.5f});
				renderer.model().draw_model(m_spot_light_model, transform, m_scene.default_lightmap_offset, m_scene.default_lightmap_scale);
			}
		}
		for (const auto& object : m_scene.objects) {
			renderer.shadow().draw_model(object.model_ptr, object.transform);
			renderer.model().draw_model(object.model_ptr, object.transform, object.lightmap_offset, object.lightmap_scale);
		}
	}

	[[nodiscard]] auto controller() noexcept -> flight_controller& {
		return m_controller;
	}

	[[nodiscard]] auto controller() const noexcept -> const flight_controller& {
		return m_controller;
	}

	[[nodiscard]] auto view_position() const noexcept -> vec3 {
		return m_controller.position();
	}

	[[nodiscard]] auto view_matrix() const noexcept -> mat4 {
		return glm::lookAt(m_controller.position(), m_controller.position() + m_controller.forward(), m_controller.up());
	}

private:
	static constexpr auto sky_color = vec3{1.0f, 1.0f, 1.0f};
	static constexpr auto lightmap_resolution = std::size_t{654};
	static constexpr auto lightmap_bounce_count = std::size_t{1};
	static constexpr auto mouse_sensitivity = 2.0f;
	static constexpr auto move_acceleration = 40.0f;
	static constexpr auto move_drag = 4.0f;
	static constexpr auto yaw_speed = 3.49066f;
	static constexpr auto pitch_speed = 3.49066f;

	[[nodiscard]] auto get_lightmap_filename() const -> std::string {
		return fmt::format("{}/lightmap.png", m_filename);
	}

	auto bake_lightmap() -> void {
		try {
			struct progress_callback final {
				auto operator()(std::string_view category, std::size_t bounce_index, std::size_t bounce_count, std::size_t object_index, std::size_t object_count,
					std::size_t mesh_index, std::size_t mesh_count, float progress) -> bool {
					using namespace std::chrono_literals;
					if (const auto now = std::chrono::steady_clock::now(); now >= next_print_time) {
						next_print_time = now + 100ms;
						fmt::print(stderr, "\r  {}: ", category);
						if (bounce_count != 0) {
							fmt::print(stderr, "Bounce {}/{}: ", bounce_index + 1, bounce_count);
						}
						if (object_count != 0) {
							fmt::print(stderr, "Object {}/{}: ", object_index + 1, object_count);
						}
						if (mesh_count != 0) {
							fmt::print(stderr, "Mesh {}/{}: ", mesh_index + 1, mesh_count);
						}
						fmt::print(stderr, "{}%                              \r", progress * 100.0f);
					}
					return true;
				}

				std::chrono::steady_clock::time_point next_print_time = std::chrono::steady_clock::now();
			};
			fmt::print(stderr, "Baking lightmap...\n");
			lightmap_generator::generate_lightmap_coordinates(m_scene, progress_callback{});
			lightmap_generator::bake_lightmap(m_scene, sky_color, lightmap_resolution, lightmap_bounce_count, progress_callback{});
			fmt::print(stderr, "\nBaking lightmap: Done!\n");
		} catch (const std::exception& e) {
			fmt::print(stderr, "Failed to bake lightmap: {}\n", e.what());
		} catch (...) {
			fmt::print(stderr, "Failed to bake lightmap!\n");
		}
	}

	auto save_lightmap() const -> void {
		if (!m_scene.lightmap) {
			fmt::print(stderr, "No lightmap to save!\n");
			return;
		}
		try {
			const auto& texture = m_scene.lightmap->get_texture();
			const auto pixels = texture.read_pixels_2d(lightmap_texture::format);
			const auto filename = get_lightmap_filename();
			save_png(image_view{pixels.data(), texture.width(), texture.height(), lightmap_texture::channel_count}, filename.c_str(), {.flip_vertically = true});
			fmt::print(stderr, "Lightmap saved as \"{}\".\n", filename);
		} catch (const std::exception& e) {
			fmt::print(stderr, "Failed to save lightmap: {}\n", e.what());
		} catch (...) {
			fmt::print(stderr, "Failed to save lightmap!\n");
		}
	}

	std::string m_filename;
	std::shared_ptr<model> m_point_light_model;
	std::shared_ptr<model> m_spot_light_model;
	scene m_scene;
	flight_controller m_controller{vec3{0.0f, 0.0f, 2.0f}, -1.57079632679f, 0.0f};
	bool m_show_lights = false;
};

#endif
