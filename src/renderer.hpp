#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "context.hpp"
#include "font.hpp"
#include "framebuffer.hpp"
#include "gui.hpp"
#include "mesh.hpp"
#include "opengl.hpp"
#include "text.hpp"
#include "texture.hpp"
#include "viewport.hpp"

#include <SDL.h>       // SDL_...
#include <glm/glm.hpp> // glm::...
#include <memory>      // std::shared_ptr
#include <string>      // std::u8string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

class renderer final {
public:
	renderer(std::shared_ptr<textured_2d_mesh> quad_mesh, SDL_Window* window, SDL_GLContext gl_context)
		: m_text_renderer(std::move(quad_mesh))
		, m_gui_renderer(window, gl_context) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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
		// TODO: SRGB

		context::check_status();
	}

	auto resize(int width, int height) -> void {
		m_text_renderer.resize(width, height);
		// TODO: Update projection matrix.
	}

	auto handle_event(const SDL_Event& e) -> void {
		m_gui_renderer.handle_event(e);
	}

	auto update() -> void {
		m_gui_renderer.update();
	}

	auto draw_text(std::shared_ptr<font> font, glm::vec2 offset, glm::vec2 scale, glm::vec4 color, std::u8string str) -> void {
		m_text_renderer.draw_text(std::move(font), offset, scale, color, std::move(str));
	}

	auto draw_text(std::shared_ptr<font> font, glm::vec2 offset, glm::vec2 scale, glm::vec4 color, std::string_view str) -> void {
		m_text_renderer.draw_text(std::move(font), offset, scale, color, str);
	}

	auto render(framebuffer& target, const viewport& viewport, const glm::mat4& view_matrix) -> void {
		glBindFramebuffer(GL_FRAMEBUFFER, target.get());
		glViewport(static_cast<GLint>(viewport.x), static_cast<GLint>(viewport.y), static_cast<GLsizei>(viewport.w), static_cast<GLsizei>(viewport.h));

		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glStencilMask(0x00);

		// TODO
		(void)view_matrix;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		m_text_renderer.render(target, viewport);
		m_gui_renderer.render();
	}

private:
	text_renderer m_text_renderer;
	gui_renderer m_gui_renderer;
};

#endif
