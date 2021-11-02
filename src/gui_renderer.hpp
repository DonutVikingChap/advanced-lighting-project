#ifndef GUI_HPP
#define GUI_HPP

#include "opengl.hpp"
#include "passkey.hpp"

#include <SDL.h>                // SDL_...
#include <imgui.h>              // ImGui
#include <imgui_impl_opengl3.h> // ImGui_ImplOpenGL3_...
#include <imgui_impl_sdl.h>     // ImGui_ImplSDL2_...
#include <memory>               // std::unique_ptr

class renderer;

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

	auto handle_event(passkey<renderer>, const SDL_Event& e) -> void {
		ImGui::SetCurrentContext(m_context.get());
		ImGui_ImplSDL2_ProcessEvent(&e);
	}

	auto update(passkey<renderer>) -> void {
		ImGui::SetCurrentContext(m_context.get());
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(m_window);
		ImGui::NewFrame();
	}

	auto render(passkey<renderer>) -> void {
		ImGui::SetCurrentContext(m_context.get());
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
};

#endif
