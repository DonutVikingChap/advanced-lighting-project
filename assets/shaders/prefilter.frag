#include "math.glsl"
#include "pbr.glsl"

in vec3 io_texture_coordinates;

out vec4 out_fragment_color;

uniform samplerCube cubemap_texture;
uniform float cubemap_resolution;
uniform float roughness;

void main() {
	vec3 n = normalize(io_texture_coordinates);
    vec3 r = n;
    vec3 v = r;
    vec3 result = vec3(0.0);
    float weight = 0.0;
    for (uint i = 0u; i < uint(SAMPLE_COUNT); ++i) {
        vec2 xi = hammersley(i, uint(SAMPLE_COUNT));
        vec3 h = importance_sample_ggx(xi, n, roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);
        float n_dot_l = max(dot(n, l), 0.0);
        if (n_dot_l > 0.0) {
            float n_dot_h = max(dot(n, h), 0.0);
            float h_dot_v = max(dot(h, v), 0.0);
            float d = distribution_ggx(n_dot_h, roughness);
            float pdf = d * n_dot_h / (4.0 * h_dot_v) + 0.0001;
            float sa_texel = 4.0 * pi / (6.0 * square(cubemap_resolution));
            float sa_sample = 1.0 / (float(SAMPLE_COUNT) * pdf * 0.0001);
            float mip_level = roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);
            result += textureLod(cubemap_texture, l, mip_level).rgb * n_dot_l;
            weight += n_dot_l;
        }
    }
    result /= weight;
	out_fragment_color = vec4(result, 1.0);
}
