#ifndef SCENE_HPP
#define SCENE_HPP

#include "../core/glsl.hpp"
#include "../render/rendering_pipeline.hpp"
#include "../resources/cubemap.hpp"
#include "../resources/light.hpp"
#include "../resources/model.hpp"
#include "asset_manager.hpp"
#include "flight_controller.hpp"

#include <SDL.h>                // SDL_...
#include <glm/glm.hpp>          // glm::identity, glm::lookAt, glm::translate, glm::scale
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <imgui.h>              // ImGui
#include <memory>               // std::shared_ptr
#include <vector>               // std::vector

class scene final {
public:
	explicit scene(asset_manager& asset_manager)
		: m_asset_manager(asset_manager) {}

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
		renderer.skybox().draw_skybox(m_sky->original());
		renderer.model().draw_environment(m_sky);
		if (renderer.gui().enabled() && !m_point_lights.empty()) {
			ImGui::Begin("Light");
			ImGui::SliderFloat3("Position", glm::value_ptr(m_point_lights[0].position), -50.0f, 50.0f);
			ImGui::SliderFloat3("Color", glm::value_ptr(m_point_lights[0].color), 0.0f, 5.0f);
			ImGui::SliderFloat("Constant", &m_point_lights[0].constant, 0.0f, 1.0f);
			ImGui::SliderFloat("Linear", &m_point_lights[0].linear, 0.0f, 1.0f);
			ImGui::SliderFloat("Quadratic", &m_point_lights[0].quadratic, 0.0f, 1.0f);
			ImGui::End();
		}
		for (const auto& light : m_directional_lights) {
			renderer.model().draw_directional_light(light);
		}
		for (const auto& light : m_point_lights) {
			renderer.model().draw_point_light(light);
		}
		for (const auto& light : m_spot_lights) {
			renderer.model().draw_spot_light(light);
		}
		for (const auto& object : m_objects) {
			renderer.model().draw_model(object.model_ptr, object.transform);
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
	struct object final {
		std::shared_ptr<model> model_ptr;
		mat4 transform;
	};

	static constexpr auto mouse_sensitivity = 2.0f;
	static constexpr auto move_acceleration = 40.0f;
	static constexpr auto move_drag = 4.0f;
	static constexpr auto yaw_speed = 3.49066f;
	static constexpr auto pitch_speed = 3.49066f;

	asset_manager& m_asset_manager;
	std::shared_ptr<environment_cubemap> m_sky = m_asset_manager.load_environment_cubemap_equirectangular_hdr("assets/textures/studio_country_hall_1k_dark.hdr", 512);
	std::vector<directional_light> m_directional_lights{};
	std::vector<point_light> m_point_lights{
		{
			.position = {-1.8f, 1.8f, 1.75f},
			.color = {0.8f, 0.8f, 0.8f},
			.constant = 1.0f,
			.linear = 0.045f,
			.quadratic = 0.0075f,
		},
	};
	std::vector<spot_light> m_spot_lights{};
	std::vector<object> m_objects{
		{
			.model_ptr = m_asset_manager.load_model("assets/models/sponza/sponza.obj", "assets/textures/"),
			.transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{0.0f, -3.0f, 0.0f}), vec3{0.0254f}),
		},
		{
			.model_ptr = m_asset_manager.load_model("assets/models/alarm_clock_01_1k.obj", "assets/textures/"),
			.transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{2.0f, 0.0f, -3.0f}), vec3{15.0f}),
		},
		{
			.model_ptr = m_asset_manager.load_model("assets/models/suzanne.obj", "assets/textures/"),
			.transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{0.0f, 0.0f, 0.0f}), vec3{1.0f}),
		},
		{
			.model_ptr = m_asset_manager.load_model("assets/models/tea_set_01_1k.obj", "assets/textures/"),
			.transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{4.0f, -1.0f, 0.0f}), vec3{10.0f}),
		},
		{
			.model_ptr = m_asset_manager.load_model("assets/models/brass_vase_01_1k.obj", "assets/textures/"),
			.transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{-3.0f, -1.0f, -2.0f}), vec3{6.0f}),
		},
		{
			.model_ptr = m_asset_manager.load_model("assets/models/Chandelier_03_1k.obj", "assets/textures/"),
			.transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{5.0f, 20.0f, -1.0f}), vec3{6.0f}),
		},
		{
			.model_ptr = m_asset_manager.load_model("assets/models/Chandelier_03_1k.obj", "assets/textures/"),
			.transform = glm::scale(glm::translate(glm::identity<mat4>(), vec3{-5.0f, 20.0f, -1.0f}), vec3{6.0f}),
		},
	};
	flight_controller m_controller{vec3{0.0f, 0.0f, 2.0f}, -1.57079632679f, 0.0f};
};

#endif
