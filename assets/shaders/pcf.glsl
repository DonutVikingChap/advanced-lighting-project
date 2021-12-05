#ifndef PCF_GLSL
#define PCF_GLSL

#define PCF_FILTER_SAMPLE_COUNT 16
const vec2 pcf_filter_sample_offsets[PCF_FILTER_SAMPLE_COUNT] = vec2[](
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760),
	vec2(-0.91588581, 0.45771432),
	vec2(-0.81544232, -0.87912464),
	vec2(-0.38277543, 0.27676845),
	vec2(0.97484398, 0.75648379),
	vec2(0.44323325, -0.97511554),
	vec2(0.53742981, -0.47373420),
	vec2(-0.26496911, -0.41893023),
	vec2(0.79197514, 0.19090188),
	vec2(-0.24188840, 0.99706507),
	vec2(-0.81409955, 0.91437590),
	vec2(0.19984126, 0.78641367),
	vec2(0.14383161, -0.14100790));

float pcf_filter(sampler2DShadow shadow_map, vec2 uv, float receiver_depth, float filter_radius) {
    vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
    float visibility = 0.0;
    for (int i = 0; i < PCF_FILTER_SAMPLE_COUNT; ++i) {
        vec2 offset = pcf_filter_sample_offsets[i] * texel_size * filter_radius;
        visibility += texture(shadow_map, vec3(uv + offset, receiver_depth));
    }
    visibility /= float(PCF_FILTER_SAMPLE_COUNT);
    return visibility;
}

#define PCF_FILTER_CUBE_SAMPLE_COUNT 20
const vec3 pcf_filter_cube_sample_offsets[PCF_FILTER_CUBE_SAMPLE_COUNT] = vec3[](
    vec3(1.0, 1.0, 1.0),  vec3(1.0, -1.0, 1.0),  vec3(-1.0, -1.0, 1.0),  vec3(-1.0, 1.0, 1.0),
    vec3(1.0, 1.0, -1.0), vec3(1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0, 1.0, -1.0),
    vec3(1.0, 1.0, 0.0),  vec3(1.0, -1.0, 0.0),  vec3(-1.0, -1.0, 0.0),  vec3(-1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0),  vec3(-1.0, 0.0, 1.0),  vec3(1.0, 0.0, -1.0),   vec3(-1.0, 0.0, -1.0),
    vec3(0.0, 1.0, 1.0),  vec3(0.0, -1.0, 1.0),  vec3(0.0, -1.0, -1.0),  vec3(0.0, 1.0, -1.0));

float pcf_filter_cube(samplerCubeShadow shadow_map, vec3 uvw, float receiver_depth, float filter_radius) {
    float visibility = 0.0;
    for (int i = 0; i < PCF_FILTER_CUBE_SAMPLE_COUNT; ++i) {
        vec3 offset = pcf_filter_cube_sample_offsets[i] * filter_radius;
        visibility += texture(shadow_map, vec4(uvw + offset, receiver_depth));
    }
    visibility /= float(PCF_FILTER_CUBE_SAMPLE_COUNT);
    return visibility;
}

#endif
