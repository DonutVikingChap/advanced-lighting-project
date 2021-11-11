in vec3 io_texture_coordinates;

out vec4 out_fragment_color;

uniform sampler2D equirectangular_texture;

void main() {
	vec3 r = normalize(io_texture_coordinates);
	vec2 uv = vec2(atan(r.z, r.x) * 0.1591, asin(r.y) * 0.3183) + 0.5;
	out_fragment_color = vec4(texture(equirectangular_texture, uv).rgb, 1.0);
}
