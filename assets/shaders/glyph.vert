layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_texture_coordinates;
layout (location = 2) in vec2 instance_offset;
layout (location = 3) in vec2 instance_scale;
layout (location = 4) in vec2 instance_texture_offset;
layout (location = 5) in vec2 instance_texture_scale;
layout (location = 6) in vec4 instance_color;

out vec2 io_texture_coordinates;
out vec4 io_color;

uniform mat4 projection_matrix;

void main() {
	io_texture_coordinates = instance_texture_offset + in_texture_coordinates * instance_texture_scale;
	io_color = instance_color;
	gl_Position = projection_matrix * vec4(instance_offset + in_position * instance_scale, 1.0, 1.0);
}
