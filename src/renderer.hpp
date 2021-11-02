#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "font.hpp"
#include "framebuffer.hpp"
#include "glsl.hpp"
#include "gui_renderer.hpp"
#include "mesh.hpp"
#include "model_renderer.hpp"
#include "opengl.hpp"
#include "text_renderer.hpp"
#include "texture.hpp"
#include "viewport.hpp"

#include <SDL.h>       // SDL_...
#include <memory>      // std::shared_ptr
#include <string>      // std::u8string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

class renderer final {
public:
	renderer(std::shared_ptr<quad> quad, SDL_Window* window, SDL_GLContext gl_context)
		: m_text_renderer(std::move(quad))
		, m_gui_renderer(window, gl_context) {
		glEnable(GL_CULL_FACE);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ZERO);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		glEnable(GL_STENCIL_TEST);
		glStencilMask(0x00);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		glEnable(GL_MULTISAMPLE);

		opengl_context::check_status();
	}

	auto resize(int width, int height, float vertical_fov, float near_z, float far_z) -> void {
		m_model_renderer.resize({}, width, height, vertical_fov, near_z, far_z);
		m_text_renderer.resize({}, width, height);
	}

	auto handle_event(const SDL_Event& e) -> void {
		m_gui_renderer.handle_event({}, e);
	}

	auto update() -> void {
		m_gui_renderer.update({});
	}

	auto render(framebuffer& target, const viewport& viewport, const mat4& view_matrix, vec3 view_position) -> void {
		glBindFramebuffer(GL_FRAMEBUFFER, target.get());
		glViewport(static_cast<GLint>(viewport.x), static_cast<GLint>(viewport.y), static_cast<GLsizei>(viewport.w), static_cast<GLsizei>(viewport.h));

		glStencilMask(0xFF);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glStencilMask(0x00);

		m_model_renderer.render({}, view_matrix, view_position);
		m_text_renderer.render({}, viewport);
		m_gui_renderer.render({});
	}

	[[nodiscard]] auto model() -> model_renderer& {
		return m_model_renderer;
	}

	[[nodiscard]] auto text() -> text_renderer& {
		return m_text_renderer;
	}

	[[nodiscard]] auto gui() -> gui_renderer& {
		return m_gui_renderer;
	}

private:
	model_renderer m_model_renderer{};
	text_renderer m_text_renderer;
	gui_renderer m_gui_renderer;
};

#endif
