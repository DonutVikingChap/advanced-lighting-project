in vec2 io_texture_coordinates;
in vec4 io_color;

out vec4 out_fragment_color;

uniform sampler2D text_texture;

void main() {
	out_fragment_color = vec4(io_color.xyz, io_color.w * texture(text_texture, io_texture_coordinates).r);
}
