#include "math.glsl"
#include "pbr.glsl"

in vec2 io_texture_coordinates;

out vec2 out_fragment_color;

void main() {
    float n_dot_v = io_texture_coordinates.x;
    float roughness = io_texture_coordinates.y;
	vec3 v = vec3(sqrt(1.0 - square(n_dot_v)), 0.0, n_dot_v);
    vec3 n = vec3(0.0, 0.0, 1.0);
    vec2 result = vec2(0.0, 0.0);
    for (uint i = 0u; i < uint(SAMPLE_COUNT); ++i) {
        vec3 h = importance_sample_ggx(hammersley(i, uint(SAMPLE_COUNT)), n, roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);
        float n_dot_l = max(l.z, 0.0);
        float n_dot_h = max(h.z, 0.0);
        float v_dot_h = max(dot(v, h), 0.0);
	    float n_dot_v = max(dot(n, v), 0.0);
        if (n_dot_l > 0.0) {
            float g_vis = (geometry_smith_v2(n_dot_v, n_dot_l, roughness) * v_dot_h) / (n_dot_h * n_dot_v);
            float f_c = pow(1.0 - v_dot_h, 5.0);
            result.x += (1.0 - f_c) * g_vis;
            result.y += f_c * g_vis;
        }
    }
    result *= (1.0 / float(SAMPLE_COUNT));
    out_fragment_color = result;
}
