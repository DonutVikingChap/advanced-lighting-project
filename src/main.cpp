#include "asset_manager.hpp"
#include "font.hpp"
#include "framebuffer.hpp"
#include "glsl.hpp"
#include "render_loop.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "viewport.hpp"

#include <chrono>       // TODO: Remove
#include <cstddef>      // std::size_t
#include <cstdio>       // stderr
#include <cstdlib>      // EXIT_SUCCESS, EXIT_FAILURE
#include <fmt/format.h> // fmt::format, fmt::print
#include <glm/glm.hpp>  // glm::identity
#include <imgui.h>      // ImGui
#include <memory>       // std::shared_ptr
#include <span>         // std::span
#include <stdexcept>    // std::exception
#include <thread>       // TODO: Remove

class application final : public render_loop {
public:
	static constexpr auto options = render_loop_options{
		.window_title = "TSBK03 Advanced Lighting Project",
		.window_width = 1280,
		.window_height = 720,
		.window_resizable = true,
		.tick_rate = 60,
		.min_fps = 10,
		.max_fps = 1200,
		.v_sync = false,
		.msaa_level = 0,
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

			ImGui::Begin("Shaders");
			if (ImGui::Button("Reload shaders")) {
				try {
					auto width = 0;
					auto height = 0;
					SDL_GetWindowSize(get_window(), &width, &height);
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
		{
			auto fps_color = vec4{0.0f, 1.0f, 0.0f, 1.0f};
			auto fps_icon = u8"✅";
			const auto fps = latest_measured_fps();
			if (fps < 60.0f) {
				fps_color = {1.0f, 0.0f, 0.0f, 1.0f};
				fps_icon = u8"❌";
			} else if (fps < 120.0f) {
				fps_color = {1.0f, 1.0f, 0.0f, 1.0f};
				fps_icon = u8"⚠";
			} else if (fps < 240.0f) {
				fps_color = vec4{1.0f, 1.0f, 1.0f, 1.0f};
				fps_icon = u8"▶";
			} else if (fps < 1000.0f) {
				fps_color = vec4{1.0f, 1.0f, 1.0f, 1.0f};
				fps_icon = u8"⏩";
			}
			m_renderer.text().draw_text(m_main_font, {2.0f, 27.0f}, {1.0f, 1.0f}, fps_color, fmt::format("     FPS: {}", fps));
			m_renderer.text().draw_text(m_emoji_font, {2.0f, 27.0f}, {1.0f, 1.0f}, fps_color, fps_icon);
		}
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

	asset_manager m_asset_manager{};
	renderer m_renderer{m_asset_manager.load_cube_map_mesh(), m_asset_manager.load_quad_mesh(), get_window(), get_gl_context()};
	std::shared_ptr<font> m_main_font = m_asset_manager.load_font("assets/fonts/liberation/LiberationSans-Regular.ttf", 32u);
	std::shared_ptr<font> m_emoji_font = m_asset_manager.load_font("assets/fonts/noto-emoji/NotoEmoji-Regular.ttf", 32u);
	scene m_scene{m_asset_manager};
	viewport m_viewport{};
};

auto main(int argc, char* argv[]) -> int {
	try {
		application{std::span{argv, static_cast<std::size_t>(argc)}}.run();
	} catch (const std::exception& e) {
		fmt::print(stderr, "Fatal error: {}\n", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		fmt::print(stderr, "Fatal error!\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
