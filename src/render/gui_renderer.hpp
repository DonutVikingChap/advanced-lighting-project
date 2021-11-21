#ifndef GUI_RENDERER_HPP
#define GUI_RENDERER_HPP

#include "../core/opengl.hpp"

#include <SDL.h>                // SDL_...
#include <imgui.h>              // ImGui
#include <imgui_impl_opengl3.h> // ImGui_ImplOpenGL3_...
#include <imgui_impl_sdl.h>     // ImGui_ImplSDL2_...
#include <memory>               // std::unique_ptr

class gui_renderer final {
public:
	gui_renderer(SDL_Window* window, SDL_GLContext gl_context)
		: m_window(window) {
		auto* const context = ImGui::CreateContext();
		ImGui_ImplSDL2_InitForOpenGL(m_window, gl_context);
		ImGui_ImplOpenGL3_Init();
		m_context.reset(context);
		ImGui::SetCurrentContext(m_context.get());
		ImGui::StyleColorsDark();
	}

	auto handle_event(const SDL_Event& e) -> void {
		ImGui::SetCurrentContext(m_context.get());
		ImGui_ImplSDL2_ProcessEvent(&e);
	}

	auto update() -> void {
		ImGui::SetCurrentContext(m_context.get());
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(m_window);
		ImGui::NewFrame();
	}

	auto render() -> void {
		ImGui::SetCurrentContext(m_context.get());
		if (enabled()) {
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		} else {
			ImGui::EndFrame();
		}
	}

	[[nodiscard]] auto enabled() const noexcept -> bool {
		return m_enabled;
	}

	auto enabled(bool enable) noexcept -> void {
		m_enabled = enable;
	}

	auto enable() noexcept -> void {
		m_enabled = true;
	}

	auto disable() noexcept -> void {
		m_enabled = false;
	}

	auto toggle_enabled() noexcept -> void {
		m_enabled = !m_enabled;
	}

private:
	struct context_deleter final {
		auto operator()(ImGuiContext* p) const noexcept -> void {
			ImGui::SetCurrentContext(p);
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
		}
	};
	using context_ptr = std::unique_ptr<ImGuiContext, context_deleter>;

	SDL_Window* m_window;
	context_ptr m_context{};
	bool m_enabled = false;
};

#endif
