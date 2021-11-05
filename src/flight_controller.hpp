#ifndef FLIGHT_CONTROLLER_HPP
#define FLIGHT_CONTROLLER_HPP

#include "glsl.hpp"

#include <SDL.h>       // SDL_...
#include <algorithm>   // std::clamp
#include <cmath>       // std::cos, std::sin
#include <glm/glm.hpp> // glm::dot, glm::cross, glm::normalize

class flight_controller final {
public:
	flight_controller(vec3 position, float yaw, float pitch)
		: m_position(position)
		, m_forward_direction(direction_vector(yaw, pitch))
		, m_right_direction(glm::cross(m_forward_direction, up_direction))
		, m_yaw(yaw)
		, m_pitch(pitch) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
#ifdef __linux__
		SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);
#endif
	}

	auto start_controlling() noexcept -> void {
		m_inputs.controlling = true;
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}

	auto stop_controlling() noexcept -> void {
		m_inputs = {};
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}

	auto handle_event(const SDL_Event& e, float mouse_sensitivity) -> void {
		if (m_inputs.controlling) {
			switch (e.type) {
				case SDL_KEYDOWN:
					switch (e.key.keysym.scancode) {
						case SDL_SCANCODE_W: m_inputs.forward = 1; break;
						case SDL_SCANCODE_S: m_inputs.back = 1; break;
						case SDL_SCANCODE_A: m_inputs.left = 1; break;
						case SDL_SCANCODE_D: m_inputs.right = 1; break;
						case SDL_SCANCODE_UP: m_inputs.aim_up = 1; break;
						case SDL_SCANCODE_DOWN: m_inputs.aim_down = 1; break;
						case SDL_SCANCODE_LEFT: m_inputs.aim_left = 1; break;
						case SDL_SCANCODE_RIGHT: m_inputs.aim_right = 1; break;
						case SDL_SCANCODE_SPACE: m_inputs.up = 1; break;
						case SDL_SCANCODE_LCTRL: m_inputs.down = 1; break;
						case SDL_SCANCODE_LSHIFT: m_inputs.speed = 1; break;
						default: break;
					}
					break;
				case SDL_KEYUP:
					switch (e.key.keysym.scancode) {
						case SDL_SCANCODE_W: m_inputs.forward = 0; break;
						case SDL_SCANCODE_S: m_inputs.back = 0; break;
						case SDL_SCANCODE_A: m_inputs.left = 0; break;
						case SDL_SCANCODE_D: m_inputs.right = 0; break;
						case SDL_SCANCODE_UP: m_inputs.aim_up = 0; break;
						case SDL_SCANCODE_DOWN: m_inputs.aim_down = 0; break;
						case SDL_SCANCODE_LEFT: m_inputs.aim_left = 0; break;
						case SDL_SCANCODE_RIGHT: m_inputs.aim_right = 0; break;
						case SDL_SCANCODE_SPACE: m_inputs.up = 0; break;
						case SDL_SCANCODE_LCTRL: m_inputs.down = 0; break;
						case SDL_SCANCODE_LSHIFT: m_inputs.speed = 0; break;
						default: break;
					}
					break;
				case SDL_MOUSEMOTION:
					m_yaw += static_cast<float>(e.motion.xrel) * mouse_sensitivity * mouse_yaw_coefficient;
					m_pitch += static_cast<float>(e.motion.yrel) * mouse_sensitivity * mouse_pitch_coefficient;
					m_pitch = std::clamp(m_pitch, pitch_min, pitch_max);
					m_forward_direction = direction_vector(m_yaw, m_pitch);
					m_right_direction = glm::cross(m_forward_direction, up_direction);
					break;
			}
		}
	}

	auto update(float delta_time, float move_acceleration, float move_drag, float yaw_speed, float pitch_speed) -> void {
		const auto input_forward = m_inputs.forward - m_inputs.back;
		const auto input_right = m_inputs.right - m_inputs.left;
		const auto input_up = m_inputs.up - m_inputs.down;
		const auto input_aim_up = m_inputs.aim_up - m_inputs.aim_down;
		const auto input_aim_right = m_inputs.aim_right - m_inputs.aim_left;

		m_yaw += static_cast<float>(input_aim_right) * yaw_speed * delta_time;
		m_pitch += static_cast<float>(input_aim_up) * pitch_speed * delta_time;
		m_pitch = std::clamp(m_pitch, pitch_min, pitch_max);
		m_forward_direction = direction_vector(m_yaw, m_pitch);
		m_right_direction = glm::cross(m_forward_direction, up_direction);

		m_acceleration = {};
		if (input_forward == 0 && input_right == 0 && input_up == 0) {
			if (glm::dot(m_velocity, m_velocity) < min_speed_squared) {
				m_velocity = {};
			} else {
				m_acceleration = m_velocity * -move_drag;
			}
		} else {
			const auto input_direction = m_forward_direction * static_cast<float>(input_forward) + m_right_direction * static_cast<float>(input_right) +
				up_direction * static_cast<float>(input_up);
			const auto input_acceleration = move_acceleration + static_cast<float>(m_inputs.speed) * move_acceleration;
			m_acceleration = glm::normalize(input_direction) * input_acceleration - m_velocity * move_drag;
		}

		const auto new_velocity = m_velocity + m_acceleration * delta_time;
		const auto average_velocity = (m_velocity + new_velocity) * 0.5f;
		m_velocity = new_velocity;
		m_position += average_velocity * delta_time;
	}

	[[nodiscard]] auto controlling() const noexcept -> bool {
		return m_inputs.controlling;
	}

	[[nodiscard]] auto position() const noexcept -> vec3 {
		return m_position;
	}

	[[nodiscard]] auto forward() const noexcept -> vec3 {
		return m_forward_direction;
	}

	[[nodiscard]] auto right() const noexcept -> vec3 {
		return m_right_direction;
	}

	[[nodiscard]] auto up() const noexcept -> vec3 {
		return up_direction;
	}

private:
	struct inputs final {
		signed char forward = 0;
		signed char back = 0;
		signed char left = 0;
		signed char right = 0;
		signed char up = 0;
		signed char down = 0;
		signed char aim_up = 0;
		signed char aim_down = 0;
		signed char aim_left = 0;
		signed char aim_right = 0;
		signed char speed = 0;
		bool controlling = false;
	};

	static constexpr auto mouse_yaw_coefficient = 0.00038397244f;
	static constexpr auto mouse_pitch_coefficient = -0.00038397244f;
	static constexpr auto pitch_min = -1.570778847f;
	static constexpr auto pitch_max = 1.570778847f;
	static constexpr auto min_speed_squared = 0.01f;
	static constexpr auto up_direction = vec3{0.0f, 1.0f, 0.0f};

	[[nodiscard]] static auto direction_vector(float yaw, float pitch) noexcept -> vec3 {
		const auto pitch_cos = std::cos(pitch);
		return vec3{std::cos(yaw) * pitch_cos, std::sin(pitch), std::sin(yaw) * pitch_cos};
	}

	vec3 m_position;
	vec3 m_velocity{};
	vec3 m_acceleration{};
	vec3 m_forward_direction;
	vec3 m_right_direction;
	float m_yaw;
	float m_pitch;
	inputs m_inputs{};
};

#endif
