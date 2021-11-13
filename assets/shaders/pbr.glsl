#ifndef PBR_GLSL
#define PBR_GLSL

#include "math.glsl"

const float specular_epsilon = 0.00001;

float radical_inverse_van_der_corput(uint bits) {
    // http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * (1.0 / (float(0xFFFFFFFFu) + 1.0));
}

vec2 hammersley(uint i, uint sample_count) {
    return vec2(float(i) / float(sample_count), radical_inverse_van_der_corput(i));
}

vec3 importance_sample_ggx(vec2 xi, vec3 n, float roughness) {
    float a = square(roughness);
    float yaw = 2.0 * pi * xi.x;
    float pitch_cos = sqrt((1.0 - xi.y) / (1.0 + (square(a) - 1.0) * xi.y));
    float pitch_sin = sqrt(1.0 - pitch_cos * pitch_cos);
    vec3 h = vec3(cos(yaw) * pitch_sin, sin(yaw) * pitch_sin, pitch_cos);
    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

float distribution_ggx(float n_dot_x, float roughness) {
	float a = square(roughness);
	float a_sq = square(a);
	return a_sq / (pi * square((square(n_dot_x) * (a_sq - 1.0) + 1.0)));
}

float geometry_schlick_ggx(float n_dot_x, float k) {
	return n_dot_x / (n_dot_x * (1.0 - k) + k);
}

float geometry_smith(float n_dot_v, float n_dot_l, float a) {
	float k = square(a + 1.0) * 0.125;
	return geometry_schlick_ggx(n_dot_v, k) * geometry_schlick_ggx(n_dot_l, k);
}

float geometry_smith_v2(float n_dot_v, float n_dot_l, float a) {
	float k = square(a) * 0.5;
	return geometry_schlick_ggx(n_dot_v, k) * geometry_schlick_ggx(n_dot_l, k);
}

vec3 fresnel_schlick(float n_dot_l, vec3 f_0) {
	return f_0 + (1.0 - f_0) * pow(clamp(1.0 - n_dot_l, 0.0, 1.0), 5.0);
}

vec3 fresnel_schlick_roughness(float n_dot_l, vec3 f_0, float roughness) {
	return f_0 + (max(vec3(1.0 - roughness), f_0) - f_0) * pow(clamp(1.0 - n_dot_l, 0.0, 1.0), 5.0);
}

vec3 pbr(
	vec3 normal,
	vec3 view_direction,
	vec3 light_direction,
	float n_dot_v,
	vec3 light_color,
	vec3 albedo,
	float metallic,
	float roughness,
	vec3 reflectivity
) {
	vec3 half_vector = normalize(light_direction + view_direction);
	float n_dot_h = max(dot(normal, half_vector), 0.0);
	float n_dot_l = max(dot(normal, light_direction), 0.0);

	float d = distribution_ggx(n_dot_h, roughness);
	vec3 f = fresnel_schlick(max(0.0, dot(half_vector, view_direction)), reflectivity);
	float g = geometry_smith(n_dot_v, n_dot_l, roughness);

	vec3 k_d = (vec3(1.0) - f) * (1.0 - metallic);
	vec3 diffuse = k_d * albedo / pi;
	vec3 specular = d * f * g * (1.0 / (max(4.0 * n_dot_l * n_dot_v, specular_epsilon)));

	return (diffuse + specular) * light_color * n_dot_l;
}

#endif
