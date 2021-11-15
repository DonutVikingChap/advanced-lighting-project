#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "../core/glsl.hpp"
#include "../render/rendering_pipeline.hpp"
#include "../resources/font.hpp"
#include "../resources/framebuffer.hpp"
#include "../resources/viewport.hpp"
#include "asset_manager.hpp"
#include "render_loop.hpp"
#include "scene.hpp"

#include <cstddef>      // std::size_t
#include <cstdio>       // stderr
#include <fmt/format.h> // fmt::format, fmt::print
#include <glm/glm.hpp>  // glm::identity
#include <imgui.h>      // ImGui
#include <memory>       // std::shared_ptr
#include <span>         // std::span
#include <stdexcept>    // std::exception
#include <string>       // std::u8string
#include <string_view>  // std::u8string_view

class application final : public render_loop {
public:
	static constexpr auto options = render_loop_options{
		.window_title = "TSBK03 Advanced Lighting Project",
		.window_width = 1280,
		.window_height = 720,
		.window_resizable = true,
		.tick_rate = 60,
		.min_fps = 10,
		.max_fps = 240,
		.v_sync = false,
		.msaa_level = 4,
	};

	static constexpr auto vertical_fov = 1.57079633f;
	static constexpr auto near_z = 0.01f;
	static constexpr auto far_z = 1000.0f;

	explicit application(std::span<char*> arguments)
		: render_loop(arguments, options) {
		m_renderer.gui().enable();
	}

private:
	auto resize(int width, int height) -> void override {
		m_renderer.resize(width, height, vertical_fov, near_z, far_z);
		m_viewport = viewport{0, 0, width, height};
	}

	auto handle_event(const SDL_Event& e) -> void override {
		switch (e.type) {
			case SDL_KEYDOWN:
				if (e.key.keysym.scancode == SDL_SCANCODE_Z || e.key.keysym.sym == SDLK_ESCAPE) {
					toggle_gui();
				}
				break;
			case SDL_WINDOWEVENT:
				if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
					enable_gui();
				}
				break;
			default: break;
		}
		m_scene.handle_event(e);
		m_renderer.handle_event(e);
	}

	auto tick(unsigned int tick_count, float delta_time) -> void override {
		m_scene.tick(tick_count, delta_time);
	}

	auto update(float elapsed_time, float delta_time) -> void override {
		m_scene.update(elapsed_time, delta_time);
		m_renderer.update();
	}

	auto display() -> void override {
		if (m_renderer.gui().enabled()) {
			ImGui::ShowDemoWindow();

			ImGui::Begin("Application");
			if (ImGui::SliderFloat("FPS Limit", &m_max_fps, 0.0f, 1000.0f)) {
				set_max_fps(m_max_fps);
			}
			if (ImGui::Button("Reload shaders")) {
				try {
					auto width = 0;
					auto height = 0;
					SDL_GetWindowSize(get_window(), &width, &height);
					m_asset_manager.reload_shaders();
					m_renderer.reload_shaders(width, height, vertical_fov, near_z, far_z);
				} catch (const std::exception& e) {
					fmt::print(stderr, "Failed to reload shaders: {}\n", e.what());
				} catch (...) {
					fmt::print(stderr, "Failed to reload shaders!\n");
				}
			}
			ImGui::End();
		}
		m_scene.draw(m_renderer);
		draw_fps_counter();
		m_renderer.render(framebuffer::get_default(), m_viewport, m_scene.view_matrix(), m_scene.view_position());
	}

	auto enable_gui() -> void {
		m_scene.controller().stop_controlling();
		m_renderer.gui().enable();
	}

	auto disable_gui() -> void {
		m_scene.controller().start_controlling();
		m_renderer.gui().disable();
	}

	auto toggle_gui() -> void {
		if (m_renderer.gui().enabled()) {
			disable_gui();
		} else {
			enable_gui();
		}
	}

	auto draw_fps_counter() -> void {
		auto fps_color = vec4{0.0f, 1.0f, 0.0f, 1.0f};
		auto fps_icon = std::u8string_view{u8"✅"};
		const auto fps = latest_measured_fps();
		if (fps < 60) {
			fps_color = {1.0f, 0.0f, 0.0f, 1.0f};
			fps_icon = u8"❌";
		} else if (fps < 120) {
			fps_color = {1.0f, 1.0f, 0.0f, 1.0f};
			fps_icon = u8"⚠";
		} else if (fps < 240) {
			fps_color = vec4{1.0f, 1.0f, 1.0f, 1.0f};
			fps_icon = u8"▶";
		} else if (fps < 1000) {
			fps_color = vec4{1.0f, 1.0f, 1.0f, 1.0f};
			fps_icon = u8"⏩";
		}
		m_renderer.text().draw_text(m_main_font, {2.0f, 27.0f}, {1.0f, 1.0f}, fps_color, fmt::format("     FPS: {}", fps));
		m_renderer.text().draw_text(m_emoji_font, {2.0f, 27.0f}, {1.0f, 1.0f}, fps_color, std::u8string{fps_icon});
	}

	asset_manager m_asset_manager{};
	rendering_pipeline m_renderer{get_window(), get_gl_context()};
	std::shared_ptr<font> m_main_font = m_asset_manager.load_font("assets/fonts/liberation/LiberationSans-Regular.ttf", 32u);
	std::shared_ptr<font> m_emoji_font = m_asset_manager.load_font("assets/fonts/noto-emoji/NotoEmoji-Regular.ttf", 32u);
	scene m_scene{m_asset_manager};
	viewport m_viewport{};
	float m_max_fps = options.max_fps;
};

#endif
