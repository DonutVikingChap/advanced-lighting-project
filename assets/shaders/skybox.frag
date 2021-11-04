in vec3 io_texture_coordinates;

out vec4 out_fragment_color;

uniform samplerCube skybox_texture;

void main() {
    out_fragment_color = texture(skybox_texture, io_texture_coordinates);
}
