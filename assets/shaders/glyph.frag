#version 330 core

in vec2 io_texture_coordinates;

out vec4 out_fragment_color;

uniform sampler2D text_texture;
uniform vec4 text_color;

void main() {
	out_fragment_color = vec4(text_color.xyz, text_color.w * texture(text_texture, io_texture_coordinates).r);
}
