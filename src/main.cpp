#include "asset_manager.hpp"
#include "font.hpp"
#include "framebuffer.hpp"
#include "render_loop.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "viewport.hpp"

#include <cstddef>      // std::size_t
#include <cstdio>       // stderr
#include <cstdlib>      // EXIT_SUCCESS, EXIT_FAILURE
#include <fmt/format.h> // fmt::...
#include <glm/glm.hpp>  // glm::...
#include <imgui.h>      // ImGui
#include <memory>       // std::shared_ptr
#include <span>         // std::span
#include <stdexcept>    // std::exception

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
		.print_fps = false,
		.v_sync = false,
		.msaa_level = 0,
	};

	explicit application(std::span<char*> arguments)
		: render_loop(arguments, options) {}

private:
	auto resize(int width, int height) -> void override {
		m_renderer.resize(width, height);
		m_viewport = viewport{0, 0, width, height};
	}

	auto handle_event(const SDL_Event& e) -> void override {
		// TODO
		(void)e;
		m_renderer.handle_event(e);
	}

	auto tick(unsigned int tick_count, float delta_time) -> void override {
		// TODO
		(void)tick_count;
		(void)delta_time;
	}

	auto update(float elapsed_time, float delta_time) -> void override {
		// TODO
		(void)elapsed_time;
		(void)delta_time;
		(void)m_scene;
		m_renderer.update();
	}

	auto display() -> void override {
		// TODO
		ImGui::ShowDemoWindow();
		(void)m_scene;
		m_renderer.draw_text(m_main_font, {2.0f, 27.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, fmt::format("FPS: {}", latest_measured_fps()));
		m_renderer.draw_text(m_main_font, {200.0f, 600.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 1.0f, 1.0f}, "Some text. Wow!\n(Cool, even parentheses work)");
		m_renderer.draw_text(m_main_font, {100.0f, 300.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 1.0f, 1.0f}, u8"Here's some crazy UTF-8 text that will probably break everything:");
		m_renderer.draw_text(m_japanese_font, {100.0f, 332.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 1.0f, 1.0f}, u8"雨にもまけず");
		m_renderer.draw_text(m_main_font, {100.0f, 364.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 1.0f, 1.0f}, u8"And here's some more:");
		m_renderer.draw_text(m_arabic_font, {100.0f, 396.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 1.0f, 1.0f}, u8"التفاصيل");
		m_renderer.draw_text(m_emoji_font, {100.0f, 428.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 1.0f, 1.0f}, u8"😀😃😄😁😆");
		m_renderer.draw_text(m_emoji_font, {100.0f, 460.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 1.0f, 1.0f}, u8"👩🏾‍💻");
		m_renderer.render(framebuffer::get_default(), m_viewport, glm::identity<glm::mat4>());
	}

	asset_manager m_asset_manager{};
	renderer m_renderer{m_asset_manager.load_quad_mesh(), get_window(), get_gl_context()};
	std::shared_ptr<font> m_main_font = m_asset_manager.load_font("assets/fonts/liberation/LiberationSans-Regular.ttf", 32u);
	std::shared_ptr<font> m_arabic_font = m_asset_manager.load_font("assets/fonts/noto/NotoSansArabic-Regular.ttf", 32u);
	std::shared_ptr<font> m_japanese_font = m_asset_manager.load_font("assets/fonts/noto/NotoSansJP-Regular.otf", 32u);
	std::shared_ptr<font> m_emoji_font = m_asset_manager.load_font("assets/fonts/noto-emoji/NotoEmoji-Regular.ttf", 32u);
	scene m_scene{};
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
