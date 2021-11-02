#ifndef SCENE_HPP
#define SCENE_HPP

#include "asset_manager.hpp"
#include "flight_controller.hpp"
#include "glsl.hpp"
#include "model.hpp"
#include "renderer.hpp"

#include <SDL.h>       // SDL_...
#include <glm/glm.hpp> // glm::identity, glm::lookAt
#include <memory>      // std::shared_ptr
#include <vector>      // std::vector

class scene final {
public:
	explicit scene(asset_manager& asset_manager)
		: m_asset_manager(asset_manager) {}

	auto handle_event(const SDL_Event& e) -> void {
		m_controller.handle_event(e, mouse_sensitivity);
	}

	auto tick(unsigned int tick_count, float delta_time) -> void {
		(void)tick_count;
		m_controller.update(delta_time, move_acceleration, move_drag, yaw_speed, pitch_speed);
	}

	auto update(float elapsed_time, float delta_time) -> void {
		// TODO
		(void)elapsed_time;
		(void)delta_time;
	}

	auto draw(renderer& renderer) const -> void {
		for (const auto& object : m_objects) {
			renderer.model().draw_model(object.model, object.transform);
		}
	}

	[[nodiscard]] auto view_position() const noexcept -> vec3 {
		return m_controller.position();
	}

	[[nodiscard]] auto view_matrix() const noexcept -> mat4 {
		return glm::lookAt(m_controller.position(), m_controller.position() + m_controller.forward(), m_controller.up());
	}

private:
	struct object final {
		std::shared_ptr<textured_model> model;
		mat4 transform;
	};

	static constexpr auto mouse_sensitivity = 2.0f;
	static constexpr auto move_acceleration = 10.0f;
	static constexpr auto move_drag = 4.0f;
	static constexpr auto yaw_speed = 3.49066f;
	static constexpr auto pitch_speed = 3.49066f;

	asset_manager& m_asset_manager;
	std::vector<object> m_objects{
		{m_asset_manager.load_textured_model("assets/models/teapot.obj", "assets/textures/"), glm::identity<mat4>()},
	};
	flight_controller m_controller{vec3{0.0f, 0.0f, 2.0f}, -1.57079632679f, -0.674740942224f};
};

#endif
