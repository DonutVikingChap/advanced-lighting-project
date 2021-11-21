#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../resources/font.hpp"
#include "../resources/glyph.hpp"
#include "../resources/shader.hpp"
#include "../utilities/utf8.hpp"

#include <cmath>                // std::round, std::floor
#include <glm/glm.hpp>          // glm::ortho
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <memory>               // std::shared_ptr
#include <string>               // std::u8string
#include <string_view>          // std::string_view
#include <unordered_map>        // std::unordered_map
#include <utility>              // std::move
#include <vector>               // std::vector

class text_renderer final {
public:
	auto resize(int width, int height) -> void {
		m_viewport_height = static_cast<float>(height);
		m_glyph_shader.resize(width, height);
	}

	auto reload_shaders(int width, int height) -> void {
		m_viewport_height = static_cast<float>(height);
		m_glyph_shader = glyph_shader{};
		m_glyph_shader.resize(width, height);
	}

	auto draw_text(std::shared_ptr<font> font, vec2 offset, vec2 scale, vec4 color, std::u8string str) -> void {
		m_text_instances[std::move(font)].emplace_back(offset, scale, color, std::move(str));
	}

	auto draw_text(std::shared_ptr<font> font, vec2 offset, vec2 scale, vec4 color, std::string_view str) -> void {
		m_text_instances[std::move(font)].emplace_back(offset, scale, color, std::u8string{str.begin(), str.end()});
	}

	auto render() -> void {
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(m_glyph_shader.program.get());
		glBindVertexArray(m_glyph_mesh.get());
		glBindBuffer(GL_ARRAY_BUFFER, m_glyph_mesh.get_instance_buffer());

		glActiveTexture(GL_TEXTURE0);

		for (const auto& [font, texts] : m_text_instances) {
			glBindTexture(GL_TEXTURE_2D, font->atlas_texture().get());
			m_glyph_instances.clear();
			for (const auto& text : texts) {
				add_glyph_instances(*font, text);
			}
			glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_glyph_instances.size() * sizeof(glyph_instance)), m_glyph_instances.data(), GL_DYNAMIC_DRAW);
			glDrawArraysInstanced(glyph_mesh::primitive_type, 0, static_cast<GLsizei>(glyph_mesh::vertices.size()), static_cast<GLsizei>(m_glyph_instances.size()));
		}
		m_text_instances.clear();

		glBlendFunc(GL_ONE, GL_ZERO);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
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
		shader_uniform text_texture{program.get(), "text_texture"};
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

	auto add_glyph_instances(font& font, const text_instance& text) -> void {
		auto x = text.offset.x;
		auto y = text.offset.y;
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
					m_viewport_height - std::floor(y) + std::round(glyph.bearing.y * text.scale.y) - glyph_scale.y,
				};
				m_glyph_instances.push_back(glyph_instance{glyph_offset, glyph_scale, glyph.texture_offset, glyph.texture_scale, text.color});
				x += (glyph.advance + font.kerning(ch, (it == code_points.end()) ? 0 : *it)) * text.scale.x;
			}
		}
	}

	using text_instance_map = std::unordered_map<std::shared_ptr<font>, std::vector<text_instance>>;

	glyph_mesh m_glyph_mesh{};
	glyph_shader m_glyph_shader{};
	text_instance_map m_text_instances{};
	std::vector<glyph_instance> m_glyph_instances{};
	float m_viewport_height = 0.0f;
};

#endif
