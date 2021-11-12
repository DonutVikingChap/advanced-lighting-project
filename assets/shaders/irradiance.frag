#include "math.glsl"

in vec3 io_texture_coordinates;

out vec4 out_fragment_color;

uniform samplerCube cubemap_texture;

void main() {
	vec3 normal = normalize(io_texture_coordinates);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), normal));
    vec3 up = normalize(cross(normal, right));

    vec3 result = vec3(0.0);
    float sample_count = 0.0;
    for (float yaw = 0.0; yaw < 2.0 * pi; yaw += SAMPLE_DELTA_ANGLE) {
        float yaw_cos = cos(yaw);
        float yaw_sin = sin(yaw);
        for (float pitch = 0.0; pitch < 0.5 * pi; pitch += SAMPLE_DELTA_ANGLE) {
            float pitch_cos = cos(pitch);
            float pitch_sin = sin(pitch);
            vec3 direction = vec3(pitch_sin * yaw_cos, pitch_sin * yaw_sin, pitch_cos);
            vec3 sample_direction = direction.x * right + direction.y * up + direction.z * normal;
            result += texture(cubemap_texture, sample_direction).rgb * pitch_cos * pitch_sin;
            ++sample_count;
        }
    }
    result *= pi / sample_count;
	out_fragment_color = vec4(result, 1.0);
}
