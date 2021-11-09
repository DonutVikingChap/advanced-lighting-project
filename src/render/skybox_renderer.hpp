#ifndef SKYBOX_RENDERER_HPP
#define SKYBOX_RENDERER_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/cubemap.hpp"
#include "../resources/shader.hpp"
#include "../resources/skybox.hpp"
#include "../utilities/passkey.hpp"

#include <glm/glm.hpp>          // glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <memory>               // std::shared_ptr
#include <utility>              // std::move

class rendering_pipeline;

class skybox_renderer final {
public:
	auto resize(passkey<rendering_pipeline>, int width, int height, float vertical_fov, float near_z, float far_z) -> void {
		m_skybox_shader.resize(width, height, vertical_fov, near_z, far_z);
	}

	auto draw_skybox(std::shared_ptr<cubemap> texture) -> void {
		m_skybox_texture = std::move(texture);
	}

	auto render(passkey<rendering_pipeline>, const mat3& view_matrix) -> void {
		if (m_skybox_texture) {
			glDepthFunc(GL_LEQUAL);

			glUseProgram(m_skybox_shader.program.get());
			glBindVertexArray(m_skybox_mesh.get());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_skybox_texture->get());

			glUniformMatrix3fv(m_skybox_shader.view_matrix.location(), 1, GL_FALSE, glm::value_ptr(view_matrix));

			glDrawArrays(skybox_mesh::primitive_type, 0, static_cast<GLsizei>(skybox_mesh::vertices.size()));

			glDepthFunc(GL_LESS);
			m_skybox_texture.reset();
		}
	}

	auto reload_shaders(int width, int height, float vertical_fov, float near_z, float far_z) -> void {
		m_skybox_shader = skybox_shader{};
		m_skybox_shader.resize(width, height, vertical_fov, near_z, far_z);
	}

private:
	struct skybox_shader final {
		skybox_shader() {
			glUseProgram(program.get());
			glUniform1i(skybox_texture.location(), 0);
		}

		auto resize(int width, int height, float vertical_fov, float near_z, float far_z) -> void { // NOLINT(readability-make-member-function-const)
			const auto aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
			const auto projection_matrix = glm::perspective(vertical_fov, aspect_ratio, near_z, far_z);
			glUseProgram(program.get());
			glUniformMatrix4fv(this->projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
		}

		shader_program program{{
			.vertex_shader_filename = "assets/shaders/skybox.vert",
			.fragment_shader_filename = "assets/shaders/skybox.frag",
		}};
		shader_uniform projection_matrix{program.get(), "projection_matrix"};
		shader_uniform view_matrix{program.get(), "view_matrix"};
		shader_uniform skybox_texture{program.get(), "skybox_texture"};
	};

	skybox_mesh m_skybox_mesh{};
	skybox_shader m_skybox_shader{};
	std::shared_ptr<cubemap> m_skybox_texture{};
};

#endif
