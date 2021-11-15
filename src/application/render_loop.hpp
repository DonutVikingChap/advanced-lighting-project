#ifndef RENDER_LOOP_HPP
#define RENDER_LOOP_HPP

#include "../core/opengl.hpp"

#include <GL/glew.h>    // GLEW_..., glew...
#include <SDL.h>        // SDL_..., Uint32, Uint64
#include <algorithm>    // std::min
#include <cmath>        // std::ceil
#include <fmt/format.h> // fmt::format
#include <memory>       // std::unique_ptr
#include <span>         // std::span
#include <stdexcept>    // std::runtime_error
#include <type_traits>  // std::remove_pointer_t

struct render_loop_error : std::runtime_error {
	explicit render_loop_error(const auto& message)
		: std::runtime_error(message) {}
};

struct render_loop_options final {
	const char* window_title = "";
	int window_width = 1280;
	int window_height = 720;
	bool window_resizable = true;
	float tick_rate = 60;
	float min_fps = 10;
	float max_fps = 240;
	bool v_sync = false;
	int msaa_level = 0;
};

class render_loop {
public:
	render_loop(std::span<char*> arguments, const render_loop_options& options)
		: m_clock_frequency(SDL_GetPerformanceFrequency())
		, m_clock_interval(1.0f / static_cast<float>(m_clock_frequency))
		, m_tick_interval(static_cast<Uint64>(std::ceil(static_cast<float>(m_clock_frequency) / options.tick_rate)))
		, m_tick_delta_time(static_cast<float>(m_tick_interval) * m_clock_interval)
		, m_min_frame_interval((options.max_fps == 0.0f) ? Uint64{0} : static_cast<Uint64>(std::ceil(static_cast<float>(m_clock_frequency) / options.max_fps)))
		, m_max_ticks_per_frame((options.tick_rate <= options.min_fps) ? Uint64{1} : static_cast<Uint64>(options.tick_rate / options.min_fps)) {
		(void)arguments; // TODO

		constexpr auto set_attribute = [](SDL_GLattr attr, int value) -> void {
			if (SDL_GL_SetAttribute(attr, value) != 0) {
				throw std::runtime_error{fmt::format("Failed to set OpenGL attribute: {}", SDL_GetError())};
			}
		};
		set_attribute(SDL_GL_DOUBLEBUFFER, 1);
		set_attribute(SDL_GL_ACCELERATED_VISUAL, 1);
		set_attribute(SDL_GL_RED_SIZE, 8);
		set_attribute(SDL_GL_GREEN_SIZE, 8);
		set_attribute(SDL_GL_BLUE_SIZE, 8);
		set_attribute(SDL_GL_ALPHA_SIZE, 8);
		set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		set_attribute(SDL_GL_STENCIL_SIZE, 1);
		set_attribute(SDL_GL_MULTISAMPLEBUFFERS, options.msaa_level > 0);
		set_attribute(SDL_GL_MULTISAMPLESAMPLES, options.msaa_level);

		auto window_flags = Uint32{SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN};
		if (options.window_resizable) {
			window_flags |= SDL_WINDOW_RESIZABLE;
		}
		m_window.reset(SDL_CreateWindow(options.window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, options.window_width, options.window_height, window_flags));
		if (!m_window) {
			throw render_loop_error{fmt::format("Failed to create window: {}", SDL_GetError())};
		}
		m_gl_context.reset(SDL_GL_CreateContext(m_window.get()));
		if (!m_gl_context) {
			throw render_loop_error{fmt::format("Failed to create OpenGL context: {}", SDL_GetError())};
		}
		if (options.v_sync) {
			if (SDL_GL_SetSwapInterval(-1) != 0 && SDL_GL_SetSwapInterval(1) != 0) {
				throw render_loop_error{fmt::format("Failed to enable V-Sync: {}", SDL_GetError())};
			}
		} else {
			if (SDL_GL_SetSwapInterval(0) != 0) {
				throw render_loop_error{fmt::format("Failed to disable V-Sync: {}", SDL_GetError())};
			}
		}
		glewExperimental = GL_TRUE;
		if (const auto glew_error = glewInit(); glew_error != GLEW_OK) {
			throw render_loop_error{fmt::format("Failed to initialize GLEW: {}", glewGetErrorString(glew_error))};
		}
	}

	virtual ~render_loop() = default;

	render_loop(const render_loop&) = delete;
	render_loop(render_loop&&) = delete;
	auto operator=(const render_loop&) -> render_loop& = delete;
	auto operator=(render_loop&&) -> render_loop& = delete;

