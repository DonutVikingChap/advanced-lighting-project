#ifndef TEXT_HPP
#define TEXT_HPP

#include "font.hpp"
#include "framebuffer.hpp"
#include "mesh.hpp"
#include "opengl.hpp"
#include "shader.hpp"
#include "utf8.hpp"
#include "viewport.hpp"

#include <cmath>                // std::round, std::floor
#include <glm/glm.hpp>          // glm::...
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <memory>               // std::shared_ptr
#include <string>               // std::u8string
#include <string_view>          // std::string_view
#include <unordered_map>        // std::unordered_map
#include <utility>              // std::move

class text_renderer final {
public:
	explicit text_renderer(std::shared_ptr<textured_2d_mesh> quad_mesh)
		: m_quad_mesh(std::move(quad_mesh)) {
		glUseProgram(m_glyph_shader.program.get());
		glUniform1i(m_glyph_shader.text_texture_location, 0);
		glUseProgram(0);
	}

	auto resize(int width, int height) -> void { // NOLINT(readability-make-member-function-const)
		glUseProgram(m_glyph_shader.program.get());
		const auto projection_matrix = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
		glUniformMatrix4fv(m_glyph_shader.projection_matrix_location, 1, GL_FALSE, glm::value_ptr(projection_matrix));
		glUseProgram(0);
	}

	auto draw_text(std::shared_ptr<font> font, glm::vec2 offset, glm::vec2 scale, glm::vec4 color, std::u8string str) -> void {
		m_text_instances[std::move(font)].emplace_back(offset, scale, color, std::move(str));
	}

	auto draw_text(std::shared_ptr<font> font, glm::vec2 offset, glm::vec2 scale, glm::vec4 color, std::string_view str) -> void {
		m_text_instances[std::move(font)].emplace_back(offset, scale, color, std::u8string{str.begin(), str.end()});
	}

	auto render(framebuffer& target, const viewport& viewport) -> void {
		glBindFramebuffer(GL_FRAMEBUFFER, target.get());
		glViewport(static_cast<GLint>(viewport.x), static_cast<GLint>(viewport.y), static_cast<GLsizei>(viewport.w), static_cast<GLsizei>(viewport.h));

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(m_glyph_shader.program.get());
		glBindVertexArray(m_quad_mesh->get());

		glActiveTexture(GL_TEXTURE0);

		for (const auto& [font, texts] : m_text_instances) {
			glBindTexture(GL_TEXTURE_2D, font->atlas().get());
			for (const auto& text : texts) {
				render_text(*font, text, viewport);
			}
		}
		m_text_instances.clear();

		glBindTexture(GL_TEXTURE_2D, 0);

		glBindVertexArray(0);
		glUseProgram(0);

		glBlendFunc(GL_ONE, GL_ZERO);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

private:
	struct glyph_shader final {
		shader_program program{"assets/shaders/glyph.vert", "assets/shaders/glyph.frag"};
		GLint projection_matrix_location = glGetUniformLocation(program.get(), "projection_matrix");
		GLint offset_location = glGetUniformLocation(program.get(), "offset");
		GLint scale_location = glGetUniformLocation(program.get(), "scale");
		GLint texture_offset_location = glGetUniformLocation(program.get(), "texture_offset");
		GLint texture_scale_location = glGetUniformLocation(program.get(), "texture_scale");
		GLint text_texture_location = glGetUniformLocation(program.get(), "text_texture");
		GLint text_color_location = glGetUniformLocation(program.get(), "text_color");
	};

	struct text_instance final {
		text_instance(glm::vec2 offset, glm::vec2 scale, glm::vec4 color, std::u8string str)
			: offset(offset)
			, scale(scale)
			, color(color)
			, str(std::move(str)) {}

		glm::vec2 offset;
		glm::vec2 scale;
		glm::vec4 color;
		std::u8string str;
	};

	auto render_text(font& font, const text_instance& text, const viewport& viewport) const -> void {
		glUniform4fv(m_glyph_shader.text_color_location, 1, glm::value_ptr(text.color));
		auto x = text.offset.x;
		auto y = text.offset.y;
		const auto top = static_cast<float>(viewport.h);
		const auto code_points = utf8_view{text.str};
		for (auto it = code_points.begin(); it != code_points.end();) {
			if (const auto ch = *it++; ch == '\n') {
				x = text.offset.x;
				y += font.line_space() * text.scale.y;
			} else {
				const auto& glyph = font.load_glyph(ch);
				const auto glyph_scale = glm::vec2{
					std::round(glyph.size.x * text.scale.x),
					std::round(glyph.size.y * text.scale.y),
				};
				const auto glyph_offset = glm::vec2{
					std::floor(x) + std::round(glyph.bearing.x * text.scale.x),
					top - std::floor(y) + std::round(glyph.bearing.y * text.scale.y) - glyph_scale.y,
				};
				glUniform2fv(m_glyph_shader.offset_location, 1, glm::value_ptr(glyph_offset));
				glUniform2fv(m_glyph_shader.scale_location, 1, glm::value_ptr(glyph_scale));
				glUniform2fv(m_glyph_shader.texture_offset_location, 1, glm::value_ptr(glyph.texture_offset));
				glUniform2fv(m_glyph_shader.texture_scale_location, 1, glm::value_ptr(glyph.texture_scale));
				glDrawArrays(GL_TRIANGLES, 0, 6);
				x += (glyph.advance + font.kerning(ch, (it == code_points.end()) ? 0 : *it)) * text.scale.x;
			}
		}
	}

	std::shared_ptr<textured_2d_mesh> m_quad_mesh;
	glyph_shader m_glyph_shader{};
	std::unordered_map<std::shared_ptr<font>, std::vector<text_instance>> m_text_instances{};
};

#endif
