#version 330 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_texture_coordinates;

out vec2 io_texture_coordinates;

uniform mat4 projection_matrix;
uniform vec2 offset;
uniform vec2 scale;
uniform vec2 texture_offset;
uniform vec2 texture_scale;

void main() {
	io_texture_coordinates = texture_offset + in_texture_coordinates * texture_scale;
	gl_Position = projection_matrix * vec4(offset + in_position * scale, 1.0, 1.0);
}