	auto run() -> void {
		m_start_time = SDL_GetPerformanceCounter();
		m_latest_tick_time = m_start_time;
		m_latest_frame_time = m_start_time;
		m_latest_fps_count_time = m_start_time;
		{
			auto width = 0;
			auto height = 0;
			SDL_GetWindowSize(m_window.get(), &width, &height);
			resize(width, height);
		}
		for (;;) {
			if (!run_frame()) {
				break;
			}
		}
	}

	auto set_max_fps(float max_fps) -> void {
		m_min_frame_interval = (max_fps == 0.0f) ? Uint64{0} : static_cast<Uint64>(std::ceil(static_cast<float>(m_clock_frequency) / max_fps));
	}

	[[nodiscard]] auto latest_measured_fps() const noexcept -> unsigned int {
		return m_latest_measured_fps;
	}

	[[nodiscard]] auto get_window() const noexcept -> SDL_Window* {
		return m_window.get();
	}

	[[nodiscard]] auto get_gl_context() const noexcept -> SDL_GLContext {
		return m_gl_context.get();
	}

private:
	struct sdl final {
		[[nodiscard]] sdl() {
			if (SDL_Init(SDL_INIT_VIDEO) != 0) {
				throw render_loop_error{"Failed to initialize SDL!"};
			}
		}
		~sdl() {
			SDL_Quit();
		}
		sdl(const sdl&) = delete;
		sdl(sdl&&) = delete;
		auto operator=(const sdl&) -> sdl& = delete;
		auto operator=(sdl&&) -> sdl& = delete;
	};

	struct sdl_initializer final {
		sdl_initializer() {
			static auto sdl_library = sdl{};
		}
	};

	virtual auto resize(int width, int height) -> void = 0;
	virtual auto handle_event(const SDL_Event& e) -> void = 0;
	virtual auto tick(unsigned int tick_count, float delta_time) -> void = 0;
	virtual auto update(float elapsed_time, float delta_time) -> void = 0;
	virtual auto display() -> void = 0;

	auto run_frame() -> bool {
		const auto current_time = SDL_GetPerformanceCounter();
		const auto time_since_latest_frame = current_time - m_latest_frame_time;
		if (current_time > m_latest_frame_time && time_since_latest_frame >= m_min_frame_interval) {
			m_latest_frame_time = current_time;
			++m_fps_count;
			if (current_time - m_latest_fps_count_time >= m_clock_frequency) {
				m_latest_fps_count_time = current_time;
				m_latest_measured_fps = m_fps_count;
				m_fps_count = 0;
			}

			for (auto e = SDL_Event{}; SDL_PollEvent(&e) != 0;) {
				switch (e.type) {
					case SDL_QUIT: return false;
					case SDL_WINDOWEVENT:
						if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
							resize(e.window.data1, e.window.data2);
						}
						break;
					default: break;
				}
				handle_event(e);
			}

			const auto time_since_latest_tick = current_time - m_latest_tick_time;
			auto ticks = time_since_latest_tick / m_tick_interval;
			m_latest_tick_time += ticks * m_tick_interval;
			for (ticks = std::min(ticks, m_max_ticks_per_frame); ticks > 0; --ticks) {
				++m_tick_count;
				tick(m_tick_count, m_tick_delta_time);
			}

			const auto elapsed_time = static_cast<float>(current_time - m_start_time) * m_clock_interval;
			const auto delta_time = static_cast<float>(time_since_latest_frame) * m_clock_interval;
			update(elapsed_time, delta_time);
			display();
			SDL_GL_SwapWindow(m_window.get());
		}
		return true;
	}

	struct window_deleter final {
		auto operator()(SDL_Window* p) const noexcept -> void {
			SDL_DestroyWindow(p);
		}
	};
	using window_ptr = std::unique_ptr<SDL_Window, window_deleter>;

	struct context_deleter final {
		auto operator()(SDL_GLContext p) const noexcept -> void {
			SDL_GL_DeleteContext(p);
		}
	};
	using context_ptr = std::unique_ptr<std::remove_pointer_t<SDL_GLContext>, context_deleter>;

	window_ptr m_window{};
	context_ptr m_gl_context{};
	[[no_unique_address]] sdl_initializer m_sdl_initializer{}; // NOLINT(clang-diagnostic-unknown-attributes)
	Uint64 m_clock_frequency;
	float m_clock_interval;
	Uint64 m_tick_interval;
	float m_tick_delta_time;
	Uint64 m_min_frame_interval;
	Uint64 m_max_ticks_per_frame;
	Uint64 m_start_time = 0;
	Uint64 m_latest_tick_time = 0;
	Uint64 m_latest_frame_time = 0;
	Uint64 m_latest_fps_count_time = 0;
	unsigned int m_latest_measured_fps = 0u;
	unsigned int m_tick_count = 0u;
	unsigned int m_fps_count = 0u;
};

#endif
