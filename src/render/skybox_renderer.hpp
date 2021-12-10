#ifndef SKYBOX_RENDERER_HPP
#define SKYBOX_RENDERER_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/cubemap.hpp"
#include "../resources/shader.hpp"

#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <memory>               // std::shared_ptr
#include <utility>              // std::move

class skybox_renderer final {
public:
	static constexpr auto gamma = 2.2f;

	auto draw_skybox(std::shared_ptr<cubemap_texture> texture) -> void {
		m_skybox_texture = std::move(texture);
	}

	auto render(const mat4& projection_matrix, const mat3& view_matrix) -> void {
		if (m_skybox_texture) {
			glDepthFunc(GL_LEQUAL);

			glUseProgram(m_skybox_shader.program.get());
			glBindVertexArray(m_cubemap_mesh.get());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_skybox_texture->get());

			glUniformMatrix4fv(m_skybox_shader.projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
			glUniformMatrix3fv(m_skybox_shader.view_matrix.location(), 1, GL_FALSE, glm::value_ptr(view_matrix));

			glDrawArrays(cubemap_mesh::primitive_type, 0, static_cast<GLsizei>(cubemap_mesh::vertices.size()));

			glDepthFunc(GL_LESS);
			m_skybox_texture.reset();
		}
	}

	auto reload_shaders() -> void {
		m_skybox_shader = skybox_shader{};
	}

private:
	struct skybox_shader final {
		skybox_shader() {
			glUseProgram(program.get());
			glUniform1i(skybox_texture.location(), 0);
		}

		shader_program program{{
			.vertex_shader_filename = "assets/shaders/skybox.vert",
			.fragment_shader_filename = "assets/shaders/skybox.frag",
			.definitions =
				{
					{"GAMMA", gamma},
				},
		}};
		shader_uniform projection_matrix{program.get(), "projection_matrix"};
		shader_uniform view_matrix{program.get(), "view_matrix"};
		shader_uniform skybox_texture{program.get(), "skybox_texture"};
	};

	cubemap_mesh m_cubemap_mesh{};
	skybox_shader m_skybox_shader{};
	std::shared_ptr<cubemap_texture> m_skybox_texture{};
};

#endif
