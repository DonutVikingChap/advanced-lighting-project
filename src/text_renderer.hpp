#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include "font.hpp"
#include "glsl.hpp"
#include "opengl.hpp"
#include "passkey.hpp"
#include "quad.hpp"
#include "shader.hpp"
#include "utf8.hpp"
#include "viewport.hpp"

#include <cmath>                // std::round, std::floor
#include <glm/glm.hpp>          // glm::ortho
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <memory>               // std::shared_ptr
#include <string>               // std::u8string
#include <string_view>          // std::string_view
#include <unordered_map>        // std::unordered_map
#include <utility>              // std::move

class renderer;

class text_renderer final {
public:
	explicit text_renderer(std::shared_ptr<quad_mesh> quad_mesh)
		: m_quad_mesh(std::move(quad_mesh)) {}

	auto resize(passkey<renderer>, int width, int height) -> void {
		m_glyph_shader.resize(width, height);
	}

	auto draw_text(std::shared_ptr<font> font, vec2 offset, vec2 scale, vec4 color, std::u8string str) -> void {
		m_text_instances[std::move(font)].emplace_back(offset, scale, color, std::move(str));
	}

	auto draw_text(std::shared_ptr<font> font, vec2 offset, vec2 scale, vec4 color, std::string_view str) -> void {
		m_text_instances[std::move(font)].emplace_back(offset, scale, color, std::u8string{str.begin(), str.end()});
	}

	auto render(passkey<renderer>, const viewport& viewport) -> void {
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

		glBlendFunc(GL_ONE, GL_ZERO);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}

	auto reload_shaders(int width, int height) -> void {
		m_glyph_shader = glyph_shader{};
		m_glyph_shader.resize(width, height);
	}

private:
	struct glyph_shader final {
		glyph_shader() {
			glUseProgram(program.get());
			glUniform1i(text_texture.location(), 0);
		}

		auto resize(int width, int height) -> void { // NOLINT(readability-make-member-function-const)
			const auto projection_matrix = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
			glUseProgram(program.get());
			glUniformMatrix4fv(this->projection_matrix.location(), 1, GL_FALSE, glm::value_ptr(projection_matrix));
		}

		shader_program program{{
			.vertex_shader_filename = "assets/shaders/glyph.vert",
			.fragment_shader_filename = "assets/shaders/glyph.frag",
		}};
		shader_uniform projection_matrix{program.get(), "projection_matrix"};
		shader_uniform offset{program.get(), "offset"};
		shader_uniform scale{program.get(), "scale"};
		shader_uniform texture_offset{program.get(), "texture_offset"};
		shader_uniform texture_scale{program.get(), "texture_scale"};
		shader_uniform text_texture{program.get(), "text_texture"};
		shader_uniform text_color{program.get(), "text_color"};
	};

	struct text_instance final {
		text_instance(vec2 offset, vec2 scale, vec4 color, std::u8string str)
			: offset(offset)
			, scale(scale)
			, color(color)
			, str(std::move(str)) {}

		vec2 offset;
		vec2 scale;
		vec4 color;
		std::u8string str;
	};

	auto render_text(font& font, const text_instance& text, const viewport& viewport) const -> void {
		glUniform4fv(m_glyph_shader.text_color.location(), 1, glm::value_ptr(text.color));
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
				const auto glyph_scale = vec2{
					std::round(glyph.size.x * text.scale.x),
					std::round(glyph.size.y * text.scale.y),
				};
				const auto glyph_offset = vec2{
					std::floor(x) + std::round(glyph.bearing.x * text.scale.x),
					top - std::floor(y) + std::round(glyph.bearing.y * text.scale.y) - glyph_scale.y,
				};
				glUniform2fv(m_glyph_shader.offset.location(), 1, glm::value_ptr(glyph_offset));
				glUniform2fv(m_glyph_shader.scale.location(), 1, glm::value_ptr(glyph_scale));
				glUniform2fv(m_glyph_shader.texture_offset.location(), 1, glm::value_ptr(glyph.texture_offset));
				glUniform2fv(m_glyph_shader.texture_scale.location(), 1, glm::value_ptr(glyph.texture_scale));
				glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(quad_mesh::vertex_count));
				x += (glyph.advance + font.kerning(ch, (it == code_points.end()) ? 0 : *it)) * text.scale.x;
			}
		}
	}

	using text_instance_map = std::unordered_map<std::shared_ptr<font>, std::vector<text_instance>>;

	std::shared_ptr<quad_mesh> m_quad_mesh;
	glyph_shader m_glyph_shader{};
	text_instance_map m_text_instances{};
};

#endif
