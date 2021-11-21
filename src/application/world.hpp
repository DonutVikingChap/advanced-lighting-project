#ifndef WORLD_HPP
#define WORLD_HPP

#include "../core/glsl.hpp"
#include "../render/rendering_pipeline.hpp"
#include "../resources/image.hpp"
#include "../resources/lightmap.hpp"
#include "../resources/scene.hpp"
#include "../resources/texture.hpp"
#include "asset_manager.hpp"
#include "flight_controller.hpp"

#include <SDL.h>                // SDL_...
#include <cmath>                // std::round
#include <cstddef>              // std::size_t
#include <cstdio>               // stderr
#include <fmt/format.h>         // fmt::format, fmt::print
#include <glm/glm.hpp>          // glm::identity, glm::lookAt, glm::translate, glm::scale
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <imgui.h>              // ImGui
#include <memory>               // std::shared_ptr, std::make_shared
#include <stdexcept>            // std::exception
#include <string>               // std::string
#include <string_view>          // std::string_view

class world final {
public:
	world(std::string filename, asset_manager& asset_manager)
		: m_filename(std::move(filename))
		, m_scene{
			  .sky = asset_manager.load_environment_cubemap_equirectangular_hdr("assets/textures/studio_country_hall_1k_dark.hdr", 512),
			  .directional_lights = {},
			  .point_lights =
				  {
					  {
						  .position = {-1.8f, 1.8f, 1.75f},
						  .color = {0.8f, 0.8f, 0.8f},
						  .constant = 1.0f,
						  .linear = 0.045f,
						  .quadratic = 0.0075f,
					  },
				  },
			  .spot_lights = {},
			  .objects =
				  {
					  {
						  .model_ptr = asset_manager.load_model("assets/models/sponza/sponza.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{0.0f, -3.0f, 0.0f}), vec3{0.0254f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/alarm_clock_01_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{2.0f, 0.0f, -3.0f}), vec3{15.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/suzanne.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{0.0f, 0.0f, 0.0f}), vec3{1.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/tea_set_01_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{4.0f, -1.0f, 0.0f}), vec3{10.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/brass_vase_01_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{-3.0f, -1.0f, -2.0f}), vec3{6.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/Chandelier_03_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{5.0f, 20.0f, -1.0f}), vec3{6.0f}),
					  },
					  {
						  .model_ptr = asset_manager.load_model("assets/models/Chandelier_03_1k.obj", "assets/textures/"),
						  .transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{-5.0f, 20.0f, -1.0f}), vec3{6.0f}),
					  },
				  },
		  } {}

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
				bake_lightmap(renderer);
			}
			ImGui::End();
			if (!m_scene.point_lights.empty()) {
				ImGui::Begin("Light");
				ImGui::SliderFloat3("Position", glm::value_ptr(m_scene.point_lights[0].position), -50.0f, 50.0f);
				ImGui::SliderFloat3("Color", glm::value_ptr(m_scene.point_lights[0].color), 0.0f, 5.0f);
				ImGui::SliderFloat("Constant", &m_scene.point_lights[0].constant, 0.0f, 1.0f);
				ImGui::SliderFloat("Linear", &m_scene.point_lights[0].linear, 0.0f, 1.0f);
				ImGui::SliderFloat("Quadratic", &m_scene.point_lights[0].quadratic, 0.0f, 1.0f);
				ImGui::End();
			}
		}
		renderer.skybox().draw_skybox(m_scene.sky->original());
		renderer.model().draw_lightmap(m_lightmap);
		renderer.model().draw_environment(m_scene.sky);
		for (const auto& light : m_scene.directional_lights) {
			renderer.model().draw_directional_light(light);
		}
		for (const auto& light : m_scene.point_lights) {
			renderer.model().draw_point_light(light);
		}
		for (const auto& light : m_scene.spot_lights) {
			renderer.model().draw_spot_light(light);
		}
		for (const auto& object : m_scene.objects) {
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
	static constexpr auto mouse_sensitivity = 2.0f;
	static constexpr auto move_acceleration = 40.0f;
	static constexpr auto move_drag = 4.0f;
	static constexpr auto yaw_speed = 3.49066f;
	static constexpr auto pitch_speed = 3.49066f;

	[[nodiscard]] auto get_lightmap_filename() const -> std::string {
		return fmt::format("{}/lightmap.png", m_filename);
	}

	auto bake_lightmap(rendering_pipeline& renderer) -> void {
		try {
			struct progress_callback final {
				auto operator()(std::string_view category, std::size_t object_index, std::size_t object_count, float progress) -> bool {
					if (object_index != next_index) {
						next_index = object_index;
						next_progress = 0.0f;
					}
					if (progress >= next_progress) {
						auto current_progress = 0.0f;
						do {
							current_progress = next_progress;
							next_progress += 0.01f;
						} while (next_progress <= progress);
						fmt::print(stderr, "Baking lightmap ({}): Object {}/{}: {}%\n", category, object_index + 1, object_count, std::round(current_progress * 100.0f));
					}
					return true;
				}

				std::size_t next_index = 0;
				float next_progress = 0.0f;
			};
			fmt::print(stderr, "Baking lightmap...\n");
			lightmap_generator::generate_lightmap_coordinates(m_scene, progress_callback{});
			m_lightmap = std::make_shared<texture>(lightmap_generator::bake_lightmap(renderer.model(), renderer.skybox(), m_scene, sky_color, progress_callback{}));
			fmt::print(stderr, "Baking lightmap: Done!\n");
		} catch (const std::exception& e) {
			fmt::print(stderr, "Failed to bake lightmap: {}\n", e.what());
		} catch (...) {
			fmt::print(stderr, "Failed to bake lightmap!\n");
		}
	}

	auto save_lightmap() const -> void {
		if (!m_lightmap) {
			fmt::print(stderr, "No lightmap to save!\n");
			return;
		}
		try {
			const auto pixels = m_lightmap->read_pixels_2d(lightmap_generator::lightmap_format);
			const auto filename = get_lightmap_filename();
			save_png(image_view{pixels.data(), m_lightmap->width(), m_lightmap->height(), lightmap_generator::lightmap_channel_count}, filename.c_str(), {.flip_vertically = true});
			fmt::print(stderr, "Lightmap saved as \"{}\".\n", filename);
		} catch (const std::exception& e) {
			fmt::print(stderr, "Failed to save lightmap: {}\n", e.what());
		} catch (...) {
			fmt::print(stderr, "Failed to save lightmap!\n");
		}
	}

	std::string m_filename;
	scene m_scene;
	std::shared_ptr<texture> m_lightmap = lightmap_generator::get_default();
	flight_controller m_controller{vec3{0.0f, 0.0f, 2.0f}, -1.57079632679f, 0.0f};
};

#endif
