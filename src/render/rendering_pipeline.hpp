#ifndef RENDERING_PIPELINE_HPP
#define RENDERING_PIPELINE_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/font.hpp"
#include "../resources/framebuffer.hpp"
#include "../resources/mesh.hpp"
#include "../resources/texture.hpp"
#include "../resources/viewport.hpp"
#include "gui_renderer.hpp"
#include "model_renderer.hpp"
#include "skybox_renderer.hpp"
#include "text_renderer.hpp"

#include <SDL.h>       // SDL_...
#include <memory>      // std::shared_ptr
#include <string>      // std::u8string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

class rendering_pipeline final {
public:
	rendering_pipeline(SDL_Window* window, SDL_GLContext gl_context)
		: m_gui_renderer(window, gl_context) {
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
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		opengl_context::check_status();
	}

	auto resize(int width, int height, float vertical_fov, float near_z, float far_z) -> void {
		m_model_renderer.resize({}, width, height, vertical_fov, near_z, far_z);
		m_skybox_renderer.resize({}, width, height, vertical_fov, near_z, far_z);
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
		m_skybox_renderer.render({}, mat3{view_matrix});
		m_text_renderer.render({}, viewport);
		m_gui_renderer.render({});
	}

	auto reload_shaders(int width, int height, float vertical_fov, float near_z, float far_z) -> void {
		m_model_renderer.reload_shaders(width, height, vertical_fov, near_z, far_z);
		m_skybox_renderer.reload_shaders(width, height, vertical_fov, near_z, far_z);
		m_text_renderer.reload_shaders(width, height);
	}

	[[nodiscard]] auto model() -> model_renderer& {
		return m_model_renderer;
	}

	[[nodiscard]] auto skybox() -> skybox_renderer& {
		return m_skybox_renderer;
	}

	[[nodiscard]] auto text() -> text_renderer& {
		return m_text_renderer;
	}

	[[nodiscard]] auto gui() -> gui_renderer& {
		return m_gui_renderer;
	}

private:
	model_renderer m_model_renderer{};
	skybox_renderer m_skybox_renderer{};
	text_renderer m_text_renderer{};
	gui_renderer m_gui_renderer;
};

#endif
