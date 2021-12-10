#ifndef RENDERING_PIPELINE_HPP
#define RENDERING_PIPELINE_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/camera.hpp"
#include "../resources/font.hpp"
#include "../resources/framebuffer.hpp"
#include "../resources/mesh.hpp"
#include "../resources/texture.hpp"
#include "../resources/viewport.hpp"
#include "gui_renderer.hpp"
#include "model_renderer.hpp"
#include "shadow_renderer.hpp"
#include "skybox_renderer.hpp"
#include "text_renderer.hpp"

#include <SDL.h>        // SDL_...
#include <cstdio>       // stderr
#include <fmt/format.h> // fmt::print
#include <memory>       // std::shared_ptr
#include <string>       // std::u8string
#include <string_view>  // std::string_view
#include <utility>      // std::move
#include <vector>       // std::vector

class rendering_pipeline final {
public:
	rendering_pipeline(SDL_Window* window, SDL_GLContext gl_context)
		: m_gui_renderer(window, gl_context) {
#ifndef NDEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(opengl_debug_output_callback, nullptr);
#endif

		glEnable(GL_CULL_FACE);

		glDisable(GL_BLEND);
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

	auto resize(int width, int height) -> void {
		m_text_renderer.resize(width, height);
	}

	auto reload_shaders(int width, int height) -> void {
		m_shadow_renderer.reload_shaders();
		m_model_renderer.reload_shaders();
		m_skybox_renderer.reload_shaders();
		m_text_renderer.reload_shaders(width, height);
	}

	auto handle_event(const SDL_Event& e) -> void {
		m_gui_renderer.handle_event(e);
	}

	auto update() -> void {
		m_gui_renderer.update();
	}

	auto render(framebuffer& target, const viewport& viewport, const camera& camera) -> void {
		m_shadow_renderer.render(camera);

		glBindFramebuffer(GL_FRAMEBUFFER, target.get());
		glViewport(static_cast<GLint>(viewport.x), static_cast<GLint>(viewport.y), static_cast<GLsizei>(viewport.w), static_cast<GLsizei>(viewport.h));

		glStencilMask(0xFF);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glStencilMask(0x00);

		m_model_renderer.render(camera);
		m_skybox_renderer.render(camera.projection_matrix, mat3{camera.view_matrix});
		m_text_renderer.render();
		m_gui_renderer.render();
	}

	[[nodiscard]] auto shadow() -> shadow_renderer& {
		return m_shadow_renderer;
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
#ifndef NDEBUG
	static void GLAPIENTRY opengl_debug_output_callback(
		GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/) {
		if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
			if (type == GL_DEBUG_TYPE_ERROR) {
				fmt::print(stderr, "OpenGL ERROR: {}\n", message);
			} else {
				fmt::print(stderr, "OpenGL: {}\n", message);
			}
		}
	}
#endif

	shadow_renderer m_shadow_renderer{};
	model_renderer m_model_renderer{false};
	skybox_renderer m_skybox_renderer{};
	text_renderer m_text_renderer{};
	gui_renderer m_gui_renderer;
};

#endif
