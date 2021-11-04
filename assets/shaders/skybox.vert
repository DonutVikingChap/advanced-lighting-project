layout (location = 0) in vec3 in_position;

out vec3 io_texture_coordinates;

uniform mat4 projection_matrix;
uniform mat3 view_matrix;

void main() {
    io_texture_coordinates = vec3(in_position.xy, -in_position.z);
    gl_Position = (projection_matrix * mat4(view_matrix) * vec4(in_position, 1.0)).xyww;
}
